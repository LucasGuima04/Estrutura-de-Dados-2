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

long *monta_indice(FILE *p_arq, int *p_qtd){
    long *indice = (long*)malloc(sizeof(long) * 10000);
    char cpf[20], nome[50], sobrenome[50], telefone[20], cidade[50];
    int qtd = 0;
    long pos_atual;

    rewind(p_arq);
    pos_atual = ftell(p_arq);

    while(pega_registro(p_arq,cpf,nome,sobrenome,telefone,cidade)){
        indice[qtd] = pos_atual;
        qtd++;
        pos_atual = ftell(p_arq);
    }

    *p_qtd = qtd;
    return indice;
}

long busca_binaria(FILE *p_arq, long *indice, int qtd, char *cpf_busca){
    char cpf[20], nome[50], sobrenome[50], telefone[20], cidade[50];
    int low = 0;
    int high = qtd - 1;
    int mid;
    mid = (low+high) / 2;

    while(low <= high){
        fseek(p_arq, indice[mid], SEEK_SET);
        pega_registro(p_arq,cpf,nome,sobrenome,telefone,cidade);
        if(!strcmp(cpf,cpf_busca)){
            return indice[mid];
        }else if(strcmp(cpf,cpf_busca) > 0){
            high = mid -1;
        }else{
            low = mid + 1;
        }
        mid = (low+high) / 2;
    }
    return -1;
}

// O índice é montado do zero a cada chamada de busca_binaria.
// Isso garante que a busca sempre reflete o estado atual do arquivo
// (mesmo que um insere_cliente/remove_cliente/update_cliente tenha
// modificado o arquivo desde a última busca), ao custo de gastar O(n)
// pra montar o índice antes de cada busca O(log n) -- trade-off
// escolhido em favor da correção/simplicidade em vez de performance.
long busca_binaria(char *cpf_busca){
    FILE *p_arq;
    char cpf[20], nome[50], sobrenome[50], telefone[20], cidade[50];
    long *indice;
    int qtd;
    long resultado = -1;

    p_arq = fopen("clientes.bin","rb");
    if(p_arq == NULL){ printf("Erro!"); exit(1); }

    indice = monta_indice(p_arq, &qtd);

    int low = 0;
    int high = qtd - 1;
    int mid;
    mid = (low+high) / 2;

    while(low <= high){
        fseek(p_arq, indice[mid], SEEK_SET);
        pega_registro(p_arq,cpf,nome,sobrenome,telefone,cidade);
        if(!strcmp(cpf,cpf_busca)){
            resultado = indice[mid];
            fclose(p_arq);
            free(indice);
            return resultado;
        }else if(strcmp(cpf,cpf_busca) > 0){
            high = mid -1;
        }else{
            low = mid + 1;
        }
        mid = (low+high) / 2;
    }

    fclose(p_arq);
    free(indice);
    return resultado;
}

int merge_arquivos(void)
{
    FILE *arq1, *arq2, *saida;
    char cpf1[20], nome1[50], sobrenome1[50], telefone1[20], cidade1[50];
    char cpf2[20], nome2[50], sobrenome2[50], telefone2[20], cidade2[50];
    int tem_reg1, tem_reg2;

    arq1 = fopen("clientes.bin","rb");
    arq2 = fopen("importados.bin","rb");
    saida = fopen("clientes_merged.bin","wb");
    if(arq1==NULL || arq2==NULL || saida==NULL){ printf("Erro!"); exit(1); }

    tem_reg1 = pega_registro(arq1, cpf1, nome1, sobrenome1, telefone1, cidade1);
    tem_reg2 = pega_registro(arq2, cpf2, nome2, sobrenome2, telefone2, cidade2);

    while (tem_reg1 && tem_reg2)
    {
        int cmp = strcmp(cpf1,cpf2);
        if(cmp > 0 ){
            grava_registro(saida,cpf2,nome2,sobrenome2,telefone2,cidade2);
            tem_reg2 = pega_registro(arq2, cpf2, nome2, sobrenome2, telefone2, cidade2);
        }else{
            grava_registro(saida,cpf1,nome1,sobrenome1,telefone1,cidade1);
            tem_reg1 = pega_registro(arq1, cpf1, nome1, sobrenome1, telefone1, cidade1);
        } 
        
    }

    while(tem_reg1){
        grava_registro(saida,cpf1,nome1,sobrenome1,telefone1,cidade1);
            tem_reg1 = pega_registro(arq1, cpf1, nome1, sobrenome1, telefone1, cidade1);
    }

    while(tem_reg2){
        grava_registro(saida,cpf2,nome2,sobrenome2,telefone2,cidade2);
            tem_reg2 = pega_registro(arq2, cpf2, nome2, sobrenome2, telefone2, cidade2);
    }
    
    fclose(arq1);
    fclose(arq2);
    fclose(saida);
    return 1;
}

void le_string(char *destino, int tamanho)
{
    fgets(destino, tamanho, stdin);
    destino[strcspn(destino, "\n")] = '\0';  // remove o \n
}

int main(void)
{
    int opcao;
    char cpf[20], nome[50], sobrenome[50], telefone[20], cidade[50];
    long offset;

    do
    {
        printf("\n1-Inserir 2-Remover 3-Atualizar 4-Buscar 5-Merge 0-Sair\n");
        printf("Opcao: ");
        scanf("%d", &opcao);
        getchar();

        switch(opcao)
        {
            case 1:
                printf("CPF: ");
                le_string(cpf, 20);
                printf("Nome: ");
                le_string(nome, 50);
                printf("Sobrenome: ");
                le_string(sobrenome, 50);
                printf("Telefone: ");
                le_string(telefone, 20);
                printf("Cidade: ");
                le_string(cidade, 50);

                insere_cliente(cpf, nome, sobrenome, telefone, cidade);
                printf("\nCliente inserido!\n");
                break;
            case 2:
                printf("CPF a ser removido: ");
                le_string(cpf, 20);

                remove_cliente(cpf);
                printf("\nCliente removido!\n");
                break;
            case 3:
                printf("CPF: ");
                le_string(cpf, 20);
                printf("Nome: ");
                le_string(nome, 50);
                printf("Sobrenome: ");
                le_string(sobrenome, 50);
                printf("Telefone: ");
                le_string(telefone, 20);
                printf("Cidade: ");
                le_string(cidade, 50);

                update_cliente(cpf,nome,sobrenome,telefone,cidade);
                break;
            case 4:
                printf("\nCPF a ser removido: ");
                le_string(cpf, 20);

                offset = busca_binaria(cpf);
                if (offset == -1){
                    printf("Cliente não encontrado!");
                } else {
                    FILE *arq;
                    arq = fopen("clientes.bin","rb");
                    if(arq == NULL){printf("Erro!"); exit(1);}
                    fseek(arq,offset,SEEK_SET);
                    pega_registro(arq,cpf,nome,sobrenome,telefone,cidade);
                    printf("\n Cliente Encontrado! \n");
                    printf("%s|%s|%s|%s|%s|",cpf,nome,sobrenome,telefone,cidade);
                    fclose(arq);
                }
                break;
            case 5:
                merge_arquivos();
                printf("\nArquivos foram unidos com sucesso!\n");
                break;
        }
    } while(opcao != 0);

    return 0;
}