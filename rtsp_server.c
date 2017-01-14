#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>

static int g_Loop = 1;

void signal_handler(int sig)
{
  printf("Receive signal %d\n", sig);
  g_Loop = 0;
}

void GetRequesLine(char *buf, char *line)
{
    sscanf(buf, "%[^\n]", line);
    printf("%s\n", line);
}

void Handle_OPTIONS(int fd, int status)
{
  char buf[] =
    "RTSP/1.0 200 OK\r\n"
    "CSeq: 2\r\n"
    "Public: DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE\r\n\r\n";

  send(fd, buf, strlen(buf), 0);
}

int main(int argc, char *argv[])
{
  struct sockaddr_in server_addr, client_addr;
  int fd = -1, reuse_addr = 1, fdmax, i;
  fd_set rdFds, wrFds, master;
  struct timeval tv;

  FD_ZERO(&rdFds);
  FD_ZERO(&wrFds);
  FD_ZERO(&master);

  tv.tv_usec = 1000;
  tv.tv_sec = 0;

  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);
  signal(SIGKILL, signal_handler);

  fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    printf("Create socket failed. %s (%d)\n", strerror(errno), errno);
    exit(1);
  }

  setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(int));
  //fcntl(fd, F_SETFL, O_NONBLOCK);

  memset(&server_addr, 0x00, sizeof(struct sockaddr_in));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(8554);

  if (bind(fd, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0) {
    printf("bind socket error: %s (%d)\n", strerror(errno), errno);
    close(fd);
    exit(1);
  }

  if (listen(fd, 10) < 0) {
    printf("listen to socket error: %s (%d)\n", strerror(errno), errno);
    close(fd);
    exit(1);
  }

  FD_SET(fd, &master);
  fdmax = fd;

  while (g_Loop) {
    socklen_t addr_len = 0;
    rdFds = master;
    wrFds = master;
    int client_fd = -1;
    if (select(fdmax + 1, &rdFds, NULL, NULL, NULL) < 0) {
      perror("select: ");
      continue;
    }

    if (FD_ISSET(fd, &rdFds)) {
      addr_len = sizeof(client_addr);
      client_fd = accept(fd, (struct sockaddr*)&client_addr, &addr_len);
      if (client_fd < 0) {
        perror("accept");
      } else {
        char remoteIP[INET_ADDRSTRLEN];
        FD_SET(client_fd, &master);
        if (client_fd > fdmax) {
          fdmax = client_fd;
        }

        printf("new connection from: on socket %d\n",
                        client_fd);
      }
    }

    for (i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &rdFds)) {
        if (i != fd) {
          char buf[1024];
          int len;

          memset(buf, 0, 1024);
          len = recv(i, buf, 1024, 0);
          if (len < 0) {
            perror("recv");
            //exit(0);
          } else if (len == 0) {
            printf("Client disconnected!\n");
            close(i);
            FD_CLR(i, &master);
          } else {
            char requestLine[256];
            buf[len] = '\0';
            printf("%s", buf);
            GetRequesLine(buf, requestLine);
            //printf("%s", requestLine);
            Handle_OPTIONS(i, 200);
          }
        }
      }

      /*if (FD_ISSET(i, &wrFds)) {
        if (i != fd) {
          char buf[1024];
          int len;
          memset(buf, 0, 1024);
          len = snprintf(buf, 1023, "Connectint.\n");
          len = send(fd, buf, len, 0);
          if (len < 0) {
            perror("send");
            continue;
          }
        }
      }*/
    }
  }
}
