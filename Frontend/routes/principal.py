"""Página inicial e apresentação do estado atual do sistema."""
from flask import Blueprint, abort, current_app, render_template

from ..backend import backend
from ..bridge import BackendError

bp = Blueprint("principal", __name__)


@bp.get("/")
def index():
    try:
        with backend() as bridge:
            total_salas = len(bridge.listar_todas_salas())
    except BackendError:
        current_app.logger.exception("Falha ao consultar salas do núcleo C")
        abort(503)

    return render_template("index.html", total_salas=total_salas)
