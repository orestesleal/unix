#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <string.h>

/* 
 * Process the statistics of a dial-up internet account from the etecsa 
 * isp (cuba) and calculate the used internet time for each number sharing
 * the account. This utility exists because the new web based interface
 * sucks.
 *
 * format of the message for each line:
 *
 *  17/05/2016  12:01:21    17/05/2016  12:05:40   76983777  00h:04m:19s
 *
 * the two last colums are of interest of this program.
 * the code compare the input number against the one on the current
 * record (or connection log) and if equal then process the last
 * column to gather statistics, and so on until the log file has
 * been read completely.
 *
 * the parameters expected by the program are: PHONENUMBER < logfile
 * the first argument is the phone number to get the connecton statistics
 * and the redireccion here is because it uses the Standard Input to read
 * the data, another form of calling would be:
 *     "cat logile | program_name PHONE_NUMBER" 
 * since here stdin data is coming via a pipe.
 *
 * NOTE that this piece of code expects only the aforementioned format, 
 * it's not resilient to other formats (and it's not coded nor expected to
 * do so) since this is not a critical (daemon like) running piece of code,
 * just a small utility, so if you feed the wrong input file to the program
 * and get a segfault you are on your own.
 *
 * Orestes Leal Rodriguez, May 2016, olealrd1981@gmail.com>
 *
 *     May 19    added support for query between range of days.
 *               for example: progname 76893321 10-12 < logfile will compute
 *               the internet used from day 10 to day 12 (three days) only.
 *     May 24    Add verbose output when the -v switch is passed
 */
#ifdef __GNUC__
  # define ATTR_INLINE  __attribute__ ((always_inline))
#else
  # define ATTR_INLINE
#endif

char *substrcpy (char end, char *src, char *dst);
char *copy_line (FILE *fp, char *dst);
static inline char * goto_column(char colnum, char *line) ATTR_INLINE;
void check_days_range (long *dst, char *argv);
long get_dayof_record(char *line);
void process_record (long *pmem, char *linep);


struct inet_enet
{
      long hr, mn, sc;     /* hours, minutes and seconds */
      unsigned int ncon;   /* lines (records or dial-up connections) processed */
      long from_to[2];     /* the range (if any) specifing the range of days desired to query */
} stats = { 0,0,0,0, {0,0} };


#define MAX_LINE       512
#define PHONE_LEN      8
#define LAST_COLUMN    6
#define RECORD_LEN     3
#define VERBOSE        "-v"

/*  checks if the range of days specified to make the query is
    correct, or not */
#define VALID_RANGE(s)   (s[0] >= 1 && s[0] < s[1]) && \
                          (s[1] > s[0] && s[1] <= 31)


/* check if verbosity is requested */
#define IS_VERBOSE(s1,s2) (s1 && !strncmp (s1, VERBOSE, strlen (VERBOSE))) || \
                          (s2 && !strncmp (s2, VERBOSE, strlen (VERBOSE)))

#define PRINT_IF_VERBOSE(line,s1,s2) if (IS_VERBOSE (s1, s2)) printf("%s\n", line)

/* 
   returns the address of the column specified as COLNUM in the current 
   line. NOTE that in the message format all columns are separated by 
   space and tabs, nonetheless, this code works for any whitespace
   characters for that matter.
 */

static inline char *
goto_column (char colnum, char *line)
{
    int i;

    for (i = 1; i < colnum; i++) {
        while (!isspace (*line)) line++;
        while (isspace (*line)) line++;
    }
    return line;
}


/* returns the day of logging of the current record */

long 
get_dayof_record(char *line)
{
    char day[3];

    strncpy (day, line, 2); /* copy the day of the connection */
    day[2] = '\0';
    return  strtol (day, NULL, 10);
}


/* validator for a range of days, some sanity checks, etc. */

void
check_days_range (long *dst, char *argv)
{
    char *car = argv;
    char day[3];
    int i, z;
    size_t len = strlen (argv);
 
    if (len > 5 || len < 3) return; /*  length of range is inconsistent */
    for (z = 0; z < 2 && isdigit (*car); z++)
    {
        for (i = 0; i < 2 && isdigit (*car); i++)
            day[i] = *car++;

        day[i] = '\0';
        dst[z] = strtol (day, NULL, 10);

        if (*car == '\0') break;
        car++;
    }

    if (!VALID_RANGE (dst)) {
        dst[0] = 0;
        dst[1] = 0;
    }
}


int main (int argc, char *argv[])
{
    char *linep, *ph_ptr;
    char line[MAX_LINE];

    long *pmem[3] = { &stats.hr, &stats.mn, &stats.sc };
    long *st = stats.from_to;
    double result;

    if (argc < 2) {
        printf ("usage: %s PHONE [ from-to ] < logfile\n", argv[0]);
        printf ("options enclosed in parenthesis specify a range of days which is optional\n");
        return EXIT_FAILURE;
    }

    if (argc > 2)
        check_days_range(stats.from_to, argv[2]);

    while (copy_line (stdin, line) != NULL)
    {
        linep = line;
        linep = goto_column (LAST_COLUMN, linep);
        
        /* locate the phone number */
        ph_ptr = goto_column (LAST_COLUMN-1, line);

        if (!strncmp (ph_ptr, argv[1], PHONE_LEN)) {  /* the input phone match, process the record */

            long d = get_dayof_record (line);
            
            if (VALID_RANGE (st)) {

                if (d >= st[0] && d <= st[1]) {      /* the day in the current column is in range */
                    process_record (*pmem, linep);
                    stats.ncon++;
                    PRINT_IF_VERBOSE(line, argv[2], argv[3]);
                }
            }
            else  {  /* there was no valid range, process all records for that phone */
                process_record(*pmem, linep);
                stats.ncon++;
                PRINT_IF_VERBOSE(line, argv[2], argv[3]);
            }
        }
    }

    
    result = ((double) (stats.sc / 60) + (double) stats.mn) / 60; 
    result += (double) stats.hr;
    printf ("%.2f hrs (%d connections)\n", result, stats.ncon);


    return EXIT_SUCCESS;
}


/* process a record (one dial-up connection) */

void process_record (long *pmem, char *linep)
{
    char rec[RECORD_LEN];
    char *precord;
    int i;
  
    for (precord = linep, i = 0; i < 3; i++) {

        precord = substrcpy (':', precord, rec);

        /* the declaration and use of PMEM allows to loop over hr, mn and sc 
           avoiding duplicate code like "stat.hr +=, stat.mn" for each member  */

        pmem[i] += strtol (rec, NULL, 10);
    }
}



/* copy a substring of the string pointed by SRC, until any of END, LF or '\0' is found
   returns the address of the next sub-record or a pointer to a linefeed character if 
   end of record */

char *
substrcpy (char end, char *src, char *dst)
{
    while (*src != end && *src != '\0' && *src != '\n')
        *dst++ = *src++;

    *dst = '\0';
    return (*src == '\n' ? src : ++src);
}


/* get a full line from the stream, when LineFeed is found, exit, if
   EOF is found exit too, FP can be anything that is a FILE pointer,
   like `stdin', an open file descriptor, etc. */

char *
copy_line (FILE *fp, char *dst)
{
    int c;
    char *dst_c = dst;

    while (isspace (c = getc(fp)));

    if (c == EOF) return NULL; /* signal end of file */
    
    while (c != '\n' && c != EOF)
    {
        if (c != 'h' && c != 'm' && c != 's') /* avoid the markers for h:m:s */
            *dst++ = c;
        
        c = getc(fp);
    }

    *dst = '\0';
    return dst_c;  /* return the address of the destination buffer */
}
