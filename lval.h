#ifndef LVAL_H
#define LVAL_H
#include <stdio.h>
#include <stdarg.h>

#include "mpc.h"

#define MAX_ERR (512)

enum {LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR, LVAL_QEXPR, LVAL_FUN, LVAL_STR };

struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;
typedef lval*(*lbuiltin)(lenv*, lval*);

struct lval
{
	int type;

	/* Basic */
	long num;
	char * err;
	char * sym;
	char * str;

	/* Function */
	lbuiltin builtin;
	lenv* env;
	lval* formals;
	lval* body;

	/* Expression */
	int count;
	struct lval ** cell;
};

lval* lval_num(long x);

lval* lval_err(char* fmt, ...);

lval* lval_sym(char* s);

lval* lval_str(char* s);

lval* lval_builtin(lbuiltin fun);

lval* lval_lambda(lval* formals, lval* body);

lval* lval_sexpr(void);

lval* lval_qexpr(void);

void lval_del(lval* v);

lval* lval_copy(lval* v);

lval* lval_add(lval* v, lval* x);

lval* lval_join(lval* x, lval* y);

lval* lval_pop(lval* v, int i);

lval* lval_take(lval* v, int i);

void lval_print(lval* v);

void lval_print_expr(lval* v, char open, char close);

void lval_print_str(lval* v);

void lval_println(lval* v);

char* ltype_name(int t);

int lval_eq(lval* x, lval* y);
#endif
