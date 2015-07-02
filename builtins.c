#include <string.h>

#include "builtins.h"
#include "macros.h"

lval* builtin_lambda(lenv* e, lval* a)
{
	/* \ {x y} {+ x y}*/
	/* Need 2 args, both Q-Expr */
	LASSERT_NUM("\\", a, 2);
	LASSERT_TYPE("\\", a, 0, LVAL_QEXPR);
	LASSERT_TYPE("\\", a, 1, LVAL_QEXPR);

	/* First Q-EXPR is set of params / formals, check it contains only symbols */
	for (int i = 0; i < a->cell[0]->count; ++i)
	{
		LASSERT(a, (a->cell[0]->cell[i]->type == LVAL_SYM),
				"Cannot define non-symbol. Got %s, Expected %s.",
				ltype_name(a->cell[0]->cell[i]->type),ltype_name(LVAL_SYM));
	}

	/* Set formals and body*/
	lval* formals = lval_pop(a, 0);
	lval* body = lval_pop(a, 0);
	lval_del(a);

	return lval_lambda(formals, body);
}

lval* builtin_list(lenv* e, lval* a)
{
	a->type = LVAL_QEXPR;
	return a;
}

lval* builtin_head(lenv* e, lval* a)
{
	/*Check error conditions */
	LASSERT_NUM("head", a, 1);
	LASSERT_TYPE("head", a, 0, LVAL_QEXPR);
	LASSERT_NOT_EMPTY("head", a, 0);

	/* Otherwise take first argument */
	lval* v = lval_take(a, 0);

	/* Delete rest of elements in list */
	while (v->count > 1)
	{
		lval_del(lval_pop(v, 1));
	}
	return v;
}

lval* builtin_tail(lenv* e, lval* a)
{
	/*Check error conditions */
	LASSERT_NUM("tail", a, 1);
	LASSERT_TYPE("tail", a, 0, LVAL_QEXPR);
	LASSERT_NOT_EMPTY("tail", a, 0);

	lval* v = lval_take(a, 0);
	/*Delete first element and return*/
	lval_del(lval_pop(v, 0));
	return v;
}

lval* builtin_eval(lenv* e, lval* a)
{
	/*Check error conditions */
	LASSERT_NUM("eval", a, 1);
	LASSERT_TYPE("eval", a, 0, LVAL_QEXPR);

	lval* x = lval_take(a, 0);
	x->type = LVAL_SEXPR;
	return lval_eval(e, x);
}

lval* builtin_join(lenv* e, lval* a)
{

	for (int i = 0; i < a->count; ++i)
	{
		LASSERT_TYPE("join", a, i, LVAL_QEXPR);
	}

	lval* x = lval_pop(a, 0);

	while (a->count)
	{
		lval* y = lval_pop(a, 0);
		x = lval_join(x, y);
	}

	lval_del(a);
	return x;
}

lval* builtin_op(lenv*e, lval* a, char* op)
{

	for (int i = 0; i < a->count; ++i)
	{
		LASSERT_TYPE(op, a, i, LVAL_NUM);
	}

	lval* x = lval_pop(a, 0);

	/* If no args & unary operator */
	if ((!strcmp(op, "-")) && a->count == 0)
	{
		x->num = - x->num;
	}

	while (a->count)
	{
		lval* y = lval_pop(a, 0);

		if (!strcmp(op, "+"))
		{
			x->num += y->num;
		}
		if (!strcmp(op, "-"))
		{
			x->num -= y->num;
		}
		if (!strcmp(op, "*"))
		{
			x->num *= y->num;
		}
		if (!strcmp(op, "/"))
		{
			if (y->num == 0)
			{
				lval_del(x);
				lval_del(y);
				x = lval_err("Division by zero!!");
				break;
			}
			x->num /= y->num;
		}

		lval_del(y);
	}

	lval_del(a);
	return x;
}

lval* builtin_add(lenv* e, lval* a)
{
	return builtin_op(e, a, "+");
}
lval* builtin_sub(lenv* e, lval* a)
{
	return builtin_op(e, a, "-");
}
lval* builtin_mul(lenv* e, lval* a)
{
	return builtin_op(e, a, "*");
}
lval* builtin_div(lenv* e, lval* a)
{
	return builtin_op(e, a, "/");
}

lval* builtin_ord(lenv* e, lval* a, char* func)
{
	LASSERT_NUM(func, a, 2);
	LASSERT_TYPE(func, a, 0, LVAL_NUM);
	LASSERT_TYPE(func, a, 1, LVAL_NUM);

	int result = 0;
	if (!strcmp(func, ">"))
	{
		result = (a->cell[0]->num > a->cell[1]->num);
	}
	else if (!strcmp(func, "<"))
	{
		result = (a->cell[0]->num < a->cell[1]->num);
	}
	else if (!strcmp(func, ">="))
	{
		result = (a->cell[0]->num >= a->cell[1]->num);
	}
	else if (!strcmp(func, "<="))
	{
		result = (a->cell[0]->num <= a->cell[1]->num);
	}
	lval_del(a);
	return lval_num(result);
}

