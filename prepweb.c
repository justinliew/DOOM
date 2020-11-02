#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main()
{
	FILE *src = fopen("index.html", "r");
	FILE *dst = fopen("linuxdoom-1.10/index.html", "w");

	fseek(src, 0L, SEEK_END);
	int size = ftell(src);
	fseek(src, 0L, SEEK_SET);

	char *src_data = malloc(size);

	char line[256];

	while (fgets(line, sizeof(line), src)) {
		/* note that fgets don't strip the terminating \n, checking its
		   presence would allow to handle lines longer that sizeof(line) */
		line[strlen(line)-1] = '\0';

		/* get the first token */
		char* token = strtok(line, "\"");
		printf("first token %s\n", token);

		char out[256];
		out[0] = '\0';
		strcat(out,"\"");
		if (token != NULL) {
			strcat(out,token);

			token = strtok(NULL, "\"");
			/* walk through other tokens */
			while( token != NULL ) {
				strcat(out, "\\\"");
				strcat(out, token);

				token = strtok(NULL, "\"");
			}
		}
		strcat(out,"\\n\"\n");
		printf("end; outputting %s\n", out);

		fputs(out,dst);
	}
	fclose(src);
	fclose(dst);
}
