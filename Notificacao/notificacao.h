
#ifndef NOTIFICACAO_H
#define NOTIFICACAO_H

#include "tipos.h"

enum {
    NOT_OK,
    NOT_USUARIO_INEXISTENTE,
    NOT_FALHA_ENTREGA,
    NOT_DADOS_INVALIDOS
};

typedef struct {
    const char *email;
    const char *mensagem;
} NotEntrega;

typedef int (*NotCanalEntrega)(const NotEntrega entregas[], int quantidade);
void not_definirCanalEntrega(NotCanalEntrega canal);

int not_enviar(char *matricula, char *mensagem);
int not_avisarCancelamento(char *matricula, char *codSala, const char *d,
                           const char *ini);
int not_avisarCancelamentos(Reserva *const reservas[], int quantidade);

#endif
