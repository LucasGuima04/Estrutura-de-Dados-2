#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#define TAM_CODIGO 3
#define DELIM '#'
#define MARCA_LIVRE '*'
 
#define ARQ_DADOS  "seguradoras.bin"
#define ARQ_INSERE "insere.bin"
#define ARQ_REMOVE "remove.bin"
#define ARQ_CTRL   "controle.bin"
 
#define MAX_REGISTROS 1000
#define TAM_PAYLOAD   200 /* buffer maximo para um registro montado */
 
typedef struct {
    int primeiro_livre; // offset do 1o espaco livre da lista; -1 = vazia
} Cabecalho;
 
typedef struct {
    int usado_insere; // quantos registros de insere.bin ja foram inseridos
    int usado_remove; // quantos codigos de remove.bin ja foram usados
} EstadoUso;
 
typedef struct {
    char codigo[4]; // TAM_CODIGO + 1
    char nome[51];
    char seguradora[51];
    char tipo[31];
} SeguradoTxt;
 
// ---------------------------------------------------------------
// Cabecalho do arquivo de dados
// ---------------------------------------------------------------
 
FILE* abre_arquivo_dados() {
    FILE* f = fopen(ARQ_DADOS, "r+b");
    if (f == NULL) {
        // arquivo ainda nao existe -> cria com cabecalho vazio
        f = fopen(ARQ_DADOS, "w+b");
        Cabecalho cab;
        cab.primeiro_livre = -1;
        fwrite(&cab, sizeof(Cabecalho), 1, f);
    }
    return f;
}
 
Cabecalho le_cabecalho(FILE* f) {
    Cabecalho cab;
    fseek(f, 0, SEEK_SET);
    fread(&cab, sizeof(Cabecalho), 1, f);
    return cab;
}
 
void grava_cabecalho(FILE* f, Cabecalho cab) {
    fseek(f, 0, SEEK_SET);
    fwrite(&cab, sizeof(Cabecalho), 1, f);
}
 
// monta "codigo#nome#seguradora#tipo#" e devolve o tamanho em bytes
int monta_payload(char* payload, const char* codigo, const char* nome,
                   const char* seguradora, const char* tipo) {
    return sprintf(payload, "%s%c%s%c%s%c%s%c",
                    codigo, DELIM, nome, DELIM, seguradora, DELIM, tipo, DELIM);
}
 
// ---------------------------------------------------------------
// 1) Insercao (first-fit na lista de espaco livre)
// ---------------------------------------------------------------
 
void insere_segurado(FILE* f, const char* codigo, const char* nome,
                      const char* seguradora, const char* tipo) {
    char payload[TAM_PAYLOAD];
    int tam_necessario = monta_payload(payload, codigo, nome, seguradora, tipo);
 
    Cabecalho cab = le_cabecalho(f);
    int anterior = -1;
    int atual = cab.primeiro_livre;
    int achou = 0;
 
    while (atual != -1) {
        fseek(f, atual, SEEK_SET);
        int tam_bloco;
        fread(&tam_bloco, sizeof(int), 1, f);
 
        char marca;
        int proximo;
        fread(&marca, sizeof(char), 1, f);
        fread(&proximo, sizeof(int), 1, f);
 
        if (tam_bloco >= tam_necessario) {
            // first-fit: usa esse bloco (fragmentacao interna aceita)
            // remove o bloco da lista de espacos livres
            if (anterior == -1) {
                cab.primeiro_livre = proximo;
                grava_cabecalho(f, cab);
            } else {
                fseek(f, anterior + (int)sizeof(int) + (int)sizeof(char), SEEK_SET);
                fwrite(&proximo, sizeof(int), 1, f);
            }
            // grava o registro (o campo de tamanho do bloco NAO muda)
            fseek(f, atual + (int)sizeof(int), SEEK_SET);
            fwrite(payload, sizeof(char), tam_necessario, f);
            achou = 1;
            break;
        }
 
        anterior = atual;
        atual = proximo;
    }
 
    if (!achou) {
        // nenhum espaco livre serviu -> acrescenta no final do arquivo
        fseek(f, 0, SEEK_END);
        fwrite(&tam_necessario, sizeof(int), 1, f);
        fwrite(payload, sizeof(char), tam_necessario, f);
    }
}
 
// ---------------------------------------------------------------
// 2) Remocao
// ---------------------------------------------------------------
 