lval* builtin_gt(lenv* e, lval* a)
{
	return builtin_ord(e, a, ">");
}

lval* builtin_lt(lenv* e, lval* a)
{
	return builtin_ord(e, a, "<");
}

lval* builtin_ge(lenv* e, lval* a)
{
	return builtin_ord(e, a, ">=");
}

lval* builtin_le(lenv* e, lval* a)
{
	return builtin_ord(e, a, "<=");
}

lval* builtin_cmp(lenv* e, lval* a, char* op)
{
	LASSERT_NUM(op, a, 2);
	int r;
	if (strcmp(op, "==") == 0)
	{
		r =  lval_eq(a->cell[0], a->cell[1]);
	}
	if (strcmp(op, "!=") == 0)
	{
		r = !lval_eq(a->cell[0], a->cell[1]);
	}
	lval_del(a);
	return lval_num(r);
}

lval* builtin_eq(lenv* e, lval* a)
{
	return builtin_cmp(e, a, "==");
}

lval* builtin_ne(lenv* e, lval* a)
{
	return builtin_cmp(e, a, "!=");
}

lval* builtin_if(lenv* e, lval* a)
{
	LASSERT_NUM("if", a, 3);
	LASSERT_TYPE("if", a, 0, LVAL_NUM);
	LASSERT_TYPE("if", a, 1, LVAL_QEXPR);
	LASSERT_TYPE("if", a, 2, LVAL_QEXPR);

	/* Mark Both Expressions as evaluable */
	lval* x;
	a->cell[1]->type = LVAL_SEXPR;
	a->cell[2]->type = LVAL_SEXPR;

	if (a->cell[0]->num)
	{
		/* If condition is true evaluate first expression */
		x = lval_eval(e, lval_pop(a, 1));
	}
	else
	{
		/* Otherwise evaluate second expression */
		x = lval_eval(e, lval_pop(a, 2));
	}

	/* Delete argument list and return */
	lval_del(a);
	return x;
}

lval* builtin_var(lenv* e, lval* a, char* func)
{
	LASSERT_TYPE(func, a, 0, LVAL_QEXPR);

	/* First arg is symbol list */
	lval* syms = a->cell[0];
	/*Ensure all elements of first list are symbols*/
	for (int i = 0; i < syms->count; ++i)
	{
		LASSERT(a, (syms->cell[i]->type == LVAL_SYM),
				"Function '%s' cannot define non-symbol. "
				"Got %s, Expected %s.", func,
				ltype_name(syms->cell[i]->type), ltype_name(LVAL_SYM));
	}

	/*Check correct number of symbols and values*/
	LASSERT(a, (syms->count == a->count-1),
			"Function '%s' passed too many arguments for symbols. Got %i, Expected %i.",
			func, syms->count, a->count-1);

	/*Assign copies of values to symbols*/
	for (int i = 0; i < syms->count; ++i)
	{
		if (!strcmp(func, "def"))
		{
			lenv_def(e, syms->cell[i], a->cell[1 + i]);
		}

		else if (!strcmp(func, "="))
		{
			lenv_put(e, syms->cell[i], a->cell[1 + i]);
		}
	}

	lval_del(a);
	return lval_sexpr();
}

lval* builtin_def(lenv* e, lval* a)
{
	return builtin_var(e, a, "def");
}

lval* builtin_put(lenv* e, lval* a)
{
	return builtin_var(e, a, "=");
}


/* Evaluation */

