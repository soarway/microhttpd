
#include "uhttpd.h"

in_addr_t server_address = INADDR_ANY;
uint16_t server_port = 80;
char *server_name = "uHttpd";
char *default_html_name = "default.html";
char *document_root = "./";

extern char workdir[256];

static void stop_process(int sig) 
{
#ifdef _WIN32
	report_fatal("stop_process()", "Signal caught, process(pid = %u) stopping",  _getpid());
#else
	report_fatal("stop_process()", "Signal '%s' caught, process(pid = %u) stopping", strsignal(sig), getpid());
#endif
}

static void show_xmval(int sig) 
{
	report_info("show_xmval()", "\nIf the worker is suspended, the following value should be 2\n#xmalloc() - #xfree() = %d", get_xmval());
}

static void worker(int client_sock, int server_sock, const char* wkdir)
{
#ifdef _WIN32
	report_info("worker()", "Worker ThreadId = %u", GetCurrentThreadId());
	//closesocket(server_sock);
#else
	report_info("worker()", "Worker forked with pid = %u", getpid());
	//close(server_sock);
#endif


#ifndef _WIN32
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR ||
		signal(SIGINT, stop_process) == SIG_ERR ||
		signal(SIGTERM, stop_process) == SIG_ERR ||
		signal(SIGTSTP, show_xmval) == SIG_ERR)
        report_fatal("worker()", "Fail to setup signals");
#endif

	while (1) 
	{
		// read packet
		packet_type *packet = read_packet(client_sock);
		if (packet == NULL)
		{
#ifdef _WIN32
			
			fprintf(stdout, "Connection closed\n");
			break;

#else
			report_fatal("worker()", "read packet Connection closed");
#endif
		}
		
		// parse request
		http_request_type *request = create_request_from_packet(packet);
		free_packet(packet);
		
		// handle request
		http_response_type *response = handle_request(request, wkdir);
		
		// create packet
		packet = create_packet_from_response(response);
		
		// write packet
		if (write_packet(client_sock, packet) == -1)
		{
#ifdef _WIN32
			
			fprintf(stdout, "Connection closed\n");
			break;
#else
			report_fatal("worker()", "write packet Connection closed");
#endif
		}
		
		// keep-alive
		if (request->connection != NULL && !xstrcmp(request->connection, "close", MAX_CONNECTION_LENGTH))
		{
#ifdef _WIN32
			
			fprintf(stdout, "Connection closed\n");
			break;
#else
			report_fatal("worker()", "close Connection closed");
#endif
		}
		
		free_packet(packet);
		free_request(request);
		free_response(response);
	}
}

typedef struct _args
{
#ifdef _WIN32
	SOCKET client_sock;
	SOCKET server_sock;
#else
	int client_sock;
	int server_sock;
#endif

}ARGS;

#ifdef _WIN32
unsigned __stdcall WorkerThread(void* arg)
{
	ARGS* parg = (ARGS*)arg;

	worker(parg->client_sock, parg->server_sock, workdir);

#ifdef _WIN32
	closesocket(parg->client_sock);
#else
	close(parg->client_sock);
#endif
	free(parg);

	return 0;
}
#endif


void start_server(const char* wkdir) 
{
#ifndef _WIN32
	// Setup signals.
	if (signal(SIGPIPE, SIG_IGN) == SIG_ERR ||
		signal(SIGINT, stop_process) == SIG_ERR ||
		signal(SIGTERM, stop_process) == SIG_ERR ||
		signal(SIGTSTP, SIG_IGN) == SIG_ERR)
        report_fatal("start_server()", "Fail to setup signals");
#endif

	// Create server socket.
	//socket(AF_INET, SOCK_STREAM, 0)

	int server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
	if (server_sock == -1)
		report_fatal("start_server()", "Fail to create server socket");
	
	int one = 1;
	if (setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one)) < 0) {
#ifdef _WIN32
		closesocket(server_sock);
#else
		close(server_sock);
#endif
		return ;
	}

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(struct sockaddr_in));

	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(server_port);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	
	
	// Bind server socket to server_address.
	if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)) == -1)
		report_fatal("start_server()", "Fail to bind server socket");
	
	// Listen on server_port.
	if (listen(server_sock, MAX_PENDING_CONNECTIONS) == -1)
		report_fatal("start_server()", "Fail to listen server socket");

	// Main loop of server.
	while (1) {
		struct sockaddr_in client_addr;
		int client_addr_len=sizeof(client_addr);
		
		// Accept a client connection.
#ifdef _WIN32
		int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_len);
#else
		int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, (socklen_t*)&client_addr_len);
#endif
		if (client_sock == -1) {
			report_error("start_server()", "Client socket is invalid");
			continue;
		}
		
#ifdef _WIN32
		HANDLE hThread;
		unsigned int threadid;
		ARGS* pargs = malloc(sizeof(ARGS));
		pargs->client_sock = client_sock;
		pargs->server_sock = server_sock;
		hThread = (HANDLE)_beginthreadex(NULL, 0, &WorkerThread, pargs, NULL, &threadid);
		CloseHandle(hThread);
#else
	if (fork() == 0) {
		// Call worker to do succeeding work.
		worker(client_sock, server_sock, wkdir);
	} else {
		// Server doesn't need talk to client.
		close(client_sock);
	}
#endif

	}
}
