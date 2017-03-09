#ifndef HTTP_UTILITY_H
#define HTTP_UTILITY_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"

char *http_header(char *buffer, int *buffer_size_left, char *code,
				  char *content_type);

char *http_content(char *buffer, int *buffer_size_left,
				   char *content, int content_size);

void http_send_code(int fd, char *error_code);

#endif /* HTTP_UTILITY_H */
