"""Testes HTTP de grade e autorizações usando a biblioteca C real."""
import shutil

import pytest

from Frontend.app import create_app
from Frontend.backend import backend
from Frontend.bridge import BackendError
from Frontend.bridge.backend_bridge import caminho_biblioteca
from Frontend.bridge.c_types import ALUNO, COMUM, ESTUDO, PROVA, PROFESSOR

CADASTRO = '/grade/cadastrar'
AUTORIZACAO = '/grade/autorizar'
AULA = dict(cod_sala='L201', disciplina='Programação Modular', mat_professor='P1',
            dia_semana='2', inicio='14:00', fim='15:00')
AUTORIZAR = dict(cod_sala='L201', data='2026-10-20', hora='14:00',
                 mat_ministrante='P1', mat_solicitante='P2')


@pytest.fixture
def app(tmp_path):
    original = caminho_biblioteca()
    isolada = tmp_path / original.name
    shutil.copy2(original, isolada)
    app = create_app({'TESTING': True, 'BACKEND_LIBRARY': isolada})
    with app.app_context(), backend() as bridge:
        bridge.cadastrar_sala('L201', COMUM, 40, 1)
        bridge.cadastrar_usuario('A1', 'Alice', 'a1@example.test', ALUNO)
        bridge.cadastrar_usuario('P1', 'Professor 1', 'p1@example.test', PROFESSOR)
        bridge.cadastrar_usuario('P2', 'Professor 2', 'p2@example.test', PROFESSOR)
    return app


@pytest.fixture
def client(app):
    return app.test_client()


def enviar(client, rota, dados):
    assert client.get(rota).status_code == 200
    with client.session_transaction() as sessao:
        token = sessao['csrf_token']
    return client.post(rota, data={'csrf_token': token, **dados})


def reservar(bridge, matricula='P2', finalidade=PROVA, data='2026-10-20',
             inicio='14:00', fim='15:00'):
    return bridge.solicitar_reserva(matricula, 'L201', data, inicio, fim, 5,
                                    finalidade, agora_override=(data, '08:00'))


def rejeitar_prova(bridge, **kwargs):
    with pytest.raises(BackendError) as exc:
        reservar(bridge, **kwargs)
    assert exc.value.codigo == 3


def test_menu(client):
    assert '/grade/' in client.get('/').get_data(as_text=True)
    resposta = client.get('/grade/')
    assert resposta.status_code == 200
    assert CADASTRO in resposta.get_data(as_text=True)
    assert AUTORIZACAO in resposta.get_data(as_text=True)


@pytest.mark.parametrize('rota, campos', [(CADASTRO, AULA), (AUTORIZACAO, AUTORIZAR)])
def test_formularios(client, rota, campos):
    resposta = client.get(rota)
    assert resposta.status_code == 200
    html = resposta.get_data(as_text=True)
    for campo in (*campos, 'csrf_token'):
        assert f'name="{campo}"' in html
    with client.session_transaction() as sessao:
        assert sessao['csrf_token'] in html
    if rota == CADASTRO:
        for dia in range(1, 8):
            assert f'value="{dia}"' in html


def test_cadastrar_aula(app, client):
    resposta = enviar(client, CADASTRO, AULA)
    assert resposta.status_code == 303
    assert resposta.headers['Location'].endswith(CADASTRO)
    assert 'Aula cadastrada.' in client.get(resposta.headers['Location']).get_data(as_text=True)
    with app.app_context(), backend() as bridge:
        rejeitar_prova(bridge, matricula='A1', finalidade=ESTUDO)
        assert bridge.buscar_salas_livres('A1', '2026-10-27', '14:00', '15:00', 5, 0) == []
        assert bridge.buscar_salas_livres('A1', '2026-10-21', '14:00', '15:00', 5, 0) == ['L201']


def test_conflito_e_horarios_adjacentes(client):
    assert enviar(client, CADASTRO, AULA).status_code == 303
    resposta = enviar(client, CADASTRO, {**AULA, 'inicio': '14:30', 'fim': '15:30'})
    assert resposta.status_code == 409
    assert 'Já existe uma aula' in resposta.get_data(as_text=True)
    assert enviar(client, CADASTRO, {**AULA, 'inicio': '15:00', 'fim': '16:00'}).status_code == 303


