
#include "grade.h"
#include "unitario.h"

static const char dataReferencia[] = "19/10/2026";

static void prepararAula(void)
{
    VERIFICAR("preparo", grade_cadastrarAula("L201", "Cálculo", "2001", 1, "09:00", "11:00") == GRADE_OK);
}

static void prepararAutorizacao(void)
{
    prepararAula();
    VERIFICAR("preparo autorização", grade_registrarAutorizacao("L201", dataReferencia,
              "09:00", "2001", "2002") == GRADE_OK);
}

static void adicionarSegundaAula(void)
{
    VERIFICAR("aulas consecutivas", grade_cadastrarAula("L201", "Física", "2003", 1, "11:00", "12:00") == GRADE_OK);
}

static void G1(void)
{
    VERIFICAR("G1", grade_cadastrarAula("L105", "Física", "2002", 1, "14:00", "16:00") == GRADE_OK);
}

static void G2(void)
{
    prepararAula();
    VERIFICAR("G2", grade_cadastrarAula("L201", "Física", "2002", 1, "10:00", "12:00") == GRADE_CONFLITO);
}

static void G3(void)
{
    VERIFICAR("G3", grade_cadastrarAula("L105", "Física", "2002", 1, "16:00", "14:00") == GRADE_HORARIO_INVALIDO);
}

static void G4(void)
{
    char ministrante[TAM];

    prepararAula();
    VERIFICAR("G4", grade_temAula("L201", dataReferencia, "09:00", "10:00", ministrante) == 1
              && strcmp(ministrante, "2001") == 0);
}

static void G5(void)
{
    prepararAula();
    VERIFICAR("G5", grade_temAula("L201", dataReferencia, "13:00", "14:00", NULL) == 0);
}

static void G6(void)
{
    prepararAula();
    VERIFICAR("G6", grade_registrarAutorizacao("L201", dataReferencia, "09:00", "2001", "2002") == GRADE_OK);
}

static void G7(void)
{
    prepararAula();
    VERIFICAR("G7", grade_registrarAutorizacao("L201", dataReferencia, "09:00", "2002", "2002") == GRADE_NAO_MINISTRANTE);
}

static void G8(void)
{
    prepararAula();
    VERIFICAR("G8", grade_registrarAutorizacao("L201", dataReferencia, "13:00", "2001", "2002") == GRADE_SEM_AULA);
}

static void G9(void)
{
    prepararAutorizacao();
    VERIFICAR("G9", grade_temAutorizacao("L201", dataReferencia, "09:00", "2002") == 1);
}

static void G9_sem_autorizacao(void)
{
    prepararAutorizacao();
    VERIFICAR("G9", grade_temAutorizacao("L201", dataReferencia, "09:00", "1001") == 0);
}

static void recorrencia_semanal(void)
{
    prepararAula();
    VERIFICAR("recorrência semanal", grade_temAula("L201", "26/10/2026", "09:00", "10:00", NULL)
              && !grade_temAula("L201", "20/10/2026", "09:00", "10:00", NULL));
}

static void autorizacao_por_data_e_hora(void)
{
    prepararAutorizacao();
    VERIFICAR("autorização por data e hora", !grade_temAutorizacao("L201", "26/10/2026", "09:00", "2002")
              && !grade_temAutorizacao("L201", dataReferencia, "09:01", "2002"));
}

static void autorizacao_parcial(void)
{
    prepararAutorizacao();
    adicionarSegundaAula();
    VERIFICAR("só uma aula autoriza", !grade_permiteProva("L201", dataReferencia, "08:00", "11:30", "2002"));
}

static void autorizacao_de_todas_as_aulas(void)
{
    prepararAutorizacao();
    adicionarSegundaAula();
    VERIFICAR("preparo segunda autorização", grade_registrarAutorizacao("L201", dataReferencia,
              "11:00", "2003", "2002") == GRADE_OK);
    VERIFICAR("todas as aulas autorizam", grade_permiteProva("L201", dataReferencia, "08:00", "11:30", "2002"));
}

static void ministrante(void)
{
    prepararAula();
    VERIFICAR("ministrante", grade_permiteProva("L201", dataReferencia, "10:00", "11:00", "2001"));
}

static void dia_invalido(void)
{
    VERIFICAR("dia inválido", grade_cadastrarAula("L201", "Física", "2003", 8, "08:00", "09:00") == GRADE_HORARIO_INVALIDO);
}

static void matricula_do_professor_invalida(void)
{
    VERIFICAR("professor com matrícula inválida", grade_cadastrarAula("L201", "Física", "P2003",
              1, "08:00", "09:00") == GRADE_DADOS_INVALIDOS);
}

static void matricula_do_solicitante_invalida(void)
{
    prepararAula();
    VERIFICAR("solicitante com matrícula inválida", grade_registrarAutorizacao("L201", dataReferencia,
              "09:00", "2001", "P2002") == GRADE_DADOS_INVALIDOS);
}

int main(int argc, char *argv[])
{
    const CasoUnitario casos[] = {
        CASO(G1), CASO(G2), CASO(G3), CASO(G4), CASO(G5), CASO(G6), CASO(G7), CASO(G8), CASO(G9),
        CASO(G9_sem_autorizacao), CASO(recorrencia_semanal), CASO(autorizacao_por_data_e_hora),
        CASO(autorizacao_parcial), CASO(autorizacao_de_todas_as_aulas), CASO(ministrante), CASO(dia_invalido),
        CASO(matricula_do_professor_invalida), CASO(matricula_do_solicitante_invalida)
    };
    return executarCasos(argc, argv, casos, sizeof casos / sizeof casos[0]);
}
