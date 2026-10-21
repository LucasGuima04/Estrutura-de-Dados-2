#include <stdio.h>

int main() {
    int lista1[] = {10, 25, 30, 45, 60};
    int lista2[] = {5, 20, 30, 40, 50, 70};

    int n = 5;
    int m = 6;
    int resultado[11];

    int i = 0;
    int j = 0;
    int k = 0;

    while (i < n && j < m) {
        if (lista1[i] <= lista2[j]) {
            resultado[k] = lista1[i];
            i++;
        } else {
            resultado[k] = lista2[j];
            j++;
        }
        k++;
    }

    while (i < n) {
        resultado[k] = lista1[i];
        i++;
        k++;
    }

    while (j < m) {
        resultado[k] = lista2[j];
        j++;
        k++;
    }

    for (i = 0; i < n + m; i++) {
        printf("%d ", resultado[i]);
    }

    printf("\n");

    return 0;
}
