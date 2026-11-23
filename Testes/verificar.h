
#ifndef VERIFICAR_H
#define VERIFICAR_H

#include <stdio.h>
#include <stdlib.h>

#define VERIFICAR(caso, condicao)                                              \
    do {                                                                      \
        if (!(condicao)) {                                                    \
            fprintf(stderr, "%s:%d [%s] falhou: %s\n", __FILE__, __LINE__,       \
                    (caso), #condicao);                                        \
            exit(EXIT_FAILURE);                                               \
        }                                                                     \
    } while (0)

#endif
