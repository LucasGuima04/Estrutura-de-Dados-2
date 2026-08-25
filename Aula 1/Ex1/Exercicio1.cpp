#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
//faça um programa capaz de ler um texto digitado do teclado e gravá-lo em um arquivo em disco. Utilize a tecla ESC (0x1B) para terminar a leitura.
//Utilize um editor de textos para ler e mostrar na tela o resultado doseu programa

#define ESC 0x1B

int main(){
    FILE *arq;
    char c;


    arq = fopen ("dados.txt","w");
    if (arq == NULL){
        printf("Erro ao abrir!");
        return 1;
    }
   
    while(1){
        c = getch();

        if (c == ESC)
            break;

        if(c == '\r'){
            printf("\n");
            fputc('\n',arq);
        } else {
            printf("%c",c);
            fputc(c,arq);
        }
    }
    fclose(arq);

    printf("\nConluido! Arquivo Criado! \n");

    return 0;
}