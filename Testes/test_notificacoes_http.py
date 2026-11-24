"""Integração HTTP de notificações, com SMTP substituído por uma função local."""
import shutil
from datetime import datetime

import pytest

from Frontend.app import create_app
from Frontend.backend import backend
from Frontend.bridge import backend_bridge
from Frontend.bridge.backend_bridge import caminho_biblioteca
from Frontend.bridge.c_types import ALUNO, PROFESSOR, COMUM
from Interface import enviar_email


@pytest.fixture
def ambiente(tmp_path, monkeypatch):
    class Relogio(datetime):
        @classmethod
        def now(cls, tz=None):
            return cls(2026, 10, 20, 12, 0, tzinfo=tz)
    monkeypatch.setattr(backend_bridge, 'datetime', Relogio)
    original = caminho_biblioteca()
    isolada = tmp_path / original.name
    shutil.copy2(original, isolada)
    return {'TESTING': True, 'BACKEND_LIBRARY': isolada}


def preparar(app):
    with app.app_context(), backend() as bridge:
        bridge.cadastrar_usuario('A1', 'Alice', 'alice@example.test', ALUNO)
        bridge.cadastrar_usuario('P1', 'Professor', 'prof@example.test', PROFESSOR)
        bridge.cadastrar_sala('L201', COMUM, 40, 1)


def solicitar(client, mat, finalidade):
    rota = '/reservas/solicitar'
    assert client.get(rota).status_code == 200
    with client.session_transaction() as sessao:
        token = sessao['csrf_token']
    return client.post(rota, data=dict(csrf_token=token, matricula=mat, cod_sala='L201',
        data='2026-10-20', inicio='14:00', fim='15:00', qtd_pessoas='5',
        finalidade=finalidade, mat_monitor=''))


def estado(app, mat):
    with app.app_context(), backend() as bridge:
        return [r['situacao'] for r in bridge.listar_reservas_usuario(mat)]


def test_menu_e_modo_simulado(ambiente, monkeypatch):
    monkeypatch.setenv('RESERVA_NOTIFICACAO', 'smtp')
    app = create_app(ambiente)
    client = app.test_client()
    assert '/notificacoes/' in client.get('/').get_data(as_text=True)
    r = client.get('/notificacoes/')
    assert r.status_code == 200 and 'Modo simulado' in r.get_data(as_text=True)
    assert client.post('/notificacoes/').status_code == 405


def test_modo_invalido(ambiente):
    with pytest.raises(ValueError, match='RESERVA_NOTIFICACAO'):
        create_app({**ambiente, 'NOTIFICACAO_MODO':'invalido'})


def test_modo_smtp(ambiente):
    app = create_app({**ambiente, 'NOTIFICACAO_MODO':'smtp'})
    assert 'Modo SMTP' in app.test_client().get('/notificacoes/').get_data(as_text=True)


@pytest.mark.parametrize('falha', [False, True])
def test_substituicao_http(ambiente, monkeypatch, falha):
    chamadas = []
    def entregar(destinatarios, corpo):
        chamadas.append((destinatarios, corpo))
        if falha:
            raise OSError('detalhe privado')
    monkeypatch.setattr(enviar_email, 'entregar', entregar)
    app = create_app({**ambiente, 'NOTIFICACAO_MODO':'smtp'})
    preparar(app)
    client = app.test_client()
    assert solicitar(client, 'A1', '0').status_code == 303
    assert chamadas == []
    r = solicitar(client, 'P1', '1')
    assert len(chamadas) == 1 and chamadas[0][0] == ['alice@example.test']
    assert 'L201 em 20/10/2026, às 14:00' in chamadas[0][1]
    if falha:
        assert r.status_code == 503
        html = r.get_data(as_text=True)
        assert 'Falha ao enviar' in html and 'detalhe privado' not in html
        assert estado(app, 'A1') == ['ATIVA'] and estado(app, 'P1') == []
    else:
        assert r.status_code == 303
        assert estado(app, 'A1') == ['CANCELADA'] and estado(app, 'P1') == ['ATIVA']


def test_simulado_nao_chama_smtp(ambiente, monkeypatch):
    chamadas = []
    monkeypatch.setattr(enviar_email, 'entregar', lambda *args: chamadas.append(args))
    app = create_app(ambiente)
    preparar(app)
    client = app.test_client()
    assert solicitar(client, 'A1', '0').status_code == 303
    assert solicitar(client, 'P1', '1').status_code == 303
    assert chamadas == [] and estado(app, 'A1') == ['CANCELADA']
