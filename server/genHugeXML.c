#include <stdio.h>
#include <stdlib.h>

/* How many blocks of XML? */
#define ITERATION 1000
/* Depth of each block */
#define DEPTH 10
/* Indentation of how many characters? */
#define INDENTATION 2
/* Number of leafs */
#define NUMLEAFS 11

FILE *fd;
int indent = 1;

void
writeTags (void)
{
	int i,j;
	char *format;
	format = (char *)malloc (DEPTH*5);
	for (i=0; i < indent * INDENTATION; i++){
		format[i] = ' ';
	}

	for (j=0; j < NUMLEAFS; j++){
		sprintf (&(format[i]), "<leaf%d>JorbaTheGreek</leaf%d>%%s\n", j, j);
		fprintf (fd, (const char *)format, " ");
	}
    free (format);
}
void
writeBlocks (int blockNum)
{
	char *format;
	char tag [100];
	int i;

	format = (char *)malloc (DEPTH*5);
	while (indent <= DEPTH){
		/* put space in the format string*/
		for (i=0; i < indent * INDENTATION; i++){
            format[i] = ' ';
		}
		sprintf (&(format[i]), "<tag%d_%d>%%s\n", blockNum, indent);
		fprintf (fd, (const char *)format, " ");
		indent++;
	}
	writeTags ();
	/* write closing tags */
    indent--;
    while (indent >=1){
    	/* put space in the format string*/
    	for (i=0; i < indent * INDENTATION; i++){
           format[i] = ' ';
    	}
    	sprintf (&(format[i]), "</tag%d_%d>%%s\n", blockNum, indent);
    	fprintf (fd, (const char *)format, " ");
    	indent--;
    }
    free (format);
}

int
main (int argc, char **argv)
{
    int i, j, k;

    if ((fd = fopen ("huge.xml", "w")) == NULL){
    	printf ("Failed to open file.\n");
    	fflush (stdout);
    	return (0);
    }
    fprintf (fd, "<huge>\n");
    for (i = 0; i < ITERATION; i++){
        writeBlocks (i);
        indent = 1;
    }
    fprintf (fd, "</huge>\n");

}
