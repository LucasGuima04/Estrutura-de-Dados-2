#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <stdint.h>

#define ARQUIVO_DADOS "catalogo.bin"
#define ARQUIVO_INSERE "insere.bin"
#define ARQUIVO_REMOVE "remove.bin"

#define TAM_CODIGO 4
#define TAM_NOME 60
#define TAM_ARTISTA 50
#define TAM_GENERO 20

#define HEADER_SIZE sizeof(int)
#define OFFSET_NULO (-1)

//Estrutura
   
typedef struct {
    char codigo[TAM_CODIGO + 1];
    char nome[TAM_NOME + 1];
    char artista[TAM_ARTISTA + 1];
    char genero[TAM_GENERO + 1];
} Musica;

//Espaços livres

typedef struct {
    int offset;
    int tamanho;
    int prox;
} EspacoLivre;


//Funções auxiliares

int tamanhoRegistro(Musica *m){
    int tamanhoDados;

    tamanhoDados = strlen(m->codigo) + 1 + strlen(m->nome) + 1 + strlen(m->artista) + 1 + strlen(m->genero) + 1; 

    return sizeof(int) + tamanhoDados;
}

//Monta a parte textual do registro.

char *montarRegistro(Musica *m, int *tamanhoDados){
    char *buffer;
    *tamanhoDados = strlen(m->codigo) + 1 + strlen(m->nome) + 1 + strlen(m->artista) + 1 + strlen(m->genero) + 1;
    
    buffer = (char *) malloc(*tamanhoDados);
    if (buffer == NULL) {
        printf("Erro de memoria.\n");
        exit(1);
    }

    snprintf(buffer,*tamanhoDados,"%s|%s|%s|%s",m->codigo,m->nome,m->artista,m->genero);

    return buffer;
}


void inicializarArquivo(){
    FILE *fp;
    int firstFree = OFFSET_NULO;

    fp = fopen(ARQUIVO_DADOS, "rb");

    if (fp != NULL) {
        fclose(fp);
        return;
    }

    fp = fopen(ARQUIVO_DADOS, "wb");

    if (fp == NULL) {
        printf("Erro ao criar arquivo.\n");
        exit(1);
    }

    fwrite(&firstFree, sizeof(int), 1, fp);

    fclose(fp);

    printf("Arquivo '%s' criado.\n", ARQUIVO_DADOS);
}


int lerHeader(FILE *fp){
    int primeiro;

    fseek(fp, 0, SEEK_SET);

    if (fread(&primeiro, sizeof(int), 1, fp) != 1) {
        return OFFSET_NULO;
    }

    return primeiro;
}

//Atualiza o offset do primeiro espaco livre.

void atualizarHeader(FILE *fp, int offset){
    fseek(fp, 0, SEEK_SET);
    fwrite(&offset, sizeof(int), 1, fp);
    fflush(fp);
}

//Retorna o tamanho do arquivo.

int tamanhoArquivo(FILE *fp){
    long atual;
    long fim;

    atual = ftell(fp);

    fseek(fp, 0, SEEK_END);
    fim = ftell(fp);

    fseek(fp, atual, SEEK_SET);

    return (int) fim;
}

//Ler espaços livres

int lerEspacoLivre(FILE *fp, int offset, EspacoLivre *espaco){
    char marcador;
    int tamanho;
    int prox;

    if (offset < HEADER_SIZE)
        return 0;

    fseek(fp, offset, SEEK_SET);

    if (fread(&tamanho, sizeof(int), 1, fp) != 1)
        return 0;

    if (fread(&marcador, sizeof(char), 1, fp) != 1)
        return 0;

    if (marcador != '@')
        return 0;

    if (fread(&prox, sizeof(int), 1, fp) != 1)
        return 0;

    espaco->offset = offset;
    espaco->tamanho = tamanho;
    espaco->prox = prox;

    return 1;
}

//Encontrar melhor espaço

int encontrarBestFit(FILE *fp, int tamanhoNecessario, EspacoLivre *melhor, int *offsetAnterior){
    int atual;
    int anterior = OFFSET_NULO;

    int encontrou = 0;

    EspacoLivre espaco;

    atual = lerHeader(fp);

    while (atual != OFFSET_NULO) {

        if (!lerEspacoLivre(fp, atual, &espaco)) {
            printf("Erro ao percorrer lista de espacos livres.\n");
            return 0;
        }

        if (espaco.tamanho >= tamanhoNecessario) {
            if (!encontrou || espaco.tamanho < melhor->tamanho) {

                *melhor = espaco;
                *offsetAnterior = anterior;
                encontrou = 1;
            }
        }
        anterior = atual;
        atual = espaco.prox;
    }

    return encontrou;
}


