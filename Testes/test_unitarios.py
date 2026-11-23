"""Executa cada caso unitário em um novo processo."""

from pathlib import Path
import subprocess
import sys


def executar_suite(caminho):
    executavel = Path(caminho).resolve()
    nomes = subprocess.run(
        [executavel, "--listar"], check=True, capture_output=True, text=True, timeout=10
    ).stdout.splitlines()
    if not nomes or len(set(nomes)) != len(nomes):
        raise ValueError(f"Lista de casos inválida em {executavel.name}")

    for nome in nomes:
        subprocess.run([executavel, nome], check=True, timeout=10)
    print(f"{executavel.stem}: {len(nomes)} casos OK", flush=True)
    return len(nomes)


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit("Informe os executáveis das suítes unitárias.")
    total = sum(executar_suite(caminho) for caminho in sys.argv[1:])
    print(f"Total: {total} casos unitários OK")
