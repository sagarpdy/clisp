#ifndef LENV_H
#define LENV_H

struct lval;
struct lenv;
typedef struct lval lval;
typedef struct lenv lenv;


struct lenv
{
	lenv* par;
	int count;
	char** sym;
	lval** vals;
};

lenv* lenv_new(void);

void lenv_del(lenv* e);

lenv* lenv_copy(lenv* e);

lval* lenv_get(lenv* e, lval* k);

void lenv_put(lenv* e, lval* k, lval* v);

void lenv_def(lenv* e, lval* k, lval* v);


#endif
