/*
 * Testes unitários do módulo Reserva (caixa-preta, automatizados).
 *
 * ISOLAMENTO DAS DEPENDÊNCIAS
 * Reserva depende de Usuário, Sala, Grade e Notificação. Neste executável
 * esses módulos NÃO são compilados (ver Makefile): no lugar deles estão os
 * stubs definidos abaixo, com respostas fixas e conhecidas. Assim, se um caso
 * falha, a causa só pode estar em Reserva. Os stubs são:
 *   - Usuário:     A1, A2, M1 = ALUNO; P1, P2 = PROFESSOR; resto inexistente.
 *   - Sala:        L201 (comum, 40, quadro), L105 (comum, 10), L301 (lab, 30);
 *                  só M1 tem biometria, e só na L301.
 *   - Grade:       aula na L201, 19/10/2026, 09h-11h, ministrante P1. Outro
 *                  professor só tem permissão se o caso fizer autorizacao = 1.
 *   - Notificação: não envia nada; conta as chamadas e guarda destinatários,
 *                  ou simula falha de entrega se recusarEntrega = 1.
 * Regras e Tempo são usados de verdade: são funções puras, sem estado, e têm
 * seus próprios testes (unit_regras, unit_tempo).
 *
 * INDEPENDÊNCIA ENTRE OS CASOS
 * O executor (Testes/test_unitarios.py) roda CADA caso em um processo novo.
 * Com isso o vetor de reservas e as variáveis dos stubs começam sempre
 * zerados: nenhum caso enxerga o que outro criou ou cancelou. Quando um caso
 * precisa de uma reserva prévia (por exemplo, a R1 de A1 das 14h às 16h), ele
 * mesmo a cria com prepararEstudo()/prepararReserva() — é o "estado inicial"
 * declarado do caso.
 *
 * MAPA DAS REGRAS DE NEGÓCIO
 *   RN1 capacidade ....... R3, capacidade_no_limite, qtd_invalida
 *   RN2 horário de aula .. R4, R5 (outro prof. sem autorização), R6 (outro prof.
 *                          com autorização), R7, R25 (próprio ministrante),
 *                          R26 (aluno pedindo prova), R27*
 *   RN3 laboratório ...... R8, R9, monitor_sem_biometria, monitor_inexistente,
 *                          professor_em_laboratorio
 *   RN4 prioridade ....... R10 (aluno x aluno), R11, R12, R13 (prof x prof),
 *                          R22-R24*, R28 (prof estudo x aluno), R28_aluno_contra_
 *                          professor, aluno_prova_contra_aluno, R29-R31
 *   RN5 cancelamento ..... R18, R19, R20, cancelar_duas_vezes
 */


#include "grade.h"
#include "notificacao.h"
#include "reserva.h"
#include "sala.h"
#include "usuario.h"
#include "unitario.h"

static const struct {
    const char *codigo;
    int capacidade;
    int quadro;
    TipoSala tipo;
} salas[] = {{"L201", 40, 1, COMUM}, {"L105", 10, 0, COMUM}, {"L301", 30, 0, LABORATORIO}};

static const char dataReferencia[] = "19/10/2026";
static const DataHora antesDasAulas = {"19/10/2026", "08:00"};
static int autorizacao;
static int recusarEntrega;
static int chamadasDeEntrega;
static int avisos;
static char destinatarios[2][TAM];

int usu_obterTipo(char *matricula, TipoUsuario *tipo)
{
    if (!matricula)
        return USU_NAO_ENCONTRADO;
    if (strcmp(matricula, "A1") == 0 || strcmp(matricula, "A2") == 0 || strcmp(matricula, "M1") == 0)
        *tipo = ALUNO;
    else if (strcmp(matricula, "P1") == 0 || strcmp(matricula, "P2") == 0)
        *tipo = PROFESSOR;
    else
        return USU_NAO_ENCONTRADO;
    return USU_OK;
}

int usu_existe(char *matricula)
{
    TipoUsuario tipo;
    return usu_obterTipo(matricula, &tipo) == USU_OK;
}

static int indiceSala(const char *codigo)
{
    if (!codigo)
        return -1;
    for (size_t i = 0; i < sizeof salas / sizeof salas[0]; i++) {
        if (strcmp(salas[i].codigo, codigo) == 0)
            return (int) i;
    }
    return -1;
}

