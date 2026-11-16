# Controle de estoque em C

Trabalho de Programacao de Software Basico, com codigo C90 e somente bibliotecas
padrao para o estoque, com API Windows apenas para configurar a saida do console.
Compativel com GCC TDM-MinGW 4.4.1 e sintaxe C90. O fonte esta em UTF-8 sem BOM.
O programa configura automaticamente a saida do console Windows para UTF-8.

## Compilar e executar

No terminal PowerShell do VS Code, dentro da pasta do projeto:

```powershell
gcc -std=c89 -Wall -Wextra -pedantic -finput-charset=UTF-8 -fexec-charset=UTF-8 estoque.c -o estoque.exe
.\estoque.exe
```

Os arquivos de dados ficam no diretorio de execucao. Execute sempre nessa pasta
para acessar o mesmo estoque. Nao sao necessarias bibliotecas externas.
Neste GCC 4.4.1, use `-std=c89` para o padrao ANSI C89/ISO C90: a opcao
`-std=c90` nao e reconhecida pela instalacao verificada.

As opcoes `-finput-charset` e `-fexec-charset` fixam UTF-8 na leitura do fonte e
nos textos do executavel. `configurarSaidaUTF8` usa `SetConsoleOutputCP(CP_UTF8)`
antes de qualquer mensagem, verifica o retorno e encerra com erro se a configuracao
falhar. Se nao ha console associado, preserva a saida UTF-8 por pipes/arquivos.
Nao faz conversao adicional dos textos nem chama `setlocale` ou `system`.
O bloco Windows e protegido por `#ifdef _WIN32`; nao exige biblioteca externa
nem recurso moderno de C. A pagina de entrada e as regras do estoque nao mudam.
Use uma fonte com suporte a portugues, como Consolas, no terminal do VS Code.
Nao e necessario executar `chcp` antes do programa. A configuracao do console vale
para a sessao atual, sem modificar configuracoes globais do Windows.
Feche uma instancia aberta pela opcao 5 antes de recompilar: o Windows bloqueia
a substituicao de um executavel que esta em uso.

## Teste de acentos sem acessar o estoque

```powershell
.\estoque.exe --testar-acentos
```

Esse modo informa a pagina de saida antes e durante a execucao e imprime
`Opção`, `Código`, `Preço` e `Atualização`. Ele retorna antes de carregar dados,
sem criar nem modificar `produtos.txt` ou auxiliares. Pagina zero no diagnostico
significa que nao ha console associado ao processo.

Para a verificacao automatizada opcional, com Python 3 instalado:

```powershell
python testes/testar_utf8.py
```

O teste valida os bytes UTF-8 redirecionados e abre consoles Windows isolados e
ocultos, iniciando em 437 e 65001. Executa o modo de diagnostico e le os caracteres
reais do buffer com `ReadConsoleOutputCharacterW`, verificando as quatro palavras
e a pagina 65001 durante a execucao. Python e usado somente pelo teste; nao e
dependencia do programa C. A verificacao do buffer nao inspeciona a fonte ou a
renderizacao visual especifica do terminal integrado do usuario.

### Diagnostico e resultados da correcao

- Antes da correcao, fonte e executavel ja continham `C3 A7` para cedilha e
  `C3 A3` para til; a saida redirecionada tambem era UTF-8 valido. Nao foi
  encontrado texto com conversao dupla nem alteracao de pagina dentro do programa.
- O processo de verificacao estava na pagina 437; o usuario informou 65001 no
  terminal integrado. A falha visual relatada nesse terminal nao foi reproduzida
  diretamente, portanto nao se pode atribui-la a bytes corrompidos ou ao GCC.
  A dependencia da pagina herdada foi removida configurando a saida no proprio C.
- Havia uma instancia de `estoque.exe` em uso e o linker retornou Permission
  denied. Apos o usuario encerrar normalmente, o executavel principal foi recompilado.
- GCC TDM-MinGW 4.4.1 aceitou ambas as opcoes de charset. Compilacao com
  `-std=c89 -Wall -Wextra -pedantic -Werror` e os dois charsets UTF-8: sem erros
  ou avisos. Os testes de bytes e de consoles reais 437 -> 65001 e 65001 -> 65001
  passaram para o executavel principal. Nenhum arquivo de dados foi criado.

## Menu

