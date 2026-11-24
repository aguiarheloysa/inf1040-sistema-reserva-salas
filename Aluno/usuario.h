
#ifndef USUARIO_H
#define USUARIO_H

#include "tipos.h"

enum {
    USU_OK,
    USU_JA_EXISTE,
    USU_DADOS_INVALIDOS,
    USU_NAO_ENCONTRADO,
    USU_LIMITE_ATINGIDO
};

int usu_cadastrar(char *matricula, char *nome, char *email, TipoUsuario tipo);
int usu_existe(char *matricula);
int usu_obterTipo(char *matricula, TipoUsuario *tipo);
int usu_obterEmail(char *matricula, char *email);

#endif
