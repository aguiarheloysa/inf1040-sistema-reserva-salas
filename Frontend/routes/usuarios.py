"""Rotas de cadastro e consulta de usuários; regras de negócio no núcleo C."""
from flask import Blueprint, flash, redirect, render_template, request, url_for

from ..backend import backend
from ..bridge import BackendError
from ..bridge.c_types import ALUNO, PROFESSOR
from ..seguranca import csrf_token, validar_csrf

bp = Blueprint("usuarios", __name__, url_prefix="/usuarios")
TIPOS = {str(ALUNO): "Aluno", str(PROFESSOR): "Professor"}


@bp.get("/")
def index():
    return render_template("usuarios/index.html")


@bp.route("/cadastrar", methods=["GET", "POST"])
def cadastrar():
    dados = {"matricula": "", "nome": "", "email": "", "tipo": str(ALUNO)}
    status = 200
    if request.method == "POST":
        validar_csrf()
        dados = {campo: request.form.get(campo, "").strip() for campo in dados}
        try:
            if dados["tipo"] not in TIPOS:
                raise ValueError("Selecione um tipo de usuário válido.")
            with backend() as bridge:
                mensagem = bridge.cadastrar_usuario(
                    dados["matricula"], dados["nome"], dados["email"], int(dados["tipo"])
                )
        except BackendError as erro:
            flash(str(erro), "error")
            status = {1: 409, 2: 400, 4: 409}.get(erro.codigo, 400)
        except ValueError as erro:
            flash(str(erro), "error")
            status = 400
        else:
            flash(mensagem, "success")
            return redirect(url_for("usuarios.cadastrar"), code=303)

    return render_template("usuarios/cadastrar.html", dados=dados,
                           tipos=TIPOS, csrf_token=csrf_token()), status


@bp.get("/consultar")
def consultar():
    matricula = request.args.get("matricula", "").strip()
    usuario = None
    status = 200
    if "matricula" in request.args:
        try:
            if not matricula:
                raise ValueError("Informe a matrícula.")
            with backend() as bridge:
                usuario = bridge.consultar_usuario(matricula)
        except BackendError as erro:
            flash(str(erro), "error")
            status = 404 if erro.codigo == 3 else 400
        except ValueError as erro:
            flash(str(erro), "error")
            status = 400
    return render_template("usuarios/consultar.html", matricula=matricula,
                           usuario=usuario), status
