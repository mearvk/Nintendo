/*
 * sleela_sql.c -- CSV-backed MySQL-subset engine (C implementation).
 * Part of mearvk/Nintendo.
 *
 * Storage model: one CSV file per table (<dir>/<name>.csv). The first line is
 * the header (column names); each following line is a record. CSV fields are
 * comma-separated and may be double-quoted; a quote inside a quoted field is
 * doubled ("" ). This is deliberately small and dependency-free.
 */
#include "sleela_sql.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>  /* strcasecmp */
#include <dirent.h>

/* ---- small string helpers -------------------------------------------- */

static char *str_dup(const char *s) {
    size_t n = strlen(s) + 1;
    char *p = (char *)malloc(n);
    if (p) memcpy(p, s, n);
    return p;
}

/* Trim leading/trailing ASCII whitespace in place; returns the start. */
static char *trim(char *s) {
    while (*s && isspace((unsigned char)*s)) s++;
    char *end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1])) *--end = '\0';
    return s;
}

/* Case-insensitive keyword compare of the first word of `s`. */
static int kw_eq(const char *s, const char *kw) {
    size_t n = strlen(kw);
    for (size_t i = 0; i < n; i++) {
        if (tolower((unsigned char)s[i]) != tolower((unsigned char)kw[i])) return 0;
    }
    /* keyword boundary: next char is end or non-identifier */
    char c = s[n];
    return (c == '\0' || isspace((unsigned char)c) || c == '(' || c == '*' || c == ';');
}

/* Strip one matching pair of surrounding single/double quotes, in place. */
static char *unquote(char *s) {
    size_t n = strlen(s);
    if (n >= 2 && ((s[0] == '\'' && s[n - 1] == '\'') ||
                   (s[0] == '"'  && s[n - 1] == '"'))) {
        s[n - 1] = '\0';
        return s + 1;
    }
    return s;
}

/*
 * Split `s` on top-level commas (commas not inside single/double quotes) into
 * up to `max` trimmed+unquoted fields. Mutates `s`. Returns the field count.
 */
static int split_fields(char *s, char **out, int max) {
    int n = 0;
    char *start = s;
    char quote = 0;
    for (char *p = s;; p++) {
        if (quote) {
            if (*p == quote) quote = 0;
        } else if (*p == '\'' || *p == '"') {
            quote = *p;
        } else if (*p == ',' || *p == '\0') {
            char save = *p;
            *p = '\0';
            if (n < max) {
                char *f = trim(start);
                out[n++] = unquote(f);
            }
            if (save == '\0') break;
            start = p + 1;
        }
    }
    return n;
}

/* ---- CSV field writing ----------------------------------------------- */

/* Write one CSV field, quoting only if it contains comma, quote, or newline. */
static void csv_write_field(FILE *out, const char *v) {
    int needs = 0;
    for (const char *p = v; *p; p++) {
        if (*p == ',' || *p == '"' || *p == '\n' || *p == '\r') { needs = 1; break; }
    }
    if (!needs) { fputs(v, out); return; }
    fputc('"', out);
    for (const char *p = v; *p; p++) {
        if (*p == '"') fputc('"', out);
        fputc(*p, out);
    }
    fputc('"', out);
}

static void csv_write_row(FILE *out, char **fields, int n) {
    for (int i = 0; i < n; i++) {
        if (i) fputc(',', out);
        csv_write_field(out, fields[i]);
    }
    fputc('\n', out);
}

/*
 * Parse one CSV line into up to `max` fields (NUL-terminated, dequoted).
 * `line` is mutated. Returns field count.
 */
static int csv_parse_line(char *line, char **out, int max) {
    /* strip trailing CR/LF */
    size_t n = strlen(line);
    while (n && (line[n - 1] == '\n' || line[n - 1] == '\r')) line[--n] = '\0';

    int count = 0;
    char *w = line;         /* write cursor (in-place compaction)          */
    char *field = line;     /* start of current field                      */
    int in_quotes = 0;
    char *p = line;
    for (;; p++) {
        char c = *p;
        if (in_quotes) {
            if (c == '"') {
                if (p[1] == '"') { *w++ = '"'; p++; }
                else in_quotes = 0;
            } else if (c == '\0') {
                break;
            } else {
                *w++ = c;
            }
        } else {
            if (c == '"') {
                in_quotes = 1;
            } else if (c == ',' || c == '\0') {
                *w = '\0';
                if (count < max) out[count++] = field;
                if (c == '\0') break;
                w++;            /* step over the NUL we just wrote          */
                field = w;
            } else {
                *w++ = c;
            }
        }
    }
    return count;
}

