"""Testes da integração C/Python e do transporte SMTP sem rede externa."""
import shutil
from email import policy
from email.parser import BytesParser
import smtplib

import pytest

from Frontend.bridge import BackendBridge, BackendError
from Frontend.bridge.backend_bridge import caminho_biblioteca
from Frontend.bridge.c_types import ALUNO, PROFESSOR, COMUM, ESTUDO, PROVA
from Frontend.notificacoes import entregar_smtp, MENSAGEM_COLETIVA
from Interface import enviar_email

DATA = '2026-10-20'


@pytest.fixture
def biblioteca(tmp_path):
    original = caminho_biblioteca()
    isolada = tmp_path / original.name
    shutil.copy2(original, isolada)
    return isolada


def preparar(biblioteca, canal):
    bridge = BackendBridge(biblioteca, canal_entrega=canal)
    for mat, tipo in [('A1', ALUNO), ('A2', ALUNO), ('P1', PROFESSOR)]:
        bridge.cadastrar_usuario(mat, mat, mat.lower()+'@example.test', tipo)
    bridge.cadastrar_sala('L201', COMUM, 40, 1)
    return bridge


def reservar(bridge, mat='A1', inicio='14:00', fim='15:00', finalidade=ESTUDO, agora='12:00'):
    return bridge.solicitar_reserva(mat, 'L201', DATA, inicio, fim, 5, finalidade,
                                    agora_override=(DATA, agora))


def estado(bridge, mat):
    return [r['situacao'] for r in bridge.listar_reservas_usuario(mat)]


def test_entrega_individual(biblioteca):
    lotes = []
    bridge = preparar(biblioteca, lambda lote: lotes.append(lote) or True)
    reservar(bridge)
    assert lotes == []
    assert reservar(bridge, 'P1', finalidade=PROVA) == 2
    assert len(lotes) == 1 and len(lotes[0]) == 1
    email, mensagem = lotes[0][0]
    assert email == 'a1@example.test'
    assert 'L201 em 20/10/2026, às 14:00' in mensagem
    assert estado(bridge, 'A1') == ['CANCELADA']
    assert estado(bridge, 'P1') == ['ATIVA']


def test_entrega_lote(biblioteca):
    lotes = []
    bridge = preparar(biblioteca, lambda lote: lotes.append(lote) or True)
    reservar(bridge)
    reservar(bridge, 'A2', '15:00', '16:00')
    reservar(bridge, 'P1', '14:00', '16:00', PROVA)
    assert len(lotes) == 1
    assert {email for email, _ in lotes[0]} == {'a1@example.test', 'a2@example.test'}
    assert estado(bridge, 'A1') == estado(bridge, 'A2') == ['CANCELADA']


@pytest.mark.parametrize('excecao', [False, True])
def test_falha_preserva_lote(biblioteca, excecao):
    def falhar(lote):
        if excecao:
            raise OSError('segredo de teste')
        return False
    bridge = preparar(biblioteca, falhar)
    reservar(bridge)
    reservar(bridge, 'A2', '15:00', '16:00')
    with pytest.raises(BackendError) as erro:
        reservar(bridge, 'P1', '14:00', '16:00', PROVA)
    assert erro.value.codigo == 14
    assert estado(bridge, 'A1') == estado(bridge, 'A2') == ['ATIVA']
    assert estado(bridge, 'P1') == []


def test_antecedencia_recusada_nao_notifica(biblioteca):
    lotes = []
    bridge = preparar(biblioteca, lambda lote: lotes.append(lote) or True)
    reservar(bridge)
    with pytest.raises(BackendError) as erro:
        reservar(bridge, 'P1', finalidade=PROVA, agora='12:01')
    assert erro.value.codigo == 6 and lotes == []
    assert estado(bridge, 'A1') == ['ATIVA']


def test_cancelamento_voluntario_nao_notifica(biblioteca):
    lotes = []
    bridge = preparar(biblioteca, lambda lote: lotes.append(lote) or True)
    identificador = reservar(bridge)
    bridge.cancelar_reserva(identificador, 'A1')
    assert lotes == [] and estado(bridge, 'A1') == ['CANCELADA']


