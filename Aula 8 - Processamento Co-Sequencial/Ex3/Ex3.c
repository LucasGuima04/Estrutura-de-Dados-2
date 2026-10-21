
#include <stdio.h>

int main() {
    int dados[] = {14, 3, 27, 9, 55, 41, 22, 18, 7, 33, 12, 29, 5, 48, 20, 11};
    int n = 16;

    int f1[16], f2[16], g1[16], g2[16];
    int tam = 1;

    for (int i = 0; i < n; i++) {
        if (i % 2 == 0)
            f1[i / 2] = dados[i];
        else
            f2[i / 2] = dados[i];
    }

    int qtd1 = (n + 1) / 2;
    int qtd2 = n / 2;

    while (tam < n) {
        int pos1 = 0, pos2 = 0;
        int dest1 = 0, dest2 = 0;
        int grupo = 0;

        while (pos1 < qtd1 || pos2 < qtd2) {
            int ini1 = pos1;
            int ini2 = pos2;
            int fim1 = pos1 + tam;
            int fim2 = pos2 + tam;

            if (fim1 > qtd1) fim1 = qtd1;
            if (fim2 > qtd2) fim2 = qtd2;

            int destino = grupo % 2;
            int *saida = destino == 0 ? g1 : g2;
            int *pSaida = destino == 0 ? &dest1 : &dest2;

            while (ini1 < fim1 && ini2 < fim2) {
                if (f1[ini1] <= f2[ini2])
                    saida[(*pSaida)++] = f1[ini1++];
                else
                    saida[(*pSaida)++] = f2[ini2++];
            }

            while (ini1 < fim1)
                saida[(*pSaida)++] = f1[ini1++];

            while (ini2 < fim2)
                saida[(*pSaida)++] = f2[ini2++];

            pos1 = fim1;
            pos2 = fim2;
            grupo++;
        }

        qtd1 = dest1;
        qtd2 = dest2;

        for (int i = 0; i < qtd1; i++)
            f1[i] = g1[i];

        for (int i = 0; i < qtd2; i++)
            f2[i] = g2[i];

        tam *= 2;
    }

    int i = 0, j = 0;

    while (i < qtd1 && j < qtd2) {
        if (f1[i] <= f2[j])
            printf("%02d ", f1[i++]);
        else
            printf("%02d ", f2[j++]);
    }

    while (i < qtd1)
        printf("%02d ", f1[i++]);

    while (j < qtd2)
        printf("%02d ", f2[j++]);

    printf("\n");

    return 0;
}
