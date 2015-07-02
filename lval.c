#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lval.h"
#include "lenv.h"

lval* lval_num(long x)
{
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_NUM;
	v->num = x;
	return v;
}

lval* lval_err(char* fmt, ...)
{
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_ERR;
	/* initialize vararg list for initialization*/
	va_list va;
	va_start(va, fmt);
	v->err = malloc(MAX_ERR + 1);
	vsnprintf(v->err, MAX_ERR, fmt, va);
	/*realloc to actual used space */
	v->err = realloc(v->err, strlen(v->err) + 1);
	va_end(va);
	return v;
}

lval* lval_sym(char* s)
{
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SYM;
	v->sym = malloc(strlen(s) + 1);
	strcpy(v->sym, s);
	return v;
}

lval* lval_str(char* s)
{
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_STR;
	v->str = malloc(strlen(s) + 1);
	strcpy(v->str, s);
	return v;
}

lval* lval_builtin(lbuiltin fun)
{
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_FUN;
	v->builtin = fun;
	return v;
}

lenv* lenv_new(void);

lval* lval_lambda(lval* formals, lval* body)
{
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_FUN;

	v->builtin = NULL;

	/* env to store local vars of function */
	v->env = lenv_new();

	/* set formals and body */
	v->formals = formals;
	v->body = body;
	return v;
}

lval* lval_sexpr(void)
{
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_SEXPR;
	v->count = 0;
	v->cell = NULL;
	return v;
}

lval* lval_qexpr(void)
{
	lval* v = malloc(sizeof(lval));
	v->type = LVAL_QEXPR;
	v->count = 0;
	v->cell = NULL;
	return v;
}

void lval_del(lval* v)
{
	switch (v->type)
	{
	case LVAL_NUM:
		break;
	case LVAL_FUN:
		if (!v->builtin)
		{
			lenv_del(v->env);
			lval_del(v->formals);
			lval_del(v->body);
		}
		break;
	case LVAL_STR:
		free(v->str);
		break;
	case LVAL_ERR:
		free(v->err);
		break;
	case LVAL_SYM:
		free(v->sym);
		break;
	case LVAL_QEXPR:
	case LVAL_SEXPR:
		for (int i = 0; i< v->count; i++)
			lval_del(v->cell[i]);
		free(v->cell);
		break;
	}
	free(v);
}

lval* lval_copy(lval* v)
{

	lval* x = malloc(sizeof(lval));
	x->type = v->type;

	switch (v->type)
	{

		/* Copy Functions and Numbers Directly */
	case LVAL_FUN:
		if (v->builtin)
		{
			x->builtin = v->builtin;
		}
		else
		{
			x->builtin = NULL;
			x->env = lenv_copy(v->env);
			x->formals = lval_copy(v->formals);
			x->body = lval_copy(v->body);
		}
		break;
	case LVAL_NUM:
		x->num = v->num;
		break;
		/* Copy Strings using malloc and strcpy */
	case LVAL_STR:
		x->str = malloc(strlen(v->str) + 1);
		strcpy(x->str, v->str);
		break;
	case LVAL_ERR:
		x->err = malloc(strlen(v->err) + 1);
		strcpy(x->err, v->err);
		break;

	case LVAL_SYM:
		x->sym = malloc(strlen(v->sym) + 1);
		strcpy(x->sym, v->sym);
		break;

		/* Copy Lists by copying each sub-expression */
	case LVAL_SEXPR:
	case LVAL_QEXPR:
		x->count = v->count;
		x->cell = malloc(sizeof(lval*) * x->count);
		for (int i = 0; i < x->count; i++)
		{
			x->cell[i] = lval_copy(v->cell[i]);
		}
		break;
	}
	return x;
}

lval* lval_add(lval* v, lval* x)
{
	if (NULL == x) return v;
	v->count++;
	v->cell = realloc(v->cell, sizeof(lval*) * v->count);
	v->cell[v->count-1] = x;
	return v;
}