def test_callback_retirado_apos_falha(biblioteca, capfd):
    bridge = preparar(biblioteca, lambda lote: False)
    reservar(bridge)
    with pytest.raises(BackendError):
        reservar(bridge, 'P1', finalidade=PROVA)
    # Mesma biblioteca, mas sem callback: o canal C original deve ter voltado.
    outra = BackendBridge(biblioteca)
    assert reservar(outra, 'P1', finalidade=PROVA) == 2
    assert 'Notificação simulada para a1@example.test' in capfd.readouterr().out


@pytest.fixture
def smtp(monkeypatch):
    for chave, valor in {'RESERVA_SMTP_HOST':'smtp.example.test',
                         'RESERVA_SMTP_REMETENTE':'reservas@example.test',
                         'RESERVA_SMTP_TLS':'nenhum', 'RESERVA_SMTP_PORTA':'25'}.items():
        monkeypatch.setenv(chave, valor)
    for chave in ('RESERVA_SMTP_USUARIO', 'RESERVA_SMTP_SENHA'):
        monkeypatch.delenv(chave, raising=False)
    class Rele:
        rejeitar = None
        sessoes = []
        def __init__(self, servidor, porta, timeout):
            assert servidor == 'smtp.example.test' and porta == 25 and timeout == 10
            self.destinatarios = []
            self.corpo = None
            self.fechado = False
            self.sessoes.append(self)
        def ehlo_or_helo_if_needed(self):
            pass
        def mail(self, remetente):
            assert remetente == 'reservas@example.test'
            return 250, b'OK'
        def rcpt(self, endereco):
            if endereco == self.rejeitar:
                return 550, b'Recusado'
            self.destinatarios.append(endereco)
            return 250, b'OK'
        def data(self, corpo):
            self.corpo = corpo
            return 250, b'OK'
        def close(self):
            self.fechado = True
    monkeypatch.setattr(enviar_email.smtplib, 'SMTP', Rele)
    return Rele


@pytest.mark.parametrize('lote', [False, True])
def test_smtp_usa_uma_transacao(biblioteca, smtp, lote):
    bridge = preparar(biblioteca, entregar_smtp)
    reservar(bridge)
    if lote:
        reservar(bridge, 'A2', '15:00', '16:00')
    reservar(bridge, 'P1', '14:00', '16:00' if lote else '15:00', PROVA)
    assert len(smtp.sessoes) == 1
    sessao = smtp.sessoes[0]
    assert sessao.destinatarios == (['a1@example.test', 'a2@example.test'] if lote else ['a1@example.test'])
    email = BytesParser(policy=policy.default).parsebytes(sessao.corpo)
    assert email['Subject'] == 'Cancelamento de reserva de sala'
    if lote:
        assert email['To'] is None
        assert MENSAGEM_COLETIVA in ' '.join(email.get_content().split())
    else:
        assert str(email['To']) == 'a1@example.test'
        assert 'L201 em 20/10/2026, às 14:00' in email.get_content()
    assert sessao.fechado


def test_smtp_recusa_antes_de_data(biblioteca, smtp):
    smtp.rejeitar = 'a2@example.test'
    bridge = preparar(biblioteca, entregar_smtp)
    reservar(bridge)
    reservar(bridge, 'A2', '15:00', '16:00')
    with pytest.raises(BackendError) as erro:
        reservar(bridge, 'P1', '14:00', '16:00', PROVA)
    assert erro.value.codigo == 14
    assert smtp.sessoes[0].corpo is None and smtp.sessoes[0].fechado
    assert estado(bridge, 'A1') == estado(bridge, 'A2') == ['ATIVA']
    assert estado(bridge, 'P1') == []


def test_smtp_sem_configuracao(biblioteca, monkeypatch):
    monkeypatch.delenv('RESERVA_SMTP_HOST', raising=False)
    bridge = preparar(biblioteca, entregar_smtp)
    reservar(bridge)
    with pytest.raises(BackendError) as erro:
        reservar(bridge, 'P1', finalidade=PROVA)
    assert erro.value.codigo == 14
    assert estado(bridge, 'A1') == ['ATIVA'] and estado(bridge, 'P1') == []
