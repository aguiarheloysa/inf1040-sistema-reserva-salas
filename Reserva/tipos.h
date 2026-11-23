#ifndef TIPOS_H
#define TIPOS_H

#define TAM 20
#define MAX_SALAS 100
#define MAX_RESERVAS 500
#define TAM_EMAIL 255

typedef char Data[11];
typedef char Hora[9];

typedef struct {
    Data data;
    Hora hora;
} DataHora;

typedef enum {
    ALUNO,
    PROFESSOR
} TipoUsuario;

typedef enum {
    COMUM,
    LABORATORIO
} TipoSala;

typedef enum {
    ESTUDO,
    PROVA
} Finalidade;

typedef enum {
    ATIVA,
    CANCELADA
} Situacao;

typedef struct {
    int id;
    char matricula[TAM];
    char codSala[TAM];
    Data data;
    Hora inicio;
    Hora fim;
    int qtdPessoas;
    Finalidade finalidade;
    Situacao situacao;
} Reserva;

#endif
