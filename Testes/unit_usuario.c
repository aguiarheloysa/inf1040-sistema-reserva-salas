
#include "usuario.h"
#include "unitario.h"

static void prepararAluno(void)
{
    VERIFICAR("preparo 1001", usu_cadastrar("1001", "Aluno", "aluno@puc-rio.br", ALUNO) == USU_OK);
}

static void U1(void)
{
    VERIFICAR("U1", usu_cadastrar("1002", "Ana", "ana@puc-rio.br", ALUNO) == USU_OK);
}

static void U2(void)
{
    prepararAluno();
    VERIFICAR("U2", usu_cadastrar("1001", "Outro", "outro@puc-rio.br", ALUNO) == USU_JA_EXISTE);
}

static void U3(void)
{
    VERIFICAR("U3", usu_cadastrar("1003", "", "a3@puc-rio.br", ALUNO) == USU_DADOS_INVALIDOS);
    VERIFICAR("cadastro inválido não gravado", !usu_existe("1003"));
}

static void U4(void)
{
    prepararAluno();
    VERIFICAR("U4", usu_existe("1001") == 1);
}

static void U4_ausente(void)
{
    VERIFICAR("U4", usu_existe("9999") == 0);
}

static void U5(void)
{
    TipoUsuario tipo;

    VERIFICAR("preparo 2001", usu_cadastrar("2001", "Professor", "prof@puc-rio.br", PROFESSOR) == USU_OK);
    VERIFICAR("U5", usu_obterTipo("2001", &tipo) == USU_OK && tipo == PROFESSOR);
}

static void U6(void)
{
    TipoUsuario tipo;

    VERIFICAR("U6", usu_obterTipo("9999", &tipo) == USU_NAO_ENCONTRADO);
}

static void U7(void)
{
    char email[TAM_EMAIL];

    prepararAluno();
    VERIFICAR("U7", usu_obterEmail("1001", email) == USU_OK && strcmp(email, "aluno@puc-rio.br") == 0);
}

static void copia_de_email(void)
{
    char original[] = "ana@puc-rio.br";
    char email[TAM_EMAIL];

    VERIFICAR("preparo 1002", usu_cadastrar("1002", "Ana", original, ALUNO) == USU_OK);
    original[0] = 'x';
    VERIFICAR("cópia do cadastro", usu_obterEmail("1002", email) == USU_OK
              && strcmp(email, "ana@puc-rio.br") == 0);
}

static void email_invalido(void)
{
    VERIFICAR("e-mail inválido", usu_cadastrar("1003", "Nome", "sem arroba", ALUNO) == USU_DADOS_INVALIDOS);
}

static void tipo_invalido(void)
{
    VERIFICAR("tipo inválido", usu_cadastrar("1003", "Nome", "a3@puc-rio.br", 7) == USU_DADOS_INVALIDOS);
}

static void email_ausente(void)
{
    char email[TAM_EMAIL];

    VERIFICAR("e-mail ausente", usu_obterEmail("9999", email) == USU_NAO_ENCONTRADO);
}

static void matricula_com_letras(void)
{
    VERIFICAR("matrícula com letras", usu_cadastrar("A1001", "Ana", "ana@puc-rio.br", ALUNO)
              == USU_DADOS_INVALIDOS);
    VERIFICAR("matrícula inválida não gravada", !usu_existe("A1001"));
}

static void matricula_com_separadores(void)
{
    VERIFICAR("matrícula com hífen", usu_cadastrar("10-01", "Ana", "ana@puc-rio.br", ALUNO)
              == USU_DADOS_INVALIDOS);
    VERIFICAR("matrícula com espaço", usu_cadastrar("1001 ", "Ana", "ana@puc-rio.br", ALUNO)
              == USU_DADOS_INVALIDOS);
}

static void matricula_vazia(void)
{
    VERIFICAR("matrícula vazia", usu_cadastrar("", "Ana", "ana@puc-rio.br", ALUNO) == USU_DADOS_INVALIDOS);
    VERIFICAR("matrícula nula", usu_cadastrar(NULL, "Ana", "ana@puc-rio.br", ALUNO) == USU_DADOS_INVALIDOS);
}

static void matricula_com_zeros(void)
{
    VERIFICAR("preserva zeros à esquerda", usu_cadastrar("001001", "Ana", "ana@puc-rio.br", ALUNO) == USU_OK
              && usu_existe("001001") && !usu_existe("1001"));
}

static void limite_da_matricula(void)
{
    VERIFICAR("matrícula de 19 dígitos", usu_cadastrar("1234567890123456789", "Ana", "ana@puc-rio.br",
              ALUNO) == USU_OK && usu_existe("1234567890123456789"));
    VERIFICAR("matrícula de 20 dígitos", usu_cadastrar("12345678901234567890", "Ana", "ana@puc-rio.br",
              ALUNO) == USU_DADOS_INVALIDOS);
}

int main(int argc, char *argv[])
{
    const CasoUnitario casos[] = {
        CASO(U1), CASO(U2), CASO(U3), CASO(U4), CASO(U4_ausente), CASO(U5), CASO(U6), CASO(U7),
        CASO(copia_de_email), CASO(email_invalido), CASO(tipo_invalido), CASO(email_ausente),
        CASO(matricula_com_letras), CASO(matricula_com_separadores), CASO(matricula_vazia),
        CASO(matricula_com_zeros), CASO(limite_da_matricula)
    };
    return executarCasos(argc, argv, casos, sizeof casos / sizeof casos[0]);
}
