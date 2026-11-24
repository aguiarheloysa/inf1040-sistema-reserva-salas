#ifndef MATRICULA_H
#define MATRICULA_H

#include <string.h>

#include "tipos.h"

static inline int matricula_valida(const char *matricula)
{
    if (!matricula)
        return 0;

    size_t digitos = strspn(matricula, "0123456789");
    return digitos > 0 && digitos < TAM && matricula[digitos] == '\0';
}

#endif
