#include "parbake_defs.h"
#include "emalloc.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

#include <limits.h>
#define BUFSIZE 2048
#if (BUFSIZE > PIPE_BUF)
#undef BUFSIZE
#define BUFSIZE PIPE_BUF
#endif

struct parbake_line_reader {
	char *prefix;
	int fd;
	size_t buf_size;
	size_t buf_len;
	char *buf;
};

static void unblock_fd(const int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

parbake_line_reader parbake_line_reader_create(int fd)
{
	struct parbake_line_reader *lr = parbake_emalloc(sizeof(struct parbake_line_reader));

	lr->prefix = 0;
	lr->fd = fd;
	unblock_fd(fd);
	lr->buf = 0;
	lr->buf_size = 0;
	lr->buf_len = 0;
	return lr;
}

void parbake_line_reader_destroy(parbake_line_reader lr)
{
	close(lr->fd);
	fwrite(lr->buf, 1, lr->buf_len, stdout);
	free(lr->buf);
	free(lr);
}

void parbake_line_reader_set_prefix(parbake_line_reader lr, char *prefix)
{
	size_t len = strnlen(prefix, PARBAKE_PREFIX_LEN);
	if (len < PARBAKE_PREFIX_LEN) {
		++len;
		lr->prefix = parbake_erealloc(lr->prefix, len);
		memcpy(lr->prefix, prefix, len);
	}
	else {
		fprintf(stderr, PFX "prefix length exceeded\n");
		exit(1);
	}
}

ssize_t parbake_line_reader_read(parbake_line_reader lr)
{
	// get more space if necessary
	if (lr->buf_len == lr->buf_size) {
		lr->buf = parbake_erealloc(lr->buf, lr->buf_size + BUFSIZE + 1);
		lr->buf_size += BUFSIZE;
	}

	ssize_t n = read(lr->fd, lr->buf + lr->buf_len, lr->buf_size - lr->buf_len);
	if (n < 0) {
		switch (errno) {
		case EINTR:
		case EAGAIN:
#if (EAGAIN != EWOULDBLOCK)
		case EWOULDBLOCK:
#endif
			break;
		default:
			perror(PFX "read error");
			exit(1);
		}
	}
	else if (n) {
		lr->buf_len += n;
		lr->buf[lr->buf_len] = '\0';

		size_t printed = 0;
		char *start = lr->buf;
		while (1) {
			char *end = strchr(start, '\n');
			if (!end) break;
			++end;
			size_t span = end - start;
			if (lr->prefix)
				printf(lr->prefix);
			if (fwrite(start, 1, span, stdout) < span) {
				perror(PFX "output error");
				exit(1);
			}
			start += span;
			printed += span;
		}

		if (printed) {
			lr->buf_len -= printed;
			memmove(lr->buf, start, lr->buf_len);
			//shrink buffer if possible
			if (lr->buf_len < BUFSIZE) {
				lr->buf = parbake_erealloc(lr->buf, BUFSIZE + 1);
				lr->buf_size = BUFSIZE;
			}
		}
	}

	return n;
}
