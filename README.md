# Sistema de Controle de Estoque em C

Aplicação de terminal desenvolvida em **linguagem C** para cadastrar e consultar produtos de uma loja de informática. Os registros são armazenados em arquivo, permitindo recuperar o estoque após fechar e reabrir o programa.

Projeto acadêmico da disciplina **Programação de Software Básico em C**.

## Funcionalidades

- **Cadastrar produtos** com código, nome, categoria, quantidade e preço unitário.
- **Listar produtos** registrados no estoque.
- **Buscar produtos** pelo código, incluindo tratamento de códigos inexistentes.
- **Calcular o valor total** do estoque a partir de quantidade × preço unitário.
- **Atualizar quantidades** de produtos cadastrados (desafio adicional).
- **Salvar e recuperar dados** no arquivo `produtos.txt`, com validação de entradas.

## Tecnologias

- C (compatível com o padrão C89/C90)
- GCC TDM-MinGW 4.4.1, no Windows
- Biblioteca padrão `stdio.h` para manipulação de arquivos
- API do Windows para exibir corretamente caracteres UTF-8 no console

## Como executar

No terminal PowerShell, dentro da pasta do projeto, compile com:

```powershell
gcc -std=c89 -Wall -Wextra -pedantic -finput-charset=UTF-8 -fexec-charset=UTF-8 estoque.c -o estoque.exe
```

Depois execute:

```powershell
.\estoque.exe
```

O programa apresenta um menu interativo com as opções de **1 a 6**; a opção **5** encerra a aplicação. Os arquivos de dados são lidos e criados na pasta a partir da qual o programa é executado.

## Estrutura do projeto

```text
estoque.c       Código-fonte do sistema
produtos.txt    Produtos usados nos testes
README.md       Documentação e instruções
testes/        Teste automatizado de codificação UTF-8
evidencias/    Capturas de tela do código e das operações
```

O arquivo `produtos.txt` utiliza um produto por linha, com campos separados por ponto e vírgula:

```text
codigo;nome;categoria;quantidade;preco
```

*Essa linha demonstra o formato e não é um cabeçalho do arquivo.* Para os testes, foram utilizados produtos fictícios.

## Testes realizados

O sistema foi executado no VS Code, com verificação manual de cadastro de cinco produtos, listagem, busca por código existente e inexistente, cálculo do estoque, atualização de quantidade e recuperação dos dados após encerrar e reiniciar o programa.

**Resultado final verificado:** cinco produtos cadastrados, com valor total de **R$ 13.904,00** após a atualização do Monitor LED de 5 para 8 unidades.

As capturas de execução e do arquivo `produtos.txt` estão disponíveis na pasta [`evidencias/`](evidencias/).

---

**Objetivo do projeto:** praticar estruturas (`struct`), funções, validação de entradas e persistência de dados em arquivos usando C.
