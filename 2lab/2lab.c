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
#define MAX_QUEUE 5
#define BUFFER_SIZE 1024

volatile sig_atomic_t sigHupReceived = 0;

void handleSIGHUP(int signo) {
    sigHupReceived = 1;
}

int main() {
    int serverSocket, clientSocket, maxDescriptor, optVal = 1;
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t clientAddressLen = sizeof(clientAddress);
    fd_set activeDescriptors, workingDescriptors;
    sigset_t blockedMask, origMask;
    char buffer[BUFFER_SIZE];

    struct sigaction signalAction;
    sigemptyset(&blockedMask);
    sigaddset(&blockedMask, SIGHUP);
    sigprocmask(SIG_BLOCK, &blockedMask, &origMask);

    signalAction.sa_handler = handleSIGHUP;
    signalAction.sa_flags = SA_RESTART;
    sigaction(SIGHUP, &signalAction, NULL);

    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }

    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(optVal));

    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(PORT);

    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) == -1) {
        perror("Error binding socket");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    if (listen(serverSocket, MAX_QUEUE) == -1) {
        perror("Error listening on socket");
        close(serverSocket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT);

    FD_ZERO(&activeDescriptors);
    FD_SET(serverSocket, &activeDescriptors);

    while (1) {
         fd_set workingDescriptors;

        FD_ZERO(&workingDescriptors);
        for (int fd = 0; fd < FD_SETSIZE; fd++) {
            if (FD_ISSET(fd, &activeDescriptors)) {
                FD_SET(fd, &workingDescriptors);
            }
        }
        
        if (pselect(FD_SETSIZE, &workingDescriptors, NULL, NULL, NULL, &origMask) == -1) {
            if (errno == EINTR) {
                if (sigHupReceived) {
                    printf("Received SIGHUP signal\n");
                    sigHupReceived = 0;
                }
                continue;
            } else {
                perror("Error during pselect");
                break;
            }
        }

        for (int fd = 0; fd < FD_SETSIZE; fd++) {
            if (FD_ISSET(fd, &workingDescriptors)) {
                if (fd == serverSocket) {
                    clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressLen);
                    if (clientSocket == -1) {
                        perror("Error accepting connection");
                        continue;
                    }
                    printf("New connection from %s:%d\n",
                        inet_ntoa(clientAddress.sin_addr), ntohs(clientAddress.sin_port));
                
                    FD_SET(clientSocket, &activeDescriptors);
                } else {
                    ssize_t bytesRead = read(fd, messageBuffer, MSG_BUFFER_SIZE);
                    if (bytesRead > 0) {
                        printf("Received %zd bytes: %.*s\n", bytesRead, (int)bytesRead, messageBuffer);
                    } else if (bytesRead == 0) {
                        printf("Client disconnected: fd %d\n", fd);
                        close(fd);
                        FD_CLR(fd, &activeDescriptors);
                    } else {
                        perror("Error reading from client");
                    }
                }
            }
        }
    }

    close(serverSocket);
    return 0;
}
