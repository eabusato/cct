# Plano - cadastro_sqlite_app

## Objetivo

Criar um exemplo/projeto em CCT que mantenha um cadastro simples de alunos com os campos:

- `nome`
- `classe`
- `media`

Os dados devem ser persistidos em SQLite e o sistema deve permitir gerenciamento simples pelo terminal, usando apenas recursos ja existentes na linguagem e na biblioteca canonica do repositório.

## Base tecnica usada para este plano

Este plano foi montado a partir do codigo ja existente no repositório, principalmente:

- `lib/cct/db_sqlite.cct`
- `examples/config_sqlite_app_20f2.cct`
- `examples/phase30_data_app/lib/data_app.cct`
- `lib/cct/io.cct`
- `lib/cct/args.cct`
- `lib/cct/parse.cct`
- `lib/cct/verbum.cct`
- `lib/cct/fmt.cct`
- `lib/cct/option.cct`
- `lib/cct/result.cct`

Decisao importante:

- nao usar `cct/orm_lite` como base principal, porque o modulo atual e focado em cenarios tipo chave/valor e nao cobre bem um cadastro de alunos com schema proprio
- usar `cct/db_sqlite` diretamente com `db_prepare`, `stmt_bind_*`, `stmt_step`, `db_query` e `rows_*`

## Forma do exemplo

O sistema deve ser implementado como projeto canonico CCT em:

```text
examples/cadastro_sqlite_app/
├── PLAN.md
├── src/main.cct
├── lib/db.cct
├── lib/ui.cct
├── lib/app.cct
├── tests/
│   ├── db_smoke.test.cct
│   └── app_flow.test.cct
└── README.md
```

Observacoes:

- `README.md` entra depois da implementacao; por enquanto este arquivo e apenas o plano
- o banco deve ser criado em um caminho local ao exemplo, por exemplo `examples/cadastro_sqlite_app/data/alunos.sqlite`
- a pasta `data/` pode ser criada em runtime com `cct/fs`

## Escopo funcional da primeira versao

Primeira versao do sistema:

1. inicializar o banco e criar a tabela se ela nao existir
2. cadastrar aluno
3. listar alunos
4. buscar alunos por nome
5. filtrar alunos por classe
6. atualizar media por `id`
7. remover aluno por `id`
8. sair do sistema com fechamento limpo do banco

Escopo propositalmente fora da v1:

- multiplas tabelas
- autenticacao
- importacao/exportacao CSV
- edicao de nome/classe no mesmo fluxo
- camada ORM generica

## Modelo de dados

Tabela principal proposta:

```sql
CREATE TABLE IF NOT EXISTS alunos (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  nome TEXT NOT NULL,
  classe TEXT NOT NULL,
  media REAL NOT NULL
);
```

Regras de dominio previstas na aplicacao:

- `nome` nao pode ser vazio
- `classe` nao pode ser vazia
- `media` deve ficar no intervalo `0.0` a `10.0`
- `id` sera usado como identificador de manutencao para update/delete

Observacao:

- mesmo que o usuario pense no cadastro como `nome + classe + media`, o `id` interno simplifica muito o gerenciamento e evita ambiguidade em nomes repetidos

## Experiencia de uso prevista

Fluxo principal recomendado para a v1:

- executar o projeto
- abrir um menu textual simples
- pedir entradas com `read_line_prompt`
- repetir o ciclo ate o usuario escolher sair

Menu previsto:

```text
1 - cadastrar aluno
2 - listar alunos
3 - buscar por nome
4 - listar por classe
5 - atualizar media
6 - remover aluno
7 - mostrar total de alunos
0 - sair
```

Motivo da escolha:

- `cct/io` ja oferece `read_line_prompt`
- um menu evita parser de comandos mais complexo na primeira entrega
- fica mais alinhado com a ideia de "gerenciar de forma simples"

Possivel extensao futura:

- aceitar subcomandos via `cct/args` para automacao (`add`, `list`, `update-media`, `delete`)

## Modulos CCT previstos

### `src/main.cct`

Responsabilidades:

- abrir o banco
- garantir schema
- chamar o loop principal da aplicacao
- fechar o banco no final
- devolver codigo de saida adequado

### `lib/db.cct`

Responsabilidades:

- `db_init_schema`
- `aluno_insert`
- `aluno_list_all`
- `aluno_find_by_nome`
- `aluno_list_by_classe`
- `aluno_update_media`
- `aluno_delete`
- `aluno_count`

Implementacao prevista:

- `CREATE TABLE` com `db_exec`
- `INSERT`, `UPDATE`, `DELETE` com `db_prepare` e `stmt_bind_*`
- `SELECT` com `db_query`, `rows_next`, `rows_get_int`, `rows_get_text`, `rows_get_real`
- writes encapsulados com `db_begin` / `db_commit` e `db_rollback` em caso de falha

### `lib/ui.cct`

Responsabilidades:

