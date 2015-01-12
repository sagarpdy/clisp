#include <stdio.h>
#include <stdlib.h>
#include <math.h>

/* If we are compiling on Windows compile these functions */
#ifdef _WIN32
#include <string.h>

static char buffer[2048];

/* Fake readline function */
char* readline(char* prompt) {
  fputs(prompt, stdout);
  fgets(buffer, 2048, stdin);
  char* cpy = malloc(strlen(buffer)+1);
  strcpy(cpy, buffer);
  cpy[strlen(cpy)-1] = '\0';
  return cpy;
}

/* Fake add_history function */
void add_history(char* unused) {}

/* Otherwise include the editline headers */
#else

#include <editline/readline.h>
#include <editline/history.h>
#endif
#include "mpc.h"

#define VERSIONINFO "clisp version 0.0.1.0"
#define PROMPT "clisp>"

enum {LVAL_NUM, LVAL_ERR, LVAL_SYM, LVAL_SEXPR };

typedef struct lval {
	int type;
	long num;

	/* Errors and symbol types as string */
	char * err;
	char * sym;

	/* Counter and pointer to lval* list */
	int count;
	struct lval ** cell;
} lval;

lval* lval_num(long x) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_NUM;
	v->num = x;
	return v;
}

lval* lval_err(char* m) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_ERR;
	v->err = malloc(strlen(m) + 1);
	strcpy(v->err, m);
	return v;
	}

lval* lval_sym(char* s) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SYM;
	v->sym = malloc(strlen(s) + 1);
	strcpy(v->sym, s);
	return v;
}

lval* lval_sexpr(void) {
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SEXPR;
	v->count = 0;
	v->cell = NULL;
	return v;
}

void lval_del(lval* v) {
	switch (v->type) {
		case LVAL_NUM: break;

		case LVAL_ERR: free(v->err); break;
		case LVAL_SYM: free(v->sym); break;

		case LVAL_SEXPR:
			for (int i = 0; i< v->count; i++)
				lval_del(v->cell[i]);
			free(v->cell);
			break;
	}
	free(v);
}

lval* lval_read_num(mpc_ast_t* t) {
	errno = 0;
	long x = strtol(t->contents, NULL, 10);
	return errno != ERANGE ? lval_num(x) : lval_err("Invalid number");
}

lval* lval_add(lval* v, lval* x) {
	if (NULL == x) return v;
	v->count++;
	v->cell = realloc(v->cell, sizeof(lval*) * v->count);
	v->cell[v->count-1] = x;
	return v;
}

lval* lval_read(mpc_ast_t* t) {
	if (strstr(t->tag, "number")) return lval_read_num(t);
	if (strstr(t->tag, "symbol")) return lval_sym(t->contents);

	/* If root(>) or sexpression then create list */
	lval* x = NULL;
	if ((strstr(t->tag, "sexpr")) || (!strcmp(t->tag, ">")))
		x = lval_sexpr();

	for (int i = 0; i < t->children_num; i++) {
		if (!strcmp(t->children[i]->contents, "(")) continue;
		if (!strcmp(t->children[i]->contents, ")")) continue;
		if (!strcmp(t->children[i]->contents, "{")) continue;
		if (!strcmp(t->children[i]->contents, "}")) continue;
		if (!strcmp(t->children[i]->contents, "regex")) continue;
		x = lval_add(x, lval_read(t->children[i]));
	}

	return x;
}

void lval_expr_print(lval* v, char open, char close);

void lval_print(lval* v) {
	switch (v->type) {
		case LVAL_NUM:
			printf("%li", v->num);
			break;
		case LVAL_ERR:
			printf("Error : %s", v->err);
			break;
		case LVAL_SYM:
			printf("%s", v->sym);
			break;
		case LVAL_SEXPR:
			lval_expr_print(v, '(', ')');
			break;
	}
}

void lval_println(lval* v) {lval_print(v);putchar('\n');}

void lval_expr_print(lval* v, char open, char close) {
	putchar(open);
	for (int i = 0; i < v->count; ++i) {
		lval_print(v->cell[i]);

		if (i != (v->count - 1))
			putchar(' ');
	}
	putchar(close);
}

lval* lval_pop(lval* v, int i) {
	lval* x = v->cell[i];
	memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * v->count - 1 - i);

	v->count--;
	v->cell = realloc(v->cell, sizeof(lval*) * v->count);
	return x;
}

lval* lval_take(lval* v, int i) {
	lval* x = lval_pop(v, i);
	lval_del(v);
	return x;
}

