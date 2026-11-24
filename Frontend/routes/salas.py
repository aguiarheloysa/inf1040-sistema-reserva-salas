"""Rotas HTTP de salas e biometria; operações de negócio delegadas à bridge."""
from flask import Blueprint, flash, redirect, render_template, request, url_for

from ..backend import backend
from ..bridge import BackendError
from ..bridge.c_types import COMUM, LABORATORIO
from ..seguranca import csrf_token, validar_csrf

bp = Blueprint("salas", __name__, url_prefix="/salas")
TIPOS = {str(COMUM): "Comum", str(LABORATORIO): "Laboratório"}


@bp.get("/")
def index():
    return render_template("salas/index.html")


@bp.route("/cadastrar", methods=["GET", "POST"])
def cadastrar():
    dados = {"codigo": "", "tipo": str(COMUM), "capacidade": "", "tem_quadro": "0"}
    status = 200
    if request.method == "POST":
        validar_csrf()
        dados = {campo: request.form.get(campo, "").strip() for campo in dados}
        try:
            if dados["tipo"] not in TIPOS or dados["tem_quadro"] not in ("0", "1"):
                raise ValueError("Selecione um tipo de sala e uma opção de quadro válidos.")
            with backend() as bridge:
                mensagem = bridge.cadastrar_sala(dados["codigo"], int(dados["tipo"]),
                                                  int(dados["capacidade"]), int(dados["tem_quadro"]))
        except BackendError as erro:
            flash(str(erro), "error")
            status = 409 if erro.codigo in (2, 7) else 400
        except ValueError:
            flash("Informe valores válidos para tipo, capacidade e quadro.", "error")
            status = 400
        else:
            flash(mensagem, "success")
            return redirect(url_for("salas.listar"), code=303)
    return render_template("salas/cadastrar.html", dados=dados, tipos=TIPOS,
                           csrf_token=csrf_token()), status


@bp.get("/listar")
def listar():
    dados = {"qtd_pessoas": request.args.get("qtd_pessoas", "1").strip(),
             "precisa_quadro": request.args.get("precisa_quadro", "0").strip()}
    salas = None
    status = 200
    try:
        with backend() as bridge:
            salas = bridge.listar_salas(int(dados["qtd_pessoas"]), int(dados["precisa_quadro"]))
    except BackendError as erro:
        flash(str(erro), "error")
        status = 400
    except ValueError:
        flash("Informe valores válidos para quantidade de pessoas e quadro.", "error")
        status = 400
    return render_template("salas/listar.html", dados=dados, salas=salas), status


@bp.route("/biometria", methods=["GET", "POST"])
def biometria():
    dados = {"codigo": "", "mat_monitor": ""}
    status = 200
    if request.method == "POST":
        validar_csrf()
        dados = {campo: request.form.get(campo, "").strip() for campo in dados}
        try:
            with backend() as bridge:
                mensagem = bridge.registrar_biometria(dados["codigo"], dados["mat_monitor"])
        except BackendError as erro:
            flash(str(erro), "error")
            status = {3: 404, 4: 404, 7: 409}.get(erro.codigo, 400)
        except ValueError as erro:
            flash(str(erro), "error")
            status = 400
        else:
            flash(mensagem, "success")
            return redirect(url_for("salas.biometria"), code=303)
    return render_template("salas/biometria.html", dados=dados, csrf_token=csrf_token()), status
