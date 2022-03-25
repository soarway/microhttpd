
#include "uhttpd.h"

int xwrite(int fd, void *buf, int count) 
{
	assert(buf != NULL);

	int bytes_write;
	int bytes_left = count;
	char *ptr = (char *)buf;
	while (bytes_left > 0) {
		bytes_write = send(fd, ptr, bytes_left, 0);
		if (bytes_write < 1) {
			// either EINTR or other error
			if (errno == EINTR) {
				// the call was interrupted before writing any data
				bytes_write = 0;
			} else {
				return -1;
			}
		}
		ptr += bytes_write;
		bytes_left -= bytes_write;
	}
	
	return count-bytes_left;
}

int xread(int fd, void *buf, int max) 
{
	assert(buf != NULL);
	
	int bytes_read = recv(fd, buf, max, 0);
	while (bytes_read == -1) {
		if (errno == EINTR)
			bytes_read = recv(fd, buf, max, 0);
		else
			return -1;
	}
	
	return bytes_read;
}
