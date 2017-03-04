#ifndef UTILITY_H
#define UTILITY_H

void errExit(const char *format, ...);

/* Print an error message (without an 'errno' diagnostic) */
void fatal(const char *format, ...);

#endif
