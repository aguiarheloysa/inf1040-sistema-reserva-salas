"""Testes HTTP de usuários, com uma biblioteca C isolada por teste."""
import shutil

import pytest

from Frontend.app import create_app
from Frontend.backend import backend
from Frontend.bridge import BackendError
from Frontend.bridge.backend_bridge import caminho_biblioteca

CADASTRO = "/usuarios/cadastrar"
CONSULTA = "/usuarios/consultar"
DADOS = {"matricula": "A1", "nome": "Alice", "email": "alice@example.test", "tipo": "0"}


@pytest.fixture
def app(tmp_path):
    original = caminho_biblioteca()
    isolada = tmp_path / original.name
    shutil.copy2(original, isolada)
    return create_app({"TESTING": True, "BACKEND_LIBRARY": isolada})


@pytest.fixture
def client(app):
    return app.test_client()


def cadastrar(client, **alteracoes):
    client.get(CADASTRO)
    with client.session_transaction() as sessao:
        token = sessao["csrf_token"]
    return client.post(CADASTRO, data={**DADOS, "csrf_token": token, **alteracoes})


def test_menu_usuarios(client):
    assert '/usuarios/' in client.get('/').get_data(as_text=True)
    resposta = client.get('/usuarios/')
    assert resposta.status_code == 200
    assert CADASTRO in resposta.get_data(as_text=True)
    assert CONSULTA in resposta.get_data(as_text=True)


def test_formulario_cadastro(client):
    resposta = client.get(CADASTRO)
    assert resposta.status_code == 200
    html = resposta.get_data(as_text=True)
    for campo in ("matricula", "nome", "email", "tipo", "csrf_token"):
        assert f'name="{campo}"' in html
    with client.session_transaction() as sessao:
        assert sessao['csrf_token'] in html


@pytest.mark.parametrize(('tipo', 'rotulo'), [('0', 'Aluno'), ('1', 'Professor')])
def test_cadastrar_usuario(app, client, tipo, rotulo):
    resposta = cadastrar(client, tipo=tipo)
    assert resposta.status_code == 303
    assert resposta.headers['Location'].endswith(CADASTRO)
    assert 'Usuário cadastrado.' in client.get(resposta.headers['Location']).get_data(as_text=True)
    with app.app_context(), backend() as bridge:
        assert bridge.consultar_usuario('A1') == {'matricula': 'A1', 'tipo': rotulo, 'email': DADOS['email']}
    html = client.get(CONSULTA, query_string={'matricula': 'A1'}).get_data(as_text=True)
    assert rotulo in html and DADOS['email'] in html


def test_consulta_formulario(client):
    resposta = client.get(CONSULTA)
    assert resposta.status_code == 200
    assert 'Dados do usuário' not in resposta.get_data(as_text=True)


def test_consulta_usuario_inexistente(client):
    resposta = client.get(CONSULTA, query_string={'matricula': 'X1'})
    assert resposta.status_code == 404
    assert 'Usuário não cadastrado.' in resposta.get_data(as_text=True)


def test_consulta_matricula_vazia(client):
    resposta = client.get(CONSULTA, query_string={'matricula': ' '})
    assert resposta.status_code == 400
    assert 'Informe a matrícula.' in resposta.get_data(as_text=True)


def test_cadastro_duplicado(app, client):
    assert cadastrar(client).status_code == 303
    resposta = cadastrar(client, email='outro@example.test')
    assert resposta.status_code == 409
    assert 'Matrícula já cadastrada.' in resposta.get_data(as_text=True)
    with app.app_context(), backend() as bridge:
        assert bridge.consultar_usuario('A1')['email'] == DADOS['email']


@pytest.mark.parametrize('alteracoes', [
    {'email': 'invalido'}, {'nome': ''}, {'tipo': '2'}, {'matricula': 'X'*20},
])
def test_cadastro_invalido(app, client, alteracoes):
    resposta = cadastrar(client, **alteracoes)
    assert resposta.status_code == 400
    with app.app_context(), backend() as bridge:
        with pytest.raises(BackendError, match='Usuário não cadastrado'):
            bridge.consultar_usuario(alteracoes.get('matricula', 'A1'))


@pytest.mark.parametrize('token', ['', 'incorreto', 'é'])
def test_csrf_invalido(app, client, token):
    resposta = cadastrar(client, csrf_token=token)
    assert resposta.status_code == 400
    with app.app_context(), backend() as bridge:
        with pytest.raises(BackendError):
            bridge.consultar_usuario('A1')


def test_csrf_sem_sessao(client):
    assert client.post(CADASTRO, data=DADOS).status_code == 400


def test_formulario_preserva_e_escapa_dados(client):
    resposta = cadastrar(client, nome='<script>alert(1)</script>', email='invalido')
    assert resposta.status_code == 400
    html = resposta.get_data(as_text=True)
    assert '&lt;script&gt;' in html
    assert '<script>alert(1)</script>' not in html
    assert 'value="invalido"' in html
