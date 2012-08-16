#include "parbake_defs.h"
#include <stdio.h>
#include <stdlib.h>
#include "emalloc.h"

void *parbake_emalloc(size_t size)
{
	void *p = malloc(size);
	if (!p) {
		fprintf(stderr, PFX "could not allocate (%u bytes)", (unsigned int)size);
		exit(1);
	}
	return p;
}

void *parbake_erealloc(void *ptr, size_t size)
{
	void *p = realloc(ptr, size);
	if (!p) {
		fprintf(stderr, PFX "could not reallocate (%u bytes)", (unsigned int)size);
		exit(1);
	}
	return p;
}
