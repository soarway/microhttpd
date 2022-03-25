

#include "uhttpd.h"

static struct module {
	struct module *link;
	char *path;
	void *handle;
	handler_type handler;
} head;

extern char workdir[256] ;

handler_type open_module(const char *path_buf, const char *wkdir) 
{
	char buffer[256] = { 0 };


#ifdef  _WIN32
	GetCurrentDirectoryA(sizeof(buffer), buffer);
	printf("open module pathbuf=%s, cwd=%s", path_buf, buffer);
#else
	chdir(wkdir);
	char* pwd = getcwd(buffer, sizeof(buffer));
	report_info("open module", "workdir=%s, pathbuf=%s, cwd=%s", wkdir, path_buf, pwd);
#endif

	// Search if exists.
	struct module *curr = head.link;
	while (curr != NULL) 
	{
		if (!xstrcmp(path_buf, curr->path, MAX_PATH_LENGTH))
			return curr->handler;
		curr = curr->link;
	}

#ifdef _WIN32
	HINSTANCE  hDll = LoadLibrary(path_buf);
	if (hDll == NULL) return NULL;
#else
	// open
	char sopath[256];
	if (path_buf[0] == '.') {
		const char* s = path_buf + 1;
		snprintf(sopath, sizeof(sopath), "%s%s", pwd, s);
	}
	else {
		snprintf(sopath, sizeof(sopath), "%s%s", pwd, path_buf);
	}
	void *handle = dlopen(sopath, RTLD_NOW);
	if (handle == NULL)
	{
		report_info("open module dlopen", "dlopen fail:%s", sopath);
		return NULL;
	}
#endif
	// new module node
	struct module *new_module = xmalloc(sizeof(struct module));
	new_module->link = head.link;
	head.link = new_module;

#ifdef _WIN32
	new_module->handle = hDll;
#else
	new_module->handle = handle;

#endif
	new_module->path = xmalloc(MAX_PATH_LENGTH);
	xstrcpy(new_module->path, path_buf, MAX_PATH_LENGTH);
	
#ifdef _WIN32
	new_module->handler = (handler_type)GetProcAddress(hDll, "handler");
#else
	// get function
	new_module->handler = (handler_type)dlsym(new_module->handle, "handler");
#endif

	if (new_module->handler == NULL) {
		xfree(new_module->path);
		xfree(new_module);
		return NULL;
	}
	
	return new_module->handler;
}
