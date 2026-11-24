"""Rotas de reservas: encaminham operações ao núcleo C, sem duplicar suas regras."""
from flask import Blueprint, flash, redirect, render_template, request, url_for

from ..backend import backend
from ..bridge import BackendError
from ..bridge.c_types import ESTUDO, PROVA
from ..seguranca import csrf_token, validar_csrf

bp = Blueprint("reservas", __name__, url_prefix="/reservas")
FINALIDADES = {str(ESTUDO): "Estudo", str(PROVA): "Prova"}
BUSCA = dict(matricula="", data="", inicio="", fim="", qtd_pessoas="1", precisa_quadro="0")
PEDIDO = dict(matricula="", cod_sala="", data="", inicio="", fim="", qtd_pessoas="1",
              finalidade=str(ESTUDO), mat_monitor="")


def _dados(origem, padrao):
    return {campo: origem.get(campo, valor).strip() for campo, valor in padrao.items()}


def _inteiro(valor, rotulo):
    try:
        return int(valor)
    except ValueError:
        raise ValueError(f"Informe um número válido para {rotulo}.") from None


def _erro(erro):
    flash(str(erro), "error")
    if isinstance(erro, BackendError):
        return {1: 404, 3: 409, 4: 409, 5: 409, 6: 409, 7: 404,
                8: 403, 11: 404, 13: 409, 14: 503}.get(erro.codigo, 400)
    return 400


@bp.get("/")
def index():
    return render_template("reservas/index.html")


@bp.get("/buscar")
def buscar():
    dados = _dados(request.args, BUSCA)
    salas = None
    status = 200
    if request.args:
        try:
            with backend() as bridge:
                salas = bridge.buscar_salas_livres(
                    dados["matricula"], dados["data"], dados["inicio"], dados["fim"],
                    _inteiro(dados["qtd_pessoas"], "quantidade de pessoas"),
                    _inteiro(dados["precisa_quadro"], "quadro"),
                )
        except (BackendError, ValueError) as erro:
            status = _erro(erro)
    return render_template("reservas/buscar.html", dados=dados, salas=salas), status


@bp.route("/solicitar", methods=["GET", "POST"])
def solicitar():
    dados = _dados(request.args, PEDIDO) if request.method == "GET" else _dados(
        request.form, dict.fromkeys(PEDIDO, ""))
    status = 200
    if request.method == "POST":
        validar_csrf()
        try:
            with backend() as bridge:
                identificador = bridge.solicitar_reserva(
                    dados["matricula"], dados["cod_sala"], dados["data"], dados["inicio"],
                    dados["fim"], _inteiro(dados["qtd_pessoas"], "quantidade de pessoas"),
                    _inteiro(dados["finalidade"], "finalidade"), dados["mat_monitor"],
                )
        except (BackendError, ValueError) as erro:
            status = _erro(erro)
        else:
            flash(f"Reserva realizada. ID: {identificador}.", "success")
            return redirect(url_for("reservas.listar", matricula=dados["matricula"]), code=303)
    return render_template("reservas/solicitar.html", dados=dados, finalidades=FINALIDADES,
                           csrf_token=csrf_token()), status


@bp.get("/listar")
def listar():
    matricula = request.args.get("matricula", "").strip()
    reservas = None
    status = 200
    if "matricula" in request.args:
        try:
            with backend() as bridge:
                reservas = bridge.listar_reservas_usuario(matricula)
        except (BackendError, ValueError) as erro:
            status = _erro(erro)
    return render_template("reservas/listar.html", matricula=matricula, reservas=reservas), status


@bp.route("/cancelar", methods=["GET", "POST"])
def cancelar():
    dados = _dados(request.args if request.method == "GET" else request.form,
                   dict(id_reserva="", matricula=""))
    status = 200
    if request.method == "POST":
        validar_csrf()
        try:
            with backend() as bridge:
                mensagem = bridge.cancelar_reserva(
                    _inteiro(dados["id_reserva"], "ID da reserva"), dados["matricula"])
        except (BackendError, ValueError) as erro:
            status = _erro(erro)
        else:
            flash(mensagem, "success")
            return redirect(url_for("reservas.listar", matricula=dados["matricula"]), code=303)
    return render_template("reservas/cancelar.html", dados=dados, csrf_token=csrf_token()), status
