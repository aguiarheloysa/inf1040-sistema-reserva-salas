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


def reservar(bridge, matricula="A1", sala="L201", finalidade=ESTUDO, agora="08:00", **kwargs):
    return bridge.solicitar_reserva(matricula, sala, "2026-10-20", "14:00", "15:00", 5,
                                    finalidade, agora_override=("2026-10-20", agora), **kwargs)


def test_listas_vazias_e_usuario_inexistente(bridge):
    assert bridge.listar_todas_salas() == []
    with pytest.raises(BackendError, match="Usuário não cadastrado"):
        bridge.listar_reservas_usuario("X")
    with pytest.raises(BackendError, match="Usuário não cadastrado"):
        bridge.buscar_salas_livres("X", "2026-10-20", "14:00", "15:00", 5, 0)
    preparar(bridge)
    assert bridge.listar_reservas_usuario("A1") == []
    assert bridge.buscar_salas_livres("A1", "2026-10-20", "14:00", "15:00", 41, 0) == []


def test_grade_exige_sala_e_professor(bridge):
    preparar(bridge)
    with pytest.raises(BackendError, match="Sala não cadastrada"):
        bridge.cadastrar_aula("L999", "Modular", "P1", 1, "09:00", "11:00")
    for matricula in ("A1", "P2"):
        with pytest.raises(BackendError, match="professor cadastrado"):
            bridge.cadastrar_aula("L201", "Modular", matricula, 1, "09:00", "11:00")
    bridge.cadastrar_aula("L201", "Modular", "P1", 1, "09:00", "11:00")
    with pytest.raises(BackendError, match="professor cadastrado"):
        bridge.autorizar_prova("L201", "2026-10-19", "09:00", "P1", "A1")


def test_laboratorio_exige_monitor_com_biometria(bridge):
    preparar(bridge)
    bridge.cadastrar_sala("L301", LABORATORIO, 30, 0)
    with pytest.raises(BackendError, match="Monitor não cadastrado"):
        bridge.registrar_biometria("L301", "M1")
    bridge.cadastrar_usuario("M1", "Monitor", "m1@example.test", ALUNO)
    with pytest.raises(BackendError) as exc:
        reservar(bridge, sala="L301", mat_monitor="M1")
    assert exc.value.codigo == 4
    bridge.registrar_biometria("L301", "M1")
    assert reservar(bridge, sala="L301", mat_monitor="M1") == 1
    assert bridge.buscar_salas_livres("A1", "2026-10-21", "14:00", "15:00", 5, 0) == ["L201"]
    assert bridge.buscar_salas_livres("P1", "2026-10-21", "14:00", "15:00", 5, 0) == ["L201", "L301"]


def test_prioridade_insuficiente_e_cancelamento_por_terceiro(bridge):
    preparar(bridge)
    id_reserva = reservar(bridge)
    with pytest.raises(BackendError) as exc:
        reservar(bridge, matricula="P1", finalidade=PROVA, agora="12:01")
    assert exc.value.codigo == 6
    with pytest.raises(BackendError) as exc:
        bridge.cancelar_reserva(id_reserva, "P1")
    assert exc.value.codigo == 8
    assert bridge.listar_reservas_usuario("A1")[0]["situacao"] == "ATIVA"
    assert bridge.listar_reservas_usuario("P1") == []
    assert bridge.buscar_salas_livres("P1", "2026-10-20", "14:00", "15:00", 5, 0) == []
    assert reservar(bridge, matricula="P1", finalidade=PROVA, agora="12:00") == 2


def test_prova_em_horario_de_aula(bridge):
    preparar(bridge)
    bridge.cadastrar_usuario("P2", "Professor 2", "p2@example.test", PROFESSOR)
    bridge.cadastrar_aula("L201", "Modular", "P1", 2, "14:00", "15:00")
    for matricula, finalidade in (("A1", ESTUDO), ("P2", PROVA)):
        with pytest.raises(BackendError) as exc:
            reservar(bridge, matricula=matricula, finalidade=finalidade)
        assert exc.value.codigo == 3
    bridge.autorizar_prova("L201", "2026-10-20", "14:00", "P1", "P2")
    assert reservar(bridge, matricula="P2", finalidade=PROVA) == 1


@pytest.mark.parametrize("data,inicio,fim,pessoas,quadro", [
    ("2026-02-30", "14:00", "15:00", 5, 0),
    ("2026-10-20", "15:00", "14:00", 5, 0),
    ("2026-10-20", "14:00", "15:00", 0, 0),
    ("2026-10-20", "14:00", "15:00", 5, 2),
])
def test_busca_invalida(bridge, data, inicio, fim, pessoas, quadro):
    preparar(bridge)
    with pytest.raises(BackendError) as exc:
        bridge.buscar_salas_livres("A1", data, inicio, fim, pessoas, quadro)
    assert exc.value.codigo == 12


def test_conversoes_invalidas(bridge):
    preparar(bridge)
    with pytest.raises(ValueError, match="intervalo"):
        bridge.cadastrar_sala("L202", COMUM, 2**40, 0)
    with pytest.raises(ValueError, match="NUL"):
        bridge.consultar_usuario("A1\x00outra")
    with pytest.raises(BackendError) as exc:
        reservar(bridge, agora="14:00:00")
    assert exc.value.codigo == 12
    assert bridge.listar_reservas_usuario("A1") == []


def test_estado_isolado(bridge):
    assert bridge.listar_todas_salas() == []
    with pytest.raises(BackendError, match="Usuário não cadastrado"):
        bridge.consultar_usuario("A1")


def test_listar_salas_com_filtros_e_quadro(bridge):
    assert bridge.listar_salas() == []
    bridge.cadastrar_sala('L201', COMUM, 40, 1)
    bridge.cadastrar_sala('L301', LABORATORIO, 30, 0)
    bridge.cadastrar_sala('L401', COMUM, 10, 1)
    assert bridge.listar_salas() == [
        {'codigo': 'L201', 'tipo': 'Comum', 'capacidade': 40, 'tem_quadro': True},
        {'codigo': 'L301', 'tipo': 'Laboratório', 'capacidade': 30, 'tem_quadro': False},
        {'codigo': 'L401', 'tipo': 'Comum', 'capacidade': 10, 'tem_quadro': True},
    ]
    assert [s['codigo'] for s in bridge.listar_salas(30, 0)] == ['L201', 'L301']
    assert [s['codigo'] for s in bridge.listar_salas(30, 1)] == ['L201']
    assert bridge.listar_salas(41, 0) == []
    assert bridge.listar_todas_salas() == [
        {'codigo': 'L201', 'tipo': 'Comum', 'capacidade': 40},
        {'codigo': 'L301', 'tipo': 'Laboratório', 'capacidade': 30},
        {'codigo': 'L401', 'tipo': 'Comum', 'capacidade': 10},
    ]


@pytest.mark.parametrize('pessoas, quadro', [(0, 0), (1, 2)])
def test_filtros_salas_invalidos(bridge, pessoas, quadro):
    with pytest.raises(BackendError, match='quantidade positiva'):
        bridge.listar_salas(pessoas, quadro)
