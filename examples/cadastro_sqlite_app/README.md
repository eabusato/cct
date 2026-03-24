# cadastro_sqlite_app

![Sigilo do sistema](./src/main.system.svg)

Projeto exemplo em CCT para cadastro simples de alunos com persistencia em SQLite.

## Visao geral

Este exemplo mostra como montar um pequeno sistema de cadastro usando:

- interface interativa em terminal
- validacao de entrada
- persistencia em SQLite
- separacao entre tipos, regras de aplicacao, acesso a banco e UI

O sistema armazena:

- `nome`
- `classe`
- `media`

## Operacoes disponiveis

O menu do programa permite:

- cadastrar aluno
- listar alunos
- buscar por nome
- listar por classe
- atualizar media por `id`
- remover aluno por `id`
- mostrar total de alunos

## Estrutura do exemplo

```text
examples/cadastro_sqlite_app/
├── PLAN.md
├── README.md
├── src/
│   ├── main.cct
│   └── main.system.svg
├── lib/
│   ├── app.cct
│   ├── db.cct
│   ├── types.cct
│   └── ui.cct
└── tests/
    ├── app_flow.test.cct
    ├── db_smoke.test.cct
    └── query_filters.test.cct
```

## Como compilar

```bash
./cct build --project examples/cadastro_sqlite_app
```

## Como executar

Opcao 1, executando pelo projeto:

```bash
./cct run --project examples/cadastro_sqlite_app
```

Opcao 2, executando o binario gerado diretamente:

```bash
cd examples/cadastro_sqlite_app/src
./main
```

## Como testar

```bash
./cct test --project examples/cadastro_sqlite_app
```

## Fluxo de uso

Ao iniciar, o programa mostra o menu:

```text
Cadastro SQLite de Alunos
1 - cadastrar aluno
2 - listar alunos
3 - buscar por nome
4 - listar por classe
5 - atualizar media
6 - remover aluno
7 - mostrar total de alunos
0 - sair
```

Fluxo basico recomendado:

1. Use `1` para cadastrar alunos.
2. Use `2` para conferir a listagem completa.
3. Use `3` para buscar por parte do nome.
4. Use `4` para filtrar por classe.
5. Use `5` para atualizar a media por `id`.
6. Use `6` para remover um cadastro.
7. Use `7` para conferir o total persistido.

## Exemplo de uso real

O trecho abaixo mostra uma sessao real de uso do executavel:

```text
➜  src git:(main) ✗ ./main

Cadastro SQLite de Alunos
1 - cadastrar aluno
2 - listar alunos
3 - buscar por nome
4 - listar por classe
5 - atualizar media
6 - remover aluno
7 - mostrar total de alunos
0 - sair
opcao> 1
nome> Erick Andrade Busato
classe> A
media> 9.5
ok: aluno cadastrado

Cadastro SQLite de Alunos
1 - cadastrar aluno
2 - listar alunos
3 - buscar por nome
4 - listar por classe
5 - atualizar media
6 - remover aluno
7 - mostrar total de alunos
0 - sair
opcao> 2
----------------------------------------------------------------
ID   | NOME                     | CLASSE       | MEDIA
----------------------------------------------------------------
1    | Erick Andrade Busato     | A            | 9.5
----------------------------------------------------------------

Cadastro SQLite de Alunos
1 - cadastrar aluno
2 - listar alunos
3 - buscar por nome
4 - listar por classe
5 - atualizar media
6 - remover aluno
7 - mostrar total de alunos
0 - sair
opcao> 1
nome> Apolo José Busato
classe> DOG
media> 10
ok: aluno cadastrado

Cadastro SQLite de Alunos
1 - cadastrar aluno
2 - listar alunos
3 - buscar por nome
4 - listar por classe
5 - atualizar media
6 - remover aluno
7 - mostrar total de alunos
0 - sair
opcao> 1
nome> Pandora Maria Busato
classe> CAT
media> 7
ok: aluno cadastrado

Cadastro SQLite de Alunos
1 - cadastrar aluno
2 - listar alunos
3 - buscar por nome
4 - listar por classe
5 - atualizar media
6 - remover aluno
7 - mostrar total de alunos
0 - sair
opcao> 1
nome> Luke Ermenegildo Busato
classe> CAT
media> 4.5
ok: aluno cadastrado

Cadastro SQLite de Alunos
1 - cadastrar aluno
2 - listar alunos
3 - buscar por nome
4 - listar por classe
5 - atualizar media
6 - remover aluno
7 - mostrar total de alunos
0 - sair
opcao> 7
total=4

Cadastro SQLite de Alunos
1 - cadastrar aluno
2 - listar alunos
3 - buscar por nome
4 - listar por classe
5 - atualizar media
6 - remover aluno
7 - mostrar total de alunos
0 - sair
opcao> 2
----------------------------------------------------------------
ID   | NOME                     | CLASSE       | MEDIA
----------------------------------------------------------------
1    | Erick Andrade Busato     | A            | 9.5
2    | Apolo José Busato        | DOG          | 10
3    | Pandora Maria Busato     | CAT          | 7
4    | Luke Ermenegildo Busato  | CAT          | 4.5
----------------------------------------------------------------

Cadastro SQLite de Alunos
1 - cadastrar aluno
2 - listar alunos
3 - buscar por nome
4 - listar por classe
5 - atualizar media
6 - remover aluno
7 - mostrar total de alunos
0 - sair
opcao> 3
nome> Er
----------------------------------------------------------------
ID   | NOME                     | CLASSE       | MEDIA
----------------------------------------------------------------
1    | Erick Andrade Busato     | A            | 9.5
4    | Luke Ermenegildo Busato  | CAT          | 4.5
----------------------------------------------------------------

Cadastro SQLite de Alunos
1 - cadastrar aluno
2 - listar alunos
3 - buscar por nome
4 - listar por classe
5 - atualizar media
6 - remover aluno
7 - mostrar total de alunos
0 - sair
opcao> 5
id> 1
nova media> 10
ok: media atualizada

Cadastro SQLite de Alunos
1 - cadastrar aluno
2 - listar alunos
3 - buscar por nome
4 - listar por classe
5 - atualizar media
6 - remover aluno
7 - mostrar total de alunos
0 - sair
opcao> 2
----------------------------------------------------------------
ID   | NOME                     | CLASSE       | MEDIA
----------------------------------------------------------------
1    | Erick Andrade Busato     | A            | 10
2    | Apolo José Busato        | DOG          | 10
3    | Pandora Maria Busato     | CAT          | 7
4    | Luke Ermenegildo Busato  | CAT          | 4.5
----------------------------------------------------------------

Cadastro SQLite de Alunos
1 - cadastrar aluno
2 - listar alunos
3 - buscar por nome
4 - listar por classe
5 - atualizar media
6 - remover aluno
7 - mostrar total de alunos
0 - sair
opcao> 4
classe> CAT
----------------------------------------------------------------
ID   | NOME                     | CLASSE       | MEDIA
----------------------------------------------------------------
3    | Pandora Maria Busato     | CAT          | 7
4    | Luke Ermenegildo Busato  | CAT          | 4.5
----------------------------------------------------------------

Cadastro SQLite de Alunos
1 - cadastrar aluno
2 - listar alunos
3 - buscar por nome
4 - listar por classe
5 - atualizar media
6 - remover aluno
7 - mostrar total de alunos
0 - sair
opcao> 4
classe> A
----------------------------------------------------------------
ID   | NOME                     | CLASSE       | MEDIA
----------------------------------------------------------------
1    | Erick Andrade Busato     | A            | 10
----------------------------------------------------------------
```

## Banco de dados

O banco e criado automaticamente em:

```text
examples/cadastro_sqlite_app/data/alunos.sqlite
```

O schema criado pelo exemplo e:

```sql
CREATE TABLE IF NOT EXISTS alunos(
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  nome TEXT NOT NULL,
  classe TEXT NOT NULL,
  media REAL NOT NULL
);
```

## Validacoes aplicadas

O exemplo valida os dados antes de gravar:

- `nome` nao pode ser vazio
- `classe` nao pode ser vazia
- `media` deve ser numerica
- `media` deve estar entre `0.0` e `10.0`
- `id` deve ser inteiro positivo para atualizar ou remover

## Organizacao interna

- [src/main.cct](/Users/eabusato/dev/cct/examples/cadastro_sqlite_app/src/main.cct): inicializa o banco, garante o schema e sobe a UI
- [lib/types.cct](/Users/eabusato/dev/cct/examples/cadastro_sqlite_app/lib/types.cct): define o tipo `Aluno`
- [lib/db.cct](/Users/eabusato/dev/cct/examples/cadastro_sqlite_app/lib/db.cct): encapsula schema, queries e operacoes SQLite
- [lib/app.cct](/Users/eabusato/dev/cct/examples/cadastro_sqlite_app/lib/app.cct): aplica validacao e regras de negocio
- [lib/ui.cct](/Users/eabusato/dev/cct/examples/cadastro_sqlite_app/lib/ui.cct): implementa o menu interativo e a renderizacao

## Observacoes tecnicas

- escritas usam `db_prepare` + `stmt_bind_*` + `stmt_step`
- leituras usam `db_query` + `rows_*`
- buscas filtradas usam SQL montado com escaping local porque a API atual de `db_sqlite` nao expoe leitura de colunas via prepared statement
- o modulo SQLite e host-only
- se a toolchain do host nao tiver suporte ao `sqlite3`, a compilacao falha claramente

## Testes

Os testes deste exemplo ficam em:

- [app_flow.test.cct](/Users/eabusato/dev/cct/examples/cadastro_sqlite_app/tests/app_flow.test.cct)
- [db_smoke.test.cct](/Users/eabusato/dev/cct/examples/cadastro_sqlite_app/tests/db_smoke.test.cct)
- [query_filters.test.cct](/Users/eabusato/dev/cct/examples/cadastro_sqlite_app/tests/query_filters.test.cct)
