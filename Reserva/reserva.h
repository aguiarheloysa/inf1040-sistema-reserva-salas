
#ifndef RESERVA_H
#define RESERVA_H

#include "tipos.h"

#define RES_OK 0
#define RES_USUARIO_INEXISTENTE 1
#define RES_CAPACIDADE_EXCEDIDA 2
#define RES_HORARIO_AULA 3
#define RES_LAB_SEM_MONITOR 4
#define RES_CONFLITO 5
#define RES_ANTECEDENCIA_INSUFICIENTE 6
#define RES_NAO_ENCONTRADA 7
#define RES_NAO_AUTORIZADO 8
#define RES_NENHUMA_SALA 9
#define RES_NENHUMA_RESERVA 10
#define RES_SALA_INEXISTENTE 11
#define RES_DADOS_INVALIDOS 12
#define RES_LIMITE_ATINGIDO 13
#define RES_FALHA_NOTIFICACAO 14

/* lista deve ter espaço para MAX_SALAS códigos. */
int res_buscarSalasLivres(const char *d, const char *ini, const char *fim, int qtdPessoas,
                         int precisaQuadro, char *matricula, char lista[][TAM], int *n);

/* Se houver mais de um conflito retorna o primeiro ID ativo encontrado. */
int res_verificarConflito(char *codSala, const char *d, const char *ini, const char *fim, int *idConflitante);

int res_solicitar(char *matricula, char *codSala, const char *d, const char *ini, const char *fim,
                  int qtdPessoas, Finalidade f, char *matMonitor, DataHora agora,
                  int *idReserva);

int res_cancelar(int idReserva, char *matricula);

/* Inclui o histórico de reservas canceladas. lista comporta MAX_RESERVAS registros. */
int res_listarPorUsuario(char *matricula, Reserva lista[], int *n);

#endif
