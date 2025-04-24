#ifndef _HTTPD_H___
#define _HTTPD_H___

#include <stdio.h>
#include <string.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

// SSL上下文和SSL对象声明
extern SSL_CTX *ssl_ctx;

// Client request
extern char *method, // "GET" or "POST"
    *uri,            // "/index.html" things before '?'
    *qs,             // "a=1&b=2" things after  '?'
    *prot,           // "HTTP/1.1"
    *payload;        // for POST

extern int payload_size;

// Server control functions
void serve_forever(const char *PORT);

char *request_header(const char *name);

typedef struct {
  char *name, *value;
} header_t;
static header_t reqhdr[17] = {{"\0", "\0"}};
header_t *request_headers(void);

// user shall implement this function

// void route();
int route(SSL *ssl);

// Response
#define RESPONSE_PROTOCOL "HTTP/1.1"

//#define HTTP_200 printf("%s 200 OK\n\n", RESPONSE_PROTOCOL)
//#define HTTP_200 SSL_write(ssl, "HTTP/1.1 200 OK\n\n", 17)
//#define HTTP_201 printf("%s 201 Created\n\n", RESPONSE_PROTOCOL)
//#define HTTP_201 SSL_write(ssl, "HTTP/1.1 201 Created\n\n", 22)
//#define HTTP_404 printf("%s 404 Not found\n\n", RESPONSE_PROTOCOL)
//#define HTTP_404 SSL_write(ssl, "HTTP/1.1 404 Not found\n\n", 23)
//#define HTTP_500 printf("%s 500 Internal Server Error\n\n", RESPONSE_PROTOCOL)
//#define HTTP_500 SSL_write(ssl, "HTTP/1.1 500 Internal Server Error\n\n", 34)

//#define HTTP_200 printf("%s 200 OK\n\n", RESPONSE_PROTOCOL)
#define HTTP_200 SSL_write(ssl, "HTTP/1.1 200 OK\n\n", 17)
//#define HTTP_201 printf("%s 201 Created\n\n", RESPONSE_PROTOCOL)
#define HTTP_201 SSL_write(ssl, "HTTP/1.1 201 Created\n\n", 22)
//#define HTTP_404 printf("%s 404 Not found\n\n", RESPONSE_PROTOCOL)
#define HTTP_404 SSL_write(ssl, "HTTP/1.1 404 Not found\n\n", 23)
//#define HTTP_500 printf("%s 500 Internal Server Error\n\n", RESPONSE_PROTOCOL)
#define HTTP_500 SSL_write(ssl, "HTTP/1.1 500 Internal Server Error\n\n", 34)

// some interesting macro for `route()`
#define ROUTE_START() if (0) {
#define ROUTE(METHOD, URI)                                                     \
  }                                                                            \
  else if (strcmp(URI, uri) == 0 && strcmp(METHOD, method) == 0) {
#define GET(URI) ROUTE("GET", URI)
#define POST(URI) ROUTE("POST", URI)
#define ROUTE_END()                                                            \
  }                                                                            \
  else HTTP_500;

// log
// Запись содержимого журнала в пользовательский текстовый журнал(IP клиента, метод, uri, статус, размер данных)
// 将日志内容写入自定义文本日志 （客户端IP， 方法， uri，状态， 数据大小）
void write_access_log(const char* client_ip, const char* method, const char* uri, int status, int data_size);

#endif
