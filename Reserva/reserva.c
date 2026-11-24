
#include "reserva.h"

#include <string.h>

#include "grade.h"
#include "notificacao.h"
#include "sala.h"
#include "tempo.h"
#include "usuario.h"

#define ANTECEDENCIA_MINUTOS 120

static Reserva reservas[MAX_RESERVAS];
static int qtdReservas;

static int conflita(const Reserva *reserva, const char *sala, const char *data,
                    int inicio, int fim)
{
    return reserva->situacao == ATIVA && strcmp(reserva->codSala, sala) == 0
        && strcmp(reserva->data, data) == 0
        && tempo_sobrepoe(tempo_minuto(reserva->inicio), tempo_minuto(reserva->fim), inicio, fim);
}

int res_verificarConflito(char *codSala, const char *d, const char *ini, const char *fim, int *idConflitante)
{
    int inicio, termino;

    if (idConflitante)
        *idConflitante = 0;
    if (!codSala || tempo_dia(d) < 0 || !tempo_intervalo(ini, fim, &inicio, &termino))
        return 0;

    for (int i = 0; i < qtdReservas; i++) {
        if (conflita(&reservas[i], codSala, d, inicio, termino)) {
            if (idConflitante)
                *idConflitante = reservas[i].id;
            return 1;
        }
    }
    return 0;
}

int res_buscarSalasLivres(const char *d, const char *ini, const char *fim, int qtdPessoas,
                         int precisaQuadro, char *matricula, char lista[][TAM], int *n)
{
    char candidatas[MAX_SALAS][TAM];
    int qtdCandidatas, inicio, termino;
    TipoUsuario tipoUsuario;

    if (!n)
        return RES_NENHUMA_SALA;
    *n = 0;
    if (!lista || tempo_dia(d) < 0 || !tempo_intervalo(ini, fim, &inicio, &termino)
            || usu_obterTipo(matricula, &tipoUsuario) != USU_OK
            || sala_listarPorCapacidade(qtdPessoas, precisaQuadro, candidatas, &qtdCandidatas) != SALA_OK)
        return RES_NENHUMA_SALA;

    for (int i = 0; i < qtdCandidatas; i++) {
        char *sala = candidatas[i];
        TipoSala tipoSala;
        if (sala_obterTipo(sala, &tipoSala) != SALA_OK
                || (tipoUsuario == ALUNO && tipoSala == LABORATORIO)
                || grade_temAula(sala, d, ini, fim, NULL)
                || res_verificarConflito(sala, d, ini, fim, NULL))
            continue;
        strcpy(lista[(*n)++], sala);
    }
    return *n ? RES_OK : RES_NENHUMA_SALA;
}

static int resolverConflitos(Reserva *conflitos[], int qtdConflitos, int prova, int64_t agora)
{
    if (qtdConflitos == 0)
        return RES_OK;

    for (int i = 0; i < qtdConflitos; i++) {
        TipoUsuario titular;
        if (!prova || usu_obterTipo(conflitos[i]->matricula, &titular) != USU_OK
                || titular != ALUNO)
            return RES_CONFLITO;
    }
    for (int i = 0; i < qtdConflitos; i++) {
        const Reserva *anterior = conflitos[i];
        int64_t inicio = (int64_t) tempo_dia(anterior->data) * 1440
            + tempo_minuto(anterior->inicio);
        if (inicio - agora < ANTECEDENCIA_MINUTOS)
            return RES_ANTECEDENCIA_INSUFICIENTE;
    }
    if (not_avisarCancelamentos(conflitos, qtdConflitos) != NOT_OK)
        return RES_FALHA_NOTIFICACAO;
    return RES_OK;
}

int res_solicitar(char *matricula, char *codSala, const char *d, const char *ini, const char *fim,
                  int qtdPessoas, Finalidade f, char *matMonitor, DataHora agora,
                  int *idReserva)
{
    int inicio, termino, capacidade;
    TipoUsuario tipoUsuario;
    TipoSala tipoSala;

    if (idReserva)
        *idReserva = 0;
    if (usu_obterTipo(matricula, &tipoUsuario) != USU_OK)
        return RES_USUARIO_INEXISTENTE;
    if (sala_obterCapacidade(codSala, &capacidade) != SALA_OK
            || sala_obterTipo(codSala, &tipoSala) != SALA_OK)
        return RES_SALA_INEXISTENTE;
    if (qtdPessoas > capacidade)
        return RES_CAPACIDADE_EXCEDIDA;
    int64_t instantePedido = tempo_instante(agora);
    if (!idReserva || qtdPessoas <= 0 || (f != ESTUDO && f != PROVA)
            || tempo_dia(d) < 0 || !tempo_intervalo(ini, fim, &inicio, &termino)
            || instantePedido < 0)
        return RES_DADOS_INVALIDOS;

    int prova = tipoUsuario == PROFESSOR && f == PROVA;
    if (prova ? !grade_permiteProva(codSala, d, ini, fim, matricula)
              : grade_temAula(codSala, d, ini, fim, NULL))
        return RES_HORARIO_AULA;
    if (tipoUsuario == ALUNO && tipoSala == LABORATORIO
            && (!usu_existe(matMonitor) || !sala_temMonitorComBiometria(codSala, matMonitor)))
        return RES_LAB_SEM_MONITOR;

    Reserva *conflitos[MAX_RESERVAS];
    int qtdConflitos = 0;
    for (int i = 0; i < qtdReservas; i++) {
        if (conflita(&reservas[i], codSala, d, inicio, termino))
            conflitos[qtdConflitos++] = &reservas[i];
    }
    if (qtdReservas == MAX_RESERVAS)
        return RES_LIMITE_ATINGIDO;

    int resultado = resolverConflitos(conflitos, qtdConflitos, prova, instantePedido);
    if (resultado != RES_OK)
        return resultado;
    for (int i = 0; i < qtdConflitos; i++)
        conflitos[i]->situacao = CANCELADA;

    Reserva *reserva = &reservas[qtdReservas++];
    reserva->id = qtdReservas;
    strcpy(reserva->matricula, matricula);
    strcpy(reserva->codSala, codSala);
    strcpy(reserva->data, d);
    strcpy(reserva->inicio, ini);
    strcpy(reserva->fim, fim);
    reserva->qtdPessoas = qtdPessoas;
    reserva->finalidade = f;
    reserva->situacao = ATIVA;
    *idReserva = reserva->id;
    return RES_OK;
}

int res_cancelar(int idReserva, char *matricula)
{
    if (idReserva <= 0 || idReserva > qtdReservas)
        return RES_NAO_ENCONTRADA;
    Reserva *reserva = &reservas[idReserva - 1];
    if (reserva->situacao != ATIVA)
        return RES_NAO_ENCONTRADA;
    if (!matricula || strcmp(reserva->matricula, matricula) != 0)
        return RES_NAO_AUTORIZADO;

    reserva->situacao = CANCELADA;
    return RES_OK;
}

int res_listarPorUsuario(char *matricula, Reserva lista[], int *n)
{
    if (!n)
        return RES_NENHUMA_RESERVA;
    *n = 0;
    if (!matricula || !lista)
        return RES_NENHUMA_RESERVA;

    for (int i = 0; i < qtdReservas; i++) {
        if (strcmp(reservas[i].matricula, matricula) == 0)
            lista[(*n)++] = reservas[i];
    }
    return *n ? RES_OK : RES_NENHUMA_RESERVA;
}
