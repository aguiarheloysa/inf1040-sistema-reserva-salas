import ctypes
import os
import sys
from datetime import datetime
from pathlib import Path
from typing import Any

from .c_types import (
    ATIVA,
    MAX_RESERVAS,
    MAX_SALAS,
    PROVA,
    PROFESSOR,
    TAM,
    TAM_EMAIL,
    DataHora,
    ReservaC,
)


class BackendError(Exception):
    """Erro de regra de negócio ou validação retornado pelo núcleo em C."""

    def __init__(self, codigo: int, mensagem: str):
        self.codigo = codigo
        self.mensagem = mensagem
        super().__init__(mensagem)


MENSAGENS_USUARIO = {
    0: "Usuário cadastrado.",
    1: "Matrícula já cadastrada.",
    2: "Dados do usuário inválidos.",
    3: "Usuário não cadastrado.",
    4: "Limite de usuários atingido.",
}

MENSAGENS_SALA = {
    0: "Sala cadastrada.",
    1: "Código da sala inválido.",
    2: "Sala já cadastrada.",
    3: "A capacidade deve ser maior que zero.",
    4: "Sala não cadastrada.",
    5: "Nenhuma sala encontrada.",
    6: "Dados inválidos para a sala ou para o laboratório.",
    7: "Limite de cadastro de salas ou biometrias atingido.",
}

MENSAGENS_GRADE = {
    0: "Aula cadastrada.",
    1: "Já existe uma aula na sala nesse horário.",
    2: "Dia da semana ou horário de aula inválido.",
    3: "Nenhuma aula no horário da autorização.",
    4: "Apenas o ministrante da aula pode autorizar.",
    5: "Dados da grade inválidos.",
    6: "Limite de aulas ou autorizações atingido.",
}

MENSAGENS_RESERVA = {
    0: "Reserva cancelada.",
    1: "Usuário não cadastrado.",
    2: "A quantidade de pessoas excede a capacidade da sala.",
    3: "Horário de aula. É necessária uma prova do ministrante ou autorizada por ele.",
    4: "O laboratório exige um monitor cadastrado com biometria na sala.",
    5: "Já existe uma reserva ativa que conflita com esse horário.",
    6: "O aviso de cancelamento exige pelo menos duas horas de antecedência.",
    7: "Reserva ativa não encontrada.",
    8: "Apenas o titular pode cancelar a reserva.",
    9: "Nenhuma sala disponível.",
    10: "Nenhuma reserva para a matrícula informada.",
    11: "Sala não cadastrada.",
    12: "Dados da reserva inválidos. Use dd/mm/aaaa e hh:mm.",
    13: "Limite de reservas atingido.",
    14: "Falha ao enviar o aviso de cancelamento. A prova não foi reservada.",
}


def _b(texto: str | None) -> bytes:
    """Converte string Python para bytes (char* em UTF-8)."""
    dados = (texto or "").strip().encode("utf-8")
    if b"\x00" in dados:
        raise ValueError("O texto não pode conter caracteres NUL.")
    return dados


def _int_c(valor: int) -> int:
    """Impede que ctypes converta silenciosamente inteiros fora do intervalo de C."""
    numero = int(valor)
    bits = ctypes.sizeof(ctypes.c_int) * 8
    if not -(1 << (bits - 1)) <= numero < (1 << (bits - 1)):
        raise ValueError("Número fora do intervalo de int do C.")
    return numero


def normalizar_data(data_str: str) -> str:
    """Converte 'YYYY-MM-DD' (HTML input date) para 'DD/MM/YYYY' (esperado por tempo.c)."""
    texto = (data_str or "").strip()
    if len(texto) == 10 and texto[4] == "-" and texto[7] == "-":
        return f"{texto[8:10]}/{texto[5:7]}/{texto[:4]}"
    return texto


def caminho_biblioteca() -> Path:
    """Localização padrão do binário compilado para a plataforma atual."""
    extensao = ".dll" if os.name == "nt" else ".dylib" if sys.platform == "darwin" else ".so"
    return Path(__file__).resolve().parent / f"libreserva{extensao}"


