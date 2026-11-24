"""Integração HTTP com o C real. Cada caso prepara uma biblioteca isolada."""
import ctypes
import shutil
from datetime import datetime

import pytest

from Frontend.app import create_app
from Frontend.backend import backend
from Frontend.bridge import backend_bridge
from Frontend.bridge.backend_bridge import caminho_biblioteca
from Frontend.bridge.c_types import ALUNO, PROFESSOR, COMUM, LABORATORIO

BUSCAR = '/reservas/buscar'
SOLICITAR = '/reservas/solicitar'
LISTAR = '/reservas/listar'
CANCELAR = '/reservas/cancelar'
DATA = '2026-10-20'  # terça-feira
PEDIDO = dict(matricula='A1', cod_sala='L201', data=DATA, inicio='14:00', fim='16:00',
              qtd_pessoas='5', finalidade='0', mat_monitor='')
BUSCA = dict(matricula='A1', data=DATA, inicio='14:00', fim='16:00',
             qtd_pessoas='30', precisa_quadro='0')


@pytest.fixture
def app(tmp_path, monkeypatch):
    class Relogio(datetime):
        @classmethod
        def now(cls, tz=None):
            return cls(2026, 10, 20, 12, 0, tzinfo=tz)
    monkeypatch.setattr(backend_bridge, 'datetime', Relogio)
    original = caminho_biblioteca()
    isolada = tmp_path / original.name
    shutil.copy2(original, isolada)
    app = create_app({'TESTING': True, 'BACKEND_LIBRARY': isolada})
    with app.app_context(), backend() as bridge:
        for matricula, tipo in [('A1', ALUNO), ('A2', ALUNO), ('M1', ALUNO),
                                ('P1', PROFESSOR), ('P2', PROFESSOR)]:
            bridge.cadastrar_usuario(matricula, matricula, matricula+'@example.test', tipo)
        bridge.cadastrar_sala('L201', COMUM, 40, 1)
        bridge.cadastrar_sala('L105', COMUM, 10, 0)
        bridge.cadastrar_sala('L301', LABORATORIO, 30, 0)
    return app


@pytest.fixture
def client(app):
    return app.test_client()


def enviar(client, rota, dados):
    assert client.get(rota).status_code == 200
    with client.session_transaction() as sessao:
        token = sessao['csrf_token']
    return client.post(rota, data={'csrf_token': token, **dados})


def solicitar(client, **alteracoes):
    return enviar(client, SOLICITAR, {**PEDIDO, **alteracoes})


def historico(app, matricula):
    with app.app_context(), backend() as bridge:
        return bridge.listar_reservas_usuario(matricula)


def aula(app):
    with app.app_context(), backend() as bridge:
        bridge.cadastrar_aula('L201', 'Cálculo', 'P1', 2, '14:00', '15:00')


def autorizar(app):
    with app.app_context(), backend() as bridge:
        bridge.autorizar_prova('L201', DATA, '14:00', 'P1', 'P2')


def test_menu(client):
    assert '/reservas/' in client.get('/').get_data(as_text=True)
    html = client.get('/reservas/').get_data(as_text=True)
    for rota in (BUSCAR, SOLICITAR, LISTAR, CANCELAR):
        assert rota in html


@pytest.mark.parametrize('rota,campos', [(BUSCAR, BUSCA), (SOLICITAR, PEDIDO),
    (LISTAR, {'matricula': ''}), (CANCELAR, {'id_reserva': '', 'matricula': ''})])
def test_formularios(client, rota, campos):
    r = client.get(rota)
    assert r.status_code == 200
    html = r.get_data(as_text=True)
    for campo in campos:
        assert f'name="{campo}"' in html
    if rota in (SOLICITAR, CANCELAR):
        assert 'name="csrf_token"' in html


@pytest.mark.parametrize('matricula,quadro,esperadas', [
    ('A1', '0', {'L201'}), ('P1', '0', {'L201', 'L301'}), ('P1', '1', {'L201'})])
def test_buscar(client, matricula, quadro, esperadas):
    r = client.get(BUSCAR, query_string={**BUSCA, 'matricula': matricula, 'precisa_quadro': quadro})
    assert r.status_code == 200
    html = r.get_data(as_text=True)
    assert f'Salas livres: {len(esperadas)}' in html
    for codigo in ('L201', 'L105', 'L301'):
        assert (f'cod_sala={codigo}' in html) == (codigo in esperadas)


def test_busca_exclui_aula(app, client):
    aula(app)
    r = client.get(BUSCAR, query_string=BUSCA)
    assert 'Nenhuma sala disponível.' in r.get_data(as_text=True)
    assert 'Solicitar diretamente' in r.get_data(as_text=True)


