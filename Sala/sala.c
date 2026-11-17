#include <sala.h>
#include <string.h>

int cadastrar_sala(char* sala, char*tipo, int capacidade){
    struct sala s;
    if (sala && tipo && capacidade){
        strcpy(s.numSala, sala);
        strcpy(s.tipoSala, tipo);
        s.capacidade = capacidade;
    }
}
int main(void){



    return 0;
}
