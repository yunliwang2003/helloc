#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/epoll.h>

#define MAX_EVENTS 10
#define PORT 8080

int setnonblocking(int fd) {
  int old_options = fcntl(fd, F_GETFL);
  int new_options = old_options | O_NONBLOCK;
  fcntl(fd, F_SETFL, new_options);
  return old_options;
}

/**
 * epoll 是 Linux 下特有的工具，其他系统不可用
 * @return
 */
int main() {
  struct epoll_event ev, events[MAX_EVENTS];
  int listen_sock, conn_sock, nfds, epollfd;

  struct sockaddr_in server_addr;
  socklen_t server_len = sizeof(server_addr);

  listen_sock = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_sock == -1) {
    perror("socket");
    return EXIT_FAILURE;
  }

  memset(&server_addr, 0, sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  server_addr.sin_port = htons(PORT);

  if (bind(listen_sock, (struct sockaddr *)&server_addr, server_len) == -1) {
    perror("bind");
    return EXIT_FAILURE;
  }

  if (listen(listen_sock, SOMAXCONN) == -1) {
    perror("listen");
    return EXIT_FAILURE;
  }

  setnonblocking(listen_sock);

  epollfd = epoll_create1(0);
  if (epollfd == -1) {
    perror("epoll_create1");
    return EXIT_FAILURE;
  }

  ev.events = EPOLLIN;
  ev.data.fd = listen_sock;
  if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sock, &ev) == -1) {
    perror("epoll_ctl: listen_sock");
    return EXIT_FAILURE;
  }

  for (;;) {
    nfds = epoll_wait(epollfd, events, MAX_EVENTS, -1);
    if (nfds == -1) {
      perror("epoll_wait");
      return EXIT_FAILURE;
    }

    for (int n = 0; n < nfds; ++n) {
      if (events[n].data.fd == listen_sock) {
        conn_sock = accept(listen_sock, (struct sockaddr *)&server_addr, &server_len);
        if (conn_sock == -1) {
          perror("accept");
          return EXIT_FAILURE;
        }
        setnonblocking(conn_sock);
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = conn_sock;
        if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock, &ev) == -1) {
          perror("epoll_ctl: conn_sock");
          return EXIT_FAILURE;
        }
      } else {
        // Here you would read/write from/to the socket
        // events[n].data.fd as necessary.
      }
    }
  }

  close(listen_sock);

  return EXIT_SUCCESS;
}