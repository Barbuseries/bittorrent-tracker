#include "utility.h"

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

void errExit(const char *format, ...)
{
    va_list argList;
    va_start(argList, format);
    vfprintf(stderr, format, argList);
    fprintf(stderr, " [ %s ] \n", strerror(errno));
    va_end(argList);

    exit(EXIT_FAILURE);
}

/* Print an error message (without an 'errno' diagnostic) */
void fatal(const char *format, ...)
{
    va_list argList;

    va_start(argList, format);
    vfprintf(stderr, format, argList);
    va_end(argList);

    exit(EXIT_FAILURE);
}

