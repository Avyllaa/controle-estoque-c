#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#ifdef _WIN32
#include <windows.h>
#endif

static int configurarSaidaUTF8(void)
{
#ifdef _WIN32
    
    if (GetConsoleOutputCP() != 0 && !SetConsoleOutputCP(CP_UTF8)) {
        fputs("Erro ao configurar o console para UTF-8.\n", stderr);
        return 0;
    }
#endif
    return 1;
}

#define MAX_PRODUTOS 1000
#define TAM_NOME 100
#define TAM_CATEGORIA 60
#define TAM_LINHA 512
#define PRECO_MAXIMO 1000000000.0
#define ARQUIVO "produtos.txt"
#define TEMPORARIO "produtos.txt.tmp"
#define BACKUP "produtos.txt.bak"

struct Produto {
    int codigo;
    char nome[TAM_NOME];
    char categoria[TAM_CATEGORIA];
    int quantidade;
    double preco;
};


static int lerLinha(FILE *arquivo, char *linha, int tamanho)
{
    char *fim;
    int c;
    if (fgets(linha, tamanho, arquivo) == NULL)
        return ferror(arquivo) ? -2 : 0;
    fim = strchr(linha, '\n');
    if (fim != NULL) {
        *fim = '\0';
    } else {
        c = fgetc(arquivo);
        if (c != '\n' && c != EOF) {
            do {
                c = fgetc(arquivo);
            } while (c != '\n' && c != EOF);
            return ferror(arquivo) ? -2 : -1;
        }
        if (ferror(arquivo))
            return -2;
    }
    fim = strchr(linha, '\r');
    if (fim != NULL && fim[1] == '\0')
        *fim = '\0';
    return 1;
}

static int converterInteiro(const char *texto, int minimo, int *valor)
{
    char *fim;
    long numero;
    errno = 0;
    numero = strtol(texto, &fim, 10);
    if (fim == texto || errno == ERANGE)
        return 0;
    while (isspace((unsigned char)*fim))
        fim++;
    if (*fim != '\0' || numero < minimo || numero > INT_MAX)
        return 0;
    *valor = (int)numero;
    return 1;
}

static int converterPreco(const char *texto, double *valor)
{
    char *fim;
    double numero;
    errno = 0;
    numero = strtod(texto, &fim);
    if (fim == texto || errno == ERANGE)
        return 0;
    while (isspace((unsigned char)*fim))
        fim++;
    if (*fim != '\0' || numero != numero || numero < 0.0 ||
        numero > PRECO_MAXIMO)
        return 0;
    *valor = numero;
    return 1;
}

static int textoValido(const char *texto, size_t limite)
{
    size_t i;
    int temConteudo;
    if (strlen(texto) == 0 || strlen(texto) >= limite)
        return 0;
    temConteudo = 0;
    for (i = 0; texto[i] != '\0'; i++) {
        if (texto[i] == ';' || iscntrl((unsigned char)texto[i]))
            return 0;
        if (!isspace((unsigned char)texto[i]))
            temConteudo = 1;
    }
    return temConteudo;
}

static int solicitarLinha(const char *mensagem, char *linha)
{
    int resultado;
    for (;;) {
        printf("%s", mensagem);
        fflush(stdout);
        resultado = lerLinha(stdin, linha, TAM_LINHA);
        if (resultado == 1)
            return 1;
        if (resultado == -1) {
            puts("Entrada muito longa. Tente novamente.");
        } else {
            if (resultado == -2)
                puts("Erro na leitura do teclado.");
            return 0;
        }
    }
}

static int lerInteiro(const char *mensagem, int minimo, int *valor)
{
    char linha[TAM_LINHA];
    while (solicitarLinha(mensagem, linha)) {
        if (converterInteiro(linha, minimo, valor))
            return 1;
        printf("Informe um inteiro de %d a %d.\n", minimo, INT_MAX);
    }
    return 0;
}

static int lerTexto(const char *mensagem, char *destino, size_t limite)
{
    char linha[TAM_LINHA];
    while (solicitarLinha(mensagem, linha)) {
        if (textoValido(linha, limite)) {
            strcpy(destino, linha);
            return 1;
        }
        printf("Use de 1 a %lu caracteres, com conteudo e sem ';' ou controles.\n",
               (unsigned long)(limite - 1));
    }
    return 0;
}

static int lerPreco(double *valor)
{
    char linha[TAM_LINHA];
    while (solicitarLinha("Preço unitario (use ponto decimal): ", linha)) {
        if (converterPreco(linha, valor))
            return 1;
        puts("Preço invalido. Informe um valor de 0 a 1000000000.");
    }
    return 0;
}

static int buscarIndicePorCodigo(const struct Produto produtos[],
                                 int total, int codigo)
{
    int i;
    for (i = 0; i < total; i++) {
        if (produtos[i].codigo == codigo)
            return i;
    }
    return -1;
}


