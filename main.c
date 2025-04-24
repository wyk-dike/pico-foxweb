#include "httpd.h"
#include <sys/stat.h>

#include <syslog.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

#define CHUNK_SIZE 1024 // read 1024 bytes at a time

// Public directory settings
#define PUBLIC_DIR "/var/www/PICO-Foxweb"
#define INDEX_HTML "/index.html"
#define NOT_FOUND_HTML "/404.html"

int main(int c, char **v) {
  char *port = c == 1 ? "8000" : v[1];

  // syslog
  // Запись содержимого журнала в системный журнал 
  // 将日志内容写入系统日志
  syslog(LOG_INFO, "Service started on port %s; Resource Working Catalogue: %s", port, PUBLIC_DIR);

  serve_forever(port);

  // 清理SSL资源
  SSL_CTX_free(ssl_ctx);
  EVP_cleanup();

  return 0;
}

int file_exists(const char *file_name) {
  struct stat buffer;
  int exists;

  exists = (stat(file_name, &buffer) == 0);

  return exists;
}

int read_file(const char *file_name, SSL *ssl) {
  char buf[CHUNK_SIZE];
  FILE *file;
  size_t nread;
  int err = 1;

  file = fopen(file_name, "r");

  if (file) {
    while ((nread = fread(buf, 1, sizeof buf, file)) > 0) {
        //fwrite(buf, 1, nread, stdout);
        SSL_write(ssl, buf, nread);
    }
      
    err = ferror(file);
    fclose(file);
  }
  return err;
}

// void route() {}
int route(SSL *ssl) {   
  // code
  int code = 0;
  
  ROUTE_START()

  GET("/") {
    char index_html[255];
    sprintf(index_html, "%s%s", PUBLIC_DIR, INDEX_HTML);

    HTTP_200;
    code = 200;
    if (file_exists(index_html)) {
      read_file(index_html, ssl);
    } else {
      //printf("Hello! You are using %s\n\n", request_header("User-Agent"));
      const char *msg = "Hello! You are using...";
      SSL_write(ssl, msg, strlen(msg));
    }
  }

  GET("/test") {
    HTTP_200;
    code = 200;
    //printf("List of request headers:\n\n");
    const char *header_msg = "List of request headers:\n\n";
    SSL_write(ssl, header_msg, strlen(header_msg));

    header_t *h = request_headers();

    while (h->name) {
      //printf("%s: %s\n", h->name, h->value);
        char line[256];
        sprintf(line, "%s: %s\n", h->name, h->value);
        SSL_write(ssl, line, strlen(line));
        h++;
    }
  }

  POST("/") {
    HTTP_201;
    code = 201;
    //printf("Wow, seems that you POSTed %d bytes.\n", payload_size);
    //printf("Fetch the data using `payload` variable.\n");
    char post_msg1[256], post_msg2[256];

    sprintf(post_msg1, "Wow, seems that you POSTed %d bytes.\n", payload_size);
    SSL_write(ssl, post_msg1, strlen(post_msg1));

    sprintf(post_msg2, "Fetch the data using `payload` variable.\n");
    SSL_write(ssl, post_msg2, strlen(post_msg2));

    if (payload_size > 0) {
        //printf("Request body: %s", payload);
        SSL_write(ssl, "Request body: ", 14);
        SSL_write(ssl, payload, payload_size);
    }
  }

  GET(uri) {
    char file_name[255];
    sprintf(file_name, "%s%s", PUBLIC_DIR, uri);

    if (file_exists(file_name)) {
      HTTP_200;
      code = 200;
      read_file(file_name, ssl);
    } else {
      HTTP_404;
      code = 404;
      sprintf(file_name, "%s%s", PUBLIC_DIR, NOT_FOUND_HTML);
      if (file_exists(file_name))
        read_file(file_name, ssl);
    }
  }

  ROUTE_END()

  return code;
}
