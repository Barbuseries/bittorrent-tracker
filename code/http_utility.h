#ifndef HTTP_UTILITY_H
#define HTTP_UTILITY_H

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"

/*
  Write a basic HTTP header in buffer (up until Content-Length):
  
  'HTTP/1.1 <code>
   Server: <SERVER_IP>
   Content-Type: <content_type>'

   Return where it stopped writing.
   
   buffer_size_left is modified to reflect the new space left in
   buffer.
 */
char *http_header(char *buffer, int *buffer_size_left, char *code,
				  char *content_type);

/*
  Write content in buffer (in HTTP format).
  
  'Content_length: <content_size>

  <content>
  '
  
  If content_size is -1, set content_size to strlen(content).
  
  Return where it stopped writing.
   
  buffer_size_left is modified to reflect the new space left in
  buffer.
 */
char *http_content(char *buffer, int *buffer_size_left,
				   char *content, int content_size);

/*
  Only send a response code (HTTP header with no content).
 */
void http_send_code(int fd, char *error_code);

#endif /* HTTP_UTILITY_H */