lval* lval_call(lenv* e, lval* f, lval* a)
{

	/* If Builtin then simply apply that */
	if (f->builtin)
	{
		return f->builtin(e, a);
	}

	/* Record Argument Counts */
	int given = a->count;
	int total = f->formals->count;

	/* While arguments still remain to be processed */
	while (a->count)
	{

		/* If we've ran out of formal arguments to bind */
		if (f->formals->count == 0)
		{
			lval_del(a);
			return lval_err(
					"Function passed too many arguments. "
					"Got %i, Expected %i.", given, total);
		}

		/* Pop the first symbol from the formals */
		lval* sym = lval_pop(f->formals, 0);

		/* Special Case to deal with '&' */
		if (strcmp(sym->sym, "&") == 0)
		{

			/* Ensure '&' is followed by another symbol */
			if (f->formals->count != 1)
			{
				lval_del(a);
				return lval_err("Function format invalid. "
						"Symbol '&' not followed by single symbol.");
			}

			/* Next formal should be bound to remaining arguments */
			lval* nsym = lval_pop(f->formals, 0);
			lenv_put(f->env, nsym, builtin_list(e, a));
			lval_del(sym);
			lval_del(nsym);
			break;
		}

		/* Pop the next argument from the list */
		lval* val = lval_pop(a, 0);

		/* Bind a copy into the function's environment */
		lenv_put(f->env, sym, val);

		/* Delete symbol and value */
		lval_del(sym);
		lval_del(val);
	}

	/* Argument list is now bound so can be cleaned up */
	lval_del(a);

	/* If '&' remains in formal list bind to empty list */
	if (f->formals->count > 0 &&
			strcmp(f->formals->cell[0]->sym, "&") == 0)
	{

		/* Check to ensure that & is not passed invalidly. */
		if (f->formals->count != 2)
		{
			return lval_err("Function format invalid. "
					"Symbol '&' not followed by single symbol.");
		}

		/* Pop and delete '&' symbol */
		lval_del(lval_pop(f->formals, 0));

		/* Pop next symbol and create empty list */
		lval* sym = lval_pop(f->formals, 0);
		lval* val = lval_qexpr();

		/* Bind to environment and delete */
		lenv_put(f->env, sym, val);
		lval_del(sym);
		lval_del(val);
	}

	/* If all formals have been bound evaluate */
	if (f->formals->count == 0)
	{

		/* Set environment parent to evaluation environment */
		f->env->par = e;

		/* Evaluate and return */
		return builtin_eval(
				f->env, lval_add(lval_sexpr(), lval_copy(f->body)));
	}
	else
	{
		/* Otherwise return partially evaluated function */
		return lval_copy(f);
	}

}

lval* lval_eval_sexpr(lenv* e, lval* v)
{
	for (int i = 0; i < v->count; ++i)
	{
		v->cell[i] = lval_eval(e, v->cell[i]);
	}

	for (int i = 0; i < v->count; ++i)
	{
		if (v->cell[i]->type == LVAL_ERR) return lval_take(v, i);
	}

	if (v->count == 0) return v;

	if (v->count == 1) return lval_take(v, 0);

	lval* f = lval_pop(v, 0);
	if (f->type != LVAL_FUN)
	{
		lval* err = lval_err(
				"S-Expression starts with incorrect type. "
				"Got %s, Expected %s.",
				ltype_name(f->type), ltype_name(LVAL_FUN));
		lval_del(f);
		lval_del(v);
		return err;
	}

	lval* result = lval_call(e, f, v);
	lval_del(f);
	return result;
}

lval* lval_eval(lenv* e, lval* v)
{
	if (v->type == LVAL_SYM)
	{
		lval* x = lenv_get(e, v);
		lval_del(v);
		return x;
	}

	if (v->type == LVAL_SEXPR) return lval_eval_sexpr(e, v);

	return v;
}

/*Reading*/

lval* lval_read_num(mpc_ast_t* t)
{
	errno = 0;
	long x = strtol(t->contents, NULL, 10);
	return errno != ERANGE ? lval_num(x) : lval_err("Invalid number");
}

lval* lval_read_str(mpc_ast_t* t)
{
	/* Cut off the final quote character */
	t->contents[strlen(t->contents)-1] = '\0';
	/* Copy the string missing out the first quote character */
	char* unescaped = malloc(strlen(t->contents+1)+1);
	strcpy(unescaped, t->contents+1);
	/* Pass through the unescape function */
	unescaped = mpcf_unescape(unescaped);
	/* Construct a new lval using the string */
	lval* str = lval_str(unescaped);
	/* Free the string and return */
	free(unescaped);
	return str;
}

lval* lval_read(mpc_ast_t* t)
{

	if (strstr(t->tag, "number")) return lval_read_num(t);
	if (strstr(t->tag, "symbol")) return lval_sym(t->contents);
	if (strstr(t->tag, "string"))
	{
		return lval_read_str(t);
	}


	/* If root(>) or sexpression then create list */
	lval* x = NULL;
	if ((strstr(t->tag, "sexpr")) || (!strcmp(t->tag, ">")))
		x = lval_sexpr();
	if (strstr(t->tag, "qexpr"))
	{
		x = lval_qexpr();
	}

	for (int i = 0; i < t->children_num; i++)
	{
		if (!strcmp(t->children[i]->contents, "(")) continue;
		if (!strcmp(t->children[i]->contents, ")")) continue;
		if (!strcmp(t->children[i]->contents, "{")) continue;
		if (!strcmp(t->children[i]->contents, "}")) continue;
		if (strstr(t->children[i]->tag, "comment")) continue;
		if (!strcmp(t->children[i]->tag, "regex")) continue;
		x = lval_add(x, lval_read(t->children[i]));
	}

	return x;
}
