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
	int total_num_written = 0;
	int size = *buffer_size_left;
	
	total_num_written += snprintf(buffer + total_num_written,
								  size - total_num_written,
								  "HTTP/1.1 %s\r\n", code);
				
	total_num_written += snprintf(buffer + total_num_written,
								  size - total_num_written,
								  "Server: %s\r\n", SERVER_IP);
				
	total_num_written += snprintf(buffer + total_num_written,
								  size - total_num_written,
								  "Content-Type: %s\r\n", content_type);

	*buffer_size_left = size;

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
	int total_num_written = 0;
	int size = *buffer_size_left;

	if (content_size == -1) {
		content_size = strlen(content);
	}

	total_num_written += snprintf(buffer + total_num_written,
								  size - total_num_written,
								  "Content-Length: %d\r\n", content_size);
	
	total_num_written += snprintf(buffer + total_num_written,
								  size - total_num_written,
								  "\r\n");
	
	total_num_written += snprintf(buffer + total_num_written,
								  size - total_num_written,
								  content);
	
	total_num_written += snprintf(buffer + total_num_written,
								  size - total_num_written,
								  "\r\n");

	return buffer + total_num_written;
}