int sala_obterCapacidade(char *codigo, int *capacidade)
{
    int i = indiceSala(codigo);
    if (i < 0)
        return SALA_NAO_ENCONTRADA;
    *capacidade = salas[i].capacidade;
    return SALA_OK;
}

int sala_obterTipo(char *codigo, TipoSala *tipo)
{
    int i = indiceSala(codigo);
    if (i < 0)
        return SALA_NAO_ENCONTRADA;
    *tipo = salas[i].tipo;
    return SALA_OK;
}

int sala_listarPorCapacidade(int qtd, int quadro, char lista[][TAM], int *n)
{
    *n = 0;
    for (size_t i = 0; i < sizeof salas / sizeof salas[0]; i++) {
        if (salas[i].capacidade >= qtd && (!quadro || salas[i].quadro))
            strcpy(lista[(*n)++], salas[i].codigo);
    }
    return *n ? SALA_OK : SALA_NENHUMA;
}

int sala_temMonitorComBiometria(char *codigo, char *matMonitor)
{
    return strcmp(codigo, "L301") == 0 && strcmp(matMonitor, "M1") == 0;
}

int grade_temAula(char *sala, const char *d, const char *ini, const char *fim, char *matMinistrante)
{
    (void) matMinistrante;
    return strcmp(sala, "L201") == 0 && strcmp(d, dataReferencia) == 0
        && strcmp(ini, "11:00") < 0 && strcmp(fim, "09:00") > 0;
}

int grade_permiteProva(char *sala, const char *d, const char *ini, const char *fim, char *solicitante)
{
    return !grade_temAula(sala, d, ini, fim, NULL)
        || strcmp(solicitante, "P1") == 0 || autorizacao;
}

int not_avisarCancelamentos(Reserva *const anteriores[], int quantidade)
{
    chamadasDeEntrega++;
    if (recusarEntrega)
        return NOT_FALHA_ENTREGA;
    VERIFICAR("tamanho do lote", quantidade >= 1 && quantidade <= 2);
    for (int i = 0; i < quantidade; i++)
        strcpy(destinatarios[i], anteriores[i]->matricula);
    avisos += quantidade;
    return NOT_OK;
}

static int solicitar(char *matricula, char *sala, const char *inicio, const char *fim,
                     Finalidade finalidade, int *id)
{
    return res_solicitar(matricula, sala, dataReferencia, inicio, fim, 5, finalidade,
                         "", antesDasAulas, id);
}

static int prepararReserva(char *matricula, const char *inicio, const char *fim, Finalidade finalidade)
{
    int id;

    VERIFICAR("preparo da reserva", solicitar(matricula, "L201", inicio, fim, finalidade, &id)
              == RES_OK && id > 0);
    return id;
}

static int prepararEstudo(void)
{
    return prepararReserva("A1", "14:00", "16:00", ESTUDO);
}

static void verificarSituacao(char *matricula, int id, Situacao situacao)
{
    Reserva lista[MAX_RESERVAS];
    int n;

    VERIFICAR("histórico do titular", res_listarPorUsuario(matricula, lista, &n) == RES_OK && n == 1);
    VERIFICAR("situação da reserva", lista[0].id == id && lista[0].situacao == situacao);
}

static void verificarSemProva(void)
{
    Reserva lista[MAX_RESERVAS];
    int n;

    VERIFICAR("sem prova registrada", res_listarPorUsuario("P1", lista, &n) == RES_NENHUMA_RESERVA && n == 0);
}

static int solicitarProvaNoHorario(const char *agora, const char *inicio, const char *fim, int *id)
{
    DataHora pedido = antesDasAulas;
    strcpy(pedido.hora, agora);
    return res_solicitar("P1", "L201", dataReferencia, inicio, fim, 5, PROVA, "", pedido, id);
}

static void verificarAceiteComPrazo(const char *agora)
{
    int anterior = prepararEstudo();
    int id;

    VERIFICAR("prova com prazo", solicitarProvaNoHorario(agora, "14:00", "16:00", &id) == RES_OK
              && id > 0 && id != anterior);
    VERIFICAR("aviso ao titular", chamadasDeEntrega == 1 && avisos == 1
              && strcmp(destinatarios[0], "A1") == 0);
    verificarSituacao("A1", anterior, CANCELADA);
    verificarSituacao("P1", id, ATIVA);
}

