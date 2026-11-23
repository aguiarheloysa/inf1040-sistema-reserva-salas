
#define _POSIX_C_SOURCE 200809L

#include "smtp.h"

#include <signal.h>
#include <stdio.h>

void smtp_preparar(void)
{
#ifndef _WIN32
    /* Se o processo de entrega fechar o pipe, a falha volta ao módulo Reserva. */
    signal(SIGPIPE, SIG_IGN);
#endif
}

int smtp_enviar(const NotEntrega entregas[], int quantidade)
{
#ifdef _WIN32
    FILE *canal = _popen("python -I Interface/enviar_email.py", "w");
#else
    FILE *canal = popen("python3 -I Interface/enviar_email.py", "w");
#endif
    if (!canal)
        return 0;

    const char *mensagem = quantidade == 1 ? entregas[0].mensagem
        : "Uma reserva de sala vinculada ao seu cadastro foi cancelada para aplicação de prova. "
          "Consulte o seu histórico de reservas para ver os detalhes.";
    int escrito = fprintf(canal, "%d\n", quantidade) >= 0;
    for (int i = 0; i < quantidade && escrito; i++)
        escrito = fprintf(canal, "%s\n", entregas[i].email) >= 0;
    escrito = escrito && fputs(mensagem, canal) >= 0 && fflush(canal) == 0;
#ifdef _WIN32
    int estado = _pclose(canal);
#else
    int estado = pclose(canal);
#endif
    return escrito && estado == 0;
}
