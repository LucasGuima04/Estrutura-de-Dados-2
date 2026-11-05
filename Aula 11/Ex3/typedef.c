#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Estado de cada posição da tabela.
   REMOVIDO existe porque, na sondagem, uma posição apagada
   não pode interromper a busca de chaves que passaram por ela. */
typedef enum { VAZIO, OCUPADO, REMOVIDO } Estado;

typedef struct {
    int  chave;
    char nome[50];
} Registro;

typedef struct {
    Estado   estado;
    Registro reg;
} Slot;

typedef struct {
    Slot *slots;
    int   M;      /* tamanho da tabela (de preferência primo) */
    int   n;      /* quantidade de registros ocupados */
} TabelaHash;

TabelaHash *criar_tabela(int M) {
    TabelaHash *t = malloc(sizeof(TabelaHash));
    t->M = M;
    t->n = 0;
    t->slots = malloc(M * sizeof(Slot));
    for (int i = 0; i < M; i++) t->slots[i].estado = VAZIO;
    return t;
}

void liberar_tabela(TabelaHash *t) {
    free(t->slots);
    free(t);
}

int h(int chave, int M) {
    int r = chave % M;
    return r < 0 ? r + M : r;            /* evita índice negativo */
}

/* i-ésima posição da sequência de sondagem quadrática */
int sondagem(int chave, int i, int M) {
    return (h(chave, M) + i * i) % M;
}

// Retorna a posição da chave ou -1 se não existir 
int buscar(TabelaHash *t, int chave) {
    for (int i = 0; i < t->M; i++) {
        int pos = sondagem(chave, i, t->M);
        if (t->slots[pos].estado == VAZIO) return -1;   /* nunca foi usada: a chave não está aqui */
        if (t->slots[pos].estado == OCUPADO &&
            t->slots[pos].reg.chave == chave) return pos;
        // REMOVIDO ou outra chave: continua sondando 
    }
    return -1;
}

//Retorna a posição onde inseriu, -1 se a chave já existe, -2 se não achou posição
int inserir(TabelaHash *t, Registro r) {
    if (buscar(t, r.chave) != -1) return -1;
    for (int i = 0; i < t->M; i++) {
        int pos = sondagem(r.chave, i, t->M);
        if (t->slots[pos].estado != OCUPADO) { 
            t->slots[pos].reg = r;
            t->slots[pos].estado = OCUPADO;
            t->n++;
            return pos;
        }
    }
    return -2;
}

int remover(TabelaHash *t, int chave) {
    int pos = buscar(t, chave);
    if (pos == -1) return 0;
    t->slots[pos].estado = REMOVIDO;
    t->n--;
    return 1;
}

void imprimir(TabelaHash *t) {
    for (int i = 0; i < t->M; i++) {
        printf("[%2d] ", i);
        if (t->slots[i].estado == OCUPADO)
            printf("%d (%s)\n", t->slots[i].reg.chave, t->slots[i].reg.nome);
        else if (t->slots[i].estado == REMOVIDO) printf("<removido>\n");
        else printf("-\n");
    }
}

int main(void) {
    TabelaHash *t = criar_tabela(13);
    int chaves[] = {13, 26, 39, 52, 65, 78};
    for (int i = 0; i < 6; i++) {
        Registro r = {chaves[i], ""};
        snprintf(r.nome, sizeof r.nome, "reg%d", chaves[i]);
        printf("inserir %d -> pos %d\n", chaves[i], inserir(t, r));
    }
    imprimir(t);

    printf("\nbuscar 65 -> pos %d\n", buscar(t, 65));
    printf("remover 26 -> %d\n", remover(t, 26));
    printf("buscar 65 apos remover 26 -> pos %d\n", buscar(t, 65));

    Registro x = {91, "reg91"};
    printf("inserir 91 -> pos %d (reaproveita a lapide)\n", inserir(t, x));
    imprimir(t);

    liberar_tabela(t);
    return 0;
}