| Opcao | Operacao |
| --- | --- |
| 1 | Cadastrar |
| 2 | Listar |
| 3 | Buscar pelo codigo |
| 4 | Calcular valor do estoque |
| 5 | Sair |
| 6 | Atualizar quantidade (desafio adicional) |

A atualizacao substitui a quantidade atual; nao soma nem subtrai automaticamente.
O cadastro e a atualizacao so sao confirmados depois da gravacao. Sair ou encerrar
a entrada durante um cadastro cancela os campos ainda nao confirmados.

## Dados e validacoes

`struct Produto` contem `codigo`, `nome`, `categoria`, `quantidade` e `preco`.
O programa comporta ate 1000 produtos em um vetor estatico.

- Codigo: inteiro positivo, exclusivo, ate `INT_MAX`.
- Nome: de 1 a 99 caracteres; categoria: de 1 a 59 caracteres.
- Nome e categoria permitem espacos, mas precisam de conteudo alem de espacos.
  Nao permitem ponto e virgula ou caracteres de controle.
- Quantidade: inteiro de zero a `INT_MAX`.
- Preco: numero de zero a 1000000000, usando ponto decimal. Notacao cientifica
  tambem e aceita pela conversao padrao. Valores fora do intervalo, nao finitos,
  lixo depois do numero e erros de conversao sao recusados.
- Linhas de entrada excessivamente longas sao descartadas antes de nova leitura.
- Codigo duplicado e recusado tanto no cadastro quanto no carregamento.

`double` e usado para o preco, conforme a proposta didatica; pode haver
arredondamento binario. A exibicao usa duas casas decimais. O arquivo usa 17
digitos significativos para preservar a precisao nas reaberturas. O limite de
preco tambem mantem os calculos dentro do intervalo de `double` neste compilador.

## Arquivo persistente

`produtos.txt` e texto, sem cabecalho, com um registro por linha e cinco campos:

```text
codigo;nome;categoria;quantidade;preco
```

A linha acima descreve o formato; nao deve ser incluida no arquivo.
Nenhum dado ficticio e fornecido. O programa cria o arquivo ao confirmar o
primeiro cadastro. Arquivo ausente ou vazio significa estoque vazio.

Linhas em branco, registros incompletos, campos extras, valores invalidos,
duplicidades e excesso de registros interrompem a inicializacao. O programa
nao continua com uma leitura parcial nem sobrescreve um arquivo invalido.
Erros de abertura, leitura e fechamento tambem sao verificados.

## Estrategia de gravacao e recuperacao

Todas as operacoes usam funcoes de `stdio.h`:

1. Verificar se existem arquivos auxiliares pendentes. Nao sobrescreve um
   temporario ou backup preexistente.
2. Gravar todo o vetor em `produtos.txt.tmp`, na mesma pasta do original.
3. Conferir `fprintf`, `fflush`, `ferror` e `fclose`. Uma falha nessa etapa
   preserva o original e tenta remover somente o temporario desta tentativa.
4. Se existe original, renomea-lo para `produtos.txt.bak`. Isso evita depender
   de `rename` sobrescrever destinos existentes no Windows.
5. Renomear o temporario completo para `produtos.txt`. Se falhar, tentar
   restaurar o backup. Os arquivos restantes sao preservados para recuperacao.
6. Apos a instalacao bem-sucedida, remover o backup da versao anterior. Se essa
   limpeza falhar, informar que os dados foram salvos e o backup permanece.

Uma falha de gravacao nao substitui o original por um arquivo parcial. Um
cadastro com falha nao entra no total em memoria; uma atualizacao com falha
restaura a quantidade anterior em memoria.

O programa recusa a inicializacao se houver backup pendente e recusa novas
gravacoes se houver temporario ou backup pendente. Para recuperar, feche o
programa e inspecione/copie os arquivos antes de qualquer alteracao:

- Se so existe `.bak`, ele contem o original anterior: restaure uma copia como
  `produtos.txt`, preservando antes os arquivos auxiliares em local seguro.
- Se existem `produtos.txt` e `.bak`, confira os conteudos. O original pode ser
  a versao nova ja instalada, com limpeza interrompida, ou uma versao restaurada.
- Um `.tmp` pode estar completo ou parcial. Sua existencia nao confirma uma
  gravacao. Nao o promova automaticamente a arquivo principal.
- Depois de escolher e preservar a versao correta, retire os arquivos auxiliares
  pendentes da pasta antes de executar novamente.