class BackendBridge:
    """Adaptador Python puro para a biblioteca compartilhada compilada em C."""

    def __init__(self, dll_path: Path | None = None):
        if dll_path is None:
            dll_path = caminho_biblioteca()
        dll_path = Path(dll_path).resolve()
        if not dll_path.exists():
            raise FileNotFoundError(
                f"Biblioteca C não encontrada em {dll_path}. Execute python Frontend/bridge/build.py antes de rodar."
            )

        self._lib = ctypes.CDLL(str(dll_path))
        self._configurar_assinaturas()

    def _checar(self, rc: int, tabela: dict[int, str], msg_sucesso: str | None = None) -> str:
        """Lança BackendError se rc != 0; caso contrário devolve a mensagem de sucesso."""
        if rc != 0:
            raise BackendError(rc, tabela.get(rc, f"Erro inesperado (código {rc})."))
        return msg_sucesso or tabela.get(0, "Operação realizada com sucesso.")

    def _configurar_assinaturas(self) -> None:
        lib = self._lib

        # Tempo (Comum/tempo.h)
        lib.tempo_dia.argtypes = [ctypes.c_char_p]
        lib.tempo_dia.restype = ctypes.c_int
        lib.tempo_intervalo.argtypes = [ctypes.c_char_p, ctypes.c_char_p,
                                         ctypes.POINTER(ctypes.c_int), ctypes.POINTER(ctypes.c_int)]
        lib.tempo_intervalo.restype = ctypes.c_int
        lib.tempo_instante.argtypes = [DataHora]
        lib.tempo_instante.restype = ctypes.c_int64

        # Usuário (Aluno/usuario.h)
        lib.usu_existe.argtypes = [ctypes.c_char_p]
        lib.usu_existe.restype = ctypes.c_int
        lib.usu_cadastrar.argtypes = [ctypes.c_char_p, ctypes.c_char_p, ctypes.c_char_p, ctypes.c_int]
        lib.usu_cadastrar.restype = ctypes.c_int

        lib.usu_obterTipo.argtypes = [ctypes.c_char_p, ctypes.POINTER(ctypes.c_int)]
        lib.usu_obterTipo.restype = ctypes.c_int

        lib.usu_obterEmail.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
        lib.usu_obterEmail.restype = ctypes.c_int

        # Sala (Sala/sala.h)
        lib.sala_cadastrar.argtypes = [ctypes.c_char_p, ctypes.c_int, ctypes.c_int, ctypes.c_int]
        lib.sala_cadastrar.restype = ctypes.c_int

        lib.sala_obterCapacidade.argtypes = [ctypes.c_char_p, ctypes.POINTER(ctypes.c_int)]
        lib.sala_obterCapacidade.restype = ctypes.c_int

        lib.sala_obterTipo.argtypes = [ctypes.c_char_p, ctypes.POINTER(ctypes.c_int)]
        lib.sala_obterTipo.restype = ctypes.c_int

        lib.sala_listarPorCapacidade.argtypes = [
            ctypes.c_int,
            ctypes.c_int,
            ctypes.POINTER(ctypes.c_char * TAM),
            ctypes.POINTER(ctypes.c_int),
        ]
        lib.sala_listarPorCapacidade.restype = ctypes.c_int

        lib.sala_registrarBiometria.argtypes = [ctypes.c_char_p, ctypes.c_char_p]
        lib.sala_registrarBiometria.restype = ctypes.c_int

        # Grade (Grade/grade.h)
        lib.grade_cadastrarAula.argtypes = [
            ctypes.c_char_p,
            ctypes.c_char_p,
            ctypes.c_char_p,
            ctypes.c_int,
            ctypes.c_char_p,
            ctypes.c_char_p,
        ]
        lib.grade_cadastrarAula.restype = ctypes.c_int

        lib.grade_registrarAutorizacao.argtypes = [
            ctypes.c_char_p,
            ctypes.c_char_p,
            ctypes.c_char_p,
            ctypes.c_char_p,
            ctypes.c_char_p,
        ]
        lib.grade_registrarAutorizacao.restype = ctypes.c_int

        # Reserva (Reserva/reserva.h)
        lib.res_buscarSalasLivres.argtypes = [
            ctypes.c_char_p,
            ctypes.c_char_p,
            ctypes.c_char_p,
            ctypes.c_int,
            ctypes.c_int,
            ctypes.c_char_p,
            ctypes.POINTER(ctypes.c_char * TAM),
            ctypes.POINTER(ctypes.c_int),
        ]
        lib.res_buscarSalasLivres.restype = ctypes.c_int

        lib.res_solicitar.argtypes = [
            ctypes.c_char_p,
            ctypes.c_char_p,
            ctypes.c_char_p,
            ctypes.c_char_p,
            ctypes.c_char_p,
            ctypes.c_int,
            ctypes.c_int,
            ctypes.c_char_p,
            DataHora,
            ctypes.POINTER(ctypes.c_int),
        ]
        lib.res_solicitar.restype = ctypes.c_int

        lib.res_cancelar.argtypes = [ctypes.c_int, ctypes.c_char_p]
        lib.res_cancelar.restype = ctypes.c_int

        lib.res_listarPorUsuario.argtypes = [
            ctypes.c_char_p,
            ctypes.POINTER(ReservaC),
            ctypes.POINTER(ctypes.c_int),
        ]
        lib.res_listarPorUsuario.restype = ctypes.c_int

    def _exigir_sala(self, codigo: str) -> None:
        tipo = ctypes.c_int()
        self._checar(self._lib.sala_obterTipo(_b(codigo), ctypes.byref(tipo)), MENSAGENS_SALA)

    def _exigir_professor(self, matricula: str) -> None:
        tipo = ctypes.c_int()
        rc = self._lib.usu_obterTipo(_b(matricula), ctypes.byref(tipo))
        if rc != 0 or tipo.value != PROFESSOR:
            raise BackendError(5, "Informe a matrícula de um professor cadastrado.")

    # --- Usuário ---
    def cadastrar_usuario(self, matricula: str, nome: str, email: str, tipo: int) -> str:
        rc = self._lib.usu_cadastrar(_b(matricula), _b(nome), _b(email), _int_c(tipo))
        return self._checar(rc, MENSAGENS_USUARIO)

    def consultar_usuario(self, matricula: str) -> dict[str, Any]:
        mat_b = _b(matricula)
        tipo = ctypes.c_int()
        rc = self._lib.usu_obterTipo(mat_b, ctypes.byref(tipo))
        self._checar(rc, MENSAGENS_USUARIO)

        email_buf = ctypes.create_string_buffer(TAM_EMAIL)
        self._checar(self._lib.usu_obterEmail(mat_b, email_buf), MENSAGENS_USUARIO)
        return {
            "matricula": matricula.strip(),
            "tipo": "Professor" if tipo.value == 1 else "Aluno",
            "email": email_buf.value.decode("utf-8"),
        }

    # --- Sala ---
    def cadastrar_sala(self, codigo: str, tipo: int, capacidade: int, tem_quadro: int) -> str:
        rc = self._lib.sala_cadastrar(_b(codigo), _int_c(tipo), _int_c(capacidade), _int_c(tem_quadro))
        return self._checar(rc, MENSAGENS_SALA)

    def listar_todas_salas(self) -> list[dict[str, Any]]:
        buffer = ((ctypes.c_char * TAM) * MAX_SALAS)()
        n = ctypes.c_int(0)
        rc = self._lib.sala_listarPorCapacidade(1, 0, buffer, ctypes.byref(n))
        if rc == 5:  # SALA_NENHUMA
            return []
        self._checar(rc, MENSAGENS_SALA)

        salas = []
        for i in range(n.value):
            cod = buffer[i].value.decode("utf-8")
            cap, tipo = ctypes.c_int(), ctypes.c_int()
            self._checar(self._lib.sala_obterCapacidade(_b(cod), ctypes.byref(cap)), MENSAGENS_SALA)
            self._checar(self._lib.sala_obterTipo(_b(cod), ctypes.byref(tipo)), MENSAGENS_SALA)
            salas.append({
                "codigo": cod,
                "capacidade": cap.value,
                "tipo": "Laboratório" if tipo.value == 1 else "Comum",
            })
        return salas

    def registrar_biometria(self, cod_sala: str, mat_monitor: str) -> str:
        if not self._lib.usu_existe(_b(mat_monitor)):
            raise BackendError(3, "Monitor não cadastrado.")
        rc = self._lib.sala_registrarBiometria(_b(cod_sala), _b(mat_monitor))
        return self._checar(rc, MENSAGENS_SALA, msg_sucesso="Biometria registrada.")

    # --- Grade ---
    def cadastrar_aula(
        self, cod_sala: str, disciplina: str, mat_professor: str, dia_semana: int, inicio: str, fim: str
    ) -> str:
        self._exigir_sala(cod_sala)
        self._exigir_professor(mat_professor)
        rc = self._lib.grade_cadastrarAula(
            _b(cod_sala), _b(disciplina), _b(mat_professor), _int_c(dia_semana), _b(inicio), _b(fim)
        )
        return self._checar(rc, MENSAGENS_GRADE)

    def autorizar_prova(
        self, cod_sala: str, data: str, hora: str, mat_ministrante: str, mat_solicitante: str
    ) -> str:
        self._exigir_sala(cod_sala)
        self._exigir_professor(mat_ministrante)
        self._exigir_professor(mat_solicitante)
        rc = self._lib.grade_registrarAutorizacao(
            _b(cod_sala), _b(normalizar_data(data)), _b(hora), _b(mat_ministrante), _b(mat_solicitante)
        )
        return self._checar(rc, MENSAGENS_GRADE, msg_sucesso="Autorização registrada.")

    # --- Reserva ---
    def buscar_salas_livres(
        self, matricula: str, data: str, inicio: str, fim: str, qtd_pessoas: int, precisa_quadro: int
    ) -> list[str]:
        buffer = ((ctypes.c_char * TAM) * MAX_SALAS)()
        n = ctypes.c_int(0)
        data_b, inicio_b, fim_b = _b(normalizar_data(data)), _b(inicio), _b(fim)
        pessoas, quadro = _int_c(qtd_pessoas), _int_c(precisa_quadro)
        if not self._lib.usu_existe(_b(matricula)):
            self._checar(1, MENSAGENS_RESERVA)
        i, f = ctypes.c_int(), ctypes.c_int()
        if (pessoas <= 0 or quadro not in (0, 1) or self._lib.tempo_dia(data_b) < 0
                or not self._lib.tempo_intervalo(inicio_b, fim_b, ctypes.byref(i), ctypes.byref(f))):
            self._checar(12, MENSAGENS_RESERVA)
        rc = self._lib.res_buscarSalasLivres(
            data_b,
            inicio_b,
            fim_b,
            pessoas,
            quadro,
            _b(matricula),
            buffer,
            ctypes.byref(n),
        )
        if rc == 9:  # RES_NENHUMA_SALA
            return []
        self._checar(rc, MENSAGENS_RESERVA)
        return [buffer[i].value.decode("utf-8") for i in range(n.value)]

    def solicitar_reserva(
        self,
        matricula: str,
        cod_sala: str,
        data: str,
        inicio: str,
        fim: str,
        qtd_pessoas: int,
        finalidade: int,
        mat_monitor: str = "",
        agora_override: tuple[str, str] | None = None,
    ) -> int:
        if agora_override:
            agora_data, agora_hora = normalizar_data(agora_override[0]), agora_override[1]
        else:
            agora_dt = datetime.now()
            agora_data = agora_dt.strftime("%d/%m/%Y")
            agora_hora = agora_dt.strftime("%H:%M")

        data_b, hora_b = _b(agora_data), _b(agora_hora)
        if len(data_b) != 10 or len(hora_b) != 5:
            self._checar(12, MENSAGENS_RESERVA)
        agora_c = DataHora(data=data_b, hora=hora_b)
        if self._lib.tempo_instante(agora_c) < 0:
            self._checar(12, MENSAGENS_RESERVA)
        id_reserva = ctypes.c_int(0)

        rc = self._lib.res_solicitar(
            _b(matricula),
            _b(cod_sala),
            _b(normalizar_data(data)),
            _b(inicio),
            _b(fim),
            _int_c(qtd_pessoas),
            _int_c(finalidade),
            _b(mat_monitor),
            agora_c,
            ctypes.byref(id_reserva),
        )
        self._checar(rc, MENSAGENS_RESERVA)
        return id_reserva.value

    def cancelar_reserva(self, id_reserva: int, matricula: str) -> str:
        rc = self._lib.res_cancelar(_int_c(id_reserva), _b(matricula))
        return self._checar(rc, MENSAGENS_RESERVA)

    def listar_reservas_usuario(self, matricula: str) -> list[dict[str, Any]]:
        buffer = (ReservaC * MAX_RESERVAS)()
        n = ctypes.c_int(0)
        if not self._lib.usu_existe(_b(matricula)):
            self._checar(1, MENSAGENS_RESERVA)
        rc = self._lib.res_listarPorUsuario(_b(matricula), buffer, ctypes.byref(n))
        if rc == 10:  # RES_NENHUMA_RESERVA
            return []
        self._checar(rc, MENSAGENS_RESERVA)

        resultado = []
        for i in range(n.value):
            r = buffer[i]
            resultado.append({
                "id": r.id,
                "matricula": r.matricula.decode("utf-8"),
                "cod_sala": r.codSala.decode("utf-8"),
                "data": r.data.decode("utf-8"),
                "inicio": r.inicio.decode("utf-8"),
                "fim": r.fim.decode("utf-8"),
                "qtd_pessoas": r.qtdPessoas,
                "finalidade": "PROVA" if r.finalidade == PROVA else "ESTUDO",
                "situacao": "ATIVA" if r.situacao == ATIVA else "CANCELADA",
            })
        return resultado