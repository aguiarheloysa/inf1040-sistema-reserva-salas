"""Adaptador de entrega: reutiliza o canal SMTP da interface original.

Não decide prioridade, prazo ou cancelamento. O núcleo entrega um lote
já preparado, e recebe somente o resultado da tentativa de comunicação.
"""
from Interface import enviar_email


MENSAGEM_COLETIVA = (
    "Uma reserva de sala vinculada ao seu cadastro foi cancelada para aplicação de prova. "
    "Consulte o seu histórico de reservas para ver os detalhes."
)


def entregar_smtp(entregas: list[tuple[str, str]]) -> bool:
    """Uma transação SMTP por lote, como Interface/smtp.c.

    O helper original verifica todos os destinatários antes do comando DATA.
    Não envia uma série de e-mails separados (que poderia aceitar só parte).
    Exceções propagam até a bridge, que informa falha ao C.
    """
    if not entregas:
        return False
    destinatarios = [email for email, _ in entregas]
    corpo = entregas[0][1] if len(entregas) == 1 else MENSAGEM_COLETIVA
    enviar_email.entregar(destinatarios, corpo)
    return True