static int existeArquivo(const char *caminho)
{
    FILE *arquivo;
    errno = 0;
    arquivo = fopen(caminho, "r");
    if (arquivo == NULL)
        return errno == ENOENT ? 0 : -1;
    if (fclose(arquivo) != 0)
        return -1;
    return 1;
}

static int interpretarRegistro(char *linha, struct Produto *produto)
{
    char *campos[5];
    char *separador;
    int i;
    campos[0] = linha;
    for (i = 0; i < 4; i++) {
        separador = strchr(campos[i], ';');
        if (separador == NULL)
            return 0;
        *separador = '\0';
        campos[i + 1] = separador + 1;
    }
    if (strchr(campos[4], ';') != NULL ||
        !converterInteiro(campos[0], 1, &produto->codigo) ||
        !textoValido(campos[1], TAM_NOME) ||
        !textoValido(campos[2], TAM_CATEGORIA) ||
        !converterInteiro(campos[3], 0, &produto->quantidade) ||
        !converterPreco(campos[4], &produto->preco))
        return 0;
    strcpy(produto->nome, campos[1]);
    strcpy(produto->categoria, campos[2]);
    return 1;
}

static int carregarProdutos(struct Produto produtos[], int *total)
{
    FILE *arquivo;
    char linha[TAM_LINHA];
    struct Produto produto;
    int resultado;
    int numeroLinha;
    int valido;
    *total = 0;
    if (existeArquivo(BACKUP) != 0) {
        puts("Backup pendente ou inacessivel. Consulte a recuperacao no README.md.");
        return 0;
    }
    errno = 0;
    arquivo = fopen(ARQUIVO, "r");
    if (arquivo == NULL) {
        if (errno == ENOENT)
            return 1;
        perror("Falha ao abrir produtos.txt");
        return 0;
    }
    numeroLinha = 0;
    valido = 1;
    while ((resultado = lerLinha(arquivo, linha, TAM_LINHA)) != 0) {
        numeroLinha++;
        if (resultado != 1 || *total >= MAX_PRODUTOS ||
            !interpretarRegistro(linha, &produto) ||
            buscarIndicePorCodigo(produtos, *total, produto.codigo) >= 0) {
            printf("Erro de leitura ou registro invalido na linha %d.\n", numeroLinha);
            valido = 0;
            break;
        }
        produtos[*total] = produto;
        (*total)++;
    }
    if (fclose(arquivo) != 0) {
        puts("Erro ao fechar o arquivo de leitura.");
        valido = 0;
    }
    return valido;
}

static int salvarProdutos(const struct Produto produtos[], int total)
{
    FILE *arquivo;
    int i;
    int sucesso;
    int tinhaOriginal;
    if (existeArquivo(TEMPORARIO) != 0 || existeArquivo(BACKUP) != 0) {
        puts("Temporario ou backup pendente/inacessivel. Consulte o README.md.");
        return 0;
    }
    tinhaOriginal = existeArquivo(ARQUIVO);
    if (tinhaOriginal < 0) {
        puts("Nao foi possivel verificar o arquivo original.");
        return 0;
    }
    arquivo = fopen(TEMPORARIO, "w");
    if (arquivo == NULL) {
        perror("Falha ao criar arquivo temporario");
        return 0;
    }
    sucesso = 1;
    for (i = 0; i < total; i++) {
        if (fprintf(arquivo, "%d;%s;%s;%d;%.17g\n", produtos[i].codigo,
                    produtos[i].nome, produtos[i].categoria,
                    produtos[i].quantidade, produtos[i].preco) < 0) {
            sucesso = 0;
            break;
        }
    }
    if (fflush(arquivo) != 0 || ferror(arquivo))
        sucesso = 0;
    if (fclose(arquivo) != 0)
        sucesso = 0;
    if (!sucesso) {
        puts("Erro de gravacao. O arquivo original foi preservado.");
        if (remove(TEMPORARIO) != 0)
            puts("Nao foi possivel remover o temporario incompleto.");
        return 0;
    }
    if (tinhaOriginal && rename(ARQUIVO, BACKUP) != 0) {
        perror("Nao foi possivel reservar o original como backup");
        if (remove(TEMPORARIO) != 0)
            puts("O temporario foi mantido para inspecao.");
        return 0;
    }
    if (rename(TEMPORARIO, ARQUIVO) != 0) {
        perror("Nao foi possivel instalar o arquivo completo");
        if (tinhaOriginal && rename(BACKUP, ARQUIVO) != 0)
            puts("Recuperacao automatica falhou: original preservado em produtos.txt.bak.");
        puts("Temporario preservado. Consulte o README.md antes de continuar.");
        return 0;
    }
    if (tinhaOriginal && remove(BACKUP) != 0)
        puts("Dados salvos, mas o backup permaneceu. Consulte o README.md.");
    return 1;
}

static void exibirProduto(const struct Produto *produto)
{
    printf("Código: %d | Nome: %s | Categoria: %s | Quantidade: %d | Preço: R$ %.2f\n",
           produto->codigo, produto->nome, produto->categoria,
           produto->quantidade, produto->preco);
}

