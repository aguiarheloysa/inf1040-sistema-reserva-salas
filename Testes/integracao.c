/*
 * Testes de integração: aqui os módulos REAIS trabalham juntos (sem stubs),
 * exceto o canal de e-mail, trocado por receber() para não enviar mensagens.
 *
 * Cada cenário roda em um processo novo (Testes/test_unitarios.py) e começa
 * chamando prepararCadastros(), que monta a mesma base inicial descrita no
 * relatório. Dentro de um cenário os passos são sequenciais de propósito:
 * o objetivo é justamente verificar que o estado deixado por uma operação
 * é o que a próxima espera. Entre cenários não há estado compartilhado.
 */


#include <string.h>

#include "grade.h"
#include "notificacao.h"
#include "reserva.h"
#include "sala.h"
#include "usuario.h"
#include "unitario.h"

static int notificacoes;
static int entregaDisponivel = 1;
static char enderecos[2][TAM_EMAIL];
static char mensagens[2][200];

static int receber(const NotEntrega lote[], int quantidade)
{
    if (!entregaDisponivel)
        return 0;
    VERIFICAR("no máximo dois avisos por lote", quantidade >= 1 && quantidade <= 2 && notificacoes == 0);
    for (int i = 0; i < quantidade; i++) {
        snprintf(enderecos[i], sizeof enderecos[0], "%s", lote[i].email);
        snprintf(mensagens[i], sizeof mensagens[0], "%s", lote[i].mensagem);
    }
    notificacoes = quantidade;
    return 1;
}

static void prepararCadastros(void)
{
    VERIFICAR("A1", usu_cadastrar("A1", "Alice", "aluno1@example.test", ALUNO) == USU_OK);
    VERIFICAR("A2", usu_cadastrar("A2", "Ana", "aluno2@example.test", ALUNO) == USU_OK);
    VERIFICAR("M1", usu_cadastrar("M1", "Monitor", "monitor@example.test", ALUNO) == USU_OK);
    VERIFICAR("P1", usu_cadastrar("P1", "Professor 1", "professor1@example.test", PROFESSOR) == USU_OK);
    VERIFICAR("P2", usu_cadastrar("P2", "Professor 2", "professor2@example.test", PROFESSOR) == USU_OK);
    VERIFICAR("L201", sala_cadastrar("L201", COMUM, 40, 1) == SALA_OK);
    VERIFICAR("L105", sala_cadastrar("L105", COMUM, 10, 0) == SALA_OK);
    VERIFICAR("L301", sala_cadastrar("L301", LABORATORIO, 30, 0) == SALA_OK);
    VERIFICAR("L302", sala_cadastrar("L302", LABORATORIO, 30, 0) == SALA_OK);
    VERIFICAR("aula de P1", grade_cadastrarAula("L201", "Cálculo", "P1", 1, "09:00", "11:00") == GRADE_OK);
    VERIFICAR("aula de P2", grade_cadastrarAula("L201", "Física", "P2", 1, "11:00", "12:00") == GRADE_OK);
    not_definirCanalEntrega(receber);
}

static void conciliarGrade(void)
{
    prepararCadastros();
    Data d = "19/10/2026";
    DataHora agora = {"19/10/2026", "07:00"};
    char salas[MAX_SALAS][TAM];
    int n, id;

    VERIFICAR("filtro por aula, quadro e capacidade",
              res_buscarSalasLivres(d, "09:00", "10:00", 20, 1, "P2", salas, &n) == RES_NENHUMA_SALA && n == 0);
    VERIFICAR("sala livre após aulas",
              res_buscarSalasLivres(d, "12:00", "13:00", 20, 1, "A1", salas, &n) == RES_OK
              && n == 1 && strcmp(salas[0], "L201") == 0);
    VERIFICAR("prova cruza aulas com ministrantes diferentes", res_solicitar("P2", "L201",
              d, "08:00", "12:00", 20, PROVA, "", agora, &id) == RES_HORARIO_AULA);
    VERIFICAR("autorização para aula de P1", grade_registrarAutorizacao("L201", d, "09:00", "P1", "P2") == GRADE_OK);
    VERIFICAR("P2 pode aplicar prova na própria aula e na autorizada", res_solicitar("P2", "L201",
              d, "08:00", "12:00", 20, PROVA, "", agora, &id) == RES_OK);
    VERIFICAR("reserva ativa remove sala dos resultados",
              res_buscarSalasLivres(d, "08:00", "09:00", 20, 1, "P2", salas, &n) == RES_NENHUMA_SALA);
    VERIFICAR("titular cancela prova", res_cancelar(id, "P2") == RES_OK);
    VERIFICAR("cancelamento libera sala", res_buscarSalasLivres(d, "08:00", "09:00", 20, 1, "A1", salas, &n)
              == RES_OK && n == 1 && strcmp(salas[0], "L201") == 0);
}

