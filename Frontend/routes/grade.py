"""Rotas HTTP de grade semanal e autorizações de prova."""
from flask import Blueprint, flash, redirect, render_template, request, url_for

from ..backend import backend
from ..bridge import BackendError
from ..seguranca import csrf_token, validar_csrf

bp = Blueprint("grade", __name__, url_prefix="/grade")
DIAS = {str(i): dia for i, dia in enumerate((
    "Segunda-feira", "Terça-feira", "Quarta-feira", "Quinta-feira",
    "Sexta-feira", "Sábado", "Domingo"
), 1)}


@bp.get("/")
def index():
    return render_template("grade/index.html")


@bp.route("/cadastrar", methods=["GET", "POST"])
def cadastrar():
    dados = {"cod_sala": "", "disciplina": "", "mat_professor": "",
             "dia_semana": "1", "inicio": "", "fim": ""}
    status = 200
    if request.method == "POST":
        validar_csrf()
        dados = {campo: request.form.get(campo, "").strip() for campo in dados}
        try:
            try:
                dia = int(dados["dia_semana"])
            except ValueError:
                raise ValueError("Informe um dia da semana válido.") from None
            with backend() as bridge:
                mensagem = bridge.cadastrar_aula(
                    dados["cod_sala"], dados["disciplina"], dados["mat_professor"],
                    dia, dados["inicio"], dados["fim"]
                )
        except BackendError as erro:
            flash(str(erro), "error")
            status = 409 if erro.codigo in (1, 6) else 400
        except ValueError as erro:
            flash(str(erro), "error")
            status = 400
        else:
            flash(mensagem, "success")
            return redirect(url_for("grade.cadastrar"), code=303)
    return render_template("grade/cadastrar.html", dados=dados, dias=DIAS,
                           csrf_token=csrf_token()), status


@bp.route("/autorizar", methods=["GET", "POST"])
def autorizar():
    dados = {"cod_sala": "", "data": "", "hora": "",
             "mat_ministrante": "", "mat_solicitante": ""}
    status = 200
    if request.method == "POST":
        validar_csrf()
        dados = {campo: request.form.get(campo, "").strip() for campo in dados}
        try:
            with backend() as bridge:
                mensagem = bridge.autorizar_prova(
                    dados["cod_sala"], dados["data"], dados["hora"],
                    dados["mat_ministrante"], dados["mat_solicitante"]
                )
        except BackendError as erro:
            # Os códigos de Sala e Grade podem coincidir. Preservar a mensagem
            # da bridge, sem presumir a origem de um código ambíguo.
            flash(str(erro), "error")
            status = 409 if erro.codigo == 6 else 400
        except ValueError as erro:
            flash(str(erro), "error")
            status = 400
        else:
            flash(mensagem, "success")
            return redirect(url_for("grade.autorizar"), code=303)
    return render_template("grade/autorizar.html", dados=dados,
                           csrf_token=csrf_token()), status
