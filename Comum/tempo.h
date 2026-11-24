
#ifndef TEMPO_H
#define TEMPO_H

#include <stdint.h>
#include "tipos.h"

int tempo_dia(const char *data);
int tempo_minuto(const char *hora);
int tempo_intervalo(const char *ini, const char *fim, int *inicio, int *termino);
int64_t tempo_instante(DataHora instante);

int tempo_diaSemana(int dia);
int tempo_sobrepoe(int inicioA, int fimA, int inicioB, int fimB);

#endif
