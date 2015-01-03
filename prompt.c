#include <stdio.h>

#define MAX 2048
#define VERSIONINFO "clisp version 0.0.0.1"
#define PROMPT "clisp>"
/* Buffer for user input */
static char input[MAX];

int main(int argc, char ** argv) {
	/* Version and exit info */
	puts(VERSIONINFO);
	puts("Press ctrl+C to exit\n");
	puts("kapuskondyachi goshta sangu?");

	/* continuous loop */
	while (1) {
		/* Show prompt */
		fputs(PROMPT, stdout);

		/* Read a line of user input */
		fgets(input, MAX, stdin);

		/* Echo it back - actual processing will be later added*/
		printf("%s kay mhantos, Kapuskondyachi goshta sangu?\n", input);
	}

	return 0;
}
