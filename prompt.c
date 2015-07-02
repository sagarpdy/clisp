#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "macros.h"
#include "lval.h"
#include "lenv.h"
#include "builtins.h"

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

/* Builtins */


lval* lval_eval(lenv* e, lval* v);

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
