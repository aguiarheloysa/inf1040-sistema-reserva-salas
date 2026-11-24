
from datetime import date, timedelta
from email import policy
from email.parser import BytesParser
from pathlib import Path
import os
import socketserver
import subprocess
import sys
import threading
import unittest


EXECUTAVEL = Path(sys.argv.pop(1)).resolve()
SEGUNDA = "19/10/2026"
DATA_FUTURA = (date.today() + timedelta(days=3)).strftime("%d/%m/%Y")
ALUNA = (1, "1001", "Alice", "alice@example.test", 0)
OUTRA_ALUNA = (1, "1002", "Ana", "ana@example.test", 0)
MONITOR = (1, "3001", "Monitor", "monitor@example.test", 0)
PROFESSOR = (1, "2001", "Professor Um", "p1@example.test", 1)
OUTRO_PROFESSOR = (1, "2002", "Professor Dois", "p2@example.test", 1)
L201 = (2, "L201", 0, 40, 1)
L105 = (2, "L105", 0, 10, 0)
L301 = (2, "L301", 1, 30, 0)


def reservar(matricula, sala, dia, inicio, fim, pessoas=5, finalidade=0, monitor=""):
    return (7, matricula, sala, dia, inicio, fim, pessoas, finalidade, monitor)


class ReleSMTP(socketserver.StreamRequestHandler):
    def responder(self, resposta):
        self.wfile.write(resposta + b"\r\n")

    def handle(self):
        self.request.settimeout(8)
        self.responder(b"220 localhost")
        remetente = None
        destinatarios = []
        while linha := self.rfile.readline():
            comando, _, valor = linha.strip().partition(b" ")
            comando = comando.upper()
            if comando in (b"EHLO", b"HELO"):
                self.responder(b"250 localhost")
            elif comando == b"MAIL":
                remetente = valor.partition(b":")[2].strip()
                self.responder(b"250 OK")
            elif comando == b"RCPT":
                if self.server.rejeitar and self.server.rejeitar in valor:
                    self.responder(b"550 Destinatario recusado")
                else:
                    destinatarios.append(valor.partition(b":")[2].strip())
                    self.responder(b"250 OK")
            elif comando == b"DATA":
                self.responder(b"354 Envie a mensagem")
                conteudo = []
                while parte := self.rfile.readline():
                    if parte == b".\r\n":
                        self.server.mensagens.append((remetente, destinatarios, b"".join(conteudo)))
                        self.responder(b"250 Mensagem aceita")
                        break
                    conteudo.append(parte[1:] if parte.startswith(b"..") else parte)
            else:
                self.responder(b"500 Comando inesperado")


