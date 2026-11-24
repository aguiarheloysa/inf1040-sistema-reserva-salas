"""Acesso sincronizado ao núcleo C pelas rotas HTTP."""
from collections.abc import Iterator
from contextlib import contextmanager
from threading import RLock

from flask import current_app

from .bridge import BackendBridge

# O bloqueio pertence ao processo: duas apps podem carregar a mesma biblioteca.
_lock = RLock()


@contextmanager
def backend() -> Iterator[BackendBridge]:
    """Disponibiliza a bridge compartilhada enquanto mantém o lock do núcleo.

    As rotas devem executar todas as chamadas C de uma operação dentro de
    um único bloco with. O lock é liberado mesmo se a bridge lançar exceção.
    """
    with _lock:
        yield current_app.extensions["backend_bridge"]
