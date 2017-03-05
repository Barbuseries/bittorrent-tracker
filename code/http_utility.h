#ifndef HTTP_UTILITY_H
#define HTTP_UTILITY_H

#include <stdio.h>
#include <string.h>

#define SERVER_IP "0.0.0.0"

char *http_header(char *buffer, int *buffer_size_left, char *code,
				  char *content_type);

char *http_content(char *buffer, int *buffer_size_left,
				   char *content, int content_size);

#endif /* HTTP_UTILITY_H */