class TesteInterface(unittest.TestCase):
    def executar(self, *acoes, args=(), env=None, encerrar=True):
        campos = [str(campo) for acao in acoes for campo in acao]
        if encerrar:
            campos.append("0")
        processo = subprocess.run(
            [str(EXECUTAVEL), *args],
            input="\n".join(campos) + "\n",
            encoding="utf-8",
            capture_output=True,
            timeout=25,
            env=env,
            check=True,
        )
        return processo

    def test_busca_reserva_listagem_e_cancelamento(self):
        saida = self.executar(
            ALUNA, PROFESSOR, L201, L105, L301,
            (6, "1001", DATA_FUTURA, "14:00", "16:00", 30, 1),
            reservar("1001", "L105", DATA_FUTURA, "14:00", "16:00", 30),
            reservar("1001", "L201", DATA_FUTURA, "14:00", "16:00", 30),
            (6, "1001", DATA_FUTURA, "14:00", "16:00", 30, 0),
            (9, "1001"),
            (8, "2001", 1),
            (8, "1001", 1),
            (9, "1001"),
            (9, "2001"),
            (6, "1001", DATA_FUTURA, "14:00", "16:00", 30, 1),
        ).stdout
        self.assertEqual(saida.count("Salas disponíveis:\nL201\nOpção:"), 2)
        self.assertIn("excede a capacidade da sala", saida)
        self.assertIn("Reserva confirmada. ID: 1", saida)
        self.assertIn("Nenhuma sala disponível.", saida)
        self.assertIn("Apenas o titular pode cancelar a reserva.", saida)
        self.assertIn("Reserva cancelada.", saida)
        self.assertIn("| 30 pessoa(s) | ESTUDO | ATIVA", saida)
        self.assertIn("| 30 pessoa(s) | ESTUDO | CANCELADA", saida)
        self.assertIn("Nenhuma reserva para a matrícula informada.", saida)

    def test_grade_autorizacao_e_biometria(self):
        saida = self.executar(
            ALUNA, MONITOR, PROFESSOR, OUTRO_PROFESSOR, L201, L301,
            (3, "L201", "Cálculo", "2001", 1, "09:00", "11:00"),
            (3, "L201", "Física", "2002", 1, "10:00", "12:00"),
            reservar("2002", "L201", SEGUNDA, "09:00", "10:00", finalidade=1),
            (5, "L201", SEGUNDA, "09:00", "2002", "2002"),
            (5, "L201", SEGUNDA, "09:00", "2001", "2002"),
            reservar("2002", "L201", SEGUNDA, "09:00", "10:00", finalidade=1),
            reservar("1001", "L301", SEGUNDA, "14:00", "15:00", monitor="3001"),
            (4, "L301", "3001"),
            reservar("1001", "L301", SEGUNDA, "14:00", "15:00", monitor="3001"),
            (9, "2002"), (9, "1001"),
        ).stdout
        self.assertIn("Aula cadastrada.", saida)
        self.assertIn("Já existe uma aula na sala nesse horário.", saida)
        self.assertIn("Horário de aula.", saida)
        self.assertIn("Apenas o ministrante da aula pode autorizar.", saida)
        self.assertIn("Autorização registrada.", saida)
        self.assertIn("O laboratório exige um monitor cadastrado com biometria na sala.", saida)
        self.assertIn("Biometria registrada.", saida)
        self.assertIn("Reserva confirmada. ID: 1", saida)
        self.assertIn("Reserva confirmada. ID: 2", saida)
        self.assertIn("| PROVA | ATIVA", saida)
        self.assertIn("| ESTUDO | ATIVA", saida)

    def test_prioridade_com_aviso_simulado(self):
        saida = self.executar(
            ALUNA, PROFESSOR, OUTRO_PROFESSOR, L201,
            reservar("1001", "L201", DATA_FUTURA, "14:00", "15:00"),
            reservar("2001", "L201", DATA_FUTURA, "14:00", "15:00", finalidade=1),
            reservar("2002", "L201", DATA_FUTURA, "14:00", "15:00", finalidade=1),
            (9, "1001"), (9, "2001"),
        ).stdout
        self.assertIn("Notificação simulada para alice@example.test:", saida)
        self.assertIn("Já existe uma reserva ativa que conflita", saida)
        self.assertEqual(saida.count("Reserva confirmada."), 2)
        self.assertIn("| ESTUDO | CANCELADA", saida)
        self.assertIn("| PROVA | ATIVA", saida)

    def test_matricula_somente_com_digitos(self):
        saida = self.executar(
            (1, "10A1", "Alice", "alice@example.test", 0),
            (1, "001001", "Alice", "alice@example.test", 0),
            ALUNA,
            (1, "001001", "Alice", "alice@example.test", 0),
        ).stdout
        self.assertEqual(saida.count("Dados do usuário inválidos."), 1)
        self.assertEqual(saida.count("Usuário cadastrado."), 2)
        self.assertEqual(saida.count("Matrícula já cadastrada."), 1)

    def test_entradas_invalidas_e_fim_de_arquivo(self):
        saida = self.executar(
            ("opção inválida",), ("x" * 300,), ("9999999999999999999999",),
            (1, "1000", "", "a0@example.test", 0), ALUNA, ALUNA, L201,
            reservar("1001", "L201", "31/02/2026", "14:00", "15:00"),
            reservar("1001", "L201", DATA_FUTURA, "16:00", "15:00"),
            (6, "9999", DATA_FUTURA, "14:00", "15:00", 1, 0),
            (1, "1002"),
            encerrar=False,
        ).stdout
        self.assertEqual(saida.count("Informe um número entre 0 e 9."), 2)
        self.assertIn("Entrada muito longa.", saida)
        self.assertIn("Dados do usuário inválidos.", saida)
        self.assertIn("Usuário cadastrado.", saida)
        self.assertIn("Matrícula já cadastrada.", saida)
        self.assertEqual(saida.count("Dados da reserva inválidos."), 2)
        self.assertIn("Usuário não cadastrado.", saida)
        self.assertIn("Nome:", saida)

    def executar_smtp(self, *acoes, rejeitar=None):
        with socketserver.ThreadingTCPServer(("127.0.0.1", 0), ReleSMTP) as servidor:
            servidor.rejeitar = rejeitar
            servidor.mensagens = []
            thread = threading.Thread(target=servidor.serve_forever, daemon=True)
            thread.start()
            ambiente = os.environ.copy()
            ambiente.pop("RESERVA_SMTP_USUARIO", None)
            ambiente.pop("RESERVA_SMTP_SENHA", None)
            ambiente.update(
                RESERVA_SMTP_HOST="127.0.0.1",
                RESERVA_SMTP_PORTA=str(servidor.server_address[1]),
                RESERVA_SMTP_REMETENTE="reservas@example.test",
                RESERVA_SMTP_TLS="nenhum",
            )
            try:
                processo = self.executar(*acoes, args=("--smtp",), env=ambiente)
            finally:
                servidor.shutdown()
                thread.join()
            return processo, servidor.mensagens

    def test_entrega_smtp(self):
        processo, mensagens = self.executar_smtp(
            ALUNA, PROFESSOR, L201,
            reservar("1001", "L201", DATA_FUTURA, "14:00", "15:00"),
            reservar("2001", "L201", DATA_FUTURA, "14:00", "15:00", finalidade=1),
            (9, "1001"),
        )
        self.assertEqual(processo.stderr, "")
        self.assertIn("| ESTUDO | CANCELADA", processo.stdout)
        self.assertIn("Reserva confirmada. ID: 2", processo.stdout)
        self.assertEqual(len(mensagens), 1)
        remetente, destinatarios, conteudo = mensagens[0]
        self.assertEqual(remetente, b"<reservas@example.test>")
        self.assertEqual(destinatarios, [b"<alice@example.test>"])
        email = BytesParser(policy=policy.default).parsebytes(conteudo)
        self.assertEqual(email["Subject"], "Cancelamento de reserva de sala")
        self.assertIn(f"L201 em {DATA_FUTURA}, às 14:00", email.get_content())

    def test_entrega_smtp_para_dois_titulares(self):
        processo, mensagens = self.executar_smtp(
            ALUNA, OUTRA_ALUNA, PROFESSOR, L201,
            reservar("1001", "L201", DATA_FUTURA, "14:00", "15:00"),
            reservar("1002", "L201", DATA_FUTURA, "15:00", "16:00"),
            reservar("2001", "L201", DATA_FUTURA, "14:00", "16:00", finalidade=1),
            (9, "1001"), (9, "1002"),
        )
        self.assertEqual(processo.stderr, "")
        self.assertEqual(processo.stdout.count("| ESTUDO | CANCELADA"), 2)
        self.assertEqual(len(mensagens), 1)
        _, destinatarios, conteudo = mensagens[0]
        self.assertEqual(destinatarios, [b"<alice@example.test>", b"<ana@example.test>"])
        email = BytesParser(policy=policy.default).parsebytes(conteudo)
        self.assertIsNone(email["To"])
        self.assertIn("cancelada para aplicação de prova", email.get_content())

    def test_recusa_smtp_preserva_todas_as_reservas(self):
        processo, mensagens = self.executar_smtp(
            ALUNA, OUTRA_ALUNA, PROFESSOR, L201,
            reservar("1001", "L201", DATA_FUTURA, "14:00", "15:00"),
            reservar("1002", "L201", DATA_FUTURA, "15:00", "16:00"),
            reservar("2001", "L201", DATA_FUTURA, "14:00", "16:00", finalidade=1),
            (9, "1001"), (9, "1002"),
            rejeitar=b"ana@example.test",
        )
        self.assertIn("Falha de entrega SMTP", processo.stderr)
        self.assertIn("Falha ao enviar o aviso de cancelamento.", processo.stdout)
        self.assertEqual(processo.stdout.count("| ESTUDO | ATIVA"), 2)
        self.assertEqual(processo.stdout.count("Reserva confirmada."), 2)
        self.assertEqual(mensagens, [])


if __name__ == "__main__":
    unittest.main(verbosity=2)
