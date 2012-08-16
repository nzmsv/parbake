#ifndef _PARBAKE_DEFS_H_
#define _PARBAKE_DEFS_H_

#include "../include/parbake.h"

typedef struct parbake_line_reader *parbake_line_reader;
parbake_line_reader parbake_line_reader_create(int fd);
void parbake_line_reader_set_prefix(parbake_line_reader lr, char *prefix);
ssize_t parbake_line_reader_read(parbake_line_reader lr);
void parbake_line_reader_destroy(parbake_line_reader lr);

typedef struct parbake_poller *parbake_poller;
parbake_poller parbake_poller_create(const int * const fds, const size_t nfds);
void parbake_poller_poll(parbake_poller p);
void parbake_poller_destroy(parbake_poller p);

#define PFX "parbake: "

#endif
