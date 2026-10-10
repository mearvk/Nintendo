# sleela-sql

A **simple, dependency-free, CSV-backed MySQL-subset engine** in C, with **two
surface languages** and **prepared statements**. Part of `mearvk/Nintendo`.

Every table is a plain **CSV file** on disk (`<db-dir>/<name>.csv`): the first
line is the header (column names), each following line is a record. There is no
server, no binary format, and no external dependency — just the C standard
library plus POSIX `dirent` for listing tables.

This follows the same division of labour as the Professional Editor: the C tool
here does the **byte/file work**, while the SLeeLa counterpart in
[`/lib/sleela-sql`](../lib/sleela-sql/) owns the **model/validation** layer and
runs on the SLeeLa Native VM.

## Two dialects, one engine

Every statement can be written in **either** language, and both compile to the
same internal operation and run on the same executor:

| | SQL | SLeeLaSQL |
|---|---|---|
| style | classic MySQL subset | fluent, method-chaining (reads like SLeeLa) |
| select | `SELECT title FROM games WHERE year = '1986'` | `from('games').select(title).where(year == '1986')` |

The dialects are **feature-complete with each other** — anything you can express
in one you can express in the other, with byte-identical results. See
[`docs/SLEELASQL.md`](docs/SLEELASQL.md) for the full SLeeLaSQL grammar.

```text
        SQL  ──┐
               ├──►  one compiled statement (ssql_stmt)  ──►  one executor
  SLeeLaSQL  ──┘
```

## Build

```sh
cd sleela-sql && make
# produces: sleela-sql (CLI) and libsleelasql.a (static library)
```

## Usage

```sh
# one statement (dialect auto-detected; --sql / --sleela force one)
sleela-sql <db-dir> [--sql|--sleela] "<statement>"

# or stream statements from stdin, one per line ('#' = comment)
sleela-sql <db-dir> [--sql|--sleela] < script
```

### The same session, two ways

```sh
# classic SQL
sleela-sql data "CREATE TABLE games (id, title, year)"
sleela-sql data "INSERT INTO games VALUES (1, 'Metroid', 1986)"
sleela-sql data "SELECT title, year FROM games WHERE year = '1986'"

# fluent SLeeLaSQL — identical effect
sleela-sql data "table('games').create(id, title, year)"
sleela-sql data "into('games').insert(1, 'Metroid', 1986)"
sleela-sql data "from('games').select(title, year).where(year == '1986')"
```

Both print:

```
title,year
Metroid,1986
```

Worked sessions: [`samples/demo.sql`](samples/demo.sql) (SQL) and
[`samples/demo.ssql`](samples/demo.ssql) (SLeeLaSQL). A single stdin stream may
freely mix both — the engine sniffs each line.

## Supported statements

Keywords are case-insensitive; a trailing `;` is optional.

| Operation | SQL | SLeeLaSQL |
|---|---|---|
| **Create** | `CREATE TABLE <t> (<col>,...)` | `table("<t>").create(<col>,...)` |
| **Drop** | `DROP TABLE [IF EXISTS] <t>` | `from("<t>").drop([ifExists])` |
| **Insert** | `INSERT INTO <t> [(c,...)] VALUES (v,...)` | `into("<t>").insert(v,...)` |
| **Select** | `SELECT * \| c,... FROM <t> [WHERE c = v]` | `from("<t>").select(* \| c,...)[.where(c == v)]` |
| **List** | `SHOW TABLES` | `tables()` |

Notes:
- Values and the `WHERE`/`.where` right-hand side may be single- or
  double-quoted; quoting is required for anything containing a comma, quote, or
  newline. `INSERT` is positional (value count must equal column count).
- `WHERE` supports a single `column = value` equality predicate (SLeeLaSQL also
  accepts `==`). Fields are stored and emitted as RFC-4180-style CSV.

## Prepared statements (the speed-up)

A statement in **either** dialect can be **compiled once** and executed many
times with different bound values — avoiding a re-parse per call, exactly like a
real DB's `PreparedStatement`. Placeholders are `?` (1-based).

```c
#include "sleela_sql.h"

ssql_db db;
ssql_open(&db, "data");

ssql_stmt *st = NULL;
ssql_prepare(&db, "into('games').insert(?, ?, ?)", SSQL_DIALECT_AUTO, &st);

for (int i = 0; i < n; i++) {
    ssql_reset(st);
    ssql_bind(st, 1, ids[i]);
    ssql_bind(st, 2, titles[i]);
    ssql_bind(st, 3, years[i]);
    ssql_run(st, NULL);          /* parsed once; bind + execute per row */
}
ssql_finalize(st);
```

The one-shot `ssql_exec(&db, text, out)` is sugar for prepare + run + finalize
with no placeholders.

## C API (summary)

| Function | Purpose |
|---|---|
| `ssql_open` | open/create a DB rooted at a directory |
| `ssql_exec` / `ssql_exec_dialect` | run one statement (auto / forced dialect) |
| `ssql_prepare` | compile a statement (either dialect) into `ssql_stmt` |
| `ssql_param_count` | number of `?` placeholders |
| `ssql_bind` / `ssql_reset` | bind a value / clear all bindings |
| `ssql_run` | execute a prepared statement (row output as CSV) |
| `ssql_finalize` | release a prepared statement |
| `ssql_strerror` / `ssql_dialect_name` | diagnostics |

Full contract: [`include/sleela_sql.h`](include/sleela_sql.h).

## Design principles

- **CSV is the database.** Tables are human-readable files you can inspect,
  diff, and edit with any text tool — no opaque storage.
- **Two surfaces, one core.** SQL and SLeeLaSQL lower to the same compiled
  statement and share one executor, so they can never drift apart.
- **Prepare once, run many.** Both dialects support bound `?` placeholders and
  reusable compiled statements.
- **Non-destructive by shape.** `INSERT` appends; `SELECT` only reads; arity and
  column checks refuse malformed statements before they touch a file.
- **Clean-room, no copyrighted content.** Original tooling only.
