/*
 * sleela_sql_cli.c -- command-line front-end for the CSV-backed SQL engine.
 * Part of mearvk/Nintendo.
 *
 * Usage:
 *   sleela-sql <db-dir> "<SQL statement>"      run one statement
 *   sleela-sql <db-dir>                         read statements from stdin
 *                                               (one per line; ';' optional)
 *
 * Examples:
 *   sleela-sql ./data "CREATE TABLE games (id, title, year)"
 *   sleela-sql ./data "INSERT INTO games VALUES (1, 'Metroid', 1986)"
 *   sleela-sql ./data "SELECT title, year FROM games WHERE id = '1'"
 *   sleela-sql ./data "SHOW TABLES"
 *
 * Row output (SELECT / SHOW TABLES) is printed to stdout as CSV.
 */
#include "sleela_sql.h"

#include <stdio.h>
#include <string.h>

static int run_one(ssql_db *db, const char *sql) {
    ssql_status s = ssql_exec(db, sql, stdout);
    if (s != SSQL_OK) {
        fprintf(stderr, "sleela-sql: %s\n", ssql_strerror(s));
        return 1;
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr,
            "usage: %s <db-dir> [\"<SQL>\"]\n"
            "  with SQL argument: run one statement\n"
            "  without:            read statements from stdin, one per line\n",
            argv[0]);
        return 2;
    }

    ssql_db db;
    if (ssql_open(&db, argv[1]) != SSQL_OK) {
        fprintf(stderr, "sleela-sql: could not open db at %s\n", argv[1]);
        return 1;
    }

    if (argc >= 3) {
        return run_one(&db, argv[2]);
    }

    /* stdin mode: one statement per line, blank lines and # comments ignored */
    char line[4096];
    int rc = 0;
    while (fgets(line, sizeof(line), stdin)) {
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0' || *p == '\n' || *p == '#') continue;
        if (run_one(&db, p) != 0) rc = 1;
    }
    return rc;
}
