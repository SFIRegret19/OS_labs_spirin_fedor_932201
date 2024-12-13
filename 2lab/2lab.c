#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 12345
#define BACKLOG 5
#define BUFFER_SIZE 1024

volatile sig_atomic_t wasSigHup = 0;

void sigHupHandler(int signo) {
    wasSigHup = 1;
}

int main() {
    int server_fd, client_fd, max_fd, opt = 1;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    fd_set active_fds, fds;
    sigset_t blockedMask, origMask;
    char buffer[BUFFER_SIZE];

    struct sigaction sa;
    sigemptyset(&blockedMask);
    sigaddset(&blockedMask, SIGHUP);
    sigprocmask(SIG_BLOCK, &blockedMask, &origMask);

    sa.sa_handler = sigHupHandler;
    sa.sa_flags = SA_RESTART;
    sigaction(SIGHUP, &sa, NULL);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, BACKLOG) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    FD_ZERO(&active_fds);
    FD_SET(server_fd, &active_fds);
    max_fd = server_fd;

    while (1) {
        fds = active_fds;

        if (pselect(max_fd + 1, &fds, NULL, NULL, NULL, &origMask) == -1) {
            if (errno == EINTR) {
                if (wasSigHup) {
                    printf("Received SIGHUP signal\n");
                    wasSigHup = 0;
                }
                continue;
            } else {
                perror("pselect");
                break;
            }
        }

        if (FD_ISSET(server_fd, &fds)) {
            client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            if (client_fd == -1) {
                perror("accept");
                continue;
            }
            printf("New connection from %s:%d\n",
                inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

            FD_SET(client_fd, &active_fds);
            if (client_fd > max_fd) {
                max_fd = client_fd;
            }
        }

        for (int fd = 0; fd <= max_fd; fd++) {
            if (fd != server_fd && FD_ISSET(fd, &fds)) {
                ssize_t bytes_read = read(fd, buffer, BUFFER_SIZE);
                if (bytes_read > 0) {
                    printf("Received %zd bytes: %.*s\n", bytes_read, (int)bytes_read, buffer);
                } else if (bytes_read == 0) {
                    printf("Client disconnected: fd %d\n", fd);
                    close(fd);
                    FD_CLR(fd, &active_fds);
                } else {
                    perror("read");
                }
            }
        }
    }

    close(server_fd);
    return 0;
}
