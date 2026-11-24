"""Testes HTTP do módulo de salas com a biblioteca C real."""
import shutil

import pytest

from Frontend.app import create_app
from Frontend.backend import backend
from Frontend.bridge import BackendError
from Frontend.bridge.backend_bridge import caminho_biblioteca
from Frontend.bridge.c_types import ALUNO, COMUM, ESTUDO, LABORATORIO

CADASTRO = '/salas/cadastrar'
LISTA = '/salas/listar'
BIOMETRIA = '/salas/biometria'
DADOS = {'codigo': 'L201', 'tipo': '0', 'capacidade': '40', 'tem_quadro': '1'}


@pytest.fixture
def app(tmp_path):
    original = caminho_biblioteca()
    isolada = tmp_path / original.name
    shutil.copy2(original, isolada)
    return create_app({'TESTING': True, 'BACKEND_LIBRARY': isolada})


@pytest.fixture
def client(app):
    return app.test_client()


def enviar(client, rota, dados):
    assert client.get(rota).status_code == 200
    with client.session_transaction() as sessao:
        token = sessao['csrf_token']
    return client.post(rota, data={'csrf_token': token, **dados})


def test_menu_salas(client):
    assert '/salas/' in client.get('/').get_data(as_text=True)
    resposta = client.get('/salas/')
    assert resposta.status_code == 200
    for rota in (CADASTRO, LISTA, BIOMETRIA):
        assert rota in resposta.get_data(as_text=True)


def test_formularios(client):
    for rota, campos in [(CADASTRO, DADOS), (BIOMETRIA, ('codigo', 'mat_monitor'))]:
        resposta = client.get(rota)
        assert resposta.status_code == 200
        html = resposta.get_data(as_text=True)
        for campo in (*campos, 'csrf_token'):
            assert f'name="{campo}"' in html
        with client.session_transaction() as sessao:
            assert sessao['csrf_token'] in html


@pytest.mark.parametrize('tipo, quadro, rotulo', [('0', '1', 'Comum'), ('1', '0', 'Laboratório')])
def test_cadastrar_sala(app, client, tipo, quadro, rotulo):
    resposta = enviar(client, CADASTRO, {**DADOS, 'tipo': tipo, 'tem_quadro': quadro})
    assert resposta.status_code == 303
    assert resposta.headers['Location'].endswith(LISTA)
    html = client.get(resposta.headers['Location']).get_data(as_text=True)
    assert 'Sala cadastrada.' in html and 'L201' in html and rotulo in html
    with app.app_context(), backend() as bridge:
        assert bridge.listar_salas() == [dict(codigo='L201', tipo=rotulo, capacidade=40, tem_quadro=quadro == '1')]
    assert 'id="total-salas">1</strong>' in client.get('/').get_data(as_text=True)


def test_sala_duplicada(app, client):
    assert enviar(client, CADASTRO, DADOS).status_code == 303
    resposta = enviar(client, CADASTRO, {**DADOS, 'capacidade': '80'})
    assert resposta.status_code == 409
    assert 'Sala já cadastrada.' in resposta.get_data(as_text=True)
    with app.app_context(), backend() as bridge:
        assert bridge.listar_salas()[0]['capacidade'] == 40


@pytest.mark.parametrize('alteracao, mensagem', [
    ({'codigo': 'abc'}, 'Código da sala inválido.'),
    ({'capacidade': '0'}, 'A capacidade deve ser maior que zero.'),
    ({'capacidade': 'abc'}, 'Informe valores válidos'),
    ({'tipo': '2'}, 'Informe valores válidos'),
    ({'tem_quadro': '2'}, 'Informe valores válidos'),
])
def test_cadastro_invalido(app, client, alteracao, mensagem):
    resposta = enviar(client, CADASTRO, {**DADOS, **alteracao})
    assert resposta.status_code == 400
    assert mensagem in resposta.get_data(as_text=True)
    with app.app_context(), backend() as bridge:
        assert bridge.listar_salas() == []


def test_listagem_vazia(client):
    resposta = client.get(LISTA)
    assert resposta.status_code == 200
    assert 'Nenhuma sala encontrada.' in resposta.get_data(as_text=True)


