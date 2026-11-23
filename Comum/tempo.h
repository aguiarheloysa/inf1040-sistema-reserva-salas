
#ifndef TEMPO_H
#define TEMPO_H

#include <stdint.h>
#include "tipos.h"

/* Dias desde 01/01/0001 e minutos desde 00:00. -1 indica formato inválido. */
int tempo_dia(const char *data);
int tempo_minuto(const char *hora);
int tempo_intervalo(const char *ini, const char *fim, int *inicio, int *termino);
int64_t tempo_instante(DataHora instante);

/* Segunda = 1, domingo = 7. */
int tempo_diaSemana(int dia);
int tempo_sobrepoe(int inicioA, int fimA, int inicioB, int fimB);

#endif