lval* builtin_op(lval* a, char* op) {
	for (int i = 0; i < a->count; ++i) {
		if (a->cell[i]->type != LVAL_NUM) {
			lval_del(a);
			return lval_err("Cannot operate on non number");
		}
	}

	lval* x = lval_pop(a, 0);

	/* If no args & unary operator */
	if ((!strcmp(op, "-")) && a->count == 0) {
		x->num = - x->num;
	}

	while (a->count) {
		lval* y = lval_pop(a, 0);

		if (!strcmp(op, "+")) {x->num += y->num;}
		if (!strcmp(op, "-")) {x->num -= y->num;}
		if (!strcmp(op, "*")) {x->num *= y->num;}
		if (!strcmp(op, "/")) {
			if (y->num == 0) {
				lval_del(x); lval_del(y);
				x = lval_err("Division by zero!!"); break;
			}
			x->num /= y->num;
		}
		lval_del(y);
	}
	lval_del(a);
	return x;
}

lval* lval_eval_sexpr(lval* v);
lval* lval_eval(lval* v)
{
	if (v->type == LVAL_SEXPR) return lval_eval_sexpr(v);

	return v;
}

lval* lval_eval_sexpr(lval* v) {
	for (int i = 0; i < v->count; ++i) {
		v->cell[i] = lval_eval(v->cell[i]);
	}

	for (int i = 0; i < v->count; ++i) {
		if (v->cell[i]->type == LVAL_ERR) return lval_take(v, i);
	}

	if (v->count == 0) return v;

	if (v->count == 1) return lval_take(v, 0);

	lval* f = lval_pop(v, 0);

	if (f->type != LVAL_SYM) {
		lval_del(f); lval_del(v);
		return lval_err("S-Expression doesn't start with symbol");
	}

	lval* result = builtin_op(v, f->sym);
	lval_del(f);
	return result;
}
/*
lval eval_op(lval x, char* op, lval y) {
	if (x.type == LVAL_ERR) {return x;}
	if (y.type == LVAL_ERR) {return y;}
	lval r = x;
	if (strlen(op) > 0) {
		switch (op[0]) {
			case '+':
				r = lval_num(x.num + y.num);
				break;
			case '-':
				r = lval_num(x.num - y.num);
				break;
			case '*':
				r = lval_num(x.num * y.num);
				break;
			case '/':
				r = (y.num == 0) ? lval_err(LERR_DIV_ZERO) : lval_num(x.num / y.num);
				break;
			case '%':
				r = lval_num(x.num % y.num);
				break;
			case '^':
				r = lval_num(pow(x.num,y.num));
				break;
			case 'm':
				if (strstr(op, "min")) {
					r = (x.num > y.num) ? y : x;
				} else if (strstr(op, "max")) {
					r = (x.num < y.num) ? y : x;
				}
		}
	}
	return r;
}

lval eval_mod(lval x, char* op) {
	lval r = x;
	if (strlen(op) > 0) {
		switch (op[0]) {
			case '-':
				r = lval_num(-x.num);
				break;
			case '~':
				r = lval_num(~x.num);
				break;
		}
	}
	return r;
}
lval eval(mpc_ast_t* node) {
	if (strstr(node->tag, "number")) {
		return lval_num(atoi(node->contents));
	}

	char* op = node->children[1]->contents;
	lval x = eval(node->children[2]);
	if (strstr(node->children[1]->tag, "symbol")) {
		for (int i = 3; strstr(node->children[i]->tag, "expr"); i++) {
			x = eval_op(x, op, eval(node->children[i]));
		}
	} else {
		x = eval_mod(x, op);
	}

	return x;
}
*/

int main(int argc, char ** argv) {
	/* Define language */
	mpc_parser_t* Number = mpc_new("number");
	mpc_parser_t* Symbol = mpc_new("symbol");
	mpc_parser_t* Modifier = mpc_new("modifier");
	mpc_parser_t* Sexpr = mpc_new("sexpr");
	mpc_parser_t* Expr = mpc_new("expr");
	mpc_parser_t* Clisp = mpc_new("clisp");

	mpca_lang(MPCA_LANG_DEFAULT,
			"\
				number : /-?[0-9]+/; \
				symbol : '+' | '-' | '*' | '/' | '%' | '^' | \"min\" | \"max\"; \
				modifier : '-' | '~' ; \
				sexpr : '(' <expr>* ')' ; \
				expr : <number> | <symbol> | <sexpr> ; \
				clisp : /^/ <expr>* /$/ ; \
			",
			Number, Symbol, Expr, Clisp, Modifier, Sexpr);
	/* Version and exit info */
	puts(VERSIONINFO);
	puts("Press ctrl+C to exit\n");

	/* continuous loop */
	while (1) {
		/* Output prompt & get input */
		char* line = readline(PROMPT);

		/* Add to history */
		add_history(line);

		/* Echo it back - actual processing will be later added*/
		mpc_result_t r;
		if (mpc_parse("<stdin>", line, Clisp, &r)) {
			/* Successful parsing */
			/*mpc_ast_print(r.output);
			mpc_ast_delete(r.output);*/
			/*lval_println(eval(r.output));*/
			lval* x = lval_eval(lval_read(r.output));
			lval_println(x);
			lval_del(x);
		} else {
			/* Syntax parsing */
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}

		free(line);
	}

	/* Cleanup mpc */
	mpc_cleanup(4, Number, Symbol, Expr, Clisp, Sexpr);
	return 0;
}
