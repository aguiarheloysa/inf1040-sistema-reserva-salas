
#include "tempo.h"
#include "unitario.h"

static void dia_de_referencia(void)
{
    VERIFICAR("dia de referência", tempo_diaSemana(tempo_dia("19/10/2026")) == 1);
}

static void domingo(void)
{
    VERIFICAR("domingo", tempo_diaSemana(tempo_dia("25/10/2026")) == 7);
}

static void ano_bissexto(void)
{
    VERIFICAR("ano bissexto", tempo_dia("01/03/2024") - tempo_dia("28/02/2024") == 2);
}

static void regra_de_seculo(void)
{
    VERIFICAR("regra de século", tempo_dia("29/02/1900") == -1 && tempo_dia("29/02/2000") >= 0);
}

static void datas_invalidas(void)
{
    VERIFICAR("datas inválidas", tempo_dia("31/04/2026") == -1 && tempo_dia("1/04/2026") == -1
              && tempo_dia("01/01/0000") == -1 && tempo_dia(NULL) == -1);
}

static void limites_da_hora(void)
{
    VERIFICAR("limites da hora", tempo_minuto("00:00") == 0 && tempo_minuto("23:59") == 1439);
}

static void horas_invalidas(void)
{
    VERIFICAR("horas inválidas", tempo_minuto("24:00") == -1 && tempo_minuto("9:00") == -1
              && tempo_minuto("10:60") == -1 && tempo_minuto(NULL) == -1);
}

static void intervalo_valido(void)
{
    int inicio, fim;

    VERIFICAR("intervalo", tempo_intervalo("09:00", "10:30", &inicio, &fim)
              && inicio == 540 && fim == 630);
}

static void intervalo_invalido(void)
{
    int inicio, fim;

    VERIFICAR("intervalo vazio ou invertido", !tempo_intervalo("09:00", "09:00", &inicio, &fim)
              && !tempo_intervalo("10:00", "09:00", &inicio, &fim));
}

static void sobreposicao(void)
{
    VERIFICAR("sobreposição", tempo_sobrepoe(540, 600, 599, 601)
              && !tempo_sobrepoe(540, 600, 600, 601));
}

static void virada_do_ano(void)
{
    DataHora antes = {"31/12/2026", "23:30"};
    DataHora depois = {"01/01/2027", "01:30"};

    VERIFICAR("virada do ano", tempo_instante(depois) - tempo_instante(antes) == 120);
}

int main(int argc, char *argv[])
{
    const CasoUnitario casos[] = {
        CASO(dia_de_referencia), CASO(domingo), CASO(ano_bissexto), CASO(regra_de_seculo),
        CASO(datas_invalidas), CASO(limites_da_hora), CASO(horas_invalidas), CASO(intervalo_valido),
        CASO(intervalo_invalido), CASO(sobreposicao), CASO(virada_do_ano)
    };
    return executarCasos(argc, argv, casos, sizeof casos / sizeof casos[0]);
}
