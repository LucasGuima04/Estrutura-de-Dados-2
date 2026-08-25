#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define TAM_MAX_LINHA 100
#define MAX_LINHAS 100

int main(){
    FILE *arq;
    FILE *arq2;
    int k,n = 0;

    char buffer[MAX_LINHAS][TAM_MAX_LINHA];

    arq = fopen("dados.txt","r");
    if(arq == NULL){
        printf("Erro ao abrir!");
        exit(1);
    }
    
    while(fgets(buffer[n],TAM_MAX_LINHA,arq) != NULL){
        n++;
    }

    printf("O arquivo possui %d linhas!\n",n);
    printf("Quantas Linhas finais deseja?\n");
    scanf("%d",&k);
    

    arq2 = fopen("resultado.txt","w");
    if(arq2 == NULL){
        printf("Erro ao abrir!");
        exit(1);
    }

    if(k > n){
        k = n;
    }
    for(int i = n-k;i<n;i++){
        fputs(buffer[i],arq2);
    }

    printf("Arquivo criado com sucesso!\n");

    fclose(arq);
    fclose(arq2);

    return 0;
}