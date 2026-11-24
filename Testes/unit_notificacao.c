
#include "notificacao.h"
#include "usuario.h"
#include "unitario.h"

static char destinatario[TAM_EMAIL];
static char mensagemRecebida[200];
static int tentativas;
static int aceitar = 1;

int usu_obterEmail(char *matricula, char *email)
{
    if (!matricula || strcmp(matricula, "1001") != 0)
        return USU_NAO_ENCONTRADO;
    strcpy(email, "a1@puc-rio.br");
    return USU_OK;
}

static int capturarEntrega(const NotEntrega lote[], int quantidade)
{
    VERIFICAR("entrega unitária", quantidade == 1);
    snprintf(destinatario, sizeof destinatario, "%s", lote[0].email);
    snprintf(mensagemRecebida, sizeof mensagemRecebida, "%s", lote[0].mensagem);
    tentativas++;
    return aceitar;
}

static void N1(void)
{
    not_definirCanalEntrega(capturarEntrega);
    VERIFICAR("N1", not_enviar("1001", "teste") == NOT_OK && tentativas == 1
              && strcmp(destinatario, "a1@puc-rio.br") == 0 && strcmp(mensagemRecebida, "teste") == 0);
}

static void N2(void)
{
    not_definirCanalEntrega(capturarEntrega);
    VERIFICAR("N2", not_enviar("9999", "teste") == NOT_USUARIO_INEXISTENTE && tentativas == 0);
}

static void N6(void)
{
    not_definirCanalEntrega(capturarEntrega);
    VERIFICAR("N6", not_avisarCancelamento("1001", "L201", "19/10/2026", "15:00") == NOT_OK);
    VERIFICAR("aviso", tentativas == 1 && strcmp(destinatario, "a1@puc-rio.br") == 0
              && strstr(mensagemRecebida, "L201") && strstr(mensagemRecebida, "19/10/2026")
              && strstr(mensagemRecebida, "15:00"));
}

static void N7(void)
{
    Reserva anterior = {.matricula = "1001", .codSala = "L201", .data = "19/10/2026", .inicio = "15:00"};
    Reserva *lote[] = {&anterior};

    not_definirCanalEntrega(capturarEntrega);
    aceitar = 0;
    VERIFICAR("N7", not_avisarCancelamentos(lote, 1) == NOT_FALHA_ENTREGA && tentativas == 1);
}

static void aviso_sem_dados(void)
{
    not_definirCanalEntrega(capturarEntrega);
    VERIFICAR("aviso sem data", not_avisarCancelamento("1001", "L201", NULL, "15:00") == NOT_DADOS_INVALIDOS
              && tentativas == 0);
}

int main(int argc, char *argv[])
{
    const CasoUnitario casos[] = {CASO(N1), CASO(N2), CASO(N6), CASO(N7), CASO(aviso_sem_dados)};
    return executarCasos(argc, argv, casos, sizeof casos / sizeof casos[0]);
}
