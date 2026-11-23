
#ifndef GRADE_H
#define GRADE_H

#include "tipos.h"

#define GRADE_OK 0
#define GRADE_CONFLITO 1
#define GRADE_HORARIO_INVALIDO 2
#define GRADE_SEM_AULA 3
#define GRADE_NAO_MINISTRANTE 4
#define GRADE_DADOS_INVALIDOS 5
#define GRADE_LIMITE_ATINGIDO 6

/* diaSemana vai de 1 (segunda-feira) a 7 (domingo). */
int grade_cadastrarAula(char *codSala, char *disciplina, char *matProfessor,
                       int diaSemana, const char *ini, const char *fim);

/* Se houver mais de uma aula, retorna o primeiro ministrante por horário.
   matMinistrante, quando fornecido, deve ter espaço para TAM caracteres. */
int grade_temAula(char *codSala, const char *d, const char *ini, const char *fim, char *matMinistrante);

int grade_registrarAutorizacao(char *codSala, const char *d, const char *ini,
                               char *matMinistrante, char *matSolicitante);

int grade_temAutorizacao(char *codSala, const char *d, const char *ini, char *matSolicitante);

/* Para cada aula sobreposta, considera o primeiro minuto da interseção. */
int grade_permiteProva(char *codSala, const char *d, const char *ini, const char *fim, char *matSolicitante);

#endif