lval* lval_join(lval* x, lval* y)
{
	/* Add each element of y to x */
	for (int i = 0; i < y->count; ++i)
	{
		x = lval_add(x, y->cell[i]);
	}
	free(y->cell);
	free(y);
	return x;
}

lval* lval_pop(lval* v, int i)
{
	lval* x = v->cell[i];
	memmove(&v->cell[i], &v->cell[i+1], sizeof(lval*) * (v->count - 1 - i));

	v->count--;
	v->cell = realloc(v->cell, sizeof(lval*) * v->count);
	return x;
}

lval* lval_take(lval* v, int i)
{
	lval* x = lval_pop(v, i);
	lval_del(v);
	return x;
}

void lval_print(lval* v);


void lval_print_expr(lval* v, char open, char close)
{
	putchar(open);
	for (int i = 0; i < v->count; ++i)
	{
		lval_print(v->cell[i]);
		if (i != (v->count - 1))
		{
			putchar(' ');
		}
	}
	putchar(close);
}

void lval_print_str(lval* v)
{
	char* escaped = malloc(strlen(v->str) + 1);
	strcpy(escaped, v->str);
	escaped = mpcf_escape(escaped);
	printf("\"%s\"", escaped);
	free(escaped);
}

void lval_print(lval* v)
{
	switch (v->type)
	{
	case LVAL_FUN:
		if (v->builtin)
		{
			printf("<builtin>");
		}
		else
		{
			printf("(\\");
			lval_print(v->formals);
			putchar(' ');
			lval_print(v->body);
			putchar(')');
		}
		break;
	case LVAL_NUM:
		printf("%li", v->num);
		break;
	case LVAL_STR:
		lval_print_str(v);
		break;
	case LVAL_ERR:
		printf("Error : %s", v->err);
		break;
	case LVAL_SYM:
		printf("%s", v->sym);
		break;
	case LVAL_SEXPR:
		lval_print_expr(v, '(', ')');
		break;
	case LVAL_QEXPR:
		lval_print_expr(v, '{', '}');
		break;
	}
}

void lval_println(lval* v)
{
	lval_print(v);
	putchar('\n');
}

char* ltype_name(int t)
{
	switch (t)
	{
	case LVAL_FUN:
		return "Function";
	case LVAL_NUM:
		return "Number";
	case LVAL_STR:
		return "String";
	case LVAL_ERR:
		return "Error";
	case LVAL_SYM:
		return "Symbol";
	case LVAL_SEXPR:
		return "S-Expression";
	case LVAL_QEXPR:
		return "Q-Expression";
	default:
		return "Unknown";
	}
}

int lval_eq(lval* x, lval* y)
{

	/* Different Types are always unequal */
	if (x->type != y->type)
	{
		return 0;
	}

	/* Compare Based upon type */
	switch (x->type)
	{
		/* Compare Number Value */
	case LVAL_NUM:
		return (x->num == y->num);

		/* Compare String Values */
	case LVAL_STR:
		return (!strcmp(x->str, y->str));
	case LVAL_ERR:
		return (!strcmp(x->err, y->err));
	case LVAL_SYM:
		return (!strcmp(x->sym, y->sym));

		/* If builtin compare, otherwise compare formals and body */
	case LVAL_FUN:
		if (x->builtin || y->builtin)
		{
			return x->builtin == y->builtin;
		}
		else
		{
			return lval_eq(x->formals, y->formals)
				&& lval_eq(x->body, y->body);
		}

		/* If list compare every individual element */
	case LVAL_QEXPR:
	case LVAL_SEXPR:
		if (x->count != y->count)
		{
			return 0;
		}
		for (int i = 0; i < x->count; i++)
		{
			/* If any element not equal then whole list not equal */
			if (!lval_eq(x->cell[i], y->cell[i]))
			{
				return 0;
			}
		}
		/* Otherwise lists must be equal */
		return 1;
		break;
	}
	return 0;
}
