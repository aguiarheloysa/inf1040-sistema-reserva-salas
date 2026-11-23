
#include "notificacao.h"

#include <stdio.h>

#include "usuario.h"

#define TAM_AVISO 200

static int simularEntregas(const NotEntrega entregas[], int quantidade)
{
    for (int i = 0; i < quantidade; i++) {
        if (printf("Notificação simulada para %s: %s\n", entregas[i].email, entregas[i].mensagem) < 0)
            return 0;
    }
    return fflush(stdout) == 0;
}

static NotCanalEntrega canalEntrega = simularEntregas;

void not_definirCanalEntrega(NotCanalEntrega canal)
{
    canalEntrega = canal ? canal : simularEntregas;
}

int not_enviar(char *matricula, char *mensagem)
{
    char email[TAM_EMAIL];

    if (usu_obterEmail(matricula, email) != USU_OK)
        return NOT_USUARIO_INEXISTENTE;
    if (!mensagem || !*mensagem)
        return NOT_DADOS_INVALIDOS;
    const NotEntrega entrega = {email, mensagem};
    return canalEntrega(&entrega, 1) ? NOT_OK : NOT_FALHA_ENTREGA;
}

static int prepararAviso(char *matricula, const char *codSala, const char *data,
                         const char *ini, char *email, char *mensagem)
{
    if (!codSala || !*codSala || !data || !*data || !ini || !*ini)
        return NOT_DADOS_INVALIDOS;
    int tamanho = snprintf(mensagem, TAM_AVISO,
        "Sua reserva da sala %s em %s, às %s, foi cancelada para aplicação de prova.",
        codSala, data, ini);
    if (tamanho < 0 || tamanho >= TAM_AVISO)
        return NOT_DADOS_INVALIDOS;
    if (usu_obterEmail(matricula, email) != USU_OK)
        return NOT_USUARIO_INEXISTENTE;
    return NOT_OK;
}

int not_avisarCancelamento(char *matricula, char *codSala, const char *d,
                           const char *ini)
{
    char email[TAM_EMAIL];
    char mensagem[TAM_AVISO];
    const NotEntrega entrega = {email, mensagem};

    int resultado = prepararAviso(matricula, codSala, d, ini, email, mensagem);
    if (resultado != NOT_OK)
        return resultado;
    return canalEntrega(&entrega, 1) ? NOT_OK : NOT_FALHA_ENTREGA;
}

int not_avisarCancelamentos(Reserva *const reservas[], int quantidade)
{
    NotEntrega entregas[MAX_RESERVAS];
    char emails[MAX_RESERVAS][TAM_EMAIL];
    char mensagens[MAX_RESERVAS][TAM_AVISO];

    if (quantidade <= 0 || quantidade > MAX_RESERVAS || !reservas)
        return NOT_DADOS_INVALIDOS;
    for (int i = 0; i < quantidade; i++) {
        const Reserva *reserva = reservas[i];
        if (!reserva)
            return NOT_DADOS_INVALIDOS;
        int resultado = prepararAviso(reservas[i]->matricula, reserva->codSala,
            reserva->data, reserva->inicio, emails[i], mensagens[i]);
        if (resultado != NOT_OK)
            return resultado;
        entregas[i] = (NotEntrega) {emails[i], mensagens[i]};
    }
    return canalEntrega(entregas, quantidade) ? NOT_OK : NOT_FALHA_ENTREGA;
}
