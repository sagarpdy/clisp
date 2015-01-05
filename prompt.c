#include <stdio.h>
#include <stdlib.h>

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

#define VERSIONINFO "clisp version 0.0.0.3"
#define PROMPT "clisp>"

int main(int argc, char ** argv) {
	/* Define language */
	mpc_parser_t* Number = mpc_new("number");
	mpc_parser_t* Operator = mpc_new("operator");
	mpc_parser_t* Expr = mpc_new("expr");
	mpc_parser_t* Clisp = mpc_new("clisp");

	mpca_lang(MPCA_LANG_DEFAULT,
			"\
				number : /-?[0-9]+/; \
				operator : '+' | '-' | '*' | '/' ; \
				expr : <number> | '(' <operator> <expr>+ ')' ; \
				clisp : /^/ <operator> <expr>+ /$/ ; \
			",
			Number, Operator, Expr, Clisp);
	/* Version and exit info */
	puts(VERSIONINFO);
	puts("Press ctrl+C to exit\n");
	puts("kapuskondyachi goshta sangu?");

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
			mpc_ast_print(r.output);
			mpc_ast_delete(r.output);
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