@pytest.mark.parametrize('alteracoes, mensagem', [
    ({'cod_sala': 'L999'}, 'Sala não cadastrada.'),
    ({'mat_professor': 'X1'}, 'professor cadastrado'),
    ({'mat_professor': 'A1'}, 'professor cadastrado'),
    ({'dia_semana': '8'}, 'horário de aula inválido'),
    ({'dia_semana': 'abc'}, 'dia da semana válido'),
    ({'inicio': '25:00'}, 'horário de aula inválido'),
    ({'fim': '13:00'}, 'horário de aula inválido'),
    ({'disciplina': ''}, 'Dados da grade inválidos'),
])
def test_cadastro_invalido(app, client, alteracoes, mensagem):
    resposta = enviar(client, CADASTRO, {**AULA, **alteracoes})
    assert resposta.status_code == 400
    assert mensagem in resposta.get_data(as_text=True)
    # A tentativa inválida não pode ocupar o horário.
    with app.app_context(), backend() as bridge:
        assert bridge.buscar_salas_livres('A1', '2026-10-20', '14:00', '15:00', 5, 0) == ['L201']


def test_autorizacao_permite_prova(app, client):
    assert enviar(client, CADASTRO, AULA).status_code == 303
    with app.app_context(), backend() as bridge:
        rejeitar_prova(bridge)
    resposta = enviar(client, AUTORIZACAO, AUTORIZAR)
    assert resposta.status_code == 303
    assert resposta.headers['Location'].endswith(AUTORIZACAO)
    assert 'Autorização registrada.' in client.get(resposta.headers['Location']).get_data(as_text=True)
    with app.app_context(), backend() as bridge:
        assert bridge.listar_reservas_usuario('P2') == []
        rejeitar_prova(bridge, data='2026-10-27')
        rejeitar_prova(bridge, inicio='14:15')
        rejeitar_prova(bridge, matricula='A1', finalidade=ESTUDO)
        assert reservar(bridge) == 1


@pytest.mark.parametrize('alteracoes, mensagem', [
    ({'mat_ministrante': 'P2'}, 'Apenas o ministrante'),
    ({'mat_solicitante': 'A1'}, 'professor cadastrado'),
    ({'mat_solicitante': 'X1'}, 'professor cadastrado'),
    ({'cod_sala': 'L999'}, 'Sala não cadastrada.'),
    ({'hora': '15:00'}, 'Nenhuma aula'),
    ({'data': '2026-02-30'}, 'Nenhuma aula'),
    ({'data': '2026-10-21'}, 'Nenhuma aula'),
])
def test_autorizacao_invalida(app, client, alteracoes, mensagem):
    assert enviar(client, CADASTRO, AULA).status_code == 303
    resposta = enviar(client, AUTORIZACAO, {**AUTORIZAR, **alteracoes})
    assert resposta.status_code == 400
    assert mensagem in resposta.get_data(as_text=True)
    with app.app_context(), backend() as bridge:
        rejeitar_prova(bridge)


def test_autorizacao_sem_aula(client):
    resposta = enviar(client, AUTORIZACAO, AUTORIZAR)
    assert resposta.status_code == 400
    assert 'Nenhuma aula' in resposta.get_data(as_text=True)


def test_autorizacao_repetida(client):
    assert enviar(client, CADASTRO, AULA).status_code == 303
    assert enviar(client, AUTORIZACAO, AUTORIZAR).status_code == 303
    assert enviar(client, AUTORIZACAO, AUTORIZAR).status_code == 303


def test_prova_sobrepoe_duas_aulas(app, client):
    assert enviar(client, CADASTRO, AULA).status_code == 303
    assert enviar(client, CADASTRO, {**AULA, 'inicio': '15:00', 'fim': '16:00'}).status_code == 303
    assert enviar(client, AUTORIZACAO, AUTORIZAR).status_code == 303
    with app.app_context(), backend() as bridge:
        rejeitar_prova(bridge, fim='16:00')
    assert enviar(client, AUTORIZACAO, {**AUTORIZAR, 'hora': '15:00'}).status_code == 303
    with app.app_context(), backend() as bridge:
        assert reservar(bridge, fim='16:00') == 1


@pytest.mark.parametrize('rota, dados', [(CADASTRO, AULA), (AUTORIZACAO, AUTORIZAR)])
def test_csrf(app, client, rota, dados):
    if rota == AUTORIZACAO:
        assert enviar(client, CADASTRO, AULA).status_code == 303
    assert client.post(rota, data=dados).status_code == 400
    assert enviar(client, rota, {**dados, 'csrf_token': 'incorreto'}).status_code == 400
    with app.app_context(), backend() as bridge:
        if rota == CADASTRO:
            assert bridge.buscar_salas_livres('A1', '2026-10-20', '14:00', '15:00', 5, 0) == ['L201']
        else:
            rejeitar_prova(bridge)


def test_preservacao_escape(client):
    resposta = enviar(client, CADASTRO, {**AULA, 'disciplina': '<script>alert(1)</script>', 'fim': '13:00'})
    assert resposta.status_code == 400
    html = resposta.get_data(as_text=True)
    assert '&lt;script&gt;alert(1)&lt;/script&gt;' in html
    assert '<script>alert(1)</script>' not in html
    assert 'value="P1"' in html
