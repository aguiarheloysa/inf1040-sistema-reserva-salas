from pathlib import Path
import subprocess
import sys


def executar(executavel, argumento, *, capturar=False):
    try:
        return subprocess.run(
            [executavel, argumento], check=True, capture_output=capturar, text=capturar, timeout=10
        )
    except OSError as erro:
        if getattr(erro, "winerror", None) == 4551:
            sys.exit(
                f"O Controle de Aplicativo do Windows bloqueou {executavel.name} (WinError 4551).\n"
                "No Ubuntu via WSL, execute make test-unit na raiz do projeto."
            )
        raise


def executar_suite(caminho):
    executavel = Path(caminho).resolve()
    nomes = executar(executavel, "--listar", capturar=True).stdout.splitlines()
    if not nomes or len(set(nomes)) != len(nomes):
        raise ValueError(f"Lista de casos inválida em {executavel.name}")

    for nome in nomes:
        executar(executavel, nome)
    print(f"{executavel.stem}: {len(nomes)} casos OK", flush=True)
    return len(nomes)


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit("Informe os executáveis das suítes unitárias.")
    total = sum(executar_suite(caminho) for caminho in sys.argv[1:])
    print(f"Total: {total} casos unitários OK")
