
#ifndef REGRAS_H
#define REGRAS_H

/*
 * Módulo Regras de Reserva
 *
 * Concentra as regras de negócio usadas pelo módulo Reserva. Todas as funções
 * são puras: não guardam estado e não chamam outros módulos. Quem decide
 * consultar Usuário, Sala ou Grade é o Reserva (coordenador); aqui só se
 * responde "a regra permite ou não?". Por isso o módulo pode ser testado sem
 * stubs (Testes/unit_regras.c).
 */

#include <stdint.h>

#include "tipos.h"

/* RN4: antecedência mínima, em minutos, para cancelar a reserva de um aluno. */
#define REGRA_ANTECEDENCIA_MINUTOS 120

/* RN1: a quantidade de pessoas precisa caber na sala.
   Retorna 1 se 0 < qtdPessoas <= capacidade. */
int regra_comportaPessoas(int qtdPessoas, int capacidade);

/* RN2: em horário de aula só se aceita PROVA pedida por PROFESSOR. Mesmo
   assim, a Grade ainda precisa confirmar que ele é o ministrante ou que foi
   autorizado (grade_permiteProva). Retorna 1 se o pedido pode seguir para
   essa verificação. */
int regra_podeUsarHorarioDeAula(TipoUsuario solicitante, Finalidade f);

/* RN3: aluno só reserva laboratório com monitor com biometria na sala.
   Retorna 1 se a combinação exige monitor. */
int regra_exigeMonitor(TipoUsuario solicitante, TipoSala sala);

/* RN4: prioridade. Só PROFESSOR pedindo PROVA substitui uma reserva, e só se
   a reserva existente for de ALUNO. Retorna 1 se pode substituir. */
int regra_podeSubstituir(TipoUsuario solicitante, Finalidade f, TipoUsuario titular);

/* RN4: a substituição só vale se faltarem pelo menos 2h para o início da
   reserva que seria cancelada. Instantes em minutos (tempo_instante).
   Retorna 1 se inicioReserva - agora >= REGRA_ANTECEDENCIA_MINUTOS. */
int regra_antecedenciaSuficiente(int64_t inicioReserva, int64_t agora);

/* RN5: somente o titular cancela a própria reserva. Retorna 1 se as
   matrículas são iguais e não vazias. */
int regra_podeCancelar(const char *matTitular, const char *matSolicitante);

#endif
