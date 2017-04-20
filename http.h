
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffer.h"

#ifdef _WIN32
#include "platform/windows.h"
#else
#include "platform/unix.h"
#endif

static int socket_close(Socket s)
{
	int ok = 0;
#ifdef _WIN32
	ok = shutdown(s, SD_BOTH);
	if (ok == 0) {
		ok = closesocket(s);
	}
#else
	ok = shutdown(s, SHUT_RDWR);
	if (ok == 0) {
		ok = close(s);
	}
#endif
	return ok;
}

typedef struct {
	Socket s;
} HttpListener;

static int http_init(void)
{
#ifdef _WIN32
	WSADATA wsa_data;
	return WSAStartup(MAKEWORD(2, 2), &wsa_data);
#else
	return 0;
#endif
}
static int http_quit(void)
{
#ifdef _WIN32
	return WSACleanup();
#else
	return 0;
#endif
}

typedef enum {
	HttpMethod_Get,
	HttpMethod_Post,
	HttpMethod_Head,
	HttpMethod_Put
} HttpMethod;

typedef struct {
	HttpMethod method;
	Buffer url;
} HttpRequest;

typedef struct {
	Socket out;
} HttpResponse;

typedef void (*HttpHandler)(HttpRequest *, HttpResponse *);

static void http_work(Socket s, struct sockaddr_in addr, HttpHandler h)
{
	Buffer buf;
	buffer_init(&buf, 512);
	int n = recv(s, buf.ptr, buf.cap, 0);
	if (n <= sizeof("GET / HTTP/1.1\r\n")) {
		buffer_destroy(&buf);
		return;
	}
	HttpRequest req;
	size_t o = 0;

#define IS_METHOD(x) (memcmp(&buf.ptr[o], x, sizeof(x) - 1) == 0)

	if (IS_METHOD("GET")) {
		req.method = HttpMethod_Get;
		o += sizeof("GET") - 1;
	} else if (IS_METHOD("POST")) {
		req.method = HttpMethod_Post;
		o += sizeof("POST") - 1;
	} else if (IS_METHOD("HEAD")) {
		req.method = HttpMethod_Head;
		o += sizeof("HEAD") - 1;
	} else if (IS_METHOD("PUT")) {
		req.method = HttpMethod_Put;
		o += sizeof("PUT") - 1;
	} else {
		printf("%.*s", 4, buf.ptr);
	}
	if (buf.ptr[o] != ' ') {
		buffer_destroy(&buf);
		return;
	}
	o++;
	buffer_init(&req.url, 16);
	uint8_t c = buf.ptr[o];
	while (c != ' ') {
		buffer_append(&req.url, c);
		o++;
		c = buf.ptr[o];
	}
	if (memcmp(&buf.ptr[o], " HTTP/1.1\r\n", sizeof(" HTTP/1.1\r\n") - 1) !=
		0) {
		buffer_destroy(&buf);
		buffer_destroy(&req.url);
		return;
	}
	buffer_destroy(&buf);
	HttpResponse res;
	res.out = s;
	h(&req, &res);
	buffer_destroy(&req.url);
}

typedef enum {
	HttpStatus_Ok = 200,
	HttpStatus_NotFound = 404,
	HttpStatus_Found = 302
} HttpStatus;

typedef struct {
	uint8_t *ptr;
	size_t len;
} String;

#define String_Static(x) ((String){x, sizeof(x) - 1})

static String HttpStatusString[] = {
		[HttpStatus_Ok] = String_Static("200 OK"),
		[HttpStatus_Found] = String_Static("302 Found"),
		[HttpStatus_NotFound] = String_Static("404 Not Found"),
};

static void http_send(HttpResponse *res, uint8_t *ptr, size_t len)
{
	send(res->out, ptr, len, 0);
}

#define http_send_const(to, x) (http_send(to, x, sizeof(x) - 1))

static void http_error(HttpResponse *res, HttpStatus code)
{
	http_send_const(res, "HTTP/1.1 ");
	String status = HttpStatusString[code];
	http_send(res, status.ptr, status.len);
	http_send_const(res, "\r\n");
	http_send_const(res, "Content-Type: text/plain\r\n");
	http_send_const(res, "Content-Length: ");
	uint8_t buf[10];
	size_t len = sprintf(buf, "%d", status.len);
	http_send(res, buf, len);
	http_send_const(res, "\r\n\r\n");
	http_send(res, status.ptr, status.len);
}

static void http_sendfile(HttpResponse *res, const char *file)
{
	File f = file_open(file);
	http_send_const(res, "HTTP/1.1 ");
	String status = HttpStatusString[200];
	http_send(res, status.ptr, status.len);
	http_send_const(res, "\r\n");
	// http_send_const(res, "Content-Type: image/gif\r\n");
	http_send_const(res, "Content-Length: ");
	uint8_t buf[10];
	size_t len = sprintf(buf, "%d", file_size(f));
	http_send(res, buf, len);
	http_send_const(res, "\r\n\r\n");
	socket_sendfile(res->out, f);
	file_close(f);
}

static void http_servedir(char *dir, HttpRequest *req, HttpResponse *res)
{
	Buffer fn;
	buffer_init(&fn, 16);
	buffer_writestring(&fn, dir);
	if (req->url.len == 1 && req->url.ptr[0] == '/') {
		buffer_write(&fn, "/index.html", sizeof("/index.html")-1);
	} else {
		buffer_write(&fn, req->url.ptr, req->url.len);
	}
	buffer_append(&fn, 0);
	http_sendfile(res, fn.ptr);
	buffer_destroy(&fn);
}

static int http_listen(HttpListener *l, const char *host, unsigned short port,
					   HttpHandler h)
{
	l->s = socket(AF_INET, SOCK_STREAM, 0);
	struct sockaddr_in sin = {0};
	sin.sin_family = AF_INET;
	inet_pton(AF_INET, host, (char *)&(sin.sin_addr));
	sin.sin_port = htons(port);
	bind(l->s, (struct sockaddr *)&sin, sizeof(sin));
	listen(l->s, SOMAXCONN);
	for (;;) {
		struct sockaddr_in addr = {0};
		int addr_len = sizeof(addr);
		Socket s = accept(l->s, (struct sockaddr *)&addr, &addr_len);
		http_work(s, addr, h);
		socket_close(s);
	}
	socket_close(l->s);
}
