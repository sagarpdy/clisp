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

#define VERSIONINFO "clisp version 0.0.0.2"
#define PROMPT "clisp>"

int main(int argc, char ** argv) {
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
		printf("%s kay mhantos, Kapuskondyachi goshta sangu?\n", line);

		free(line);
	}

	return 0;
}
