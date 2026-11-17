#include <stdio.h>
#include <string.h>
#include <usuario.h>

int usuario_cadastro (char* matricula, char* nome, char* email, int telefone){
    struct usuario u;
    if (matricula && nome && email){
        strcpy(u.matricula, matricula);
        strcpy(u.nome, nome);
        strcpy(u.email, email);
        u.telefone = telefone;
    }
    if (matricula == NULL){
        printf("matricula inválida.\n");
    }
    if (email == NULL){
        printf("Email inválido.\n");
    }
    if (nome == NULL){
        printf("Nome inválido.\n");
    }
    
}

int usuario_existe (char* matricula);

int usuario_tipo (char* matricula, char* email);

int main(void){

}