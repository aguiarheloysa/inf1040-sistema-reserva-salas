/*
 * Testes unitários do módulo Regras.
 *
 * Como as funções são puras (sem estado e sem chamar outros módulos), não há
 * stubs nem preparação de base: o resultado depende só da entrada. Isso
 * permite cobrir cada regra de negócio por completo, combinação a combinação,
 * em vez de apenas por função.
 */

#include "regras.h"
#include "unitario.h"

/* RN1 — capacidade */

static void RG1_capacidade(void)
{
    VERIFICAR("abaixo do limite", regra_comportaPessoas(5, 10));
    VERIFICAR("exatamente no limite", regra_comportaPessoas(10, 10));
    VERIFICAR("acima do limite", !regra_comportaPessoas(11, 10));
    VERIFICAR("zero pessoas", !regra_comportaPessoas(0, 10));
}

/* RN2 — quem pode tentar usar horário de aula */

static void RG2_horario_de_aula(void)
{
    VERIFICAR("professor + prova", regra_podeUsarHorarioDeAula(PROFESSOR, PROVA));
    VERIFICAR("professor + estudo", !regra_podeUsarHorarioDeAula(PROFESSOR, ESTUDO));
    VERIFICAR("aluno + prova", !regra_podeUsarHorarioDeAula(ALUNO, PROVA));
    VERIFICAR("aluno + estudo", !regra_podeUsarHorarioDeAula(ALUNO, ESTUDO));
}

/* RN3 — laboratório */

static void RG3_monitor(void)
{
    VERIFICAR("aluno em laboratório", regra_exigeMonitor(ALUNO, LABORATORIO));
    VERIFICAR("aluno em sala comum", !regra_exigeMonitor(ALUNO, COMUM));
    VERIFICAR("professor em laboratório", !regra_exigeMonitor(PROFESSOR, LABORATORIO));
    VERIFICAR("professor em sala comum", !regra_exigeMonitor(PROFESSOR, COMUM));
}

/* RN4 — prioridade: todas as 8 combinações solicitante x finalidade x titular */

static void RG4_prioridade(void)
{
    VERIFICAR("professor/prova x aluno", regra_podeSubstituir(PROFESSOR, PROVA, ALUNO));
    VERIFICAR("professor/prova x professor", !regra_podeSubstituir(PROFESSOR, PROVA, PROFESSOR));
    VERIFICAR("professor/estudo x aluno", !regra_podeSubstituir(PROFESSOR, ESTUDO, ALUNO));
    VERIFICAR("professor/estudo x professor", !regra_podeSubstituir(PROFESSOR, ESTUDO, PROFESSOR));
    VERIFICAR("aluno/prova x aluno", !regra_podeSubstituir(ALUNO, PROVA, ALUNO));
    VERIFICAR("aluno/prova x professor", !regra_podeSubstituir(ALUNO, PROVA, PROFESSOR));
    VERIFICAR("aluno/estudo x aluno", !regra_podeSubstituir(ALUNO, ESTUDO, ALUNO));
    VERIFICAR("aluno/estudo x professor", !regra_podeSubstituir(ALUNO, ESTUDO, PROFESSOR));
}

/* RN4 — antecedência (valores-limite em torno de 120 minutos) */

static void RG5_antecedencia(void)
{
    const int64_t inicio = 1000;

    VERIFICAR("4 horas antes", regra_antecedenciaSuficiente(inicio, inicio - 240));
    VERIFICAR("121 minutos antes", regra_antecedenciaSuficiente(inicio, inicio - 121));
    VERIFICAR("exatamente 120 minutos", regra_antecedenciaSuficiente(inicio, inicio - 120));
    VERIFICAR("119 minutos antes", !regra_antecedenciaSuficiente(inicio, inicio - 119));
    VERIFICAR("no próprio início", !regra_antecedenciaSuficiente(inicio, inicio));
    VERIFICAR("depois do início", !regra_antecedenciaSuficiente(inicio, inicio + 30));
}

/* RN5 — cancelamento */

static void RG6_cancelamento(void)
{
    VERIFICAR("titular", regra_podeCancelar("A1", "A1"));
    VERIFICAR("outro usuário", !regra_podeCancelar("A1", "P2"));
    VERIFICAR("matrícula nula", !regra_podeCancelar("A1", NULL));
    VERIFICAR("matrícula vazia", !regra_podeCancelar("", ""));
}

int main(int argc, char *argv[])
{
    const CasoUnitario casos[] = {
        CASO(RG1_capacidade), CASO(RG2_horario_de_aula), CASO(RG3_monitor),
        CASO(RG4_prioridade), CASO(RG5_antecedencia), CASO(RG6_cancelamento)
    };
    return executarCasos(argc, argv, casos, sizeof casos / sizeof casos[0]);
}