// procura um registro ocupado pelo codigo; devolve 1 se achou e
// preenche offset/tamanho do bloco
int busca_offset_por_codigo(FILE* f, const char* codigo, long* offset_out, int* tam_out) {
    fseek(f, 0, SEEK_END);
    long fim = ftell(f);
    long pos = sizeof(Cabecalho);
 
    while (pos < fim) {
        fseek(f, pos, SEEK_SET);
        int tam;
        fread(&tam, sizeof(int), 1, f);
 
        char primeiro_byte;
        fread(&primeiro_byte, sizeof(char), 1, f);
 
        if (primeiro_byte != MARCA_LIVRE) {
            fseek(f, pos + (long)sizeof(int), SEEK_SET);
            char campo_codigo[TAM_CODIGO + 1];
            fread(campo_codigo, sizeof(char), TAM_CODIGO, f);
            campo_codigo[TAM_CODIGO] = '\0';
 
            if (strcmp(campo_codigo, codigo) == 0) {
                *offset_out = pos;
                *tam_out = tam;
                return 1;
            }
        }
 
        pos += sizeof(int) + tam;
    }
    return 0;
}
 
void remove_segurado(FILE* f, const char* codigo) {
    long offset;
    int tam;
    if (!busca_offset_por_codigo(f, codigo, &offset, &tam)) {
        printf("Segurado com codigo %s nao encontrado.\n", codigo);
        return;
    }
 
    Cabecalho cab = le_cabecalho(f);
    int antigo_primeiro = cab.primeiro_livre;
 
    // grava o "no" de espaco livre no lugar do registro removido
    fseek(f, offset + (long)sizeof(int), SEEK_SET);
    char marca = MARCA_LIVRE;
    fwrite(&marca, sizeof(char), 1, f);
    fwrite(&antigo_primeiro, sizeof(int), 1, f);
 
    // o novo espaco entra no INICIO da lista de livres
    cab.primeiro_livre = (int)offset;
    grava_cabecalho(f, cab);
 
    printf("Segurado %s removido.\n", codigo);
}
 
// ---------------------------------------------------------------
// 3) Compactacao
// ---------------------------------------------------------------
 
void compacta_arquivo() {
    FILE* f = fopen(ARQ_DADOS, "rb");
    if (f == NULL) {
        printf("Arquivo de dados ainda nao existe.\n");
        return;
    }
 
    const char* nome_temp = "seguradoras_temp.bin";
    FILE* temp = fopen(nome_temp, "w+b");
    Cabecalho cab_novo;
    cab_novo.primeiro_livre = -1;
    fwrite(&cab_novo, sizeof(Cabecalho), 1, temp);
 
    fseek(f, 0, SEEK_END);
    long fim = ftell(f);
    long pos = sizeof(Cabecalho);
 
    while (pos < fim) {
        fseek(f, pos, SEEK_SET);
        int tam;
        fread(&tam, sizeof(int), 1, f);
 
        char primeiro_byte;
        fread(&primeiro_byte, sizeof(char), 1, f);
 
        if (primeiro_byte != MARCA_LIVRE) {
            // volta ao inicio do payload e le ate o 4o delimitador,
            // eliminando tambem a fragmentacao interna
            fseek(f, pos + (long)sizeof(int), SEEK_SET);
            char payload[TAM_PAYLOAD];
            int idx = 0;
            int delimitadores = 0;
            char c;
            while (delimitadores < 4) {
                fread(&c, sizeof(char), 1, f);
                payload[idx++] = c;
                if (c == DELIM) delimitadores++;
            }
            fwrite(&idx, sizeof(int), 1, temp);
            fwrite(payload, sizeof(char), idx, temp);
        }
        // registros livres simplesmente nao sao copiados (fragmentacao
        // externa eliminada)
 
        pos += sizeof(int) + tam;
    }
 
    fclose(f);
    fclose(temp);
    remove(ARQ_DADOS);
    rename(nome_temp, ARQ_DADOS);
 
    printf("Arquivo compactado.\n");
}
 
// ---------------------------------------------------------------
// 4) Dump do arquivo
// ---------------------------------------------------------------
 
void dump(FILE* f) {
    Cabecalho cab = le_cabecalho(f);
    printf("--- Cabecalho: primeiro_livre = %d ---\n", cab.primeiro_livre);
 
    fseek(f, 0, SEEK_END);
    long fim = ftell(f);
    long pos = sizeof(Cabecalho);
 
    while (pos < fim) {
        fseek(f, pos, SEEK_SET);
        int tam;
        fread(&tam, sizeof(int), 1, f);
 
        char primeiro_byte;
        fread(&primeiro_byte, sizeof(char), 1, f);
 
        if (primeiro_byte == MARCA_LIVRE) {
            int prox;
            fread(&prox, sizeof(int), 1, f);
            printf("[LIVRE]    offset=%-6ld tam_bloco=%-4d proximo=%d\n", pos, tam, prox);
        } else {
            fseek(f, pos + (long)sizeof(int), SEEK_SET);
            char payload[TAM_PAYLOAD];
            int idx = 0;
            int delimitadores = 0;
            char c;
            while (delimitadores < 4 && idx < TAM_PAYLOAD - 1) {
                fread(&c, sizeof(char), 1, f);
                payload[idx++] = c;
                if (c == DELIM) delimitadores++;
            }
            payload[idx] = '\0';
            printf("[OCUPADO]  offset=%-6ld tam_bloco=%-4d dados=%s\n", pos, tam, payload);
        }
 
        pos += sizeof(int) + tam;
    }
}
 
