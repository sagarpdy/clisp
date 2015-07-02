#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lval.h"
#include "lenv.h"

lenv* lenv_new(void)
{
	lenv* e = malloc(sizeof(lenv));
	e->par = NULL;
	e->count = 0;
	e->sym = NULL;
	e->vals = NULL;
	return e;
}

void lenv_del(lenv* e)
{
	for (int i = 0; i < e->count; ++i)
	{
		free(e->sym[i]);
		lval_del(e->vals[i]);
	}
	free(e->sym);
	free(e->vals);
	free(e);
}

lenv* lenv_copy(lenv* e)
{
	lenv* n = malloc(sizeof(lenv));
	n->par = e->par;
	n->count = e->count;
	n->sym = malloc(sizeof(char*) * n->count);
	n->vals = malloc(sizeof(lval*) * n->count);
	for (int i = 0; i < e->count; i++)
	{
		n->sym[i] = malloc(strlen(e->sym[i]) + 1);
		strcpy(n->sym[i], e->sym[i]);
		n->vals[i] = lval_copy(e->vals[i]);
	}
	return n;
}

lval* lenv_get(lenv* e, lval* k)
{
	for (int i = 0; i < e->count; ++i)
	{
		if (!strcmp(e->sym[i], k->sym))
		{
			return lval_copy(e->vals[i]);
		}
	}

	if (e->par)
	{
		return lenv_get(e->par, k);
	}
	else
	{
		return lval_err("Unbound Symbol '%s'", k->sym);
	}
}

void lenv_put(lenv* e, lval* k, lval* v)
{
	for (int i = 0; i < e->count; ++i)
	{
		if ((!strcmp(e->sym[i], k->sym)))
		{
			lval_del(e->vals[i]);
			e->vals[i] = lval_copy(v);
			return;
		}
	}

	e->count++;
	e->vals = realloc(e->vals, sizeof(lval*) * e->count);
	e->sym = realloc(e->sym, sizeof(char*) * e->count);
	e->vals[e->count - 1] = lval_copy(v);
	e->sym[e->count - 1] = malloc(strlen(k->sym) + 1);
	strcpy(e->sym[e->count - 1], k->sym);
}

void lenv_def(lenv* e, lval* k, lval* v)
{
	while (e->par)
	{
		e = e->par;
	}

	lenv_put(e, k, v);
}
