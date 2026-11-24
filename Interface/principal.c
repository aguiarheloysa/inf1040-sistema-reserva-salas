
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "grade.h"
#include "notificacao.h"
#include "reserva.h"
#include "sala.h"
#include "smtp.h"
#include "tempo.h"
#include "usuario.h"

#define TAM_ENTRADA 256
#define QUANTIDADE(vetor) (sizeof(vetor) / sizeof((vetor)[0]))

static const char *const mensagensUsuario[] = {
    [USU_OK] = "Usuário cadastrado.",
    [USU_JA_EXISTE] = "Matrícula já cadastrada.",
    [USU_DADOS_INVALIDOS] = "Dados do usuário inválidos.",
    [USU_NAO_ENCONTRADO] = "Usuário não cadastrado.",
    [USU_LIMITE_ATINGIDO] = "Limite de usuários atingido."
};

static const char *const mensagensSala[] = {
    [SALA_OK] = "Sala cadastrada.",
    [SALA_CODIGO_INVALIDO] = "Código da sala inválido.",
    [SALA_JA_EXISTE] = "Sala já cadastrada.",
    [SALA_CAPACIDADE_INVALIDA] = "A capacidade deve ser maior que zero.",
    [SALA_NAO_ENCONTRADA] = "Sala não cadastrada.",
    [SALA_NENHUMA] = "Nenhuma sala encontrada.",
    [SALA_DADOS_INVALIDOS] = "Dados inválidos para a sala ou para o laboratório.",
    [SALA_LIMITE_ATINGIDO] = "Limite de cadastro de salas ou biometrias atingido."
};

static const char *const mensagensGrade[] = {
    [GRADE_OK] = "Aula cadastrada.",
    [GRADE_CONFLITO] = "Já existe uma aula na sala nesse horário.",
    [GRADE_HORARIO_INVALIDO] = "Dia da semana ou horário de aula inválido.",
    [GRADE_SEM_AULA] = "Nenhuma aula no horário da autorização.",
    [GRADE_NAO_MINISTRANTE] = "Apenas o ministrante da aula pode autorizar.",
    [GRADE_DADOS_INVALIDOS] = "Dados da grade inválidos.",
    [GRADE_LIMITE_ATINGIDO] = "Limite de aulas ou autorizações atingido."
};

static const char *const mensagensReserva[] = {
    [RES_OK] = "Reserva cancelada.",
    [RES_USUARIO_INEXISTENTE] = "Usuário não cadastrado.",
    [RES_CAPACIDADE_EXCEDIDA] = "A quantidade de pessoas excede a capacidade da sala.",
    [RES_HORARIO_AULA] = "Horário de aula. É necessária uma prova do ministrante ou autorizada por ele.",
    [RES_LAB_SEM_MONITOR] = "O laboratório exige um monitor cadastrado com biometria na sala.",
    [RES_CONFLITO] = "Já existe uma reserva ativa que conflita com esse horário.",
    [RES_ANTECEDENCIA_INSUFICIENTE] = "O aviso de cancelamento exige pelo menos duas horas de antecedência.",
    [RES_NAO_ENCONTRADA] = "Reserva ativa não encontrada.",
    [RES_NAO_AUTORIZADO] = "Apenas o titular pode cancelar a reserva.",
    [RES_NENHUMA_SALA] = "Nenhuma sala disponível.",
    [RES_NENHUMA_RESERVA] = "Nenhuma reserva para a matrícula informada.",
    [RES_SALA_INEXISTENTE] = "Sala não cadastrada.",
    [RES_DADOS_INVALIDOS] = "Dados da reserva inválidos. Use dd/mm/aaaa e hh:mm.",
    [RES_LIMITE_ATINGIDO] = "Limite de reservas atingido.",
    [RES_FALHA_NOTIFICACAO] = "Falha ao enviar o aviso de cancelamento. A prova não foi reservada."
};

static int lerLinha(const char *pergunta, char texto[TAM_ENTRADA])
{
    printf("%s ", pergunta);
    fflush(stdout);
    if (!fgets(texto, TAM_ENTRADA, stdin))
        return 0;

    size_t fim = strcspn(texto, "\r\n");
    if (texto[fim] == '\0' && fim == TAM_ENTRADA - 1) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF) {
        }
        puts("Entrada muito longa.");
        return 0;
    }
    texto[fim] = '\0';
    return 1;
}

static int lerFormulario(const char *const perguntas[], size_t n, char campos[][TAM_ENTRADA])
{
    for (size_t i = 0; i < n; i++) {
        if (!lerLinha(perguntas[i], campos[i]))
            return 0;
    }
    return 1;
}

static int lerInteiro(const char *texto, int minimo, int maximo, int *valor)
{
    char *fim;
    errno = 0;
    long numero = strtol(texto, &fim, 10);
    if (fim == texto || *fim || errno == ERANGE || numero < minimo || numero > maximo) {
        printf("Informe um número entre %d e %d.\n", minimo, maximo);
        return 0;
    }
    *valor = (int) numero;
    return 1;
}