void removerDaListaLivre(
    FILE *fp,
    EspacoLivre *espaco,
    int offsetAnterior
)
{
    if (offsetAnterior == OFFSET_NULO) {
        /* Era o primeiro elemento */
        atualizarHeader(fp, espaco->prox);
    } else {
        /* Atualiza o ponteiro do anterior */
        fseek(fp, offsetAnterior + sizeof(int) + 1, SEEK_SET);
        fwrite(&espaco->prox,sizeof(int),1,fp);
        fflush(fp);
    }
}


void adicionarListaLivre(FILE *fp,int offset,int tamanho){
    int primeiro;
    int atual;

    int proximo;

    primeiro = lerHeader(fp);

    if (primeiro == OFFSET_NULO) {

        fseek(fp, offset, SEEK_SET);
        fwrite(&tamanho, sizeof(int), 1, fp);
        fputc('@', fp);
        proximo = OFFSET_NULO;
        fwrite(&proximo, sizeof(int), 1, fp);
        fflush(fp);
        atualizarHeader(fp, offset);
        return;
    }

    //Percorre ate o ultimo elemento.
    atual = primeiro;

    while (1) {

        EspacoLivre espaco;

        if (!lerEspacoLivre(fp, atual, &espaco)) {
            printf("Erro na lista de espacos livres.\n");
            return;
        }

        if (espaco.prox == OFFSET_NULO)
            break;

        atual = espaco.prox;
    }

    //Atualiza o ultimo elemento para apontar para o novo espaco.
    fseek(fp, atual + sizeof(int) + 1, SEEK_SET);

    fwrite(&offset,sizeof(int),1,fp);

    //Grava o novo espaco.
    fseek(fp, offset, SEEK_SET);

    fwrite(&tamanho, sizeof(int), 1, fp);
    fputc('@', fp);
    proximo = OFFSET_NULO;
    fwrite(&proximo,sizeof(int),1,fp);
    fflush(fp);
}


/* =========================================================
                         INSERCAO
   ========================================================= */

/*
    Escreve uma musica dentro de um determinado offset.

    Se o espaco livre for maior que o registro, fazemos
    fragmentacao interna: o restante continua fazendo parte
    daquele espaco.

    Isso e permitido pelo enunciado.
*/
void escreverMusicaNoOffset(FILE *fp,Musica *m,int offset,int tamanhoEspaco)
{
    char *registro;
    int tamanhoDados;
    int tamanhoRegistroAtual;

    registro = montarRegistro(m, &tamanhoDados);

    tamanhoRegistroAtual =
        sizeof(int) + tamanhoDados;

    fseek(fp, offset, SEEK_SET);

    //Primeiro o tamanho do registro.
    fwrite(&tamanhoRegistroAtual,sizeof(int),1,fp);

    //Depois os dados.
    fwrite(registro,tamanhoDados,1,fp);

    //Preenche o restante do espaco com zeros.
    if (tamanhoEspaco > tamanhoRegistroAtual) {

        int restante =tamanhoEspaco - tamanhoRegistroAtual;
        char zero = 0;

        while (restante > 0) {
            fwrite(&zero, 1, 1, fp);
            restante--;
        }
    }

    fflush(fp);
    free(registro);
}


/*
    Insercao completa.

    1. Calcula tamanho.
    2. Procura best-fit.
    3. Se encontrar, reutiliza.
    4. Caso contrario, grava no final.
*/
void inserirMusica(Musica *m)
{
    FILE *fp;

    int tamanhoNecessario;

    EspacoLivre melhor;
    int anterior;

    fp = fopen(ARQUIVO_DADOS, "r+b");

    if (fp == NULL) {
        printf("Erro ao abrir arquivo.\n");
        return;
    }

    tamanhoNecessario = tamanhoRegistro(m);

    //Procura o melhor espaco.
    if (encontrarBestFit(fp,tamanhoNecessario,&melhor,&anterior)) {

        printf("Reutilizando espaco no offset %d (tamanho %d).\n",melhor.offset,melhor.tamanho);

        //Remove o espaco da lista livre.
        removerDaListaLivre(fp,&melhor,anterior);

        //Grava o novo registro.
        escreverMusicaNoOffset(fp,m,melhor.offset,melhor.tamanho);

    } else {

        //Nenhum espaco serviu, vai para o final do arquivo.
        int offsetFinal;
        offsetFinal = tamanhoArquivo(fp);
        printf("Nenhum espaco adequado. Inserindo no final, offset %d.\n",offsetFinal);
        escreverMusicaNoOffset(fp,m,offsetFinal,tamanhoNecessario);
    }

    fclose(fp);
}