Use apenas uma instancia do programa por pasta. Esta estrategia protege contra
falhas detectadas de leitura/escrita e permite recuperacao entre renomeacoes;
nao e uma transacao atomica entre todos os passos. C90/stdio nao oferece garantia
portavel de sincronizacao fisica em disco contra queda de energia.

## Principais funcoes

| Funcao | Responsabilidade |
| --- | --- |
| `main`, `exibirMenu` | Inicializacao e controle das opcoes |
| `cadastrarProduto` | Cadastro validado e persistencia |
| `listarProdutos`, `exibirProduto` | Exibicao com categoria |
| `buscarIndicePorCodigo`, `buscarProduto` | Busca e controle de duplicidade |
| `calcularValorEstoque` | Soma de quantidade multiplicada pelo preco |
| `atualizarQuantidade` | Atualizacao e reversao em memoria se falhar a gravacao |
| `carregarProdutos`, `interpretarRegistro` | Leitura e validacao integral do arquivo |
| `salvarProdutos`, `existeArquivo` | Gravacao protegida e verificacao de arquivos |
| `lerLinha`, `solicitarLinha`, `lerInteiro`, `lerTexto`, `lerPreco` | Entrada validada |
| `converterInteiro`, `converterPreco`, `textoValido` | Conversao e regras dos campos |

## Roteiro de testes manuais

Os casos abaixo sao um roteiro, nao uma declaracao de testes executados.
Testes de gravacao devem usar uma pasta de teste isolada dentro do projeto,
sem alterar dados reais. Nao foi fornecido um arquivo de estoque com exemplos.

| Caso | Resultado esperado |
| --- | --- |
| Compilar com os sinalizadores indicados | Sem erros ou avisos |
| Iniciar sem arquivo e listar/calcular | Estoque vazio e total zero |
| Cadastrar com nome e categoria com espacos | Campos preservados em todas as telas |
| Repetir o codigo | Cadastro recusado |
| Buscar codigo existente/inexistente | Dados completos ou aviso |
| Informar letras, negativos, inteiro enorme, preco com virgula | Entrada recusada |
| Informar campo vazio, somente espacos, `;` ou texto longo | Entrada recusada |
| Informar opcao fora de 1 a 6 | Aviso e retorno ao menu |
| Cadastrar 3 unidades a 10 e 2 unidades a 2.50 | Total esperado de 35.00 |
| Atualizar quantidade para zero e reabrir | Quantidade preservada e total recalculado |
| Encerrar no meio de um cadastro | Cadastro incompleto nao confirmado |
| Reabrir apos cadastro ou atualizacao | Todos os campos persistidos |
| Arquivo vazio | Inicializacao normal |
| Registro invalido, duplicado, longo ou acima do limite | Inicializacao interrompida |
| Sem permissao de leitura/escrita | Erro informado, sem confirmacao falsa |
| Falhar escrita, fechamento ou renomeacao em ambiente controlado | Original ou backup preservado |
| Auxiliar `.tmp` ou `.bak` pendente | Bloqueio conforme a estrategia de recuperacao |

## Verificacoes executadas

Compilador local identificado por `gcc --version`:
`gcc.exe (TDM-2 mingw32) 4.4.1`.

Compilacao concluida com codigo de saida zero e sem diagnosticos:

```text
gcc -std=c89 -Wall -Wextra -pedantic -Werror estoque.c -o estoque.exe
```

Sete cenarios foram executados com entradas pelo stdin, verificando a saida e o
codigo de encerramento do executavel:

1. Listagem vazia, calculo de total zero e saida pela opcao 5.
2. Busca e atualizacao de codigo inexistente.
3. Letras, negativo e inteiro excessivo no menu, alem das opcoes 0 e 7.
4. Linha de 700 caracteres e recuperacao da leitura na opcao seguinte.
5. Nome vazio, somente espacos e contendo ponto e virgula; cancelamento por EOF.
6. Categoria vazia e contendo ponto e virgula; cancelamento por EOF.
7. Precos negativo, com virgula e com expoente excessivo; cancelamento por EOF.

Todos esses cenarios passaram. Nenhum cadastro foi confirmado e nenhum arquivo
`produtos.txt`, `.tmp` ou `.bak` foi criado. O executavel `estoque.exe` foi gerado.
Nao foram executados testes de persistencia com cadastros, duplicidade em arquivo,
limite de capacidade, recuperacao ou falhas de disco/permissoes/renomeacao. Esses
itens permanecem no roteiro manual; sua implementacao foi revisada no codigo.