/* ---- path helpers ---------------------------------------------------- */

static void table_path(const ssql_db *db, const char *name, char *buf, size_t cap) {
    snprintf(buf, cap, "%s/%s.csv", db->dir, name);
}

static int table_exists(const ssql_db *db, const char *name) {
    char path[SSQL_MAX_FIELD * 2];
    table_path(db, name, path, sizeof(path));
    FILE *f = fopen(path, "rb");
    if (f) { fclose(f); return 1; }
    return 0;
}

/* ---- statement handlers ---------------------------------------------- */

/* CREATE TABLE <name> (<c1>, <c2>, ...) */
static ssql_status do_create(ssql_db *db, char *rest) {
    char *lp = strchr(rest, '(');
    char *rp = strrchr(rest, ')');
    if (!lp || !rp || rp < lp) return SSQL_ERR_SYNTAX;
    *lp = '\0';
    char *name = trim(rest);
    if (!*name) return SSQL_ERR_SYNTAX;
    if (table_exists(db, name)) return SSQL_ERR_EXISTS;

    *rp = '\0';
    char *cols[SSQL_MAX_COLS];
    int ncols = split_fields(lp + 1, cols, SSQL_MAX_COLS);
    if (ncols < 1) return SSQL_ERR_SYNTAX;

    char path[SSQL_MAX_FIELD * 2];
    table_path(db, name, path, sizeof(path));
    FILE *f = fopen(path, "wb");
    if (!f) return SSQL_ERR_IO;
    csv_write_row(f, cols, ncols);
    fclose(f);
    return SSQL_OK;
}

/* DROP TABLE [IF EXISTS] <name> */
static ssql_status do_drop(ssql_db *db, char *rest) {
    char *name = trim(rest);
    int if_exists = 0;
    if (kw_eq(name, "if")) {
        name = trim(name + 2);
        if (kw_eq(name, "exists")) { name = trim(name + 6); if_exists = 1; }
    }
    if (!*name) return SSQL_ERR_SYNTAX;
    if (!table_exists(db, name)) return if_exists ? SSQL_OK : SSQL_ERR_NOTABLE;
    char path[SSQL_MAX_FIELD * 2];
    table_path(db, name, path, sizeof(path));
    return remove(path) == 0 ? SSQL_OK : SSQL_ERR_IO;
}

/* Read the header row of a table into `cols` (caller frees each). */
static ssql_status read_header(const ssql_db *db, const char *name,
                               char *hdrbuf, size_t cap, char **cols, int *ncols) {
    char path[SSQL_MAX_FIELD * 2];
    table_path(db, name, path, sizeof(path));
    FILE *f = fopen(path, "rb");
    if (!f) return SSQL_ERR_NOTABLE;
    if (!fgets(hdrbuf, (int)cap, f)) { fclose(f); hdrbuf[0] = '\0'; }
    fclose(f);
    *ncols = csv_parse_line(hdrbuf, cols, SSQL_MAX_COLS);
    return SSQL_OK;
}

/* INSERT INTO <name> [(c,...)] VALUES (v,...) */
static ssql_status do_insert(ssql_db *db, char *rest) {
    if (!kw_eq(rest, "into")) return SSQL_ERR_SYNTAX;
    rest = trim(rest + 4);

    /* table name ends at '(' or the VALUES keyword */
    char *vpos = rest;
    while (*vpos && !kw_eq(vpos, "values")) vpos++;
    if (!*vpos) return SSQL_ERR_SYNTAX;

    /* carve the segment before VALUES: "<name>" or "<name> (c,..)" */
    char nameseg[SSQL_MAX_FIELD];
    size_t seglen = (size_t)(vpos - rest);
    if (seglen >= sizeof(nameseg)) return SSQL_ERR_SYNTAX;
    memcpy(nameseg, rest, seglen);
    nameseg[seglen] = '\0';
    char *paren = strchr(nameseg, '(');
    if (paren) *paren = '\0';          /* ignore explicit column list; positional */
    char *name = trim(nameseg);
    if (!*name) return SSQL_ERR_SYNTAX;

    /* values list */
    char *lp = strchr(vpos, '(');
    char *rp = strrchr(vpos, ')');
    if (!lp || !rp || rp < lp) return SSQL_ERR_SYNTAX;
    *rp = '\0';
    char *vals[SSQL_MAX_COLS];
    int nvals = split_fields(lp + 1, vals, SSQL_MAX_COLS);
    if (nvals < 1) return SSQL_ERR_SYNTAX;

    /* arity check against the header */
    char hdr[SSQL_MAX_FIELD * 4];
    char *cols[SSQL_MAX_COLS];
    int ncols = 0;
    ssql_status s = read_header(db, name, hdr, sizeof(hdr), cols, &ncols);
    if (s != SSQL_OK) return s;
    if (nvals != ncols) return SSQL_ERR_ARITY;

    char path[SSQL_MAX_FIELD * 2];
    table_path(db, name, path, sizeof(path));
    FILE *f = fopen(path, "ab");
    if (!f) return SSQL_ERR_IO;
    csv_write_row(f, vals, nvals);
    fclose(f);
    return SSQL_OK;
}