def test_busca_exclui_reserva(client):
    assert solicitar(client).status_code == 303
    r = client.get(BUSCAR, query_string=BUSCA)
    assert 'Nenhuma sala disponível.' in r.get_data(as_text=True)


@pytest.mark.parametrize('mudanca,status', [({'matricula':'X1'},404),
    ({'qtd_pessoas':'0'},400), ({'precisa_quadro':'2'},400), ({'data':'2026-02-30'},400)])
def test_busca_invalida(client, mudanca, status):
    assert client.get(BUSCAR, query_string={**BUSCA, **mudanca}).status_code == status


def test_prefill_sem_mutacao(app, client):
    r = client.get(SOLICITAR, query_string=PEDIDO)
    assert r.status_code == 200
    assert 'value="L201"' in r.get_data(as_text=True)
    assert historico(app, 'A1') == []


def test_solicitar_e_historico(app, client):
    r = solicitar(client)
    assert r.status_code == 303
    assert r.headers['Location'].endswith('/reservas/listar?matricula=A1')
    html = client.get(r.headers['Location']).get_data(as_text=True)
    assert 'Reserva realizada. ID: 1.' in html
    assert 'ATIVA' in html and '20/10/2026' in html and 'ESTUDO' in html
    assert historico(app, 'A1')[0]['situacao'] == 'ATIVA'
    assert client.get(r.headers['Location']).status_code == 200
    assert len(historico(app, 'A1')) == 1


@pytest.mark.parametrize('mudanca,status,mensagem', [
    ({'matricula':'X1'},404,'Usuário não cadastrado'),
    ({'cod_sala':'L999'},404,'Sala não cadastrada'),
    ({'qtd_pessoas':'41'},400,'excede a capacidade'),
    ({'qtd_pessoas':'0'},400,'Dados da reserva inválidos'),
    ({'qtd_pessoas':'abc'},400,'número válido'),
    ({'data':'2026-02-30'},400,'Dados da reserva inválidos'),
    ({'fim':'13:00'},400,'Dados da reserva inválidos'),
    ({'finalidade':'9'},400,'Dados da reserva inválidos'),
])
def test_pedido_invalido(app, client, mudanca, status, mensagem):
    r = solicitar(client, **mudanca)
    assert r.status_code == status
    assert mensagem in r.get_data(as_text=True)
    assert historico(app, 'A1') == []


@pytest.mark.parametrize('matricula,finalidade,autorizacao,status', [
    ('A1','0',False,409), ('P1','1',False,303), ('P2','1',False,409),
    ('P2','1',True,303), ('P2','0',True,409), ('P1','0',False,409)])
def test_regra_horario_aula(app, client, matricula, finalidade, autorizacao, status):
    aula(app)
    if autorizacao:
        autorizar(app)
    r = solicitar(client, matricula=matricula, finalidade=finalidade)
    assert r.status_code == status
    assert len(historico(app, matricula)) == (1 if status == 303 else 0)
    if status == 409:
        assert 'Horário de aula' in r.get_data(as_text=True)


@pytest.mark.parametrize('matricula,monitor,registrar,status', [
    ('A1','',False,409), ('A1','M1',False,409), ('A1','M1',True,303),
    ('A1','A2',True,409), ('P1','',False,303)])
def test_laboratorio(app, client, matricula, monitor, registrar, status):
    if registrar:
        with app.app_context(), backend() as bridge:
            bridge.registrar_biometria('L301', 'M1')
    r = solicitar(client, matricula=matricula, cod_sala='L301', mat_monitor=monitor)
    assert r.status_code == status
    assert len(historico(app, matricula)) == (1 if status == 303 else 0)


@pytest.mark.parametrize('titular,solicitante,finalidade,status', [
    ('A1','A2','0',409), ('A1','P1','0',409), ('P1','P2','1',409),
    ('P1','A1','0',409), ('A1','P1','1',303)])
def test_prioridade(app, client, titular, solicitante, finalidade, status):
    assert solicitar(client, matricula=titular).status_code == 303
    r = solicitar(client, matricula=solicitante, finalidade=finalidade)
    assert r.status_code == status
    assert historico(app, titular)[0]['situacao'] == ('CANCELADA' if status == 303 else 'ATIVA')
    assert len(historico(app, solicitante)) == (1 if status == 303 else 0)


def test_antecedencia_insuficiente(app, client):
    assert solicitar(client, inicio='13:59').status_code == 303
    r = solicitar(client, matricula='P1', finalidade='1', inicio='13:59')
    assert r.status_code == 409 and 'duas horas' in r.get_data(as_text=True)
    assert historico(app, 'A1')[0]['situacao'] == 'ATIVA'
    assert historico(app, 'P1') == []


def test_horarios_adjacentes(app, client):
    assert solicitar(client).status_code == 303
    assert solicitar(client, matricula='A2', inicio='16:00', fim='17:00').status_code == 303
    assert historico(app, 'A1')[0]['situacao'] == 'ATIVA'


