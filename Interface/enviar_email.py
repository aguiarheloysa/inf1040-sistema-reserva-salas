
"""Entrega um aviso por SMTP. Lê a quantidade, os endereços e o corpo via stdin."""

import os
import smtplib
import ssl
import sys
from email.errors import MessageError
from email.headerregistry import Address
from email.message import EmailMessage
from email.policy import SMTP


def entregar(destinatarios, corpo):
    servidor = os.environ["RESERVA_SMTP_HOST"]
    remetente = Address(addr_spec=os.environ["RESERVA_SMTP_REMETENTE"]).addr_spec
    modo = os.environ.get("RESERVA_SMTP_TLS", "starttls")
    portas = {"starttls": 587, "ssl": 465, "nenhum": 25}
    porta = int(os.environ.get("RESERVA_SMTP_PORTA", portas[modo]))
    usuario = os.environ.get("RESERVA_SMTP_USUARIO")
    senha = os.environ.get("RESERVA_SMTP_SENHA")
    if (usuario is None) != (senha is None):
        raise ValueError("configure usuário e senha SMTP em conjunto")
    if usuario is not None and modo == "nenhum":
        raise ValueError("autenticação SMTP exige TLS")

    enderecos = [Address(addr_spec=endereco) for endereco in dict.fromkeys(destinatarios)]
    mensagem = EmailMessage(policy=SMTP)
    mensagem["From"] = remetente
    if len(destinatarios) == 1:
        mensagem["To"] = enderecos[0]
    mensagem["Subject"] = "Cancelamento de reserva de sala"
    mensagem.set_content(corpo, cte="quoted-printable")

    if modo == "ssl":
        cliente = smtplib.SMTP_SSL(servidor, porta, timeout=10, context=ssl.create_default_context())
    else:
        cliente = smtplib.SMTP(servidor, porta, timeout=10)
    try:
        cliente.ehlo_or_helo_if_needed()
        if modo == "starttls":
            cliente.starttls(context=ssl.create_default_context())
            cliente.ehlo()
        if usuario is not None:
            cliente.login(usuario, senha)
        codigo, resposta = cliente.mail(remetente)
        if codigo != 250:
            raise smtplib.SMTPSenderRefused(codigo, resposta, remetente)
        for endereco in enderecos:
            codigo, resposta = cliente.rcpt(endereco.addr_spec)
            if codigo not in (250, 251):
                raise smtplib.SMTPRecipientsRefused({endereco.addr_spec: (codigo, resposta)})
        # Envia o corpo somente após o aceite de todos os destinatários.
        codigo, resposta = cliente.data(mensagem.as_bytes())
        if codigo != 250:
            raise smtplib.SMTPDataError(codigo, resposta)
    finally:
        cliente.close()


if __name__ == "__main__":
    try:
        entrada = sys.stdin.buffer
        quantidade = int(entrada.readline())
        if quantidade <= 0:
            raise ValueError("notificação sem destinatários")
        destinatarios = [entrada.readline().decode("utf-8").rstrip("\r\n") for _ in range(quantidade)]
        corpo = entrada.read().decode("utf-8")
        if not all(destinatarios) or not corpo:
            raise ValueError("notificação sem destinatário ou corpo")
        entregar(destinatarios, corpo)
    except (KeyError, ValueError, OSError, MessageError) as erro:
        print(f"Falha de entrega SMTP ({type(erro).__name__}).", file=sys.stderr)
        sys.exit(1)