// ---------------------------------------------------------------
// 5) Carrega insere.bin / remove.bin (+ controle de progresso)
// ---------------------------------------------------------------
 
EstadoUso le_estado() {
    EstadoUso e = {0, 0};
    FILE* f = fopen(ARQ_CTRL, "rb");
    if (f != NULL) {
        fread(&e, sizeof(EstadoUso), 1, f);
        fclose(f);
    }
    return e;
}
 
void grava_estado(EstadoUso e) {
    FILE* f = fopen(ARQ_CTRL, "wb");
    fwrite(&e, sizeof(EstadoUso), 1, f);
    fclose(f);
}
 
// le insere.bin como texto, uma linha por registro, campos com '#'
int carrega_insere(SeguradoTxt* vetor, int tam_max) {
    FILE* f = fopen(ARQ_INSERE, "rt");
    if (f == NULL) return 0;
 
    int n = 0;
    char linha[200];
    while (n < tam_max && fgets(linha, sizeof(linha), f) != NULL) {
        linha[strcspn(linha, "\r\n")] = '\0';
        if (strlen(linha) == 0) continue;
 
        char* tok = strtok(linha, "#");
        strcpy(vetor[n].codigo, tok ? tok : "");
        tok = strtok(NULL, "#");
        strcpy(vetor[n].nome, tok ? tok : "");
        tok = strtok(NULL, "#");
        strcpy(vetor[n].seguradora, tok ? tok : "");
        tok = strtok(NULL, "#");
        strcpy(vetor[n].tipo, tok ? tok : "");
        n++;
    }
    fclose(f);
    return n;
}
 
// le remove.bin como texto, um codigo por linha
int carrega_remove(char vetor[][TAM_CODIGO + 1], int tam_max) {
    FILE* f = fopen(ARQ_REMOVE, "rt");
    if (f == NULL) return 0;
 
    int n = 0;
    char linha[20];
    while (n < tam_max && fgets(linha, sizeof(linha), f) != NULL) {
        linha[strcspn(linha, "\r\n")] = '\0';
        if (strlen(linha) == 0) continue;
        strncpy(vetor[n], linha, TAM_CODIGO);
        vetor[n][TAM_CODIGO] = '\0';
        n++;
    }
    fclose(f);
    return n;
}
 
// ---------------------------------------------------------------
// main
// ---------------------------------------------------------------
 
int main() {
    FILE* f = abre_arquivo_dados();
 
    static SeguradoTxt vetor_insere[MAX_REGISTROS];
    static char vetor_remove[MAX_REGISTROS][TAM_CODIGO + 1];
    int total_insere = carrega_insere(vetor_insere, MAX_REGISTROS);
    int total_remove = carrega_remove(vetor_remove, MAX_REGISTROS);
 
    EstadoUso estado = le_estado();
 
    int opcao;
    do {
        printf("\n===== Cadastro de Seguradoras =====\n");
        printf("1 - Inserir proximo registro de insere.bin (%d/%d usados)\n", estado.usado_insere, total_insere);
        printf("2 - Remover proximo codigo de remove.bin (%d/%d usados)\n", estado.usado_remove, total_remove);
        printf("3 - Compactar arquivo\n");
        printf("4 - Dump do arquivo\n");
        printf("0 - Sair\n");
        printf("Opcao: ");
        if (scanf("%d", &opcao) != 1) break;
        getchar(); // limpa o \n deixado pelo scanf
 
        switch (opcao) {
            case 1: {
                if (estado.usado_insere < total_insere) {
                    SeguradoTxt* s = &vetor_insere[estado.usado_insere];
                    insere_segurado(f, s->codigo, s->nome, s->seguradora, s->tipo);
                    estado.usado_insere++;
                    grava_estado(estado);
                    printf("Inserido: %s - %s\n", s->codigo, s->nome);
                } else {
                    printf("Nao ha mais registros disponiveis em insere.bin.\n");
                }
                break;
            }
            case 2: {
                if (estado.usado_remove < total_remove) {
                    remove_segurado(f, vetor_remove[estado.usado_remove]);
                    estado.usado_remove++;
                    grava_estado(estado);
                } else {
                    printf("Nao ha mais codigos disponiveis em remove.bin.\n");
                }
                break;
            }
            case 3:
                fclose(f);
                compacta_arquivo();
                f = fopen(ARQ_DADOS, "r+b");
                break;
            case 4:
                dump(f);
                break;
            case 0:
                printf("Encerrando.\n");
                break;
            default:
                printf("Opcao invalida.\n");
        }
    } while (opcao != 0);
 
    fclose(f);
    return 0;
}