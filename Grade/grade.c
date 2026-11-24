
#include "grade.h"

#include <string.h>

#include "matricula.h"
#include "tempo.h"

#define MAX_AULAS 500
#define MAX_AUTORIZACOES 1000
#define TAM_DISCIPLINA 100

typedef struct {
    char codSala[TAM];
    char disciplina[TAM_DISCIPLINA];
    char matProfessor[TAM];
    int diaSemana;
    int inicio;
    int fim;
} Aula;

typedef struct {
    const Aula *aula;
    int dia;
    int inicio;
    char matSolicitante[TAM];
} Autorizacao;

static Aula aulas[MAX_AULAS];
static Autorizacao autorizacoes[MAX_AUTORIZACOES];
static int qtdAulas;
static int qtdAutorizacoes;

static int campoValido(const char *texto, size_t limite)
{
    return texto && *texto && strlen(texto) < limite;
}

static int aulaSobrepoe(const Aula *aula, const char *codSala, int diaSemana,
                        int inicio, int fim)
{
    return aula->diaSemana == diaSemana && strcmp(aula->codSala, codSala) == 0
        && tempo_sobrepoe(aula->inicio, aula->fim, inicio, fim);
}

static const Aula *buscarAula(const char *codSala, int diaSemana, int inicio, int fim)
{
    const Aula *primeira = NULL;

    for (int i = 0; i < qtdAulas; i++) {
        const Aula *aula = &aulas[i];
        if (aulaSobrepoe(aula, codSala, diaSemana, inicio, fim)
                && (!primeira || aula->inicio < primeira->inicio))
            primeira = aula;
    }
    return primeira;
}

static int temAutorizacao(const Aula *aula, int dia, int inicio, const char *matSolicitante)
{
    for (int i = 0; i < qtdAutorizacoes; i++) {
        const Autorizacao *autorizacao = &autorizacoes[i];
        if (autorizacao->aula == aula && autorizacao->dia == dia
                && autorizacao->inicio == inicio
                && strcmp(autorizacao->matSolicitante, matSolicitante) == 0)
            return 1;
    }
    return 0;
}

int grade_cadastrarAula(char *codSala, char *disciplina, char *matProfessor,
                       int diaSemana, const char *ini, const char *fim)
{
    int inicio, termino;

    if (diaSemana < 1 || diaSemana > 7 || !tempo_intervalo(ini, fim, &inicio, &termino))
        return GRADE_HORARIO_INVALIDO;
    if (!campoValido(codSala, TAM) || !campoValido(disciplina, TAM_DISCIPLINA)
            || !matricula_valida(matProfessor))
        return GRADE_DADOS_INVALIDOS;
    if (buscarAula(codSala, diaSemana, inicio, termino))
        return GRADE_CONFLITO;
    if (qtdAulas == MAX_AULAS)
        return GRADE_LIMITE_ATINGIDO;

    Aula *aula = &aulas[qtdAulas++];
    strcpy(aula->codSala, codSala);
    strcpy(aula->disciplina, disciplina);
    strcpy(aula->matProfessor, matProfessor);
    aula->diaSemana = diaSemana;
    aula->inicio = inicio;
    aula->fim = termino;
    return GRADE_OK;
}

int grade_temAula(char *codSala, const char *d, const char *ini, const char *fim, char *matMinistrante)
{
    int dia = tempo_dia(d);
    int inicio, termino;

    if (matMinistrante)
        *matMinistrante = '\0';
    if (!codSala || dia < 0 || !tempo_intervalo(ini, fim, &inicio, &termino))
        return 0;

    const Aula *aula = buscarAula(codSala, tempo_diaSemana(dia), inicio, termino);
    if (aula && matMinistrante)
        strcpy(matMinistrante, aula->matProfessor);
    return aula != NULL;
}

int grade_registrarAutorizacao(char *codSala, const char *d, const char *ini,
                               char *matMinistrante, char *matSolicitante)
{
    int dia = tempo_dia(d);
    int inicio = tempo_minuto(ini);

    if (!codSala || dia < 0 || inicio < 0)
        return GRADE_SEM_AULA;
    const Aula *aula = buscarAula(codSala, tempo_diaSemana(dia), inicio, inicio + 1);
    if (!aula)
        return GRADE_SEM_AULA;
    if (!matMinistrante || strcmp(aula->matProfessor, matMinistrante) != 0)
        return GRADE_NAO_MINISTRANTE;
    if (!matricula_valida(matSolicitante))
        return GRADE_DADOS_INVALIDOS;
    if (temAutorizacao(aula, dia, inicio, matSolicitante))
        return GRADE_OK;
    if (qtdAutorizacoes == MAX_AUTORIZACOES)
        return GRADE_LIMITE_ATINGIDO;

    Autorizacao *autorizacao = &autorizacoes[qtdAutorizacoes++];
    autorizacao->aula = aula;
    autorizacao->dia = dia;
    autorizacao->inicio = inicio;
    strcpy(autorizacao->matSolicitante, matSolicitante);
    return GRADE_OK;
}

int grade_temAutorizacao(char *codSala, const char *d, const char *ini, char *matSolicitante)
{
    int dia = tempo_dia(d);
    int inicio = tempo_minuto(ini);

    if (!codSala || !matSolicitante || dia < 0 || inicio < 0)
        return 0;
    const Aula *aula = buscarAula(codSala, tempo_diaSemana(dia), inicio, inicio + 1);
    return aula && temAutorizacao(aula, dia, inicio, matSolicitante);
}

int grade_permiteProva(char *codSala, const char *d, const char *ini, const char *fim, char *matSolicitante)
{
    int dia = tempo_dia(d);
    int inicio, termino;

    if (!codSala || !matSolicitante || dia < 0 || !tempo_intervalo(ini, fim, &inicio, &termino))
        return 0;

    int diaSemana = tempo_diaSemana(dia);
    for (int i = 0; i < qtdAulas; i++) {
        const Aula *aula = &aulas[i];
        if (!aulaSobrepoe(aula, codSala, diaSemana, inicio, termino)
                || strcmp(aula->matProfessor, matSolicitante) == 0)
            continue;
        int inicioIntersecao = inicio > aula->inicio ? inicio : aula->inicio;
        if (!temAutorizacao(aula, dia, inicioIntersecao, matSolicitante))
            return 0;
    }
    return 1;
}
