# sleela-sql

A **simple, dependency-free, CSV-backed MySQL-subset engine** in C. Part of
`mearvk/Nintendo`.

Every table is a plain **CSV file** on disk (`<db-dir>/<name>.csv`): the first
line is the header (column names), each following line is a record. There is no
server, no binary format, and no external dependency — just the C standard
library plus POSIX `dirent` for `SHOW TABLES`.

This follows the same division of labour as the Professional Editor: the C tool
here does the **byte/file work** (parsing SQL, reading/writing CSV), while the
SLeeLa counterpart in [`/lib/sleela-sql`](../lib/sleela-sql/) owns the
**model/validation** layer and runs on the SLeeLa Native VM.

## Build

```sh
cd sleela-sql && make
# produces: sleela-sql (CLI) and libsleelasql.a (static library)
```

## Usage

```sh
# one statement at a time (row output printed as CSV to stdout)
sleela-sql <db-dir> "<SQL statement>"

# or stream statements from stdin, one per line (';' optional, '#' = comment)
sleela-sql <db-dir> < script.sql
```

### Example

```sh
mkdir -p data
sleela-sql data "CREATE TABLE games (id, title, year)"
sleela-sql data "INSERT INTO games VALUES (1, 'Metroid', 1986)"
sleela-sql data "INSERT INTO games VALUES (2, 'Mega Man', 1987)"
sleela-sql data "SELECT title, year FROM games WHERE year = '1986'"
```

```
title,year
Metroid,1986
```

A full worked session is in [`samples/demo.sql`](samples/demo.sql):

```sh
sleela-sql data < samples/demo.sql
```

## Supported SQL (a MySQL-flavoured subset)

Keywords are case-insensitive; a trailing `;` is optional.

| Statement | Form |
|---|---|
| **CREATE TABLE** | `CREATE TABLE <name> (<col> [, <col> ...])` |
| **DROP TABLE** | `DROP TABLE [IF EXISTS] <name>` |
| **INSERT** | `INSERT INTO <name> [(<col>,...)] VALUES (<v>,...)` |
| **SELECT** | `SELECT * \| <col>,... FROM <name> [WHERE <col> = '<value>']` |
| **SHOW TABLES** | `SHOW TABLES` |

Notes:
- Values and the `WHERE` right-hand side may be single- or double-quoted;
  quoting is required for anything containing a comma, quote, or newline.
- `INSERT` is **positional** — the value count must equal the column count
  (an explicit column list is accepted but ignored for ordering).
- `WHERE` supports a single `column = value` equality predicate.
- Fields are stored and emitted as RFC-4180-style CSV (a field is quoted only
  when it contains `,` `"` or a newline; embedded `"` is doubled).

## C API

Link against `libsleelasql.a` and include [`include/sleela_sql.h`](include/sleela_sql.h):

```c
ssql_db db;
ssql_open(&db, "data");
ssql_exec(&db, "CREATE TABLE games (id, title, year)", NULL);
ssql_exec(&db, "SELECT * FROM games", stdout);   /* CSV to the FILE* */
```

Every call returns an `ssql_status`; `ssql_strerror()` maps it to a message.

## Design principles

- **CSV is the database.** Tables are human-readable files you can inspect,
  diff, and edit with any text tool — no opaque storage.
- **Non-destructive by shape.** `INSERT` appends; `SELECT` only reads; arity and
  column checks refuse malformed statements before they touch a file.
- **Model/byte split.** The byte engine (this folder) and the SLeeLa model
  (`/lib/sleela-sql`) agree on the same invariants — SLeeLa validates the plan,
  C executes it.
- **Clean-room, no copyrighted content.** Original tooling only.