static void verificarRecusaPorPrazo(const char *agora)
{
    int anterior = prepararEstudo();
    int id;

    VERIFICAR("prazo insuficiente", solicitarProvaNoHorario(agora, "14:00", "16:00", &id)
              == RES_ANTECEDENCIA_INSUFICIENTE && id == 0);
    VERIFICAR("nenhuma tentativa de aviso", chamadasDeEntrega == 0);
    verificarSituacao("A1", anterior, ATIVA);
    verificarSemProva();
}

static void R1(void)
{
    int id;

    VERIFICAR("R1", solicitar("A1", "L201", "14:00", "16:00", ESTUDO, &id) == RES_OK && id > 0);
    verificarSituacao("A1", id, ATIVA);
}

static void R2(void)
{
    int id;

    VERIFICAR("R2", solicitar("X99", "L201", "14:00", "16:00", ESTUDO, &id) == RES_USUARIO_INEXISTENTE);
}

static void R3(void)
{
    int id;

    VERIFICAR("R3", res_solicitar("A1", "L105", dataReferencia, "14:00", "16:00", 20, ESTUDO,
              "", antesDasAulas, &id) == RES_CAPACIDADE_EXCEDIDA);
}

static void R4(void)
{
    int id;

    VERIFICAR("R4", solicitar("A1", "L201", "09:00", "10:00", ESTUDO, &id) == RES_HORARIO_AULA);
}

static void R5(void)
{
    int id;

    VERIFICAR("R5", solicitar("P2", "L201", "09:00", "10:00", PROVA, &id) == RES_HORARIO_AULA);
}

static void R6(void)
{
    int id;

    autorizacao = 1;
    VERIFICAR("R6", solicitar("P2", "L201", "09:00", "10:00", PROVA, &id) == RES_OK && id > 0);
    verificarSituacao("P2", id, ATIVA);
}

static void R7(void)
{
    int id;

    autorizacao = 1;
    VERIFICAR("R7", solicitar("P2", "L201", "09:00", "10:00", ESTUDO, &id) == RES_HORARIO_AULA);
}

static void R8(void)
{
    int id;

    VERIFICAR("R8", solicitar("A1", "L301", "14:00", "16:00", ESTUDO, &id) == RES_LAB_SEM_MONITOR);
}

static void R9(void)
{
    int id;

    VERIFICAR("R9", res_solicitar("A1", "L301", dataReferencia, "14:00", "16:00", 5, ESTUDO,
              "M1", antesDasAulas, &id) == RES_OK && id > 0);
}

static void R10(void)
{
    int anterior = prepararEstudo();
    int id;

    VERIFICAR("R10", solicitar("A2", "L201", "15:00", "17:00", ESTUDO, &id) == RES_CONFLITO);
    verificarSituacao("A1", anterior, ATIVA);
}

static void R11(void)
{
    verificarAceiteComPrazo("10:00");
}

static void R12(void)
{
    verificarRecusaPorPrazo("13:00");
}

static void R13(void)
{
    int anterior = prepararReserva("P1", "14:00", "16:00", PROVA);
    int id;

    VERIFICAR("R13", solicitar("P2", "L201", "14:00", "16:00", PROVA, &id)
              == RES_CONFLITO && chamadasDeEntrega == 0);
    verificarSituacao("P1", anterior, ATIVA);
}

static void R14(void)
{
    int anterior = prepararEstudo();
    int id;

    VERIFICAR("R14", res_verificarConflito("L201", dataReferencia, "15:00", "16:00", &id) == 1
              && id == anterior);
}

static void R15(void)
{
    int id = prepararEstudo();

    VERIFICAR("R15", res_verificarConflito("L201", dataReferencia, "16:00", "17:00", &id) == 0 && id == 0);
}

static void R16(void)
{
    char livres[MAX_SALAS][TAM];
    int n;

    prepararEstudo();
    VERIFICAR("R16", res_buscarSalasLivres(dataReferencia, "14:00", "16:00", 30, 0, "A1", livres, &n)
              == RES_NENHUMA_SALA && n == 0);
}

