"""Proteção CSRF reutilizável para formulários que alteram o estado."""
import hmac
import secrets

from flask import abort, request, session


def csrf_token() -> str:
    """Retorna um token aleatório vinculado à sessão do navegador."""
    if "csrf_token" not in session:
        session["csrf_token"] = secrets.token_urlsafe(32)
    return session["csrf_token"]


def validar_csrf() -> None:
    """Rejeita POST sem token válido antes de acessar o núcleo C."""
    esperado = session.get("csrf_token")
    recebido = request.form.get("csrf_token", "")
    if not isinstance(esperado, str) or not hmac.compare_digest(
        esperado.encode("utf-8"), recebido.encode("utf-8")
    ):
        abort(400)
