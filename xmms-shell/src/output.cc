#include <ctype.h>
#include "output.h"

void output_indented(const char *text, int start, int indent, int max, FILE *f)
{
	int i, j, pos;

	i = 0;
	pos = start;
	while(text[i]) {
		while(text[i] && isspace(text[i]))
			i++;
		if(!text[i])
			break;
		while(pos < indent) {
			fprintf(f, " ");
			pos++;
		}
		for(j = i + 1; text[j] && !isspace(text[j]); j++);
		if(pos != indent && j - i + pos + 1 >= max) {
			fprintf(f, "\n");
			pos = 0;
			continue;
		}
		if(pos != indent) {
			fprintf(f, " ");
			pos++;
		}
		pos += j - i;
		while(i < j)
			fprintf(f, "%c", text[i++]);
	}
}

