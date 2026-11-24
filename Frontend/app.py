"""Configuração e ponto de entrada da aplicação web."""
import os
import secrets
from pathlib import Path

from flask import Flask, render_template

from .bridge import BackendBridge
from .notificacoes import entregar_smtp
from .routes.principal import bp as principal_bp
from .routes.usuarios import bp as usuarios_bp
from .routes.salas import bp as salas_bp
from .routes.grade import bp as grade_bp
from .routes.reservas import bp as reservas_bp
from .routes.notificacoes import bp as notificacoes_bp


def create_app(config: dict | None = None) -> Flask:
    """Cria uma aplicação com uma única instância da bridge por app."""
    app = Flask(__name__)
    app.config.from_mapping(
        SECRET_KEY=os.environ.get("RESERVA_SECRET_KEY") or secrets.token_hex(32),
        BACKEND_LIBRARY=None,
        NOTIFICACAO_MODO=os.environ.get("RESERVA_NOTIFICACAO", "simulado"),
        SESSION_COOKIE_HTTPONLY=True,
        SESSION_COOKIE_SAMESITE="Lax",
    )
    if config is not None:
        app.config.update(config)

    # Os testes nunca usam SMTP por efeito de uma variável de ambiente.
    if app.testing and (config is None or "NOTIFICACAO_MODO" not in config):
        app.config["NOTIFICACAO_MODO"] = "simulado"
    modo = app.config["NOTIFICACAO_MODO"]
    if modo not in ("simulado", "smtp"):
        raise ValueError("RESERVA_NOTIFICACAO deve ser simulado ou smtp.")

    caminho = app.config["BACKEND_LIBRARY"]
    app.extensions["backend_bridge"] = BackendBridge(
        dll_path=Path(caminho) if caminho is not None else None,
        canal_entrega=entregar_smtp if modo == "smtp" else None
    )
    app.register_blueprint(principal_bp)
    app.register_blueprint(usuarios_bp)
    app.register_blueprint(salas_bp)
    app.register_blueprint(grade_bp)
    app.register_blueprint(reservas_bp)
    app.register_blueprint(notificacoes_bp)

    @app.errorhandler(400)
    def requisicao_invalida(erro):
        return render_template("erro.html", codigo=400,
                               mensagem="Requisição inválida. Reabra o formulário e tente novamente."), 400

    @app.errorhandler(404)
    def pagina_nao_encontrada(erro):
        return render_template("erro.html", codigo=404,
                               mensagem="Página não encontrada."), 404

    @app.errorhandler(405)
    def metodo_nao_permitido(erro):
        resposta = erro.get_response()
        resposta.set_data(render_template("erro.html", codigo=405,
                                         mensagem="Método não permitido."))
        return resposta

    @app.errorhandler(503)
    def backend_indisponivel(erro):
        return render_template("erro.html", codigo=503,
                               mensagem="Não foi possível consultar o sistema."), 503

    return app


def main() -> None:
    # O acesso ao núcleo C é sincronizado pelo RLock de backend.py.
    create_app().run(host="127.0.0.1", port=5000, debug=False,
                     threaded=True, use_reloader=False)


if __name__ == "__main__":
    main()
