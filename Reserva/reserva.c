
/*
 * Módulo Reserva
 *
 * Papel: coordenar as operações de reserva e guardar as reservas
 * (encapsuladas no vetor estático abaixo). As regras de negócio em si ficam no
 * módulo Regras; os dados de usuários, salas e aulas ficam nos seus módulos.
 *
 * Consistência: res_solicitar faz TODAS as verificações (inclusive o envio dos
 * avisos) antes de alterar qualquer reserva. Se algo falha, o estado fica
 * exatamente como estava antes da chamada.
 */

#include "reserva.h"

#include <string.h>

#include "grade.h"
#include "notificacao.h"
#include "regras.h"
#include "sala.h"
#include "tempo.h"
#include "usuario.h"

static Reserva reservas[MAX_RESERVAS];
static int qtdReservas;

/* Dados de um pedido já validado, para não recalcular em cada etapa. */
typedef struct {
    TipoUsuario tipoUsuario;
    TipoSala tipoSala;
    int inicio;
    int termino;
    int64_t agora;
} Pedido;

/* ---------------------------------------------------------------- auxiliares */

/* Datas e horas armazenadas já têm formato validado e canônico. */
static int conflita(const Reserva *reserva, const char *sala, const char *data,
                    int inicio, int fim)
{
    return reserva->situacao == ATIVA && strcmp(reserva->codSala, sala) == 0
        && strcmp(reserva->data, data) == 0
        && tempo_sobrepoe(tempo_minuto(reserva->inicio), tempo_minuto(reserva->fim), inicio, fim);
}

static int64_t instanteInicio(const Reserva *reserva)
{
    DataHora inicio;

    strcpy(inicio.data, reserva->data);
    strcpy(inicio.hora, reserva->inicio);
    return tempo_instante(inicio);
}

static Reserva *buscarPorId(int idReserva)
{
    if (idReserva <= 0 || idReserva > qtdReservas)
        return NULL;
    return &reservas[idReserva - 1];
}

/* ------------------------------------------------- etapas de res_solicitar */

/* Etapa 1: usuário, sala e formato dos dados. */
static int validarPedido(char *matricula, char *codSala, const char *d, const char *ini,
                         const char *fim, int qtdPessoas, Finalidade f, DataHora agora,
                         int *idReserva, Pedido *pedido)
{
    int capacidade;

    if (usu_obterTipo(matricula, &pedido->tipoUsuario) != USU_OK)
        return RES_USUARIO_INEXISTENTE;
    if (sala_obterCapacidade(codSala, &capacidade) != SALA_OK
            || sala_obterTipo(codSala, &pedido->tipoSala) != SALA_OK)
        return RES_SALA_INEXISTENTE;

    pedido->agora = tempo_instante(agora);
    if (!idReserva || qtdPessoas <= 0 || (f != ESTUDO && f != PROVA)
            || tempo_dia(d) < 0 || !tempo_intervalo(ini, fim, &pedido->inicio, &pedido->termino)
            || pedido->agora < 0)
        return RES_DADOS_INVALIDOS;

    if (!regra_comportaPessoas(qtdPessoas, capacidade))
        return RES_CAPACIDADE_EXCEDIDA;
    return RES_OK;
}

/* Etapa 2: horário de aula (RN2). A Grade diz se há aula e se o professor é
   ministrante/autorizado; a regra diz quem pode sequer tentar. */
static int verificarHorarioDeAula(char *matricula, char *codSala, const char *d,
                                  const char *ini, const char *fim, Finalidade f,
                                  const Pedido *pedido)
{
    if (!grade_temAula(codSala, d, ini, fim, NULL))
        return RES_OK;
    if (regra_podeUsarHorarioDeAula(pedido->tipoUsuario, f)
            && grade_permiteProva(codSala, d, ini, fim, matricula))
        return RES_OK;
    return RES_HORARIO_AULA;
}

