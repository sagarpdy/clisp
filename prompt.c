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

long eval_op(long x, char* op, long y) {
	long r = x;
	if (strlen(op) > 0) {
		switch (op[0]) {
			case '+':
				r = x + y;
				break;
			case '-':
				r = x - y;
				break;
			case '*':
				r = x * y;
				break;
			case '/':
				r = x / y;
				break;
			case '%':
				r = x % y;
				break;
			case '^':
				r = pow(x, y);
				break;
			case 'm':
				if (strstr(op, "min")) {
					r = (x > y) ? y : x;
				} else if (strstr(op, "max")) {
					r = (x < y) ? y : x;
				}
		}
	}
	return r;
}

long eval_mod(long x, char* op) {
	long r = x;
	if (strlen(op) > 0) {
		switch (op[0]) {
			case '-':
				r = -x;
				break;
			case '~':
				r = ~x;
				break;
		}
	}
	return r;
}

long eval(mpc_ast_t* node) {
	if (strstr(node->tag, "number")) {
		return atoi(node->contents);
	}

	char* op = node->children[1]->contents;
	long x = eval(node->children[2]);
	if (strstr(node->children[1]->tag, "operator")) {
		for (int i = 3; strstr(node->children[i]->tag, "expr"); i++) {
			x = eval_op(x, op, eval(node->children[i]));
		}
	} else {
		x = eval_mod(x, op);
	}

	return x;
}

int main(int argc, char ** argv) {
	/* Define language */
	mpc_parser_t* Number = mpc_new("number");
	mpc_parser_t* Operator = mpc_new("operator");
	mpc_parser_t* Modifier = mpc_new("modifier");
	mpc_parser_t* Expr = mpc_new("expr");
	mpc_parser_t* Clisp = mpc_new("clisp");

	mpca_lang(MPCA_LANG_DEFAULT,
			"\
				number : /-?[0-9]+/; \
				operator : '+' | '-' | '*' | '/' | '%' | '^' | \"min\" | \"max\"; \
				modifier : '-' | '~' ; \
				expr : <number> | '(' <operator> <expr> <expr>+ ')' | '(' <modifier> <expr> ')' ; \
				clisp : /^/ <operator> <expr> <expr>+ /$/ | /^/ <modifier> <expr> /$/ ; \
			",
			Number, Operator, Expr, Clisp, Modifier);
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
			printf("%li\n", eval(r.output));
		} else {
			/* Syntax parsing */
			mpc_err_print(r.error);
			mpc_err_delete(r.error);
		}

		free(line);
	}

	/* Cleanup mpc */
	mpc_cleanup(4, Number, Operator, Expr, Clisp);
	return 0;
}
