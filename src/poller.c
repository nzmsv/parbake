#include "parbake_defs.h"
#include "emalloc.h"
#include <stdlib.h>
#include <poll.h>

struct parbake_poller {
	struct pollfd *fds;
	size_t nfds;
	size_t nactive;
	parbake_line_reader *readers;
};

parbake_poller parbake_poller_create(const int * const fds, const size_t nfds)
{
	parbake_poller p = parbake_emalloc(sizeof(struct parbake_poller));

	p->fds = parbake_emalloc(nfds * sizeof(struct pollfd));
	p->nfds = p->nactive = nfds;

	p->readers = parbake_emalloc(nfds * sizeof(parbake_line_reader));
	int i;
	for (i = 0; i < nfds; ++i) {
		p->fds[i].fd = fds[i];
		p->fds[i].events = POLLIN;
		p->fds[i].revents = 0;
		p->readers[i] = parbake_line_reader_create(fds[i]);
		//prefix output from workers with a unique ID
		char prefix[32];
		sprintf(prefix, "%u ", (unsigned int)i);
		parbake_line_reader_set_prefix(p->readers[i], prefix);
	}

	return p;
}

void parbake_poller_destroy(parbake_poller p)
{
	int i;
	for (i = 0; i < p->nfds; ++i) {
		if (p->readers[i])
			parbake_line_reader_destroy(p->readers[i]);
	}
	free(p->readers);
	free(p->fds);
	free(p);
}

void parbake_poller_poll(parbake_poller p)
{
	while (p->nactive > 0) {
		if (poll(p->fds, p->nfds, -1) < 1)
			break;

		int i;
		for (i = 0; i < p->nfds; ++i) {
			int mask = p->fds[i].revents;
			if (mask) {
				if ((mask & POLLIN) || (mask & POLLHUP)) {
					if (!parbake_line_reader_read(p->readers[i])) {
						//close on EOF
						parbake_line_reader_destroy(p->readers[i]);
						p->readers[i] = 0;
						p->fds[i].fd = -1;
						p->nactive--;
					}
					mask &= !(POLLIN | POLLHUP);
				}
				if (mask) {
					perror(PFX "poll error");
					exit(1);
				}
			}
		}
	}
}
