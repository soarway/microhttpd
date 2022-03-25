
#ifndef UHTTPD_H_INCLUDED
#define UHTTPD_H_INCLUDED

#ifdef _WIN32
typedef unsigned long  in_addr_t;
#include <Windows.h>
#include <winsock.h>
#include <process.h>

#else
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <dlfcn.h>
#include <unistd.h>
#endif

// #define NDEBUG
#include <assert.h>

#include <errno.h>

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>


#define MAX_PENDING_CONNECTIONS 1
#define MAX_PACKET_SIZE 65536
#define MAX_INTERNAL_HTML_SIZE 1024
#define MAX_PATH_LENGTH 127
#define MAX_ARGUMENT_STRING_LENGTH 127
#define MAX_CONNECTION_LENGTH 15
#define MAX_URL_LENGTH 255
#define MAX_SUFFIX_LENGTH 8
#define MAX_METHOD_LENGTH 8
#define MAX_VERSION_LENGTH 12
#define MAX_HEADER_FIELD_LENGTH 32
#define MAX_HEADER_VALUE_LENGTH 128




extern in_addr_t server_address;
extern uint16_t server_port;
extern char *server_name;
extern char *default_html_name;
extern char *document_root;

/*
 * Logging
 */
void open_logfile(const char *);
void report_fatal(const char *, const char *, ...);
void report_error(const char *, const char *, ...);
void report_info(const char *, const char *, ...);

/*
 * Extended memory allocation functions
 */
void *xmalloc(size_t);
void xfree(void *);
// Return a value that increments if xmalloc called and decrements if xfree called
int get_xmval();

/*
 * Extended write and read functions
 */
int xwrite(int, void *, int);
int xread(int, void *, int);

/*
 * Server related
 */
void start_server(const char* dir);

/*
 * MIME type querying
 */
char *get_mime_type(char *);

/*
 * URL and arguments processor
 */
int parse_url(const char *, char *, char *, char *);

#define MAX_KEY_VALUE_LENGTH 32

typedef struct arg_node_tag {
	struct arg_node_tag *link;
	char *key;
	char *value;
} arg_node_type, *arg_list_type;

arg_list_type create_arg_list(const char *);
void free_arg_list(arg_list_type);

/*
 * module
 */
typedef size_t (*handler_type)(char *, arg_list_type);
handler_type open_module(const char *urlpath, const char* wkdir);

/*
 * Safe-version string operations
 */
char *xstrcpy(char *, const char *, int);
const char *copy_to_stop_char(char *, int, const char *, char, int);
char *empty_string(char *);
int xstrlen(const char *, int);
int xstrcmp(const char *, const char *, int);
char *xstralloc(int);

/*
 * HTTP structures and operations
 */
typedef struct http_request_tag {
	char *method;
	char *url;
	char *version;
	char *connection;
	char *content_type;
	size_t content_length;
	char *content;
} http_request_type;

typedef struct http_response_tag {
	const char *version;
	const char *code_plus_desc;
	const char *content_type;
	size_t content_length;
	const char *connection;
	char *content;
} http_response_type;

typedef struct {
	char *buf;
	int size;
} packet_type;

packet_type *read_packet(int client_sock);
http_request_type *create_request_from_packet(const packet_type *packet);
void free_packet(packet_type *packet);
http_response_type *handle_request(http_request_type *request, const char* wkdir);
packet_type *create_packet_from_response(http_response_type *response);
int write_packet(int client_sock, packet_type *packet);
void free_request(http_request_type *request);
void free_response(http_response_type *response);

#endif // UHTTPD_H_INCLUDED
