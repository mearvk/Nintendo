/*
 * sleela_sql.h -- a simple CSV-backed MySQL-subset engine (C API).
 *
 * Part of mearvk/Nintendo. This is a self-contained, dependency-free SQL
 * engine: every table is a plain CSV file on disk (header row = column names,
 * one data row per record). It ships no copyrighted content.
 *
 * The division of labour mirrors the Professional Editor: this C engine does
 * the byte/file I/O (parsing SQL text, reading and writing CSV), while the
 * SLeeLa counterpart in /lib/sleela-sql owns the *model and validation* layer
 * (column arity, the WHERE predicate shape, type verdicts) -- SLeeLa's file
 * surface is string-oriented, so it reasons about the plan rather than touching
 * the CSV bytes.
 *
 * Supported statements (a MySQL-flavoured subset, case-insensitive keywords):
 *   CREATE TABLE <name> (<col> [, <col> ...]);
 *   DROP TABLE [IF EXISTS] <name>;
 *   INSERT INTO <name> [(<col>,...)] VALUES (<v>,...);
 *   SELECT * | <col>,... FROM <name> [WHERE <col> = '<value>'];
 *   SHOW TABLES;
 */
#ifndef SLEELA_SQL_H
#define SLEELA_SQL_H

#include <stddef.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SSQL_MAX_COLS   64
#define SSQL_MAX_FIELD  512

typedef enum {
    SSQL_OK = 0,
    SSQL_ERR_IO       = -1,  /* file could not be read or written           */
    SSQL_ERR_SYNTAX   = -2,  /* the statement did not parse                 */
    SSQL_ERR_NOTABLE  = -3,  /* table (CSV file) does not exist             */
    SSQL_ERR_EXISTS   = -4,  /* CREATE TABLE on an existing table           */
    SSQL_ERR_NOCOL    = -5,  /* referenced a column the table does not have */
    SSQL_ERR_ARITY    = -6,  /* INSERT value count != column count          */
    SSQL_ERR_ARG      = -7   /* bad argument                                */
} ssql_status;

/* An engine instance: all tables live as <name>.csv under `dir`. */
typedef struct {
    char dir[SSQL_MAX_FIELD]; /* directory holding the .csv table files */
} ssql_db;

/* Open (or create) a database rooted at a directory. */
ssql_status ssql_open(ssql_db *db, const char *dir);

/*
 * Execute a single SQL statement. Any row output (SELECT / SHOW TABLES) is
 * written to `out` as CSV. Pass NULL to discard output. Returns a status code.
 */
ssql_status ssql_exec(ssql_db *db, const char *sql, FILE *out);

/* Human-readable message for a status code. */
const char *ssql_strerror(ssql_status s);

#ifdef __cplusplus
}
#endif

#endif /* SLEELA_SQL_H */
