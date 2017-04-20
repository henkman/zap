#include <arpa/inet.h>
#include <netdb.h> /* Needed for getaddrinfo() and freeaddrinfo() */
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <unistd.h> /* Needed for close() */
typedef int Socket;

typedef int File;

static File file_open(const char *file) { return 0; }
static void file_close(File f) {}
static uint64_t file_size(File f) { return 0; }
static void socket_sendfile(Socket s, File f) {}