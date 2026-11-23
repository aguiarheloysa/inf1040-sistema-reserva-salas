#ifndef UNITARIO_H
#define UNITARIO_H

#include <string.h>

#include "verificar.h"

typedef struct {
    const char *nome;
    void (*executar)(void);
} CasoUnitario;

static inline int executarCasos(int argc, char *argv[], const CasoUnitario casos[], size_t n)
{
    if (argc != 2) {
        fprintf(stderr, "Uso: %s --listar | nome_do_caso\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "--listar") == 0) {
        for (size_t i = 0; i < n; i++)
            puts(casos[i].nome);
        return EXIT_SUCCESS;
    }

    for (size_t i = 0; i < n; i++) {
        if (strcmp(argv[1], casos[i].nome) == 0) {
            casos[i].executar();
            return EXIT_SUCCESS;
        }
    }
    fprintf(stderr, "Caso desconhecido: %s\n", argv[1]);
    return EXIT_FAILURE;
}

#define CASO(funcao) {#funcao, funcao}

#endif
