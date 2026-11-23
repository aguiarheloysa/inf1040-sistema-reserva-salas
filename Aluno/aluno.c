
#include "usuario.h"

#include <ctype.h>
#include <string.h>

#define MAX_USUARIOS 500
#define TAM_NOME 100

typedef struct {
    char matricula[TAM];
    char nome[TAM_NOME];
    char email[TAM_EMAIL];
    TipoUsuario tipo;
} Usuario;

static Usuario usuarios[MAX_USUARIOS];
static int qtdUsuarios;

static const Usuario *buscar(const char *matricula)
{
    if (!matricula)
        return NULL;
    for (int i = 0; i < qtdUsuarios; i++) {
        if (strcmp(usuarios[i].matricula, matricula) == 0)
            return &usuarios[i];
    }
    return NULL;
}

static int campoValido(const char *texto, size_t limite)
{
    if (!texto || strlen(texto) >= limite || texto[strspn(texto, " \t\r\n")] == '\0')
        return 0;
    return strchr(texto, '\r') == NULL && strchr(texto, '\n') == NULL;
}

static int emailValido(const char *email)
{
    if (!campoValido(email, TAM_EMAIL))
        return 0;

    const char *arroba = strchr(email, '@');
    if (!arroba || arroba == email || arroba[1] == '\0' || strchr(arroba + 1, '@'))
        return 0;
    for (const unsigned char *c = (const unsigned char *) email; *c; c++) {
        if (isspace(*c) || iscntrl(*c))
            return 0;
    }
    return 1;
}

int usu_cadastrar(char *matricula, char *nome, char *email, TipoUsuario tipo)
{
    if (!campoValido(matricula, TAM))
        return USU_DADOS_INVALIDOS;
    if (buscar(matricula))
        return USU_JA_EXISTE;
    if (!campoValido(nome, TAM_NOME) || !emailValido(email)
            || (tipo != ALUNO && tipo != PROFESSOR))
        return USU_DADOS_INVALIDOS;
    if (qtdUsuarios == MAX_USUARIOS)
        return USU_LIMITE_ATINGIDO;

    Usuario *usuario = &usuarios[qtdUsuarios++];
    strcpy(usuario->matricula, matricula);
    strcpy(usuario->nome, nome);
    strcpy(usuario->email, email);
    usuario->tipo = tipo;
    return USU_OK;
}

int usu_existe(char *matricula)
{
    return buscar(matricula) != NULL;
}

int usu_obterTipo(char *matricula, TipoUsuario *tipo)
{
    const Usuario *usuario = buscar(matricula);

    if (!usuario || !tipo)
        return USU_NAO_ENCONTRADO;
    *tipo = usuario->tipo;
    return USU_OK;
}

int usu_obterEmail(char *matricula, char *email)
{
    const Usuario *usuario = buscar(matricula);

    if (!usuario || !email)
        return USU_NAO_ENCONTRADO;
    strcpy(email, usuario->email);
    return USU_OK;
}
