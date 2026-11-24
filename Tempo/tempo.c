
#include "tempo.h"

#include <string.h>

static int lerNumero(const char *texto, int largura)
{
    int numero = 0;

    for (int i = 0; i < largura; i++) {
        if (texto[i] < '0' || texto[i] > '9')
            return -1;
        numero = numero * 10 + texto[i] - '0';
    }
    return numero;
}

int tempo_dia(const char *data)
{
    static const int diasNoMes[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    static const int diasAntesDoMes[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    int dia, mes, ano, bissexto, anosCompletos;

    if (!data || strlen(data) != 10 || data[2] != '/' || data[5] != '/')
        return -1;

    dia = lerNumero(data, 2);
    mes = lerNumero(data + 3, 2);
    ano = lerNumero(data + 6, 4);
    if (dia < 1 || mes < 1 || mes > 12 || ano < 1)
        return -1;

    bissexto = ano % 4 == 0 && (ano % 100 != 0 || ano % 400 == 0);
    if (dia > diasNoMes[mes - 1] + (mes == 2 && bissexto))
        return -1;

    anosCompletos = ano - 1;
    return 365 * anosCompletos + anosCompletos / 4 - anosCompletos / 100
        + anosCompletos / 400 + diasAntesDoMes[mes - 1] + (mes > 2 && bissexto) + dia - 1;
}

int tempo_minuto(const char *hora)
{
    int horas, minutos;

    if (!hora || strlen(hora) != 5 || hora[2] != ':')
        return -1;
    horas = lerNumero(hora, 2);
    minutos = lerNumero(hora + 3, 2);
    if (horas < 0 || horas >= 24 || minutos < 0 || minutos >= 60)
        return -1;
    return 60 * horas + minutos;
}

int tempo_intervalo(const char *ini, const char *fim, int *inicio, int *termino)
{
    *inicio = tempo_minuto(ini);
    *termino = tempo_minuto(fim);
    return *inicio >= 0 && *inicio < *termino;
}

int64_t tempo_instante(DataHora instante)
{
    int dia = tempo_dia(instante.data);
    int minuto = tempo_minuto(instante.hora);

    if (dia < 0 || minuto < 0)
        return -1;
    return (int64_t) dia * 1440 + minuto;
}

int tempo_diaSemana(int dia)
{
    return dia % 7 + 1;
}

int tempo_sobrepoe(int inicioA, int fimA, int inicioB, int fimB)
{
    return inicioA < fimB && inicioB < fimA;
}