static void R17(void)
{
    char livres[MAX_SALAS][TAM];
    int n;

    prepararEstudo();
    VERIFICAR("R17", res_buscarSalasLivres(dataReferencia, "14:00", "16:00", 30, 0, "P1", livres, &n)
              == RES_OK && n == 1 && strcmp(livres[0], "L301") == 0);
}

static void R18(void)
{
    int id = prepararEstudo();

    VERIFICAR("R18", res_cancelar(id, "A1") == RES_OK
              && res_verificarConflito("L201", dataReferencia, "14:00", "16:00", NULL) == 0);
    verificarSituacao("A1", id, CANCELADA);
}

static void R19(void)
{
    int id = prepararEstudo();

    VERIFICAR("R19", res_cancelar(id, "P2") == RES_NAO_AUTORIZADO);
    verificarSituacao("A1", id, ATIVA);
}

static void R20(void)
{
    VERIFICAR("R20", res_cancelar(9999, "A1") == RES_NAO_ENCONTRADA);
}

static void R21(void)
{
    int id = prepararEstudo();
    Reserva lista[MAX_RESERVAS];
    int n;

    VERIFICAR("R21", res_listarPorUsuario("A1", lista, &n) == RES_OK && n == 1
              && lista[0].id == id && lista[0].situacao == ATIVA
              && lista[0].qtdPessoas == 5 && lista[0].finalidade == ESTUDO
              && strcmp(lista[0].matricula, "A1") == 0 && strcmp(lista[0].codSala, "L201") == 0
              && strcmp(lista[0].data, dataReferencia) == 0 && strcmp(lista[0].inicio, "14:00") == 0
              && strcmp(lista[0].fim, "16:00") == 0);
}

static void R21_sem_reservas(void)
{
    Reserva lista[MAX_RESERVAS];
    int n;

    VERIFICAR("R21 sem reservas", res_listarPorUsuario("A2", lista, &n) == RES_NENHUMA_RESERVA && n == 0);
}

static void R22(void)
{
    verificarAceiteComPrazo("11:00");
}

static void R23(void)
{
    verificarAceiteComPrazo("12:00");
}

static void R24(void)
{
    verificarRecusaPorPrazo("12:01");
}

static void R24_noventa_minutos(void)
{
    verificarRecusaPorPrazo("12:30");
}

static void R25(void)
{
    int id;

    VERIFICAR("R25", solicitar("P1", "L201", "09:00", "10:00", PROVA, &id) == RES_OK && id > 0);
}

static void R26(void)
{
    int id;

    autorizacao = 1;
    VERIFICAR("R26", solicitar("A1", "L201", "09:00", "10:00", PROVA, &id) == RES_HORARIO_AULA);
}

static void R27(void)
{
    int id;

    VERIFICAR("R27 recusa da Grade", solicitar("P2", "L201", "10:00", "12:00", PROVA, &id)
              == RES_HORARIO_AULA);
}

static void R27_ambas_aulas_autorizam(void)
{
    int id;

    autorizacao = 1;
    VERIFICAR("R27 aceite da Grade", solicitar("P2", "L201", "10:00", "12:00", PROVA, &id)
              == RES_OK && id > 0);
}

static void R28(void)
{
    int anterior = prepararEstudo();
    int id;

    VERIFICAR("R28 estudo de professor", solicitar("P1", "L201", "14:00", "16:00", ESTUDO, &id)
              == RES_CONFLITO && chamadasDeEntrega == 0);
    verificarSituacao("A1", anterior, ATIVA);
}

static void R28_aluno_contra_professor(void)
{
    int anterior = prepararReserva("P1", "14:00", "16:00", PROVA);
    int id;

    VERIFICAR("R28 aluno contra professor", solicitar("A1", "L201", "14:00", "16:00", ESTUDO, &id)
              == RES_CONFLITO && chamadasDeEntrega == 0);
    verificarSituacao("P1", anterior, ATIVA);
}