static int exigirSala(char *codigo)
{
    TipoSala tipo;
    if (sala_obterTipo(codigo, &tipo) == SALA_OK)
        return 1;
    puts(mensagensSala[SALA_NAO_ENCONTRADA]);
    return 0;
}

static int exigirProfessor(char *matricula)
{
    TipoUsuario tipo;
    if (usu_obterTipo(matricula, &tipo) == USU_OK && tipo == PROFESSOR)
        return 1;
    puts("Informe a matrícula de um professor cadastrado.");
    return 0;
}

static void cadastrarUsuario(void)
{
    const char *perguntas[] = {"Matrícula (somente números):", "Nome:", "E-mail:", "Tipo (0 aluno, 1 professor):"};
    char campos[QUANTIDADE(perguntas)][TAM_ENTRADA];
    int tipo;

    if (lerFormulario(perguntas, QUANTIDADE(perguntas), campos)
            && lerInteiro(campos[3], ALUNO, PROFESSOR, &tipo))
        puts(mensagensUsuario[usu_cadastrar(campos[0], campos[1], campos[2], tipo)]);
}

static void cadastrarSala(void)
{
    const char *perguntas[] = {"Código da sala:", "Tipo (0 comum, 1 laboratório):",
        "Capacidade:", "Tem quadro? (0 não, 1 sim):"};
    char campos[QUANTIDADE(perguntas)][TAM_ENTRADA];
    int tipo, capacidade, quadro;

    if (lerFormulario(perguntas, QUANTIDADE(perguntas), campos)
            && lerInteiro(campos[1], COMUM, LABORATORIO, &tipo)
            && lerInteiro(campos[2], 1, INT_MAX, &capacidade)
            && lerInteiro(campos[3], 0, 1, &quadro))
        puts(mensagensSala[sala_cadastrar(campos[0], tipo, capacidade, quadro)]);
}

static void cadastrarAula(void)
{
    const char *perguntas[] = {"Código da sala:", "Disciplina:", "Matrícula do professor:",
        "Dia da semana (1 segunda a 7 domingo):", "Início (hh:mm):", "Fim (hh:mm):"};
    char campos[QUANTIDADE(perguntas)][TAM_ENTRADA];
    int dia;

    if (lerFormulario(perguntas, QUANTIDADE(perguntas), campos)
            && lerInteiro(campos[3], 1, 7, &dia)
            && exigirSala(campos[0]) && exigirProfessor(campos[2]))
        puts(mensagensGrade[grade_cadastrarAula(campos[0], campos[1], campos[2],
                                               dia, campos[4], campos[5])]);
}

static void registrarBiometria(void)
{
    const char *perguntas[] = {"Código do laboratório:", "Matrícula do monitor:"};
    char campos[QUANTIDADE(perguntas)][TAM_ENTRADA];

    if (!lerFormulario(perguntas, QUANTIDADE(perguntas), campos))
        return;
    if (!usu_existe(campos[1])) {
        puts("Monitor não cadastrado.");
        return;
    }
    int resultado = sala_registrarBiometria(campos[0], campos[1]);
    puts(resultado == SALA_OK ? "Biometria registrada." : mensagensSala[resultado]);
}

static void autorizarProva(void)
{
    const char *perguntas[] = {"Código da sala:", "Data (dd/mm/aaaa):", "Hora (hh:mm):",
        "Matrícula do ministrante:", "Matrícula do professor solicitante:"};
    char campos[QUANTIDADE(perguntas)][TAM_ENTRADA];

    if (!lerFormulario(perguntas, QUANTIDADE(perguntas), campos)
            || !exigirSala(campos[0]) || !exigirProfessor(campos[3]) || !exigirProfessor(campos[4]))
        return;
    int resultado = grade_registrarAutorizacao(campos[0], campos[1], campos[2], campos[3], campos[4]);
    puts(resultado == GRADE_OK ? "Autorização registrada." : mensagensGrade[resultado]);
}

static void buscarSalas(void)
{
    const char *perguntas[] = {"Matrícula:", "Data (dd/mm/aaaa):", "Início (hh:mm):",
        "Fim (hh:mm):", "Quantidade de pessoas:", "Precisa de quadro? (0 não, 1 sim):"};
    char campos[QUANTIDADE(perguntas)][TAM_ENTRADA];
    char salas[MAX_SALAS][TAM];
    int quantidade, quadro, n, inicio, fim;

    if (!lerFormulario(perguntas, QUANTIDADE(perguntas), campos)
            || !lerInteiro(campos[4], 1, INT_MAX, &quantidade)
            || !lerInteiro(campos[5], 0, 1, &quadro))
        return;
    if (!usu_existe(campos[0])) {
        puts(mensagensReserva[RES_USUARIO_INEXISTENTE]);
        return;
    }
    if (tempo_dia(campos[1]) < 0 || !tempo_intervalo(campos[2], campos[3], &inicio, &fim)) {
        puts(mensagensReserva[RES_DADOS_INVALIDOS]);
        return;
    }
    int resultado = res_buscarSalasLivres(campos[1], campos[2], campos[3], quantidade,
                                         quadro, campos[0], salas, &n);
    if (resultado != RES_OK) {
        puts(mensagensReserva[resultado]);
        return;
    }
    puts("Salas disponíveis:");
    for (int i = 0; i < n; i++)
        puts(salas[i]);
}

