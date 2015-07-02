#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "macros.h"
#include "lval.h"
#include "lenv.h"

/* If we are compiling on Windows compile these functions */
#ifdef _WIN32
#include <string.h>

static char buffer[2048];

/* Fake readline function */
char* readline(char* prompt)
{
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

#define VERSIONINFO "clisp version 1.0.0.0"
#define PROMPT "clisp>"

mpc_parser_t* Number;
mpc_parser_t* Symbol;
mpc_parser_t* String;
mpc_parser_t* Comment;
mpc_parser_t* Modifier;
mpc_parser_t* Sexpr;
mpc_parser_t* Qexpr;
mpc_parser_t* Expr;
mpc_parser_t* Clisp;

struct lval;
struct lenv;



lenv* lenv_new(void);
void lenv_del(lenv* e);
lenv* lenv_copy(lenv* e);
void lenv_put(lenv* e, lval* k, lval* v);
lval* lenv_get(lenv* e, lval* k);

/* Builtins */


lval* lval_eval(lenv* e, lval* v);

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

lval* lval_read(mpc_ast_t* t);
lval* builtin_load(lenv* e, lval* a)
{
	LASSERT_NUM("load", a, 1);
	LASSERT_TYPE("load", a, 0, LVAL_STR);

	/* Parse File given by string name */
	mpc_result_t r;
	if (mpc_parse_contents(a->cell[0]->str, Clisp, &r))
	{

		/* Read contents */
		lval* expr = lval_read(r.output);
		mpc_ast_delete(r.output);

		/* Evaluate each Expression */
		while (expr->count)
		{
			lval* x = lval_eval(e, lval_pop(expr, 0));
			/* If Evaluation leads to error print it */
			if (x->type == LVAL_ERR)
			{
				lval_println(x);
			}
			lval_del(x);
		}

		/* Delete expressions and arguments */
		lval_del(expr);
		lval_del(a);

		/* Return empty list */
		return lval_sexpr();

	}
	else
	{
		/* Get Parse Error as String */
		char* err_msg = mpc_err_string(r.error);
		mpc_err_delete(r.error);

		/* Create new error message using it */
		lval* err = lval_err("Could not load Library %s", err_msg);
		free(err_msg);
		lval_del(a);

		/* Cleanup and return error */
		return err;
	}
}

lval* builtin_error(lenv* e, lval* a)
{
	LASSERT_NUM("error", a, 1);
	LASSERT_TYPE("error", a, 0, LVAL_STR);

	/* Construct Error from first argument */
	lval* err = lval_err(a->cell[0]->str);

	/* Delete arguments and return */
	lval_del(a);
	return err;
}

lval* builtin_print(lenv* e, lval* a)
{

	/* Print each argument followed by a space */
	for (int i = 0; i < a->count; i++)
	{
		lval_print(a->cell[i]);
		putchar(' ');
	}

	/* Print a newline and delete arguments */
	putchar('\n');
	lval_del(a);

	return lval_sexpr();
}

void lenv_add_builtin(lenv* e, char* name, lbuiltin func)
{
	lval* k = lval_sym(name);
	lval* v = lval_builtin(func);
	lenv_put(e, k, v);
	lval_del(k);
	lval_del(v);
}

void lenv_add_builtins(lenv* e)
{
	/* Vailable functions */
	lenv_add_builtin(e, "\\", builtin_lambda);
	lenv_add_builtin(e, "def", builtin_def);
	lenv_add_builtin(e, "=",   builtin_put);

	/* List Functions */
	lenv_add_builtin(e, "list", builtin_list);
	lenv_add_builtin(e, "head", builtin_head);
	lenv_add_builtin(e, "tail", builtin_tail);
	lenv_add_builtin(e, "eval", builtin_eval);
	lenv_add_builtin(e, "join", builtin_join);

	/* Mathematical Functions */
	lenv_add_builtin(e, "+", builtin_add);
	lenv_add_builtin(e, "-", builtin_sub);
	lenv_add_builtin(e, "*", builtin_mul);
	lenv_add_builtin(e, "/", builtin_div);

	/* Comparison Functions */
	lenv_add_builtin(e, "if", builtin_if);
	lenv_add_builtin(e, "==", builtin_eq);
	lenv_add_builtin(e, "!=", builtin_ne);
	lenv_add_builtin(e, ">",  builtin_gt);
	lenv_add_builtin(e, "<",  builtin_lt);
	lenv_add_builtin(e, ">=", builtin_ge);
	lenv_add_builtin(e, "<=", builtin_le);

	/* String Functions */
	lenv_add_builtin(e, "load",  builtin_load);
	lenv_add_builtin(e, "error", builtin_error);
	lenv_add_builtin(e, "print", builtin_print);
	/* Shell Functions * /
	   lenv_add_builtin(e, "exit", builtin_exit);*/
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

/* Main */
int main(int argc, char ** argv)
{
	/* Define language */
	Number = mpc_new("number");
	Symbol = mpc_new("symbol");
	String = mpc_new("string");
	Comment = mpc_new("comment");
	Modifier = mpc_new("modifier");
	Sexpr = mpc_new("sexpr");
	Qexpr = mpc_new("qexpr");
	Expr = mpc_new("expr");
	Clisp = mpc_new("clisp");

	mpca_lang(MPCA_LANG_DEFAULT,
			"\
			number : /-?[0-9]+/; \
			symbol : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ; \
			string  : /\"(\\\\.|[^\"])*\"/ ; \
			modifier : '-' | '~' ; \
			sexpr : '(' <expr>* ')' ; \
			qexpr : '{' <expr>* '}' ; \
			comment : /;[^\\r\\n]*/ ; \
			expr : <number> | <symbol> | <sexpr> | \
			<qexpr> | <string> | <comment>; \
			clisp : /^/ <expr>* /$/ ; \
			",
			Number, Symbol, String, Comment, Expr, Clisp, Modifier, Sexpr, Qexpr);
	/* Version and exit info */
	puts(VERSIONINFO);
	puts("Enter exit () to exit\n");

	lenv* e = lenv_new();
	lenv_add_builtins(e);

	/* Supplied with list of files */
	if (argc >= 2)
	{

		/* loop over each supplied filename (starting from 1) */
		for (int i = 1; i < argc; i++)
		{

			/* Argument list with a single argument, the filename */
			lval* args = lval_add(lval_sexpr(), lval_str(argv[i]));

			/* Pass to builtin load and get the result */
			lval* x = builtin_load(e, args);

			/* If the result is an error be sure to print it */
			if (x->type == LVAL_ERR)
			{
				lval_println(x);
			}
			lval_del(x);
		}
	}

	/* continuous loop */
	while (1)
	{
		/* Output prompt & get input */
		char* line = readline(PROMPT);

		/* Add to history */
		add_history(line);

		/* Echo it back - actual processing will be later added*/
		mpc_result_t r;
		if (mpc_parse("<stdin>", line, Clisp, &r))
		{
			/* Successful parsing */
			/*mpc_ast_print(r.output);
			  mpc_ast_delete(r.output);*/
			/*lval_println(eval(r.output));*/
			lval* x = lval_eval(e, lval_read(r.output));
			if (x)
			{
				lval_println(x);
				lval_del(x);
			}
			else
			{
				printf("Bye Bye\n");
				free(line);
				break;
			}
		}
		else
		{
			/* Syntax parsing */
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}

		if (line) free(line);
	}

	lenv_del(e);
	/* Cleanup mpc */
	mpc_cleanup(8, Number, Symbol, Expr, Clisp, Sexpr, Qexpr, String, Comment);
	return 0;
}




lval* builtin_exit(lenv* e, lval* a)
{
	return NULL;
}
