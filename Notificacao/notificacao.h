
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

/* Retorna 1 quando o canal aceita a entrega a todos os destinatários. */
typedef int (*NotCanalEntrega)(const NotEntrega entregas[], int quantidade);
/* NULL restaura a entrega simulada no terminal. */
void not_definirCanalEntrega(NotCanalEntrega canal);

int not_enviar(char *matricula, char *mensagem);
/* Encaminha cancelamentos já aprovados pelo módulo Reserva. */
int not_avisarCancelamento(char *matricula, char *codSala, const char *d,
                           const char *ini);
int not_avisarCancelamentos(Reserva *const reservas[], int quantidade);

#endif