def test_cancelar_titular(app, client):
    assert solicitar(client).status_code == 303
    r = enviar(client, CANCELAR, {'id_reserva':'1', 'matricula':'A1'})
    assert r.status_code == 303
    assert 'Reserva cancelada.' in client.get(r.headers['Location']).get_data(as_text=True)
    assert historico(app, 'A1')[0]['situacao'] == 'CANCELADA'
    assert enviar(client, CANCELAR, {'id_reserva':'1', 'matricula':'A1'}).status_code == 404
    assert solicitar(client, matricula='A2').status_code == 303


def test_cancelar_terceiro(app, client):
    assert solicitar(client).status_code == 303
    r = enviar(client, CANCELAR, {'id_reserva':'1', 'matricula':'P1'})
    assert r.status_code == 403
    assert 'Apenas o titular' in r.get_data(as_text=True)
    assert historico(app, 'A1')[0]['situacao'] == 'ATIVA'


def test_cancelar_id_inexistente(client):
    assert enviar(client, CANCELAR, {'id_reserva':'9999', 'matricula':'A1'}).status_code == 404


def test_historico_vazio_e_inexistente(client):
    r = client.get(LISTAR, query_string={'matricula':'A2'})
    assert r.status_code == 200 and 'Nenhuma reserva' in r.get_data(as_text=True)
    assert client.get(LISTAR, query_string={'matricula':'X1'}).status_code == 404


@pytest.mark.parametrize('rota,dados', [(SOLICITAR,PEDIDO), (CANCELAR,{'id_reserva':'1','matricula':'A1'})])
def test_csrf(app, client, rota, dados):
    if rota == CANCELAR:
        assert solicitar(client).status_code == 303
    assert client.post(rota, data=dados).status_code == 400
    assert enviar(client, rota, {**dados, 'csrf_token':'invalido'}).status_code == 400
    estado = historico(app, 'A1')
    assert [r['situacao'] for r in estado] == (['ATIVA'] if rota == CANCELAR else [])


def test_falha_notificacao_preserva_estado(app, client):
    # Controla somente o canal público de entrega. Reserva, Regras e demais módulos são reais.
    assert solicitar(client).status_code == 303
    lib = ctypes.CDLL(str(app.config['BACKEND_LIBRARY']))
    callback_type = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_int)
    falhar = callback_type(lambda entregas, quantidade: 0)
    definir = lib.not_definirCanalEntrega
    definir.argtypes = [ctypes.c_void_p]
    definir.restype = None
    definir(ctypes.cast(falhar, ctypes.c_void_p))
    try:
        r = solicitar(client, matricula='P1', finalidade='1')
        assert r.status_code == 503 and 'Falha ao enviar' in r.get_data(as_text=True)
        assert historico(app, 'A1')[0]['situacao'] == 'ATIVA'
        assert historico(app, 'P1') == []
    finally:
        definir(None)


def test_fluxo_http_entre_modulos(app, client):
    # Novo professor e sala entram pelo HTTP, assim como aula e autorização.
    assert enviar(client, '/usuarios/cadastrar', dict(matricula='P3', nome='Prof 3',
        email='p3@example.test', tipo='1')).status_code == 303
    assert enviar(client, '/salas/cadastrar', dict(codigo='L402', tipo='0',
        capacidade='25', tem_quadro='1')).status_code == 303
    assert enviar(client, '/grade/cadastrar', dict(cod_sala='L402', disciplina='INF1040',
        mat_professor='P3', dia_semana='2', inicio='14:00', fim='15:00')).status_code == 303
    assert solicitar(client, matricula='P2', cod_sala='L402', finalidade='1').status_code == 409
    assert enviar(client, '/grade/autorizar', dict(cod_sala='L402', data=DATA, hora='14:00',
        mat_ministrante='P3', mat_solicitante='P2')).status_code == 303
    assert solicitar(client, matricula='P2', cod_sala='L402', finalidade='1').status_code == 303
    assert historico(app, 'P2')[0]['cod_sala'] == 'L402'
    assert enviar(client, CANCELAR, {'id_reserva':'1', 'matricula':'P2'}).status_code == 303
    assert historico(app, 'P2')[0]['situacao'] == 'CANCELADA'


def test_conflitos_multiplos_sem_cancelamento_parcial(app, client):
    assert solicitar(client, fim='15:00').status_code == 303
    assert solicitar(client, matricula='P2', inicio='15:00').status_code == 303
    r = solicitar(client, matricula='P1', finalidade='1')
    assert r.status_code == 409
    assert historico(app, 'A1')[0]['situacao'] == 'ATIVA'
    assert historico(app, 'P2')[0]['situacao'] == 'ATIVA'
    assert historico(app, 'P1') == []
