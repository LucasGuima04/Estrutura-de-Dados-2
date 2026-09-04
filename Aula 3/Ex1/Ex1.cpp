#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <string.h>

void grava_registro(FILE *p_arq, char *cpf, char *nome, char *sobrenome, char *telefone, char *cidade){
     char registro[200];
     
     sprintf(registro, "%s|%s|%s|%s|%s|", cpf, nome, sobrenome, telefone, cidade);
     fwrite(registro, sizeof(char), strlen(registro), p_arq);
}

int pega_campo(FILE *p_arq, char *p_campo){
     char ch;
     int i=0;

     p_campo[i] = '\0';
     
     while (fread(&ch,sizeof(char),1,p_arq))
       {
           if (ch == '|')
             break;
           else p_campo[i] = ch;
           
           i++;
       }
     
     p_campo[i] = '\0';
     
     return strlen(p_campo);
    }

int pega_registro(FILE *p_arq, char *cpf, char *nome, char *sobrenome, char *telefone, char *cidade){
    int tam_cpf;
    tam_cpf = pega_campo(p_arq, cpf);

    if(tam_cpf == 0 && feof(p_arq)){
        return 0;
    }

    int tam_nome, tam_sobrenome, tam_telefone, tam_cidade;
    tam_nome = pega_campo(p_arq, nome);
    tam_sobrenome = pega_campo(p_arq, sobrenome);
    tam_telefone = pega_campo(p_arq, telefone);
    tam_cidade = pega_campo(p_arq, cidade);

    return 1;
}

int remove_cliente(char *cpf_remover){
    FILE *arq1, *arq2;
    char cpf[20], nome[50], sobrenome[50], telefone[20], cidade[50];

    arq1 = fopen("clientes.bin","rb");
    arq2 = fopen("temp.bin","wb");
    if(arq1 == NULL || arq2 == NULL){
        printf("Erro na memoria!");
        exit(1);
    }

    while (pega_registro(arq1, cpf, nome, sobrenome,telefone, cidade)){
        if(strcmp(cpf,cpf_remover)){
            grava_registro(arq2,cpf,nome,sobrenome,telefone,cidade);
        }
    }

    fclose(arq1);
    fclose(arq2);
    remove("clientes.bin");
    rename("temp.bin","clientes.bin");

    return 1;
}

int update_cliente(char *cpf_atualizar, char *novo_nome, char *novo_sobrenome, char *novo_telefone, char *nova_cidade){
    FILE *arq1, *arq2;
    char cpf[20], nome[50], sobrenome[50], telefone[20], cidade[50];

    arq1 = fopen("clientes.bin","rb");
    arq2 = fopen("temp.bin","wb");

    if(arq1 == NULL || arq2 == NULL){
        printf("Erro na memoria!");
        exit(1);
    }

    while (pega_registro(arq1, cpf, nome, sobrenome,telefone, cidade)){
        if(strcmp(cpf,cpf_atualizar)){
            grava_registro(arq2,cpf,nome,sobrenome,telefone,cidade);
        }else{
            grava_registro(arq2,cpf_atualizar,novo_nome,novo_sobrenome,novo_telefone,nova_cidade);
        }
    }

    fclose(arq1);
    fclose(arq2);
    remove("clientes.bin");
    rename("temp.bin","clientes.bin");

    return 1;
}

int str_cpf(char *a, char *b){
    if(a>b){
        return 1;
    }else if(a == b){
        return 0;
    } else{
        return -1;
    }
}

int insere_cliente(char *cpf, char *nome, char *sobrenome, char *telefone, char *cidade)
{
    FILE *arq1, *arq2;
    char cpf_lido[20], nome_lido[50], sobrenome_lido[50], telefone_lido[20], cidade_lido[50];
    int ja_inseriu = 0;

    arq1 = fopen("clientes.bin","rb");
    arq2 = fopen("temp.bin","wb");
    if(arq1 == NULL || arq2 == NULL){ printf("Erro!"); exit(1); }

    while ( pega_registro(arq1, cpf_lido, nome_lido, sobrenome_lido, telefone_lido, cidade_lido) ){
        if(!ja_inseriu && strcmp(cpf_lido,cpf) > 0){
            grava_registro(arq2,cpf,nome,sobrenome,telefone,cidade);
            ja_inseriu = 1;
        }
        grava_registro(arq2,cpf_lido,nome_lido,sobrenome_lido,telefone_lido,cidade_lido);
    }
    
    if(!ja_inseriu){
        grava_registro(arq2,cpf,nome,sobrenome,telefone,cidade);
    }

    fclose(arq1);
    fclose(arq2);
    remove("clientes.bin");
    rename("temp.bin","clientes.bin");
    return 1;
}