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
		char *p = line;
		char* q = strchr(line, '\"');
		char out[256];
		out[0] = '\0';
		strcat(out,"\"");
		int index=1;
		while (q != NULL) {
			char found[256];
			strncpy(found, p, q-p);
			strncpy(&out[index], found, q-p);
			strncpy(&out[index+(q-p)], "\\\"", 2);
			index += (q-p + 2);
			p = q+1;
			q = strchr(p, '\"');
		}
		strcpy(&out[index], p);

		out[index+strlen(p)] = '\0';
		strcat(out,"\\n\"\n");
		fputs(out,dst);
	}
	fclose(src);
	fclose(dst);
}
