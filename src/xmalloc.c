
#include "uhttpd.h"

static int xmval = 0;

void *xmalloc(size_t size) {
	void *p = malloc(size);
	if (p == NULL)
		report_fatal("xmalloc()", "not enough memory");
	++xmval;

	return p;
}

void xfree(void *ptr) {
	if (ptr == NULL)
		return;
	--xmval;
	free(ptr);
}

int get_xmval() {
	return xmval;
}