static void reservarLaboratorio(void)
{
    prepararCadastros();
    Data d = "19/10/2026";
    DataHora agora = {"19/10/2026", "07:00"};
    int id;

    VERIFICAR("monitor cadastrado sem biometria", res_solicitar("A1", "L301", d, "12:00", "13:00",
              30, ESTUDO, "M1", agora, &id) == RES_LAB_SEM_MONITOR);
    VERIFICAR("registrar biometria", sala_registrarBiometria("L301", "M1") == SALA_OK);
    VERIFICAR("biometria de outro laboratório", res_solicitar("A1", "L302", d, "12:00", "13:00",
              30, ESTUDO, "M1", agora, &id) == RES_LAB_SEM_MONITOR);
    VERIFICAR("laboratório na capacidade máxima", res_solicitar("A1", "L301", d, "12:00", "13:00",
              30, ESTUDO, "M1", agora, &id) == RES_OK);
    VERIFICAR("professor sem monitor", res_solicitar("P1", "L302", d, "12:00", "13:00",
              30, PROVA, NULL, agora, &id) == RES_OK);
}

static void priorizarProva(void)
{
    prepararCadastros();
    Data d = "20/10/2026";
    DataHora preparo = {"20/10/2026", "08:00"};
    DataHora limite = {"20/10/2026", "12:00"};
    DataHora tarde = {"20/10/2026", "12:01"};
    Reserva lista[MAX_RESERVAS];
    int primeiro, segundo, prova, id, n;

    VERIFICAR("primeiro estudo", res_solicitar("A1", "L201", d, "14:00", "15:00",
              5, ESTUDO, "", preparo, &primeiro) == RES_OK);
    VERIFICAR("segundo estudo, adjacente", res_solicitar("A2", "L201", d, "15:00", "16:00",
              5, ESTUDO, "", preparo, &segundo) == RES_OK);
    VERIFICAR("um dos estudos já está a menos de duas horas", res_solicitar("P1", "L201", d,
              "14:30", "15:30", 20, PROVA, "", tarde, &prova) == RES_ANTECEDENCIA_INSUFICIENTE
              && prova == 0 && notificacoes == 0);
    VERIFICAR("primeiro continua ativo", res_verificarConflito("L201", d, "14:00", "15:00", &id)
              && id == primeiro);
    VERIFICAR("segundo continua ativo", res_verificarConflito("L201", d, "15:00", "16:00", &id)
              && id == segundo);

    entregaDisponivel = 0;
    VERIFICAR("falha no canal de notificação", res_solicitar("P1", "L201", d, "14:30", "15:30",
              20, PROVA, "", limite, &prova) == RES_FALHA_NOTIFICACAO && prova == 0 && notificacoes == 0);
    VERIFICAR("atividades preservadas após falha", res_verificarConflito("L201", d, "14:00", "15:00", &id)
              && id == primeiro && res_verificarConflito("L201", d, "15:00", "16:00", &id) && id == segundo);

    entregaDisponivel = 1;
    VERIFICAR("prova no limite de duas horas", res_solicitar("P1", "L201", d, "14:30", "15:30",
              20, PROVA, "", limite, &prova) == RES_OK && notificacoes == 2);
    VERIFICAR("e-mails e horários dos titulares cancelados", strcmp(enderecos[0], "aluno1@example.test") == 0
              && strcmp(enderecos[1], "aluno2@example.test") == 0 && strstr(mensagens[0], "14:00")
              && strstr(mensagens[1], "15:00") && strstr(mensagens[0], "20/10/2026"));
    VERIFICAR("primeiro histórico de cancelamento", res_listarPorUsuario("A1", lista, &n) == RES_OK
              && n == 1 && lista[0].id == primeiro && lista[0].situacao == CANCELADA);
    VERIFICAR("segundo histórico de cancelamento", res_listarPorUsuario("A2", lista, &n) == RES_OK
              && n == 1 && lista[0].id == segundo && lista[0].situacao == CANCELADA);
    VERIFICAR("novo ID é o único conflito", res_verificarConflito("L201", d, "14:30", "15:30", &id)
              && id == prova && !res_verificarConflito("L201", d, "14:00", "14:30", NULL)
              && !res_verificarConflito("L201", d, "15:30", "16:00", NULL));
}

