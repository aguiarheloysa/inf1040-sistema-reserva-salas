"""Compila o núcleo C como biblioteca compartilhada, sem a interface de terminal."""
import os
from pathlib import Path
import shlex
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[2]
MODULOS = (
    "Aluno/aluno.c", "Sala/sala.c", "Grade/grade.c", "Reserva/reserva.c",
    "Comum/tempo.c", "Notificacao/notificacao.c", "Regras/regras.c",
)
INCLUDES = ("Aluno", "Sala", "Grade", "Reserva", "Comum", "Notificacao", "Regras")


def main() -> None:
    if os.name == "nt":
        extensao, flags = ".dll", ["-shared", "-Wl,--no-undefined", "-Wl,--export-all-symbols"]
    elif sys.platform == "darwin":
        extensao, flags = ".dylib", ["-dynamiclib", "-fPIC", "-Wl,-undefined,error"]
    else:
        extensao, flags = ".so", ["-shared", "-fPIC", "-Wl,--no-undefined"]
    destino = ROOT / "Frontend" / "bridge" / f"libreserva{extensao}"
    comando = [os.environ.get("CC", "gcc"), "-std=c11", "-O2", "-Wall", "-Wextra", "-Wpedantic",
               *flags, *MODULOS, *(f"-I{pasta}" for pasta in INCLUDES), "-o", str(destino)]
    print(subprocess.list2cmdline(comando) if os.name == "nt" else shlex.join(comando), flush=True)
    subprocess.run(comando, cwd=ROOT, check=True)
    print(f"Biblioteca compilada: {destino}")


if __name__ == "__main__":
    main()
