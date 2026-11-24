import ctypes

TAM = 20
MAX_SALAS = 100
MAX_RESERVAS = 500
TAM_EMAIL = 255

# Enums (tipos.h)
ALUNO = 0
PROFESSOR = 1

COMUM = 0
LABORATORIO = 1

ESTUDO = 0
PROVA = 1

ATIVA = 0
CANCELADA = 1


class DataHora(ctypes.Structure):
    _fields_ = [
        ("data", ctypes.c_char * 11),
        ("hora", ctypes.c_char * 9),
    ]


class ReservaC(ctypes.Structure):
    _fields_ = [
        ("id", ctypes.c_int),
        ("matricula", ctypes.c_char * TAM),
        ("codSala", ctypes.c_char * TAM),
        ("data", ctypes.c_char * 11),
        ("inicio", ctypes.c_char * 9),
        ("fim", ctypes.c_char * 9),
        ("qtdPessoas", ctypes.c_int),
        ("finalidade", ctypes.c_int),
        ("situacao", ctypes.c_int),
    ]