static int obterAgora(DataHora *agora)
{
    time_t instante = time(NULL);
    struct tm *local = instante == (time_t) -1 ? NULL : localtime(&instante);
    return local
        && strftime(agora->data, sizeof agora->data, "%d/%m/%Y", local) != 0
        && strftime(agora->hora, sizeof agora->hora, "%H:%M", local) != 0;
}

static void solicitarReserva(void)
{
    const char *perguntas[] = {"Matrícula:", "Código da sala:", "Data (dd/mm/aaaa):",
        "Início (hh:mm):", "Fim (hh:mm):", "Quantidade de pessoas:",
        "Finalidade (0 estudo, 1 prova):", "Matrícula do monitor (Enter se não se aplica):"};
    char campos[QUANTIDADE(perguntas)][TAM_ENTRADA];
    int quantidade, finalidade, id;
    DataHora agora = {0};

    if (!lerFormulario(perguntas, QUANTIDADE(perguntas), campos)
            || !lerInteiro(campos[5], 1, INT_MAX, &quantidade)
            || !lerInteiro(campos[6], ESTUDO, PROVA, &finalidade))
        return;
    if (!obterAgora(&agora)) {
        puts("Não foi possível obter a data e a hora locais.");
        return;
    }
    int resultado = res_solicitar(campos[0], campos[1], campos[2], campos[3], campos[4],
                                  quantidade, finalidade, campos[7], agora, &id);
    if (resultado == RES_OK)
        printf("Reserva confirmada. ID: %d\n", id);
    else
        puts(mensagensReserva[resultado]);
}

static void cancelarReserva(void)
{
    const char *perguntas[] = {"Matrícula:", "ID da reserva:"};
    char campos[QUANTIDADE(perguntas)][TAM_ENTRADA];
    int id;

    if (lerFormulario(perguntas, QUANTIDADE(perguntas), campos)
            && lerInteiro(campos[1], 1, INT_MAX, &id))
        puts(mensagensReserva[res_cancelar(id, campos[0])]);
}

static void listarReservas(void)
{
    char matricula[TAM_ENTRADA];
    Reserva reservas[MAX_RESERVAS];
    int n;

    if (!lerLinha("Matrícula:", matricula))
        return;
    int resultado = res_listarPorUsuario(matricula, reservas, &n);
    if (resultado != RES_OK) {
        puts(mensagensReserva[resultado]);
        return;
    }
    for (int i = 0; i < n; i++) {
        const Reserva *r = &reservas[i];
        printf("#%d | %s | %s %s–%s | %d pessoa(s) | %s | %s\n",
               r->id, r->codSala, r->data, r->inicio, r->fim, r->qtdPessoas,
               r->finalidade == PROVA ? "PROVA" : "ESTUDO",
               r->situacao == ATIVA ? "ATIVA" : "CANCELADA");
    }
}

int main(int argc, char *argv[])
{
    if (argc == 2 && strcmp(argv[1], "--smtp") == 0) {
        smtp_preparar();
        not_definirCanalEntrega(smtp_enviar);
    } else if (argc != 1) {
        fprintf(stderr, "Uso: %s [--smtp]\n", argv[0]);
        return EXIT_FAILURE;
    }

    void (*const acoes[])(void) = {NULL, cadastrarUsuario, cadastrarSala, cadastrarAula,
        registrarBiometria, autorizarProva, buscarSalas, solicitarReserva, cancelarReserva,
        listarReservas};

    puts("Sistema de Reserva de Salas de Aula\n"
         "1 Cadastrar usuário\n2 Cadastrar sala\n3 Cadastrar aula\n"
         "4 Registrar biometria\n5 Registrar autorização para prova\n"
         "6 Buscar salas livres\n7 Solicitar reserva\n8 Cancelar reserva\n"
         "9 Listar reservas por usuário\n0 Sair");
    while (!feof(stdin) && !ferror(stdin)) {
        char opcao[TAM_ENTRADA];
        int numero;
        if (!lerLinha("Opção:", opcao) || !lerInteiro(opcao, 0, (int) QUANTIDADE(acoes) - 1, &numero))
            continue;
        if (numero == 0)
            break;
        acoes[numero]();
    }
    return ferror(stdin) ? EXIT_FAILURE : EXIT_SUCCESS;
}
