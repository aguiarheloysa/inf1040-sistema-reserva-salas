
#ifndef SMTP_H
#define SMTP_H

#include "notificacao.h"

void smtp_preparar(void);
int smtp_enviar(const NotEntrega entregas[], int quantidade);

#endif