static int find_col(char **cols, int ncols, const char *name) {
    for (int i = 0; i < ncols; i++)
        if (strcasecmp(cols[i], name) == 0) return i;
    return -1;
}

/* SELECT * | c,... FROM <name> [WHERE col = 'v'] */
static ssql_status do_select(ssql_db *db, char *rest, FILE *out) {
    /* projection: everything up to FROM */
    char *fpos = rest;
    while (*fpos && !kw_eq(fpos, "from")) fpos++;
    if (!*fpos) return SSQL_ERR_SYNTAX;
    char projbuf[SSQL_MAX_FIELD];
    size_t plen = (size_t)(fpos - rest);
    if (plen >= sizeof(projbuf)) return SSQL_ERR_SYNTAX;
    memcpy(projbuf, rest, plen);
    projbuf[plen] = '\0';
    char *proj = trim(projbuf);

    /* after FROM: "<name> [WHERE ...]" */
    char *after = trim(fpos + 4);
    char *wpos = after;
    while (*wpos && !kw_eq(wpos, "where")) wpos++;
    char namebuf[SSQL_MAX_FIELD];
    size_t nlen = (size_t)(wpos - after);
    if (nlen >= sizeof(namebuf)) return SSQL_ERR_SYNTAX;
    memcpy(namebuf, after, nlen);
    namebuf[nlen] = '\0';
    char *name = trim(namebuf);
    if (!*name) return SSQL_ERR_SYNTAX;

    /* optional WHERE col = value (equality only) */
    char where_col[SSQL_MAX_FIELD] = {0};
    char where_val[SSQL_MAX_FIELD] = {0};
    int have_where = 0;
    if (*wpos) {
        char *expr = trim(wpos + 5);
        char *eq = strchr(expr, '=');
        if (!eq) return SSQL_ERR_SYNTAX;
        *eq = '\0';
        char *wc = trim(expr);
        char *wv = unquote(trim(eq + 1));
        if (!*wc) return SSQL_ERR_SYNTAX;
        snprintf(where_col, sizeof(where_col), "%s", wc);
        snprintf(where_val, sizeof(where_val), "%s", wv);
        have_where = 1;
    }

    char path[SSQL_MAX_FIELD * 2];
    table_path(db, name, path, sizeof(path));
    FILE *f = fopen(path, "rb");
    if (!f) return SSQL_ERR_NOTABLE;

    /* header -- parsed in place in hdrbuf, which stays live for this call */
    char line[SSQL_MAX_FIELD * 8];
    char hdrbuf[SSQL_MAX_FIELD * 8];
    char *cols[SSQL_MAX_COLS];
    int ncols = 0;
    if (fgets(hdrbuf, sizeof(hdrbuf), f)) {
        ncols = csv_parse_line(hdrbuf, cols, SSQL_MAX_COLS);
    }

    /* resolve projection to column indices */
    int proj_idx[SSQL_MAX_COLS];
    int nproj = 0;
    int star = (strcmp(proj, "*") == 0);
    if (star) {
        for (int i = 0; i < ncols; i++) proj_idx[nproj++] = i;
    } else {
        char projtmp[SSQL_MAX_FIELD];
        snprintf(projtmp, sizeof(projtmp), "%s", proj);
        char *pcols[SSQL_MAX_COLS];
        int np = split_fields(projtmp, pcols, SSQL_MAX_COLS);
        for (int i = 0; i < np; i++) {
            int idx = find_col(cols, ncols, pcols[i]);
            if (idx < 0) { fclose(f); return SSQL_ERR_NOCOL; }
            proj_idx[nproj++] = idx;
        }
    }

    int where_idx = -1;
    if (have_where) {
        where_idx = find_col(cols, ncols, where_col);
        if (where_idx < 0) { fclose(f); return SSQL_ERR_NOCOL; }
    }

    /* header row of the result set */
    if (out) {
        for (int i = 0; i < nproj; i++) {
            if (i) fputc(',', out);
            csv_write_field(out, cols[proj_idx[i]]);
        }
        fputc('\n', out);
    }

    /* data rows */
    while (fgets(line, sizeof(line), f)) {
        char rowtmp[SSQL_MAX_FIELD * 8];
        snprintf(rowtmp, sizeof(rowtmp), "%s", line);
        char *fields[SSQL_MAX_COLS];
        int nf = csv_parse_line(rowtmp, fields, SSQL_MAX_COLS);
        if (nf == 0) continue;
        if (have_where) {
            if (where_idx >= nf) continue;
            if (strcmp(fields[where_idx], where_val) != 0) continue;
        }
        if (out) {
            for (int i = 0; i < nproj; i++) {
                if (i) fputc(',', out);
                int idx = proj_idx[i];
                csv_write_field(out, idx < nf ? fields[idx] : "");
            }
            fputc('\n', out);
        }
    }
    fclose(f);
    return SSQL_OK;
}

