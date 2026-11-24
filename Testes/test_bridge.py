import shutil
from pathlib import Path
import pytest

from Frontend.bridge import BackendBridge, BackendError
from Frontend.bridge.backend_bridge import caminho_biblioteca
from Frontend.bridge.c_types import ALUNO, COMUM, ESTUDO, LABORATORIO, PROFESSOR, PROVA


@pytest.fixture
def bridge(tmp_path: Path) -> BackendBridge:
    """Cria uma instância isolada da DLL em pasta temporária para zerar a RAM do C a cada teste."""
    dll_original = caminho_biblioteca()
    dll_isolada = tmp_path / dll_original.name
    shutil.copy2(dll_original, dll_isolada)
    return BackendBridge(dll_path=dll_isolada)


def test_usuarios(bridge: BackendBridge):
    assert bridge.cadastrar_usuario("A1", "Alice", "alice@puc-rio.br", ALUNO) == "Usuário cadastrado."
    assert bridge.cadastrar_usuario("P1", "Prof", "prof@puc-rio.br", PROFESSOR) == "Usuário cadastrado."

    dados = bridge.consultar_usuario("A1")
    assert dados == {"matricula": "A1", "tipo": "Aluno", "email": "alice@puc-rio.br"}

    with pytest.raises(BackendError, match="Matrícula já cadastrada"):
        bridge.cadastrar_usuario("A1", "Outra", "outra@puc-rio.br", ALUNO)


def test_salas_e_biometria(bridge: BackendBridge):
    assert bridge.cadastrar_sala("L201", COMUM, 40, 1) == "Sala cadastrada."
    assert bridge.cadastrar_sala("L301", LABORATORIO, 30, 0) == "Sala cadastrada."

    with pytest.raises(BackendError, match="Código da sala inválido"):
        bridge.cadastrar_sala("abc", COMUM, 20, 0)

    salas = bridge.listar_todas_salas()
    assert [s["codigo"] for s in salas] == ["L201", "L301"]

    bridge.cadastrar_usuario("M1", "Monitor", "m1@puc-rio.br", ALUNO)
    assert bridge.registrar_biometria("L301", "M1") == "Biometria registrada."


def test_grade_e_autorizacao(bridge: BackendBridge):
    bridge.cadastrar_usuario("P1", "Prof 1", "p1@puc-rio.br", PROFESSOR)
    bridge.cadastrar_usuario("P2", "Prof 2", "p2@puc-rio.br", PROFESSOR)
    bridge.cadastrar_sala("L201", COMUM, 40, 1)
    assert bridge.cadastrar_aula("L201", "Programação Modular", "P1", 1, "09:00", "11:00") == "Aula cadastrada."

    with pytest.raises(BackendError, match="Já existe uma aula"):
        bridge.cadastrar_aula("L201", "Cálculo", "P2", 1, "10:00", "12:00")

    with pytest.raises(BackendError, match="Apenas o ministrante"):
        bridge.autorizar_prova("L201", "2026-10-19", "09:00", "P2", "P2")

    assert bridge.autorizar_prova("L201", "2026-10-19", "09:00", "P1", "P2") == "Autorização registrada."


def test_fluxo_reservas_e_prioridade(bridge: BackendBridge):
    bridge.cadastrar_usuario("A1", "Alice", "alice@puc-rio.br", ALUNO)
    bridge.cadastrar_usuario("P1", "Prof", "prof@puc-rio.br", PROFESSOR)
    bridge.cadastrar_sala("L201", COMUM, 40, 1)

    livres = bridge.buscar_salas_livres("A1", "2026-10-20", "14:00", "15:00", 10, 1)
    assert livres == ["L201"]

    # Reserva de estudo do aluno (data no formato YYYY-MM-DD do HTML)
    id_estudo = bridge.solicitar_reserva(
        "A1", "L201", "2026-10-20", "14:00", "15:00", 5, ESTUDO,
        agora_override=("2026-10-20", "08:00"),
    )
    assert id_estudo == 1

    # Professor solicita prova com >= 2h de antecedência -> substitui a reserva do aluno
    id_prova = bridge.solicitar_reserva(
        "P1", "L201", "2026-10-20", "14:00", "15:00", 20, PROVA,
        agora_override=("2026-10-20", "12:00"),
    )
    assert id_prova == 2

    reservas_a1 = bridge.listar_reservas_usuario("A1")
    assert reservas_a1 == [{"id": 1, "matricula": "A1", "cod_sala": "L201",
                           "data": "20/10/2026", "inicio": "14:00", "fim": "15:00",
                           "qtd_pessoas": 5, "finalidade": "ESTUDO", "situacao": "CANCELADA"}]

    # Professor cancela a própria reserva
    assert bridge.cancelar_reserva(id_prova, "P1") == "Reserva cancelada."
    reservas_p1 = bridge.listar_reservas_usuario("P1")
    assert reservas_p1[0]["situacao"] == "CANCELADA"


def preparar(bridge):
    bridge.cadastrar_usuario("A1", "Alice", "alice@example.test", ALUNO)
    bridge.cadastrar_usuario("P1", "Professor", "p1@example.test", PROFESSOR)
    bridge.cadastrar_sala("L201", COMUM, 40, 1)