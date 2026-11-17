struct usuario{
    char* matricula;
    char* nome;
    char* email;
    int telefone;
};
int usuario_cadastro (char* matricula, char* nome, char* email);

int usuario_existe (char* matricula);

int usuario_tipo (char* matricula, char* email);