static void autorizarProvaDuranteAulas(void)
{
    prepararCadastros();
    Data d = "19/10/2026";
    DataHora agora = {"19/10/2026", "08:00"};
    int id;

    VERIFICAR("preparo P3", usu_cadastrar("P3", "Professor 3", "professor3@example.test", PROFESSOR) == USU_OK);
    VERIFICAR("preparo L202", sala_cadastrar("L202", COMUM, 20, 1) == SALA_OK);
    VERIFICAR("primeira aula", grade_cadastrarAula("L202", "Cálculo", "P1", 1, "09:00", "11:00") == GRADE_OK);
    VERIFICAR("segunda aula", grade_cadastrarAula("L202", "Física", "P3", 1, "11:00", "12:00") == GRADE_OK);

    VERIFICAR("ministrante na própria aula", res_solicitar("P1", "L202", d, "09:00", "10:00",
              5, PROVA, "", agora, &id) == RES_OK);
    VERIFICAR("primeira autorização", grade_registrarAutorizacao("L202", d, "10:00", "P1", "P2") == GRADE_OK);
    VERIFICAR("uma autorização é insuficiente", res_solicitar("P2", "L202", d, "10:00", "12:00",
              5, PROVA, "", agora, &id) == RES_HORARIO_AULA);
    VERIFICAR("segunda autorização", grade_registrarAutorizacao("L202", d, "11:00", "P3", "P2") == GRADE_OK);
    VERIFICAR("ambas as aulas autorizam", res_solicitar("P2", "L202", d, "10:00", "12:00",
              5, PROVA, "", agora, &id) == RES_OK);
}

static void regraDeAutorizacaoCompleta(void)
{
    prepararCadastros();
    Data d = "19/10/2026";
    DataHora agora = {"19/10/2026", "07:00"};
    int id;

    /* Aula de P1 na L201, 09h-11h. Quatro solicitantes para o mesmo horário. */
    VERIFICAR("aluno não aplica prova", res_solicitar("A1", "L201", d, "09:00", "10:00",
              5, PROVA, "", agora, &id) == RES_HORARIO_AULA);
    VERIFICAR("outro professor sem autorização", res_solicitar("P2", "L201", d, "09:00", "10:00",
              5, PROVA, "", agora, &id) == RES_HORARIO_AULA);
    VERIFICAR("só o ministrante autoriza", grade_registrarAutorizacao("L201", d, "09:00", "P2", "P2")
              == GRADE_NAO_MINISTRANTE);
    VERIFICAR("ministrante autoriza P2", grade_registrarAutorizacao("L201", d, "09:00", "P1", "P2") == GRADE_OK);
    VERIFICAR("autorização não vale para estudo", res_solicitar("P2", "L201", d, "09:00", "10:00",
              5, ESTUDO, "", agora, &id) == RES_HORARIO_AULA);
    VERIFICAR("outro professor com autorização", res_solicitar("P2", "L201", d, "09:00", "10:00",
              5, PROVA, "", agora, &id) == RES_OK && id > 0);
    VERIFICAR("próprio ministrante, horário restante", res_solicitar("P1", "L201", d, "10:00", "11:00",
              5, PROVA, "", agora, &id) == RES_OK && id > 0);
    VERIFICAR("autorização é só para aquela data", res_solicitar("P2", "L201", "26/10/2026", "09:00", "10:00",
              5, PROVA, "", agora, &id) == RES_HORARIO_AULA);
}

static void fluxoCompletoDoAluno(void)
{
    prepararCadastros();
    Data d = "21/10/2026";
    DataHora agora = {"21/10/2026", "08:00"};
    char salas[MAX_SALAS][TAM];
    Reserva lista[MAX_RESERVAS];
    int n, id;

    VERIFICAR("busca", res_buscarSalasLivres(d, "14:00", "16:00", 8, 0, "A1", salas, &n) == RES_OK
              && n == 2);
    VERIFICAR("reserva da primeira sala livre", res_solicitar("A1", salas[0], d, "14:00", "16:00",
              8, ESTUDO, "", agora, &id) == RES_OK);
    VERIFICAR("sala some da busca", res_buscarSalasLivres(d, "14:00", "16:00", 8, 0, "A1", salas, &n)
              == RES_OK && n == 1);
    VERIFICAR("listagem", res_listarPorUsuario("A1", lista, &n) == RES_OK && n == 1
              && lista[0].id == id && lista[0].situacao == ATIVA);
    VERIFICAR("outro usuário não cancela", res_cancelar(id, "A2") == RES_NAO_AUTORIZADO);
    VERIFICAR("titular cancela", res_cancelar(id, "A1") == RES_OK);
    VERIFICAR("sala volta à busca", res_buscarSalasLivres(d, "14:00", "16:00", 8, 0, "A1", salas, &n)
              == RES_OK && n == 2);
    VERIFICAR("nenhum aviso enviado", notificacoes == 0);
}

int main(int argc, char *argv[])
{
    const CasoUnitario cenarios[] = {
        CASO(conciliarGrade), CASO(reservarLaboratorio), CASO(priorizarProva),
        CASO(autorizarProvaDuranteAulas), CASO(regraDeAutorizacaoCompleta), CASO(fluxoCompletoDoAluno)
    };
    return executarCasos(argc, argv, cenarios, sizeof cenarios / sizeof cenarios[0]);
}