static void R29(void)
{
    int anterior = prepararEstudo();
    int id;

    recusarEntrega = 1;
    VERIFICAR("R29", solicitarProvaNoHorario("10:00", "14:00", "16:00", &id)
              == RES_FALHA_NOTIFICACAO && id == 0 && chamadasDeEntrega == 1 && avisos == 0);
    verificarSituacao("A1", anterior, ATIVA);
    verificarSemProva();
}

static void R30(void)
{
    int primeiro = prepararReserva("A1", "14:00", "15:00", ESTUDO);
    int segundo = prepararReserva("A2", "15:00", "16:00", ESTUDO);
    int id;

    VERIFICAR("R30", solicitarProvaNoHorario("12:00", "14:30", "15:30", &id) == RES_OK
              && chamadasDeEntrega == 1 && avisos == 2
              && strcmp(destinatarios[0], "A1") == 0 && strcmp(destinatarios[1], "A2") == 0);
    verificarSituacao("A1", primeiro, CANCELADA);
    verificarSituacao("A2", segundo, CANCELADA);
    verificarSituacao("P1", id, ATIVA);
}

static void R31(void)
{
    /* O primeiro conflito encontrado tem prazo suficiente. */
    int segundo = prepararReserva("A2", "15:00", "16:00", ESTUDO);
    int primeiro = prepararReserva("A1", "14:00", "15:00", ESTUDO);
    int id;

    VERIFICAR("R31", solicitarProvaNoHorario("12:01", "14:30", "15:30", &id)
              == RES_ANTECEDENCIA_INSUFICIENTE && id == 0 && chamadasDeEntrega == 0);
    verificarSituacao("A1", primeiro, ATIVA);
    verificarSituacao("A2", segundo, ATIVA);
    verificarSemProva();
}

static void prazo_na_virada_do_ano(void)
{
    const char data[] = "01/01/2027";
    const DataHora antes = {"31/12/2026", "23:30"};
    int anterior, id;

    VERIFICAR("preparo estudo", res_solicitar("A1", "L201", data, "01:30", "02:30", 5,
              ESTUDO, "", antes, &anterior) == RES_OK);
    VERIFICAR("duas horas na virada do ano", res_solicitar("P1", "L201", data, "01:30", "02:30",
              5, PROVA, "", antes, &id) == RES_OK && chamadasDeEntrega == 1 && avisos == 1);
    verificarSituacao("A1", anterior, CANCELADA);
    verificarSituacao("P1", id, ATIVA);
}

static void laboratorio_liberado(void)
{
    int id;

    VERIFICAR("preparo laboratório", res_solicitar("A1", "L301", dataReferencia, "14:00", "16:00",
              5, ESTUDO, "M1", antesDasAulas, &id) == RES_OK);
    VERIFICAR("cancelar estudo", res_cancelar(id, "A1") == RES_OK);
    VERIFICAR("lab liberado após cancelamento", solicitar("P1", "L301", "14:00", "16:00", PROVA, &id) == RES_OK);
}

static void sala_ausente(void)
{
    int id;

    VERIFICAR("sala ausente", solicitar("P1", "L999", "14:00", "16:00", PROVA, &id) == RES_SALA_INEXISTENTE
              && id == 0);
}

static void intervalo_invalido(void)
{
    int id;

    VERIFICAR("intervalo inválido", solicitar("P1", "L201", "16:00", "14:00", PROVA, &id) == RES_DADOS_INVALIDOS
              && id == 0);
}

static void capacidade_no_limite(void)
{
    int id;

    VERIFICAR("capacidade no limite", res_solicitar("A1", "L105", dataReferencia, "14:00", "16:00", 10,
              ESTUDO, "", antesDasAulas, &id) == RES_OK && id > 0);
}

static void qtd_invalida(void)
{
    int id;

    VERIFICAR("zero pessoas", res_solicitar("A1", "L201", dataReferencia, "14:00", "16:00", 0,
              ESTUDO, "", antesDasAulas, &id) == RES_DADOS_INVALIDOS && id == 0);
}

static void monitor_sem_biometria(void)
{
    int id;

    /* A2 existe, mas não tem biometria na L301. */
    VERIFICAR("monitor sem biometria", res_solicitar("A1", "L301", dataReferencia, "14:00", "16:00", 5,
              ESTUDO, "A2", antesDasAulas, &id) == RES_LAB_SEM_MONITOR && id == 0);
}