/* Etapa 3: laboratório (RN3). */
static int verificarLaboratorio(char *codSala, char *matMonitor, const Pedido *pedido)
{
    if (!regra_exigeMonitor(pedido->tipoUsuario, pedido->tipoSala))
        return RES_OK;
    if (usu_existe(matMonitor) && sala_temMonitorComBiometria(codSala, matMonitor))
        return RES_OK;
    return RES_LAB_SEM_MONITOR;
}

/* Etapa 4: reservas ativas que se sobrepõem ao pedido. */
static int coletarConflitos(char *codSala, const char *d, const Pedido *pedido,
                            Reserva *conflitos[])
{
    int qtd = 0;

    for (int i = 0; i < qtdReservas; i++) {
        if (conflita(&reservas[i], codSala, d, pedido->inicio, pedido->termino))
            conflitos[qtd++] = &reservas[i];
    }
    return qtd;
}

/* Etapa 5: prioridade (RN4). Só consulta e avisa; não altera nada. */
static int resolverConflitos(Reserva *conflitos[], int qtdConflitos, Finalidade f,
                             const Pedido *pedido)
{
    if (qtdConflitos == 0)
        return RES_OK;

    for (int i = 0; i < qtdConflitos; i++) {
        TipoUsuario titular;
        if (usu_obterTipo(conflitos[i]->matricula, &titular) != USU_OK
                || !regra_podeSubstituir(pedido->tipoUsuario, f, titular))
            return RES_CONFLITO;
    }
    for (int i = 0; i < qtdConflitos; i++) {
        if (!regra_antecedenciaSuficiente(instanteInicio(conflitos[i]), pedido->agora))
            return RES_ANTECEDENCIA_INSUFICIENTE;
    }
    if (not_avisarCancelamentos(conflitos, qtdConflitos) != NOT_OK)
        return RES_FALHA_NOTIFICACAO;
    return RES_OK;
}

/* Etapa 6: única etapa que altera o estado. */
static int registrarReserva(char *matricula, char *codSala, const char *d, const char *ini,
                            const char *fim, int qtdPessoas, Finalidade f)
{
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
    return reserva->id;
}

/* --------------------------------------------------------------- interface */

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
                || regra_exigeMonitor(tipoUsuario, tipoSala)
                || grade_temAula(sala, d, ini, fim, NULL)
                || res_verificarConflito(sala, d, ini, fim, NULL))
            continue;
        strcpy(lista[(*n)++], sala);
    }
    return *n ? RES_OK : RES_NENHUMA_SALA;
}

int res_solicitar(char *matricula, char *codSala, const char *d, const char *ini, const char *fim,
                  int qtdPessoas, Finalidade f, char *matMonitor, DataHora agora,
                  int *idReserva)
{
    Pedido pedido;
    Reserva *conflitos[MAX_RESERVAS];
    int resultado;

    if (idReserva)
        *idReserva = 0;

    resultado = validarPedido(matricula, codSala, d, ini, fim, qtdPessoas, f, agora,
                              idReserva, &pedido);
    if (resultado == RES_OK)
        resultado = verificarHorarioDeAula(matricula, codSala, d, ini, fim, f, &pedido);
    if (resultado == RES_OK)
        resultado = verificarLaboratorio(codSala, matMonitor, &pedido);
    if (resultado != RES_OK)
        return resultado;

    int qtdConflitos = coletarConflitos(codSala, d, &pedido, conflitos);
    if (qtdReservas == MAX_RESERVAS)
        return RES_LIMITE_ATINGIDO;
    resultado = resolverConflitos(conflitos, qtdConflitos, f, &pedido);
    if (resultado != RES_OK)
        return resultado;

    /* A partir daqui nada pode falhar: o estado muda de uma vez. */
    for (int i = 0; i < qtdConflitos; i++)
        conflitos[i]->situacao = CANCELADA;
    *idReserva = registrarReserva(matricula, codSala, d, ini, fim, qtdPessoas, f);
    return RES_OK;
}

int res_cancelar(int idReserva, char *matricula)
{
    Reserva *reserva = buscarPorId(idReserva);

    if (!reserva || reserva->situacao != ATIVA)
        return RES_NAO_ENCONTRADA;
    if (!regra_podeCancelar(reserva->matricula, matricula))
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
