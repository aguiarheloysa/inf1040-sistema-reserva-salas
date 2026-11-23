
#ifndef SALA_H
#define SALA_H

#include "tipos.h"

#define TAM_COD 7

typedef enum {
    SALA_OK,
    SALA_CODIGO_INVALIDO,
    SALA_JA_EXISTE,
    SALA_CAPACIDADE_INVALIDA,
    SALA_NAO_ENCONTRADA,
    SALA_NENHUMA,
    SALA_DADOS_INVALIDOS,
    SALA_LIMITE_ATINGIDO
} SALA_tpCondicaoRet;

int sala_validarCodigo(char *codigo);
int sala_verificarCadastro(char *codigo, TipoSala tipo, int capacidade, int temQuadro);
int sala_cadastrar(char *codigo, TipoSala tipo, int capacidade, int temQuadro);
int sala_obterCapacidade(char *codigo, int *capacidade);
int sala_obterTipo(char *codigo, TipoSala *tipo);
/* lista deve ter espaço para MAX_SALAS códigos. */
int sala_listarPorCapacidade(int qtdPessoas, int precisaQuadro, char lista[][TAM], int *n);
int sala_registrarBiometria(char *codigo, char *matMonitor);
int sala_temMonitorComBiometria(char *codigo, char *matMonitor);

#endif
