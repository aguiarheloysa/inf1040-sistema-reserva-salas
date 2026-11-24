"""Testes HTTP da infraestrutura Flask com o núcleo C real."""
import shutil

import pytest

from Frontend.app import create_app
from Frontend.backend import backend
from Frontend.bridge import BackendError
from Frontend.bridge.backend_bridge import caminho_biblioteca
from Frontend.bridge.c_types import COMUM


@pytest.fixture
def app(tmp_path):
    original = caminho_biblioteca()
    isolada = tmp_path / original.name
    shutil.copy2(original, isolada)
    return create_app({"TESTING": True, "BACKEND_LIBRARY": isolada})


@pytest.fixture
def client(app):
    return app.test_client()


def test_pagina_inicial(client):
    resposta = client.get("/")
    assert resposta.status_code == 200
    assert resposta.mimetype == "text/html"
    assert "Sistema de Reserva de Salas" in resposta.get_data(as_text=True)
    assert 'id="total-salas">0</strong>' in resposta.get_data(as_text=True)
    assert "Serviços" in resposta.get_data(as_text=True)
    assert "Conexão com o núcleo C" not in resposta.get_data(as_text=True)
    assert "Módulos disponíveis" not in resposta.get_data(as_text=True)


def test_css(client):
    resposta = client.get("/static/css/style.css")
    assert resposta.status_code == 200
    assert resposta.mimetype == "text/css"
    assert b".cabecalho" in resposta.data


def test_estado_compartilhado(app, client):
    assert 'id="total-salas">0</strong>' in client.get("/").get_data(as_text=True)
    with app.app_context(), backend() as bridge:
        assert bridge is app.extensions["backend_bridge"]
        bridge.cadastrar_sala("L201", COMUM, 40, 1)
    assert 'id="total-salas">1</strong>' in client.get("/").get_data(as_text=True)
    assert 'id="total-salas">1</strong>' in app.test_client().get("/").get_data(as_text=True)


def test_estado_isolado_entre_apps(app, tmp_path):
    with app.app_context(), backend() as bridge:
        bridge.cadastrar_sala("L201", COMUM, 40, 1)
    original = caminho_biblioteca()
    pasta = tmp_path / "outra_app"
    pasta.mkdir()
    isolada = pasta / original.name
    shutil.copy2(original, isolada)
    outra = create_app({"TESTING": True, "BACKEND_LIBRARY": isolada})
    assert 'id="total-salas">0</strong>' in outra.test_client().get("/").get_data(as_text=True)
    assert 'id="total-salas">1</strong>' in app.test_client().get("/").get_data(as_text=True)


def test_pagina_nao_encontrada(client):
    resposta = client.get("/rota-inexistente")
    assert resposta.status_code == 404
    assert "Página não encontrada." in resposta.get_data(as_text=True)


def test_metodo_nao_permitido(client):
    resposta = client.post("/")
    assert resposta.status_code == 405
    assert "GET" in resposta.headers["Allow"]
    assert "Método não permitido." in resposta.get_data(as_text=True)


def test_erro_do_backend(app, client, monkeypatch):
    def falhar():
        raise BackendError(99, "detalhe interno")

    with monkeypatch.context() as m:
        m.setattr(app.extensions["backend_bridge"], "listar_todas_salas", falhar)
        resposta = client.get("/")
        assert resposta.status_code == 503
        assert "Não foi possível consultar o sistema." in resposta.get_data(as_text=True)
        assert "detalhe interno" not in resposta.get_data(as_text=True)
    assert client.get("/").status_code == 200


def test_biblioteca_ausente(tmp_path):
    with pytest.raises(FileNotFoundError, match="build.py"):
        create_app({"TESTING": True, "BACKEND_LIBRARY": tmp_path / "inexistente.dll"})
