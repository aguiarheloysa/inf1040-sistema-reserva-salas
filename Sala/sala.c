
#include "sala.h"

#include <string.h>

#define MAX_BIOMETRIAS 1000

typedef struct {
    char codigo[TAM_COD];
    TipoSala tipo;
    int capacidade;
    int temQuadro;
} Sala;

typedef struct {
    const Sala *sala;
    char matricula[TAM];
} Biometria;

static Sala salas[MAX_SALAS];
static Biometria biometrias[MAX_BIOMETRIAS];
static int qtdSalas;
static int qtdBiometrias;

static const Sala *buscar(const char *codigo)
{
    if (!codigo)
        return NULL;
    for (int i = 0; i < qtdSalas; i++) {
        if (strcmp(salas[i].codigo, codigo) == 0)
            return &salas[i];
    }
    return NULL;
}

int sala_validarCodigo(char *codigo)
{
    if (!codigo)
        return 0;
    size_t tamanho = strlen(codigo);
    if (tamanho < 3 || tamanho >= TAM_COD || codigo[0] < 'A' || codigo[0] > 'Z')
        return 0;
    for (size_t i = 1; i < tamanho; i++) {
        if (codigo[i] < '0' || codigo[i] > '9')
            return 0;
    }
    return 1;
}

int sala_verificarCadastro(char *codigo, TipoSala tipo, int capacidade, int temQuadro)
{
    if (!sala_validarCodigo(codigo))
        return SALA_CODIGO_INVALIDO;
    if (buscar(codigo))
        return SALA_JA_EXISTE;
    if (capacidade <= 0)
        return SALA_CAPACIDADE_INVALIDA;
    if ((tipo != COMUM && tipo != LABORATORIO) || (temQuadro != 0 && temQuadro != 1))
        return SALA_DADOS_INVALIDOS;
    if (qtdSalas == MAX_SALAS)
        return SALA_LIMITE_ATINGIDO;
    return SALA_OK;
}

int sala_cadastrar(char *codigo, TipoSala tipo, int capacidade, int temQuadro)
{
    int resultado = sala_verificarCadastro(codigo, tipo, capacidade, temQuadro);
    if (resultado != SALA_OK)
        return resultado;

    Sala *sala = &salas[qtdSalas++];
    strcpy(sala->codigo, codigo);
    sala->tipo = tipo;
    sala->capacidade = capacidade;
    sala->temQuadro = temQuadro;
    return SALA_OK;
}

int sala_obterCapacidade(char *codigo, int *capacidade)
{
    const Sala *sala = buscar(codigo);

    if (!sala || !capacidade)
        return SALA_NAO_ENCONTRADA;
    *capacidade = sala->capacidade;
    return SALA_OK;
}

int sala_obterTipo(char *codigo, TipoSala *tipo)
{
    const Sala *sala = buscar(codigo);

    if (!sala || !tipo)
        return SALA_NAO_ENCONTRADA;
    *tipo = sala->tipo;
    return SALA_OK;
}

int sala_listarPorCapacidade(int qtdPessoas, int precisaQuadro, char lista[][TAM], int *n)
{
    if (!n)
        return SALA_NENHUMA;
    *n = 0;
    if (!lista || qtdPessoas <= 0)
        return SALA_NENHUMA;

    for (int i = 0; i < qtdSalas; i++) {
        if (salas[i].capacidade >= qtdPessoas && (!precisaQuadro || salas[i].temQuadro))
            strcpy(lista[(*n)++], salas[i].codigo);
    }
    return *n ? SALA_OK : SALA_NENHUMA;
}

static int temBiometria(const Sala *sala, const char *matricula)
{
    for (int i = 0; i < qtdBiometrias; i++) {
        if (biometrias[i].sala == sala && strcmp(biometrias[i].matricula, matricula) == 0)
            return 1;
    }
    return 0;
}

int sala_registrarBiometria(char *codigo, char *matMonitor)
{
    const Sala *sala = buscar(codigo);

    if (!sala)
        return SALA_NAO_ENCONTRADA;
    if (!matMonitor || !*matMonitor || strlen(matMonitor) >= TAM || sala->tipo != LABORATORIO)
        return SALA_DADOS_INVALIDOS;
    if (temBiometria(sala, matMonitor))
        return SALA_OK;
    if (qtdBiometrias == MAX_BIOMETRIAS)
        return SALA_LIMITE_ATINGIDO;

    Biometria *biometria = &biometrias[qtdBiometrias++];
    biometria->sala = sala;
    strcpy(biometria->matricula, matMonitor);
    return SALA_OK;
}

int sala_temMonitorComBiometria(char *codigo, char *matMonitor)
{
    const Sala *sala = buscar(codigo);
    return sala && matMonitor && temBiometria(sala, matMonitor);
}
