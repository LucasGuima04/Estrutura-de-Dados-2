#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <ctype.h>

#define ESC 0x1B
#define BACKSPACE 0x86 //Isso é para não gravar o lixo de memoria deixado pelo backspace.
#define TAM 10000

void gerarArquivo(){
    FILE *arq;
    char buffer[TAM];
    int i = 0;
    int c;

    arq = fopen("dados.txt", "w");
    if(arq == NULL){
        printf("Erro ao abrir!\n");
        exit(1);
    }

    printf("Digite o texto! \n\n");

    while(1){
        c = getch();

        if (c == ESC){
            break;
        }else if(c == BACKSPACE){
            if(i>0){
                i--;
                printf("\b \b");
            }
        }else if(c == '\r'){
            buffer[i++] = '\n';
            printf("\n");
        }else{
            buffer[i++] = c;
            printf("%c",c);
        }
    }
    fwrite(buffer, sizeof(char),i,arq);
    fclose(arq);

    printf("\nArquivo criado com sucesso!\n");

}

void dumpArquivo(){
    FILE *arq;
    unsigned char linha[16]; //Quantidade de bytes
    int n, i;

    arq = fopen("dados.txt", "rb"); //Ler binário
    if(arq == NULL){
        printf("Erro ao abrir!\n");
        exit(1);
    }

    printf("\nDump do arquivo\n");

    while((n = fread(linha,sizeof(unsigned char),16,arq))> 0 ) {

        //Coluna da esqueda
        for(i = 0;i < 16;i++){
            if (i < n){
                printf("%02X ", linha[i]);
            } else{
                printf("   ");
            }
        }

        printf("   ");

        for(i = 0; i< n; i++){
            if(isprint(linha[i])){
                printf("%c", linha[i]);
            }else {
                printf(".");
            }
        }

        printf("\n");
    }

    fclose(arq);
}

int main(void) {
    gerarArquivo();
    dumpArquivo();
    return 0;
}