#include "httpd.h"

#include <arpa/inet.h>
#include <ctype.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <unistd.h>

#include <time.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

// 全局变量
SSL_CTX *ssl_ctx = NULL;

#define MAX_CONNECTIONS 1000
#define BUF_SIZE 65535
#define QUEUE_SIZE 1000000

static int listenfd;
int *clients;
static void start_server(const char *);
static void respond(int, struct sockaddr_in, socklen_t);

static char *buf;

// Client request
char *method, // "GET" or "POST"
    *uri,     // "/index.html" things before '?'
    *qs,      // "a=1&b=2" things after  '?'
    *prot,    // "HTTP/1.1"
    *payload; // for POST

int payload_size;


// 初始化SSL上下文
static void init_openssl() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
    ssl_ctx = SSL_CTX_new(TLS_server_method());
    if (!ssl_ctx) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
}

// 加载证书和私钥
static void configure_ssl(const char* cert_file, const char* key_file) {
    if (SSL_CTX_use_certificate_file(ssl_ctx, cert_file, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    if (SSL_CTX_use_PrivateKey_file(ssl_ctx, key_file, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
}


void serve_forever(const char *PORT) {
    // 初始化OpenSSL
    init_openssl();
    configure_ssl("/etc/PICO-Foxweb/certs/server.crt", "/etc/PICO-Foxweb/certs/server.key");   

  struct sockaddr_in clientaddr;
  socklen_t addrlen;

  int slot = 0;

  //printf("Server started %shttp://127.0.0.1:%s%s\n", "\033[92m", PORT, "\033[0m");

  // create shared memory for client slot array
  clients = mmap(NULL, sizeof(*clients) * MAX_CONNECTIONS,
                 PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, -1, 0);

  // Setting all elements to -1: signifies there is no client connected
  int i;
  for (i = 0; i < MAX_CONNECTIONS; i++)
    clients[i] = -1;
  start_server(PORT);

  // Ignore SIGCHLD to avoid zombie threads
  signal(SIGCHLD, SIG_IGN);

  // ACCEPT connections
  while (1) {
    addrlen = sizeof(clientaddr);
    clients[slot] = accept(listenfd, (struct sockaddr *)&clientaddr, &addrlen);

    if (clients[slot] < 0) {
      perror("accept() error");
      exit(1);
    } else {
      if (fork() == 0) {
        close(listenfd);
        respond(slot, clientaddr, addrlen);
        close(clients[slot]);
        clients[slot] = -1;
        exit(0);
      } else {
        close(clients[slot]);
      }
    }

    while (clients[slot] != -1)
      slot = (slot + 1) % MAX_CONNECTIONS;
  }
}

// start server
void start_server(const char *port) {
  struct addrinfo hints, *res, *p;

  // getaddrinfo for host
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;
  if (getaddrinfo(NULL, port, &hints, &res) != 0) {
    perror("getaddrinfo() error");
    exit(1);
  }
  // socket and bind
  for (p = res; p != NULL; p = p->ai_next) {
    int option = 1;
    listenfd = socket(p->ai_family, p->ai_socktype, 0);
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    if (listenfd == -1)
      continue;
    if (bind(listenfd, p->ai_addr, p->ai_addrlen) == 0)
      break;
  }
  if (p == NULL) {
    perror("socket() or bind()");
    exit(1);
  }

  freeaddrinfo(res);

  // listen for incoming connections
  if (listen(listenfd, QUEUE_SIZE) != 0) {
    perror("listen() error");
    exit(1);
  }
}

// get request header by name
char *request_header(const char *name) {
  header_t *h = reqhdr;
  while (h->name) {
    if (strcmp(h->name, name) == 0)
      return h->value;
    h++;
  }
  return NULL;
}

// get all request headers
header_t *request_headers(void) { return reqhdr; }

// Handle escape characters (%xx)
static void uri_unescape(char *uri) {
  char chr = 0;
  char *src = uri;
  char *dst = uri;

  // Skip inital non encoded character
  while (*src && !isspace((int)(*src)) && (*src != '%'))
    src++;

  // Replace encoded characters with corresponding code.
  dst = src;
  while (*src && !isspace((int)(*src))) {
    if (*src == '+')
      chr = ' ';
    else if ((*src == '%') && src[1] && src[2]) {
      src++;
      chr = ((*src & 0x0F) + 9 * (*src > '9')) * 16;
      src++;
      chr += ((*src & 0x0F) + 9 * (*src > '9'));
    } else
      chr = *src;
    *dst++ = chr;
    src++;
  }
  *dst = '\0';
}

// client connection
void respond(int slot, struct sockaddr_in client_addr, socklen_t addr_len) {
    // Получить IP-адрес клиента
    // 初始化默认值
    char client_ip[INET_ADDRSTRLEN] = "unknown";
    // 获取客户端地址信息至 “ client_addr ”
    int gpn = getpeername(clients[slot], (struct sockaddr*)&client_addr, &addr_len);
    // 检查getpeername是否成功
    if (gpn == 0) {
        // 成功获取客户端地址信息
        // 转换为可读IP字符串 并 检查inet_ntop是否成功
        if (inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip)) == NULL) {
            // 不成功
            strcpy(client_ip, "invalid");
        }
    }

    // 创建SSL对象并绑定到套接字
    SSL *ssl = SSL_new(ssl_ctx);
    SSL_set_fd(ssl, clients[slot]);

    // 执行SSL握手
    if (SSL_accept(ssl) <= 0) {
        ERR_print_errors_fp(stderr);
        SSL_free(ssl);
        return;
    }

  int rcvd;

  buf = malloc(BUF_SIZE);
  //rcvd = recv(clients[slot], buf, BUF_SIZE, 0);
  rcvd = SSL_read(ssl, buf, BUF_SIZE);

  if (rcvd < 0) // receive error
    fprintf(stderr, ("recv() error\n"));
  else if (rcvd == 0) // receive socket closed
    fprintf(stderr, "Client disconnected upexpectedly.\n");
  else // message received
  {
    buf[rcvd] = '\0';

    method = strtok(buf, " \t\r\n");
    uri = strtok(NULL, " \t");
    prot = strtok(NULL, " \t\r\n");

    uri_unescape(uri);

    fprintf(stderr, "\x1b[32m + [%s] %s\x1b[0m\n", method, uri);

    qs = strchr(uri, '?');

    if (qs)
      *qs++ = '\0'; // split URI
    else
      qs = uri - 1; // use an empty string

    header_t *h = reqhdr;
    char *t, *t2;
    while (h < reqhdr + 16) {
      char *key, *val;

      key = strtok(NULL, "\r\n: \t");
      if (!key)
        break;

      val = strtok(NULL, "\r\n");
      while (*val && *val == ' ')
        val++;

      h->name = key;
      h->value = val;
      h++;
      fprintf(stderr, "[H] %s: %s\n", key, val);
      t = val + 1 + strlen(val);
      if (t[1] == '\r' && t[2] == '\n')
        break;
    }
    t = strtok(NULL, "\r\n");
    t2 = request_header("Content-Length"); // and the related header if there is
    payload = t;
    payload_size = t2 ? atol(t2) : (rcvd - (t - buf));

    // bind clientfd to stdout, making it easier to write
    int clientfd = clients[slot];
    //dup2(clientfd, STDOUT_FILENO);
    //close(clientfd);

    // call router
    int status_code = route(ssl);

    // log
    // Запись содержимого журнала в пользовательский текстовый журнал(IP клиента, метод, uri, статус, размер данных)
    // 将日志内容写入自定义文本日志 （客户端IP， 方法， uri，状态， 数据大小）
    write_access_log(client_ip, method, uri, status_code, rcvd);

    // 清理SSL资源
    SSL_shutdown(ssl);
    SSL_free(ssl);

    // tidy up
    fflush(stdout);
    shutdown(STDOUT_FILENO, SHUT_WR);
    close(STDOUT_FILENO);
  }

  free(buf);
}

// log
// Запись содержимого журнала в пользовательский текстовый журнал(IP клиента, метод, uri, статус, размер данных)
// 将日志内容写入自定义文本日志 （客户端IP， 方法， uri，状态， 数据大小）
void write_access_log(const char* client_ip, const char* method, const char* uri, int status, int data_size) {
    // Создайте файл журнала(если он не существует)
    // 创建日志文件（如果不存在）
    FILE* logFile = fopen("/var/log/PICOFoxweb_log.log", "a");

    if (logFile != NULL) {
        // Получить текущее время
        // 获取当前时间
        time_t nowTime = time(NULL);
        struct tm* tm = localtime(&nowTime);
        char timestamp[64];
        strftime(timestamp, sizeof(timestamp), "%d/%b/%Y:%H:%M:%S %z", tm);

        // Запись в журнал
        // 写入日志
        fprintf(logFile, "%s - - [%s] \"%s %s HTTP/1.1\" %d (%d bytes)\n",
            client_ip,
            timestamp,
            method,
            uri,
            status,
            data_size);

        fclose(logFile);
    }
}