static void monitor_inexistente(void)
{
    int id;

    VERIFICAR("monitor inexistente", res_solicitar("A1", "L301", dataReferencia, "14:00", "16:00", 5,
              ESTUDO, "X99", antesDasAulas, &id) == RES_LAB_SEM_MONITOR && id == 0);
}

static void professor_em_laboratorio(void)
{
    int id;

    VERIFICAR("professor dispensa monitor", solicitar("P2", "L301", "14:00", "16:00", ESTUDO, &id)
              == RES_OK && id > 0);
}

static void aluno_prova_contra_aluno(void)
{
    int anterior = prepararEstudo();
    int id;

    /* Finalidade PROVA não dá prioridade a aluno. */
    VERIFICAR("aluno pedindo prova", solicitar("A2", "L201", "14:00", "16:00", PROVA, &id)
              == RES_CONFLITO && chamadasDeEntrega == 0);
    verificarSituacao("A1", anterior, ATIVA);
}

static void prova_sem_conflito_nao_avisa(void)
{
    int id;

    VERIFICAR("prova em sala livre", solicitar("P1", "L201", "14:00", "16:00", PROVA, &id) == RES_OK
              && id > 0 && chamadasDeEntrega == 0);
}

static void cancelar_duas_vezes(void)
{
    int id = prepararEstudo();

    VERIFICAR("primeiro cancelamento", res_cancelar(id, "A1") == RES_OK);
    VERIFICAR("segundo cancelamento", res_cancelar(id, "A1") == RES_NAO_ENCONTRADA);
    verificarSituacao("A1", id, CANCELADA);
}

static void busca_com_quadro(void)
{
    char livres[MAX_SALAS][TAM];
    int n;

    VERIFICAR("só L201 tem quadro", res_buscarSalasLivres(dataReferencia, "14:00", "16:00", 5, 1, "P1",
              livres, &n) == RES_OK && n == 1 && strcmp(livres[0], "L201") == 0);
}

static void busca_ignora_cancelada(void)
{
    char livres[MAX_SALAS][TAM];
    int id = prepararEstudo();
    int n;

    VERIFICAR("cancelar", res_cancelar(id, "A1") == RES_OK);
    VERIFICAR("sala volta à busca", res_buscarSalasLivres(dataReferencia, "14:00", "16:00", 30, 0, "A1",
              livres, &n) == RES_OK && n == 1 && strcmp(livres[0], "L201") == 0);
}

static void ids_sequenciais(void)
{
    int primeiro = prepararReserva("A1", "12:00", "13:00", ESTUDO);
    int segundo = prepararReserva("A2", "13:00", "14:00", ESTUDO);

    VERIFICAR("ids sequenciais", primeiro == 1 && segundo == 2);
}

int main(int argc, char *argv[])
{
    const CasoUnitario casos[] = {
        CASO(R1), CASO(R2), CASO(R3), CASO(R4), CASO(R5), CASO(R6), CASO(R7), CASO(R8), CASO(R9),
        CASO(R10), CASO(R11), CASO(R12), CASO(R13), CASO(R14), CASO(R15), CASO(R16), CASO(R17),
        CASO(R18), CASO(R19), CASO(R20), CASO(R21), CASO(R21_sem_reservas), CASO(R22), CASO(R23),
        CASO(R24), CASO(R24_noventa_minutos), CASO(R25), CASO(R26), CASO(R27),
        CASO(R27_ambas_aulas_autorizam), CASO(R28), CASO(R28_aluno_contra_professor), CASO(R29),
        CASO(R30), CASO(R31), CASO(prazo_na_virada_do_ano), CASO(laboratorio_liberado),
        CASO(sala_ausente), CASO(intervalo_invalido), CASO(capacidade_no_limite), CASO(qtd_invalida),
        CASO(monitor_sem_biometria), CASO(monitor_inexistente), CASO(professor_em_laboratorio),
        CASO(aluno_prova_contra_aluno), CASO(prova_sem_conflito_nao_avisa), CASO(cancelar_duas_vezes),
        CASO(busca_com_quadro), CASO(busca_ignora_cancelada), CASO(ids_sequenciais)
    };
    return executarCasos(argc, argv, casos, sizeof casos / sizeof casos[0]);
}
