#include "http_utility.h"

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
									 "\r\n",
									 content_size, content);

	*buffer_size_left -= total_num_written;
	return buffer + total_num_written;
}

void
http_send_code(int fd, char *error_code) {
	char http_response_buffer[255];
			
	char *response_write_pos = http_response_buffer;
	int response_len_left = sizeof(http_response_buffer);
			
	response_write_pos = http_header(response_write_pos, &response_len_left, error_code, "text/plain");
	response_write_pos = http_content(response_write_pos, &response_len_left, "", -1);

	write(fd, http_response_buffer, response_write_pos - http_response_buffer);
}
