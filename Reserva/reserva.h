
#ifndef RESERVA_H
#define RESERVA_H

/*
 * Módulo Reserva — coordena as reservas de salas.
 *
 * Interface explícita: as funções abaixo e seus códigos de retorno.
 * Interface implícita: o módulo depende de Usuário, Sala, Grade, Notificação,
 * Regras e Tempo, e guarda as reservas em memória (estado interno). Os IDs são
 * sequenciais a partir de 1 e nunca são reutilizados; reservas canceladas
 * continuam no histórico.
 *
 * Garantia de consistência (vale para todas as funções): quando o retorno é
 * diferente de RES_OK, nenhuma reserva foi criada nem teve a situação alterada.
 */

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

/* Lista as salas que o usuário poderia reservar no período: capacidade e
   quadro atendidos, sem aula, sem reserva ativa e, para aluno, sem laboratório.
   Entrada: lista com espaço para MAX_SALAS códigos; n não nulo.
   Saída: n = quantidade de salas em lista; RES_OK se n > 0, senão
   RES_NENHUMA_SALA. Não altera o estado. */
int res_buscarSalasLivres(const char *d, const char *ini, const char *fim, int qtdPessoas,
                         int precisaQuadro, char *matricula, char lista[][TAM], int *n);

/* Retorna 1 se existe reserva ATIVA na sala com horário sobreposto
   ([ini, fim) — reservas que só se encostam não conflitam), 0 se não.
   Se houver mais de um conflito, idConflitante recebe o primeiro encontrado
   (0 quando não há). Não altera o estado. */
int res_verificarConflito(char *codSala, const char *d, const char *ini, const char *fim, int *idConflitante);

/* Solicita uma reserva. Ordem das verificações (o primeiro erro é retornado):
     1. usuário cadastrado ............................ RES_USUARIO_INEXISTENTE
     2. sala cadastrada ............................... RES_SALA_INEXISTENTE
     3. data/hora válidas, ini < fim, qtdPessoas > 0 .. RES_DADOS_INVALIDOS
     4. capacidade (RN1) .............................. RES_CAPACIDADE_EXCEDIDA
     5. horário de aula (RN2) ......................... RES_HORARIO_AULA
     6. laboratório para aluno (RN3) .................. RES_LAB_SEM_MONITOR
     7. conflito com reserva ativa (RN4) .............. RES_CONFLITO,
        RES_ANTECEDENCIA_INSUFICIENTE, RES_FALHA_NOTIFICACAO
   Entrada: idReserva não nulo; agora é o instante do pedido.
   Saída com RES_OK: *idReserva > 0, nova reserva ATIVA; reservas de alunos
   substituídas por prova passam a CANCELADA e os titulares foram avisados.
   Saída com erro: *idReserva = 0 e nenhuma reserva foi alterada. */
int res_solicitar(char *matricula, char *codSala, const char *d, const char *ini, const char *fim,
                  int qtdPessoas, Finalidade f, char *matMonitor, DataHora agora,
                  int *idReserva);

/* Cancela uma reserva ATIVA. Só o titular pode cancelar (RN5).
   Saída: RES_OK e reserva CANCELADA; RES_NAO_ENCONTRADA se o id não existe
   ou a reserva já está cancelada; RES_NAO_AUTORIZADO se não é o titular. */
int res_cancelar(int idReserva, char *matricula);

/* Lista todas as reservas do usuário, incluindo as canceladas, em ordem de
   criação. lista comporta MAX_RESERVAS registros. Não altera o estado. */
int res_listarPorUsuario(char *matricula, Reserva lista[], int *n);

#endif
