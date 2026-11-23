
#include "usuario.h"
#include "unitario.h"

static void prepararAluno(void)
{
    VERIFICAR("preparo A1", usu_cadastrar("A1", "Aluno", "aluno@puc-rio.br", ALUNO) == USU_OK);
}

static void U1(void)
{
    VERIFICAR("U1", usu_cadastrar("A2", "Ana", "ana@puc-rio.br", ALUNO) == USU_OK);
}

static void U2(void)
{
    prepararAluno();
    VERIFICAR("U2", usu_cadastrar("A1", "Outro", "outro@puc-rio.br", ALUNO) == USU_JA_EXISTE);
}

static void U3(void)
{
    VERIFICAR("U3", usu_cadastrar("A3", "", "a3@puc-rio.br", ALUNO) == USU_DADOS_INVALIDOS);
    VERIFICAR("cadastro inválido não gravado", !usu_existe("A3"));
}

static void U4(void)
{
    prepararAluno();
    VERIFICAR("U4", usu_existe("A1") == 1);
}

static void U4_ausente(void)
{
    VERIFICAR("U4", usu_existe("X99") == 0);
}

static void U5(void)
{
    TipoUsuario tipo;

    VERIFICAR("preparo P1", usu_cadastrar("P1", "Professor", "prof@puc-rio.br", PROFESSOR) == USU_OK);
    VERIFICAR("U5", usu_obterTipo("P1", &tipo) == USU_OK && tipo == PROFESSOR);
}

static void U6(void)
{
    TipoUsuario tipo;

    VERIFICAR("U6", usu_obterTipo("X99", &tipo) == USU_NAO_ENCONTRADO);
}

static void U7(void)
{
    char email[TAM_EMAIL];

    prepararAluno();
    VERIFICAR("U7", usu_obterEmail("A1", email) == USU_OK && strcmp(email, "aluno@puc-rio.br") == 0);
}

static void copia_de_email(void)
{
    char original[] = "ana@puc-rio.br";
    char email[TAM_EMAIL];

    VERIFICAR("preparo A2", usu_cadastrar("A2", "Ana", original, ALUNO) == USU_OK);
    original[0] = 'x';
    VERIFICAR("cópia do cadastro", usu_obterEmail("A2", email) == USU_OK
              && strcmp(email, "ana@puc-rio.br") == 0);
}

static void email_invalido(void)
{
    VERIFICAR("e-mail inválido", usu_cadastrar("A3", "Nome", "sem arroba", ALUNO) == USU_DADOS_INVALIDOS);
}

static void tipo_invalido(void)
{
    VERIFICAR("tipo inválido", usu_cadastrar("A3", "Nome", "a3@puc-rio.br", 7) == USU_DADOS_INVALIDOS);
}

static void email_ausente(void)
{
    char email[TAM_EMAIL];

    VERIFICAR("e-mail ausente", usu_obterEmail("X99", email) == USU_NAO_ENCONTRADO);
}

int main(int argc, char *argv[])
{
    const CasoUnitario casos[] = {
        CASO(U1), CASO(U2), CASO(U3), CASO(U4), CASO(U4_ausente), CASO(U5), CASO(U6), CASO(U7),
        CASO(copia_de_email), CASO(email_invalido), CASO(tipo_invalido), CASO(email_ausente)
    };
    return executarCasos(argc, argv, casos, sizeof casos / sizeof casos[0]);
}