def test_filtros(app, client):
    with app.app_context(), backend() as bridge:
        bridge.cadastrar_sala('L201', COMUM, 40, 1)
        bridge.cadastrar_sala('L301', LABORATORIO, 30, 0)
        bridge.cadastrar_sala('L401', COMUM, 10, 1)
    html = client.get(LISTA).get_data(as_text=True)
    assert '<td>L201</td><td>Comum</td><td>40</td><td>Sim</td>' in html
    assert '<td>L301</td><td>Laboratório</td><td>30</td><td>Não</td>' in html
    assert 'L401' in html
    for pessoas, quadro, esperadas in [(30, 0, ('L201', 'L301')), (30, 1, ('L201',)), (41, 0, ())]:
        resposta = client.get(LISTA, query_string={'qtd_pessoas': pessoas, 'precisa_quadro': quadro})
        assert resposta.status_code == 200
        html = resposta.get_data(as_text=True)
        for codigo in ('L201', 'L301', 'L401'):
            assert (f'<td>{codigo}</td>' in html) == (codigo in esperadas)


@pytest.mark.parametrize('filtros', [{'qtd_pessoas': '0'}, {'qtd_pessoas': 'abc'}, {'precisa_quadro': '2'}])
def test_filtro_invalido(client, filtros):
    assert client.get(LISTA, query_string=filtros).status_code == 400


def preparar_laboratorio(app):
    with app.app_context(), backend() as bridge:
        bridge.cadastrar_usuario('A1', 'Alice', 'a1@example.test', ALUNO)
        bridge.cadastrar_usuario('M1', 'Monitor', 'm1@example.test', ALUNO)
        bridge.cadastrar_sala('L301', LABORATORIO, 30, 0)
        bridge.cadastrar_sala('L201', COMUM, 40, 1)


def reservar_laboratorio(bridge):
    return bridge.solicitar_reserva('A1', 'L301', '2026-10-20', '14:00', '15:00', 5,
                                    ESTUDO, mat_monitor='M1', agora_override=('2026-10-20', '08:00'))


def test_biometria_habilita_reserva(app, client):
    preparar_laboratorio(app)
    with app.app_context(), backend() as bridge:
        with pytest.raises(BackendError) as exc:
            reservar_laboratorio(bridge)
        assert exc.value.codigo == 4
    dados = {'codigo': 'L301', 'mat_monitor': 'M1'}
    resposta = enviar(client, BIOMETRIA, dados)
    assert resposta.status_code == 303
    assert 'Biometria registrada.' in client.get(resposta.headers['Location']).get_data(as_text=True)
    # O C permite registrar novamente a mesma associação (idempotência).
    assert enviar(client, BIOMETRIA, dados).status_code == 303
    with app.app_context(), backend() as bridge:
        assert reservar_laboratorio(bridge) == 1


@pytest.mark.parametrize('codigo, monitor, status, mensagem', [
    ('L301', 'X1', 404, 'Monitor não cadastrado.'),
    ('L999', 'M1', 404, 'Sala não cadastrada.'),
    ('L201', 'M1', 400, 'Dados inválidos para a sala ou para o laboratório.'),
])
def test_biometria_invalida(app, client, codigo, monitor, status, mensagem):
    preparar_laboratorio(app)
    resposta = enviar(client, BIOMETRIA, {'codigo': codigo, 'mat_monitor': monitor})
    assert resposta.status_code == status
    assert mensagem in resposta.get_data(as_text=True)
    with app.app_context(), backend() as bridge:
        with pytest.raises(BackendError) as exc:
            reservar_laboratorio(bridge)
        assert exc.value.codigo == 4


@pytest.mark.parametrize('rota, dados', [(CADASTRO, DADOS), (BIOMETRIA, {'codigo': 'L301', 'mat_monitor': 'M1'})])
def test_csrf_invalido(app, client, rota, dados):
    preparar_laboratorio(app)
    if rota == CADASTRO:
        dados = {**dados, 'codigo': 'L501'}
    assert client.post(rota, data=dados).status_code == 400
    assert enviar(client, rota, {**dados, 'csrf_token': 'incorreto'}).status_code == 400
    with app.app_context(), backend() as bridge:
        assert [s['codigo'] for s in bridge.listar_salas()] == ['L301', 'L201']
        with pytest.raises(BackendError) as exc:
            reservar_laboratorio(bridge)
        assert exc.value.codigo == 4
