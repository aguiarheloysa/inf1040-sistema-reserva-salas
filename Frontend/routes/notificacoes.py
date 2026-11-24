"""Página informativa do canal de notificações, sem envio manual."""
from flask import Blueprint, current_app, render_template

bp = Blueprint("notificacoes", __name__, url_prefix="/notificacoes")


@bp.get("/")
def index():
    return render_template("notificacoes/index.html", modo=current_app.config["NOTIFICACAO_MODO"])