//BUSCA

int registroRemovido(FILE *fp, int offset)
{
    char marcador;

    fseek(fp,offset + sizeof(int),SEEK_SET);

    if (fread(&marcador, sizeof(char), 1, fp) != 1)
        return 0;

    return marcador == '@';
}

//Buscar música pelo codigo
int buscarCodigo(
    FILE *fp,
    const char *codigo,
    int *offsetEncontrado,
    int *tamanhoEncontrado
)
{
    int offset;
    int tamanho;
    char *buffer;

    offset = HEADER_SIZE;

    while (offset < tamanhoArquivo(fp)) {

        fseek(fp, offset, SEEK_SET);

        if (fread(&tamanho, sizeof(int), 1, fp) != 1)
            break;

        if (tamanho <= 0)
            break;

        //Se for espaco removido, pula.
        if (registroRemovido(fp, offset)) {

            offset += tamanho;
            continue;
        }

        //Le a parte textual.
        buffer = (char *) malloc(tamanho - sizeof(int));

        if (buffer == NULL) {
            printf("Erro de memoria.\n");
            return 0;
        }

        fseek(fp,offset + sizeof(int),SEEK_SET);
        fread(buffer,tamanho - sizeof(int),1,fp);

        //Garante terminacao.
        buffer[tamanho - sizeof(int) - 1] = '\0';

        //O codigo esta antes do primeiro '|'.
        {
            char codigoEncontrado[TAM_CODIGO + 1];

            int i = 0;

            while (buffer[i] != '|' && buffer[i] != '\0' && i < TAM_CODIGO) {
                codigoEncontrado[i] = buffer[i];
                i++;
            }
            codigoEncontrado[i] = '\0';

            if (strcmp(codigoEncontrado, codigo) == 0) {

                *offsetEncontrado = offset;
                *tamanhoEncontrado = tamanho;

                free(buffer);

                return 1;
            }
        }

        free(buffer);

        offset += tamanho;
    }

    return 0;
}

//Remoção
void removerMusica(const char *codigo)
{
    FILE *fp;

    int offset;
    int tamanho;

    fp = fopen(ARQUIVO_DADOS, "r+b");

    if (fp == NULL) {
        printf("Erro ao abrir arquivo.\n");
        return;
    }

    if (!buscarCodigo(fp,codigo,&offset,&tamanho)) {
        printf("Codigo %s nao encontrado.\n",codigo);
        fclose(fp);
        return;
    }

    printf("Registro encontrado no offset %d.\n",offset);

    //Adiciona o espaco no FINAL da lista.
    adicionarListaLivre(fp,offset,tamanho);
    fclose(fp);
    printf("Registro %s removido com sucesso.\n",codigo);
}