- imprimir menu
- pedir e sanitizar entradas
- imprimir tabelas/listagens simples
- mostrar mensagens de sucesso e erro

Implementacao prevista:

- `println`, `print`, `read_line_prompt`
- `trim`, `len`, `concat`
- `stringify_int`, `stringify_real_fixed`

### `lib/app.cct`

Responsabilidades:

- orquestrar o fluxo do menu
- validar dados antes de chamar a camada de banco
- decidir mensagens para o usuario

## Estrategia de persistencia

Uso direto da API existente em `lib/cct/db_sqlite.cct`:

- abrir conexao com `db_open`
- executar schema com `db_exec`
- inserir com `db_prepare` + `stmt_bind_text` + `stmt_bind_real`
- consultar com `db_query`
- iterar linhas com `rows_next`
- ler colunas com `rows_get_int`, `rows_get_text`, `rows_get_real`
- fechar cursores com `rows_close`
- finalizar statements com `stmt_finalize`
- fechar conexao com `db_close`

Decisoes de seguranca e robustez:

- evitar SQL montado com concatenacao para operacoes com entrada do usuario
- preferir prepared statements para insert/update/delete e buscas com filtro
- sempre fechar cursor/statement antes de sair do rituale

## Validacao de entrada

Validacoes previstas:

- `nome`: `trim`, obrigatorio, minimo de 1 caractere util
- `classe`: `trim`, obrigatorio
- `media`: parse seguro com `try_real` ou encapsulamento com `Result`
- `media`: rejeitar valores fora de `0.0..10.0`
- `id`: parse para inteiro e checagem `> 0`

Estrategia para `media`:

- usar `cct/parse` para converter
- usar `cct/option` ou `cct/result` para nao depender de excecao como fluxo normal de menu

## Saida esperada da v1

Exemplo de listagem:

```text
ID | NOME            | CLASSE     | MEDIA
1  | Ana Souza       | 9A         | 8.50
2  | Bruno Lima      | 9A         | 7.25
3  | Carla Mendes    | 8B         | 9.10
```

Exemplo de fluxo:

```text
1 - cadastrar aluno
2 - listar alunos
3 - buscar por nome
4 - listar por classe
5 - atualizar media
6 - remover aluno
7 - mostrar total de alunos
0 - sair

opcao> 1
nome> Ana Souza
classe> 9A
media> 8.5
ok: aluno cadastrado
```

## Plano de implementacao

### Etapa 1 - Estrutura do projeto

- criar `examples/cadastro_sqlite_app/`
- criar `src/main.cct`
- criar `lib/db.cct`
- criar `lib/ui.cct`
- criar `lib/app.cct`
- preparar pasta de testes

### Etapa 2 - Camada de banco

- implementar criacao do schema
- implementar insert com prepared statement
- implementar select geral
- implementar busca por nome
- implementar filtro por classe
- implementar update de media por `id`
- implementar delete por `id`
- implementar contagem total

### Etapa 3 - Validacao e regras

- validar entradas de texto
- validar parse de media
- validar faixa de media
- validar `id` positivo
- padronizar mensagens de erro

### Etapa 4 - Interface textual

- menu principal
- prompts de entrada
- saida tabular simples
- loop principal ate opcao `0`

### Etapa 5 - Testes

- teste de schema e insert
- teste de listagem
- teste de busca por nome
- teste de update de media
- teste de delete
- teste de fluxo principal minimo

### Etapa 6 - Documentacao do exemplo

- criar `README.md` do exemplo
- documentar como compilar e executar
- documentar local do banco
- documentar limitacoes atuais

## Validacao prevista

Validacao de desenvolvimento:

```bash
./cct build --project examples/cadastro_sqlite_app
./cct run --project examples/cadastro_sqlite_app
./cct test --project examples/cadastro_sqlite_app
```

Validacao funcional minima:

- iniciar sem banco existente
- cadastrar 2 ou 3 alunos
- listar e confirmar persistencia
- atualizar media de um registro
- remover um registro
- reiniciar o programa e confirmar que os dados permaneceram

## Riscos e cuidados

- o modulo SQLite e host-only; se o host nao tiver `sqlite3`, a compilacao deve falhar claramente
- entradas invalidas no menu precisam ser tratadas sem encerrar a aplicacao
- statements e cursores precisam ser fechados corretamente em caminhos de sucesso e erro
- o caminho do banco deve ser local ao exemplo e previsivel

## Decisao final deste plano

Implementar a primeira versao como um projeto CCT modular com menu interativo no terminal, persistencia SQLite via `cct/db_sqlite`, e operacoes basicas de CRUD sobre a tabela `alunos`.

Proximo passo depois deste plano:

- materializar a estrutura `examples/cadastro_sqlite_app/`
- implementar primeiro a camada `lib/db.cct`
- depois ligar isso a uma UI de menu simples em `lib/ui.cct` e `lib/app.cct`
