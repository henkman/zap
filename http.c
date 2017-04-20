
// gcc -o http http.c -s -O2 -lws2_32 -lmswsock

#include "http.h"

/*
static void handler(HttpRequest *req, HttpResponse *res)
{
	printf("%d %.*s\n", req->method, req->url.len, req->url.ptr);
	char pre[] =
		"<html>"
		"<head><title>testme</title></head>"
		"<body>";
	char post[] =
		"</body>"
		"</html>";
	http_send_const(res,
					"HTTP/1.0 200 OK\r\n"
					"Content-Type: text/html\r\n");
	http_send_const(res, "Content-Length: ");
	uint8_t buf[10];
	size_t len =
		sprintf(buf, "%d", sizeof(pre) - 1 + sizeof(post) - 1 + req->url.len);
	http_send(res, buf, len);
	http_send_const(res, "\r\n\r\n");
	http_send_const(res, pre);
	http_send(res, req->url.ptr, req->url.len);
	http_send_const(res, post);
}
*/

static void handler(HttpRequest *req, HttpResponse *res)
{
	printf("%d %.*s\n", req->method, req->url.len, req->url.ptr);
	http_servedir("./html", req, res);
}

int main(void)
{
	HttpListener l;
	http_init();
	http_listen(&l, "0.0.0.0", 8080, &handler);
	http_quit();

	return 0;
}
