
#include "sala.h"
#include "unitario.h"

static void prepararLaboratorio(void)
{
    VERIFICAR("preparo L301", sala_cadastrar("L301", LABORATORIO, 30, 0) == SALA_OK);
    VERIFICAR("preparo M1", sala_registrarBiometria("L301", "M1") == SALA_OK);
}

static void prepararSalas(void)
{
    VERIFICAR("preparo L201", sala_cadastrar("L201", COMUM, 40, 1) == SALA_OK);
    VERIFICAR("preparo L105", sala_cadastrar("L105", COMUM, 10, 0) == SALA_OK);
    prepararLaboratorio();
}

static void S1(void)
{
    VERIFICAR("S1", sala_validarCodigo("L201") == 1);
}

static void S2(void)
{
    VERIFICAR("S2", !sala_validarCodigo("201L") && !sala_validarCodigo("L")
              && !sala_validarCodigo("LX01"));
}

static void S3(void)
{
    VERIFICAR("S3", sala_cadastrar("L402", COMUM, 25, 1) == SALA_OK);
}

static void S4(void)
{
    VERIFICAR("S4", sala_cadastrar("abc", COMUM, 25, 1) == SALA_CODIGO_INVALIDO);
}

static void S5(void)
{
    VERIFICAR("preparo L201", sala_cadastrar("L201", COMUM, 40, 1) == SALA_OK);
    VERIFICAR("S5", sala_cadastrar("L201", COMUM, 25, 1) == SALA_JA_EXISTE);
}

static void S6(void)
{
    VERIFICAR("S6", sala_cadastrar("L111", COMUM, 0, 0) == SALA_CAPACIDADE_INVALIDA);
}

static void S7(void)
{
    int capacidade;

    VERIFICAR("preparo L105", sala_cadastrar("L105", COMUM, 10, 0) == SALA_OK);
    VERIFICAR("S7", sala_obterCapacidade("L105", &capacidade) == SALA_OK && capacidade == 10);
}

static void S7_ausente(void)
{
    int capacidade;

    VERIFICAR("S7", sala_obterCapacidade("L999", &capacidade) == SALA_NAO_ENCONTRADA);
}

static void S8(void)
{
    TipoSala tipo;

    prepararLaboratorio();
    VERIFICAR("S8", sala_obterTipo("L301", &tipo) == SALA_OK && tipo == LABORATORIO);
}

static void S8_ausente(void)
{
    TipoSala tipo;

    VERIFICAR("S8", sala_obterTipo("L999", &tipo) == SALA_NAO_ENCONTRADA);
}

static void S9(void)
{
    char salas[MAX_SALAS][TAM];
    int n;

    prepararSalas();
    VERIFICAR("S9", sala_listarPorCapacidade(30, 0, salas, &n) == SALA_OK && n == 2
              && strcmp(salas[0], "L201") == 0 && strcmp(salas[1], "L301") == 0);
}

static void S10(void)
{
    char salas[MAX_SALAS][TAM];
    int n;

    prepararSalas();
    VERIFICAR("S10", sala_listarPorCapacidade(15, 1, salas, &n) == SALA_OK && n == 1
              && strcmp(salas[0], "L201") == 0);
}

static void S11(void)
{
    char salas[MAX_SALAS][TAM];
    int n;

    prepararSalas();
    VERIFICAR("S11", sala_listarPorCapacidade(100, 0, salas, &n) == SALA_NENHUMA && n == 0);
}

static void S12(void)
{
    VERIFICAR("S12", sala_registrarBiometria("L999", "M1") == SALA_NAO_ENCONTRADA);
}

static void S13(void)
{
    prepararLaboratorio();
    VERIFICAR("S13", sala_temMonitorComBiometria("L301", "M1") == 1);
}

static void S13_sem_vinculo(void)
{
    prepararLaboratorio();
    VERIFICAR("S13", sala_temMonitorComBiometria("L301", "A1") == 0);
}

static void validar_sem_cadastrar(void)
{
    int capacidade;

    VERIFICAR("validar sem cadastrar", sala_verificarCadastro("L402", COMUM, 25, 1) == SALA_OK
              && sala_obterCapacidade("L402", &capacidade) == SALA_NAO_ENCONTRADA);
}

static void biometria_por_laboratorio(void)
{
    prepararLaboratorio();
    VERIFICAR("preparo L302", sala_cadastrar("L302", LABORATORIO, 30, 0) == SALA_OK);
    VERIFICAR("biometria em laboratório específico", !sala_temMonitorComBiometria("L302", "M1"));
}

static void repetir_biometria(void)
{
    prepararLaboratorio();
    VERIFICAR("repetição de biometria", sala_registrarBiometria("L301", "M1") == SALA_OK);
}

int main(int argc, char *argv[])
{
    const CasoUnitario casos[] = {
        CASO(S1), CASO(S2), CASO(S3), CASO(S4), CASO(S5), CASO(S6), CASO(S7), CASO(S7_ausente),
        CASO(S8), CASO(S8_ausente), CASO(S9), CASO(S10), CASO(S11), CASO(S12), CASO(S13),
        CASO(S13_sem_vinculo), CASO(validar_sem_cadastrar), CASO(biometria_por_laboratorio),
        CASO(repetir_biometria)
    };
    return executarCasos(argc, argv, casos, sizeof casos / sizeof casos[0]);
}