static void cadastrarProduto(struct Produto produtos[], int *total)
{
    struct Produto produto;
    if (*total >= MAX_PRODUTOS) {
        puts("Limite de produtos atingido.");
        return;
    }
    if (!lerInteiro("Código: ", 1, &produto.codigo))
        return;
    if (buscarIndicePorCodigo(produtos, *total, produto.codigo) >= 0) {
        puts("Código ja cadastrado.");
        return;
    }
    if (!lerTexto("Nome: ", produto.nome, TAM_NOME) ||
        !lerTexto("Categoria: ", produto.categoria, TAM_CATEGORIA) ||
        !lerInteiro("Quantidade: ", 0, &produto.quantidade) ||
        !lerPreco(&produto.preco))
        return;
    putchar('\n');
    produtos[*total] = produto;
    if (salvarProdutos(produtos, *total + 1)) {
        (*total)++;
        puts("Produto cadastrado e salvo.");
    } else {
        puts("Cadastro nao confirmado.");
    }
}

static void listarProdutos(const struct Produto produtos[], int total)
{
    int i;
    if (total == 0)
        puts("Estoque vazio.");
    for (i = 0; i < total; i++)
        exibirProduto(&produtos[i]);
}

static void buscarProduto(const struct Produto produtos[], int total)
{
    int codigo;
    int indice;
    if (!lerInteiro("Código para busca: ", 1, &codigo))
        return;
    indice = buscarIndicePorCodigo(produtos, total, codigo);
    if (indice < 0)
        puts("Produto nao encontrado.");
    else
        exibirProduto(&produtos[indice]);
}

static double calcularValorEstoque(const struct Produto produtos[], int total)
{
    double valor;
    int i;
    valor = 0.0;
    for (i = 0; i < total; i++)
        valor += produtos[i].quantidade * produtos[i].preco;
    return valor;
}

static void atualizarQuantidade(struct Produto produtos[], int total)
{
    int codigo;
    int indice;
    int novaQuantidade;
    int anterior;
    if (!lerInteiro("Código para atualizar: ", 1, &codigo))
        return;
    indice = buscarIndicePorCodigo(produtos, total, codigo);
    if (indice < 0) {
        puts("Produto nao encontrado.");
        return;
    }
    exibirProduto(&produtos[indice]);
    if (!lerInteiro("Nova quantidade (substitui a atual): ", 0, &novaQuantidade))
        return;
    putchar('\n');
    anterior = produtos[indice].quantidade;
    produtos[indice].quantidade = novaQuantidade;
    if (salvarProdutos(produtos, total)) {
        puts("Quantidade atualizada e salva.");
    } else {
        produtos[indice].quantidade = anterior;
        puts("Atualizacao nao confirmada; quantidade anterior mantida na memoria.");
    }
}

static void exibirMenu(void)
{
    puts("\n\n===== CONTROLE DE ESTOQUE =====\n");
    puts("1 - Cadastrar produto");
    puts("2 - Listar produtos");
    puts("3 - Buscar produto");
    puts("4 - Calcular valor do estoque");
    puts("5 - Sair");
    puts("6 - Atualizar quantidade\n");
}

int main(int argc, char *argv[])
{
    static struct Produto produtos[MAX_PRODUTOS];
    int total;
    int opcao;
#ifdef _WIN32
    UINT paginaAnterior;
    paginaAnterior = GetConsoleOutputCP();
#endif
    if (!configurarSaidaUTF8())
        return EXIT_FAILURE;
    
    if (argc == 2 && strcmp(argv[1], "--testar-acentos") == 0) {
#ifdef _WIN32
        printf("Pagina de saida antes: %u; durante: %u (0 = sem console)\n",
               (unsigned int)paginaAnterior, (unsigned int)GetConsoleOutputCP());
#endif
        puts("Opção\nCódigo\nPreço\nAtualização");
        return fflush(stdout) == 0 && !ferror(stdout) ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    if (!carregarProdutos(produtos, &total)) {
        puts("Inicializacao interrompida. Nenhum dado sera sobrescrito.");
        return EXIT_FAILURE;
    }
    for (;;) {
        exibirMenu();
        if (!lerInteiro("Escolha uma opção:\n", 0, &opcao))
            return ferror(stdin) ? EXIT_FAILURE : EXIT_SUCCESS;
        putchar('\n');
        switch (opcao) {
            case 1: cadastrarProduto(produtos, &total); break;
            case 2: listarProdutos(produtos, total); break;
            case 3: buscarProduto(produtos, total); break;
            case 4:
                printf("Valor total do estoque: R$ %.2f\n",
                       calcularValorEstoque(produtos, total));
                break;
            case 5: puts("Programa encerrado."); return EXIT_SUCCESS;
            case 6: atualizarQuantidade(produtos, total); break;
            default: puts("Opção inválida. Escolha de 1 a 6."); break;
        }
    }
}
