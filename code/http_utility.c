#include "http_utility.h"

/*
  Write a basic HTTP header in buffer (up until Content-Length):
  
  'HTTP/1.1 <code>
   Server: <SERVER_IP>
   Content-Type: <content_type>'

   Return where it stopped writing.
   
   buffer_size_left is modified to reflect the new space left in
   buffer.
 */
char *
http_header(char *buffer, int *buffer_size_left, char *code,
			char *content_type)
{
	int total_num_written = snprintf(buffer, *buffer_size_left,
									 "HTTP/1.1 %s\r\n"					\
									 "Server: %s\r\n"					\
									 "Content-Type: %s\r\n",
									 code, SERVER_IP, content_type);
				
	*buffer_size_left -= total_num_written;

	return buffer + total_num_written;
}

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
char*
http_content(char *buffer, int *buffer_size_left,
			 char *content, int content_size)
{
	/* TODO: Reduce content-size if it exceeds *buffer_size_left +
	 *       http overhead. */
	if (content_size == -1) {
		content_size = strlen(content);
	}

	int total_num_written = snprintf(buffer, *buffer_size_left,
									 "Content-Length: %d\r\n"	\
									 "\r\n"						\
									 "%s"						\
									 "\r\n" ,
									 content_size, content);

	*buffer_size_left -= total_num_written;

	return buffer + total_num_written;
}
