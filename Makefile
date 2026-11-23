
CC ?= gcc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic
CPPFLAGS += -IAluno -ISala -IGrade -INotificacao -IReserva -IComum

ifeq ($(OS),Windows_NT)
EXE := .exe
PYTHON ?= python
else
PYTHON ?= python3
endif

BUILD := build
TEMPO := Comum/tempo.c
USUARIO := Aluno/aluno.c
SALA := Sala/sala.c
GRADE := Grade/grade.c
NOTIFICACAO := Notificacao/notificacao.c
RESERVA := Reserva/reserva.c
MODULOS := $(TEMPO) $(USUARIO) $(SALA) $(GRADE) $(NOTIFICACAO) $(RESERVA)
CABECALHOS := $(wildcard Aluno/*.h Sala/*.h Grade/*.h Notificacao/*.h Reserva/*.h Comum/*.h)
APP := $(BUILD)/sistema_reserva$(EXE)
UNITARIOS := $(addprefix $(BUILD)/unit_,$(addsuffix $(EXE),tempo usuario sala grade notificacao reserva))
INTEGRACAO := $(BUILD)/integracao$(EXE)

.PHONY: all test test-unit test-integration test-interface
all: $(APP)

$(BUILD):
	mkdir "$@"

$(APP): Interface/principal.c Interface/smtp.c Interface/smtp.h $(MODULOS) $(CABECALHOS) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c,$^) $(LDFLAGS) $(LDLIBS) -o "$@"

$(BUILD)/unit_tempo$(EXE): Testes/unit_tempo.c $(TEMPO)
$(BUILD)/unit_usuario$(EXE): Testes/unit_usuario.c $(USUARIO)
$(BUILD)/unit_sala$(EXE): Testes/unit_sala.c $(SALA)
$(BUILD)/unit_grade$(EXE): Testes/unit_grade.c $(GRADE) $(TEMPO)
$(BUILD)/unit_notificacao$(EXE): Testes/unit_notificacao.c $(NOTIFICACAO)
$(BUILD)/unit_reserva$(EXE): Testes/unit_reserva.c $(RESERVA) $(TEMPO)
$(INTEGRACAO): Testes/integracao.c $(MODULOS)
$(UNITARIOS): Testes/unitario.h
$(UNITARIOS) $(INTEGRACAO): Testes/verificar.h $(CABECALHOS) | $(BUILD)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c,$^) $(LDFLAGS) $(LDLIBS) -o "$@"

test: test-unit test-integration test-interface

test-unit: $(UNITARIOS)
	$(PYTHON) Testes/test_unitarios.py $(UNITARIOS)

test-integration: $(INTEGRACAO)
	$(INTEGRACAO)

test-interface: $(APP)
	$(PYTHON) Testes/test_interface.py $(APP)