void dumpArquivo(){
    FILE *fp;

    int offset;
    int tamanho;

    fp = fopen(ARQUIVO_DADOS, "rb");

    if (fp == NULL) {
        printf("Arquivo inexistente.\n");
        return;
    }

    printf("\n");
    printf("========================================\n");
    printf("              DUMP ARQUIVO\n");
    printf("========================================\n");

    {
        int primeiro = lerHeader(fp);

        printf(
            "HEADER -> primeiro espaco livre: %d\n",
            primeiro
        );
    }

    printf("----------------------------------------\n");

    offset = HEADER_SIZE;

    while (offset < tamanhoArquivo(fp)) {

        fseek(fp, offset, SEEK_SET);

        if (fread(&tamanho, sizeof(int), 1, fp) != 1)
            break;

        if (tamanho <= 0)
            break;

        printf("Offset: %d | Tamanho: %d | ",offset,tamanho);

        //Registro removido.
        if (registroRemovido(fp, offset)) {

            int prox;
            char marcador;

            fseek(fp,offset + sizeof(int),SEEK_SET);
            fread(&marcador,sizeof(char),1,fp);
            fread(&prox,sizeof(int),1,fp);
            printf("LIVRE | %c | Proximo: %d\n",marcador,prox);

        } else {

            char *buffer;
            buffer = malloc(tamanho - sizeof(int));

            if (buffer == NULL) {
                fclose(fp);
                return;
            }

            fseek(fp,offset + sizeof(int),SEEK_SET);

            fread(buffer, tamanho - sizeof(int), 1, fp);
 
            buffer[tamanho - sizeof(int) - 1] = '\0';
 
            printf("OCUPADO | %s\n", buffer);
 
            free(buffer);
        }
 
        offset += tamanho;
    }
 
    printf("========================================\n");
 
    fclose(fp);
}
 
 
/* =========================================================
                  COMPACTACAO
   ========================================================= */
 
/*
    Compacta o arquivo.
 
    A ideia e:
 
        arquivo antigo
              |
              v
        le registro por registro
              |
              v
        copia somente os ocupados
              |
              v
        novo arquivo temporario
              |
              v
        substitui o arquivo antigo
 
    Depois da compactacao:
 
        HEADER -> -1
 
    pois nao existem mais espacos livres.
*/
void compactarArquivo()
{
    FILE *origem;
    FILE *destino;
 
    int offset;
    int tamanho;
 
    origem = fopen(ARQUIVO_DADOS, "rb");
 
    if (origem == NULL) {
        printf("Arquivo inexistente.\n");
        return;
    }
 
    destino = fopen("catalogo_temp.bin", "wb");
 
    if (destino == NULL) {
        printf("Erro ao criar arquivo temporario.\n");
        fclose(origem);
        return;
    }
 
    /*
        Novo arquivo inicialmente nao possui
        espacos livres.
    */
    {
        int header = OFFSET_NULO;
 
        fwrite(&header, sizeof(int), 1, destino);
    }
 
    offset = HEADER_SIZE;
 
    while (offset < tamanhoArquivo(origem)) {
 
        fseek(origem, offset, SEEK_SET);
 
        if (fread(&tamanho, sizeof(int), 1, origem) != 1)
            break;
 
        if (tamanho <= 0)
            break;
 
        /*
            Se nao estiver removido, copia.
        */
        if (!registroRemovido(origem, offset)) {
 
            char *buffer;
 
            buffer = malloc(tamanho);
 
            if (buffer == NULL) {
                fclose(origem);
                fclose(destino);
                return;
            }
 
            fseek(origem, offset, SEEK_SET);
 
            fread(buffer, tamanho, 1, origem);
 
            fwrite(buffer, tamanho, 1, destino);
 
            free(buffer);
        }
 
        offset += tamanho;
    }
 
    fclose(origem);
    fclose(destino);
 
    /*
        Substitui o arquivo antigo.
    */
    remove(ARQUIVO_DADOS);
 
    if (rename("catalogo_temp.bin", ARQUIVO_DADOS) != 0) {
 
        printf("Erro ao substituir arquivo.\n");
 
        return;
    }
 
    printf("Arquivo compactado com sucesso.\n");
}
 
 
/* =========================================================
                  LEITURA PELO TECLADO
   ========================================================= */
 
void lerMusica(Musica *m)
{
    printf("Codigo da faixa: ");
    scanf("%4s", m->codigo);
 
    getchar();
 
    printf("Nome da faixa: ");
    fgets(m->nome, sizeof(m->nome), stdin);
    m->nome[strcspn(m->nome, "\n")] = '\0';
 
    printf("Artista: ");
    fgets(m->artista, sizeof(m->artista), stdin);
    m->artista[strcspn(m->artista, "\n")] = '\0';
 
    printf("Genero musical: ");
    fgets(m->genero, sizeof(m->genero), stdin);
    m->genero[strcspn(m->genero, "\n")] = '\0';
}
 
 
/* =========================================================
                  CARREGAR INSERE.BIN
   ========================================================= */
 