/* SHOW TABLES -- lists <name> for every <name>.csv in the dir (POSIX dirent). */
static ssql_status do_show_tables(ssql_db *db, FILE *out) {
    DIR *d = opendir(db->dir);
    if (!d) return SSQL_ERR_IO;
    if (out) fputs("Tables_in_sleela_sql\n", out);
    struct dirent *e;
    while ((e = readdir(d)) != NULL) {
        const char *n = e->d_name;
        size_t len = strlen(n);
        if (len > 4 && strcmp(n + len - 4, ".csv") == 0) {
            if (out) {
                char base[SSQL_MAX_FIELD];
                size_t blen = len - 4;
                if (blen >= sizeof(base)) blen = sizeof(base) - 1;
                memcpy(base, n, blen);
                base[blen] = '\0';
                csv_write_field(out, base);
                fputc('\n', out);
            }
        }
    }
    closedir(d);
    return SSQL_OK;
}

/* ---- public API ------------------------------------------------------ */

ssql_status ssql_open(ssql_db *db, const char *dir) {
    if (!db || !dir) return SSQL_ERR_ARG;
    snprintf(db->dir, sizeof(db->dir), "%s", dir);
    return SSQL_OK;
}

ssql_status ssql_exec(ssql_db *db, const char *sql, FILE *out) {
    if (!db || !sql) return SSQL_ERR_ARG;

    /* work on a trimmed, semicolon-free copy */
    char *buf = str_dup(sql);
    if (!buf) return SSQL_ERR_IO;
    char *stmt = trim(buf);
    size_t n = strlen(stmt);
    while (n && (stmt[n - 1] == ';' || isspace((unsigned char)stmt[n - 1])))
        stmt[--n] = '\0';
    if (!*stmt) { free(buf); return SSQL_ERR_SYNTAX; }

    ssql_status s;
    if (kw_eq(stmt, "create")) {
        char *r = trim(stmt + 6);
        if (!kw_eq(r, "table")) s = SSQL_ERR_SYNTAX;
        else s = do_create(db, trim(r + 5));
    } else if (kw_eq(stmt, "drop")) {
        char *r = trim(stmt + 4);
        if (!kw_eq(r, "table")) s = SSQL_ERR_SYNTAX;
        else s = do_drop(db, trim(r + 5));
    } else if (kw_eq(stmt, "insert")) {
        s = do_insert(db, trim(stmt + 6));
    } else if (kw_eq(stmt, "select")) {
        s = do_select(db, trim(stmt + 6), out);
    } else if (kw_eq(stmt, "show")) {
        char *r = trim(stmt + 4);
        if (kw_eq(r, "tables")) s = do_show_tables(db, out);
        else s = SSQL_ERR_SYNTAX;
    } else {
        s = SSQL_ERR_SYNTAX;
    }

    free(buf);
    return s;
}

const char *ssql_strerror(ssql_status s) {
    switch (s) {
        case SSQL_OK:          return "ok";
        case SSQL_ERR_IO:      return "I/O error";
        case SSQL_ERR_SYNTAX:  return "syntax error";
        case SSQL_ERR_NOTABLE: return "no such table";
        case SSQL_ERR_EXISTS:  return "table already exists";
        case SSQL_ERR_NOCOL:   return "no such column";
        case SSQL_ERR_ARITY:   return "value count does not match column count";
        case SSQL_ERR_ARG:     return "bad argument";
        default:               return "unknown error";
    }
}
