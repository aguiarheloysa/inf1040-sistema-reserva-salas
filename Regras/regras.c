
#include "regras.h"

#include <string.h>

int regra_comportaPessoas(int qtdPessoas, int capacidade)
{
    return qtdPessoas > 0 && qtdPessoas <= capacidade;
}

int regra_podeUsarHorarioDeAula(TipoUsuario solicitante, Finalidade f)
{
    return solicitante == PROFESSOR && f == PROVA;
}

int regra_exigeMonitor(TipoUsuario solicitante, TipoSala sala)
{
    return solicitante == ALUNO && sala == LABORATORIO;
}

int regra_podeSubstituir(TipoUsuario solicitante, Finalidade f, TipoUsuario titular)
{
    return solicitante == PROFESSOR && f == PROVA && titular == ALUNO;
}

int regra_antecedenciaSuficiente(int64_t inicioReserva, int64_t agora)
{
    return inicioReserva - agora >= REGRA_ANTECEDENCIA_MINUTOS;
}

int regra_podeCancelar(const char *matTitular, const char *matSolicitante)
{
    return matTitular && matSolicitante && *matTitular
        && strcmp(matTitular, matSolicitante) == 0;
}