/*
    Para facilitar os testes, este programa considera
    insere.bin como um arquivo contendo registros no mesmo
    formato textual:
 
        codigo|nome|artista|genero\0
 
    um registro apos o outro.
 
    Caso o professor forneca insere.bin em outro formato,
    somente esta funcao precisa ser adaptada.
*/
int lerRegistroInsere(FILE *fp, Musica *m)
{
    char linha[256];
 
    if (fgets(linha, sizeof(linha), fp) == NULL)
        return 0;
 
    linha[strcspn(linha, "\r\n")] = '\0';
 
    if (sscanf(linha, "%4[^|]|%60[^|]|%50[^|]|%20[^\n]", m->codigo, m->nome, m->artista, m->genero) != 4) {
        return 0;
    }
 
    return 1;
}
 
 
/*
    Carrega todas as musicas do insere.bin e insere.
*/
void carregarInsere()
{
    FILE *fp;
 
    Musica m;
 
    fp = fopen(ARQUIVO_INSERE, "r");
 
    if (fp == NULL) {
        printf("Nao foi possivel abrir %s.\n", ARQUIVO_INSERE);
        return;
    }
 
    printf("\nCarregando registros de %s...\n", ARQUIVO_INSERE);
 
    while (lerRegistroInsere(fp, &m)) {
 
        printf("Inserindo %s - %s\n", m.codigo, m.nome);
 
        inserirMusica(&m);
    }
 
    fclose(fp);
 
    printf("Carregamento finalizado.\n");
}
 
 
/* =========================================================
                  CARREGAR REMOVE.BIN
   ========================================================= */
 
/*
    Considera remove.bin como um arquivo texto contendo
    um codigo por linha.
*/
void carregarRemove()
{
    FILE *fp;
 
    char codigo[TAM_CODIGO + 1];
 
    fp = fopen(ARQUIVO_REMOVE, "r");
 
    if (fp == NULL) {
        printf("Nao foi possivel abrir %s.\n", ARQUIVO_REMOVE);
        return;
    }
 
    printf("\nExecutando remocoes de %s...\n", ARQUIVO_REMOVE);
 
    while (fgets(codigo, sizeof(codigo), fp) != NULL) {
 
        codigo[strcspn(codigo, "\r\n")] = '\0';
 
        if (strlen(codigo) == 0)
            continue;
 
        printf("\nRemovendo codigo: %s\n", codigo);
 
        removerMusica(codigo);
    }
 
    fclose(fp);
 
    printf("\nRemocoes finalizadas.\n");
}
 
 
/* =========================================================
                         MENU
   ========================================================= */
 
void menu()
{
    printf("\n");
    printf("========================================\n");
    printf("       CATALOGO MUSICAL - STREAMING\n");
    printf("========================================\n");
    printf("1 - Insercao\n");
    printf("2 - Remocao\n");
    printf("3 - Compactacao\n");
    printf("4 - Dump arquivo\n");
    printf("5 - Carrega insere.bin\n");
    printf("6 - Carrega remove.bin\n");
    printf("0 - Sair\n");
    printf("========================================\n");
    printf("Opcao: ");
}
 
 
/* =========================================================
                         MAIN
   ========================================================= */
 
int main()
{
    int opcao;
 
    /*
        IMPORTANTE:
 
        O arquivo somente e criado caso ainda nao exista.
        Isso atende a observacao do exercicio.
    */
    inicializarArquivo();
 
    do {
 
        menu();
 
        scanf("%d", &opcao);
 
        switch (opcao) {
 
            case 1:
            {
                Musica m;
 
                printf("\n--- INSERCAO ---\n");
 
                lerMusica(&m);
 
                inserirMusica(&m);
 
                break;
            }
 
            case 2:
            {
                char codigo[TAM_CODIGO + 1];
 
                printf("\n--- REMOCAO ---\n");
 
                printf("Codigo da faixa: ");
 
                scanf("%4s", codigo);
 
                removerMusica(codigo);
 
                break;
            }
 
            case 3:
 
                printf("\n--- COMPACTACAO ---\n");
 
                compactarArquivo();
 
                break;
 
            case 4:
 
                dumpArquivo();
 
                break;
 
            case 5:
 
                carregarInsere();
 
                break;
 
            case 6:
 
                carregarRemove();
 
                break;
 
            case 0:
 
                printf("\nPrograma encerrado.\n");
 
                break;
 
            default:
 
                printf("\nOpcao invalida.\n");
        }
 
    } while (opcao != 0);
 
    return 0;
}
 