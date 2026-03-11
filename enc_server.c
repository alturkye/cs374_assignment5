#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>

// receive loop for large data
void receive_all(int socket, char* buffer, int n) {
    int total = 0;
    while (total < n) {
        int bytes = recv(socket, buffer + total, n - total, 0);
        if (bytes <= 0) break;
        total += bytes;
    }
}

// send loop
void send_all(int socket, char* buffer, int n) {
    int total = 0;
    while (total < n) {
        int bytes = send(socket, buffer + total, n - total, 0);
        if (bytes <= 0) break;
        total += bytes;
    }
}

// OTP math 
char encrypt_char(char p, char k) {
    int p_val = (p == ' ') ? 26 : p - 'A';
    int k_val = (k == ' ') ? 26 : k - 'A';
    int res = (p_val + k_val) % 27;
    return (res == 26) ? ' ' : res + 'A';
}

int main(int argc, char *argv[]) {
    int listenSocket, connectionSocket;
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t sizeOfClientInfo = sizeof(clientAddress);

    if (argc < 2) { fprintf(stderr,"USAGE: %s port\n", argv[0]); exit(1); }

    listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    int yes = 1;
    setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    memset((char *)&serverAddress, '\0', sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(atoi(argv[1]));
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(listenSocket, (struct sockaddr *)&serverAddress,
                sizeof(serverAddress)) < 0) exit(1);
    listen(listenSocket, 5);

    while(1) {
        connectionSocket = accept(listenSocket, 
                (struct sockaddr *)&clientAddress, &sizeOfClientInfo);
        
        pid_t spawnPid = fork();
        if (spawnPid == 0) {
            close(listenSocket);
            char type;
            recv(connectionSocket, &type, 1, 0);
            if (type != 'E') { // handshake - only accept enc_client
                send(connectionSocket, "N", 1, 0);
                close(connectionSocket); exit(2);
            }
            send(connectionSocket, "Y", 1, 0);

            int dataLen;
            recv(connectionSocket, &dataLen, sizeof(dataLen), 0);
            dataLen = ntohl(dataLen);

            char *plaintext = malloc(dataLen);
            char *key = malloc(dataLen);
            receive_all(connectionSocket, plaintext, dataLen);
            receive_all(connectionSocket, key, dataLen);

            char *ciphertext = malloc(dataLen);
            for (int i = 0; i < dataLen; i++) ciphertext[i] =
                encrypt_char(plaintext[i], key[i]);

            send_all(connectionSocket, ciphertext, dataLen);
            free(plaintext); free(key); free(ciphertext);
            close(connectionSocket); exit(0);
        } else {
            close(connectionSocket);
            while (waitpid(-1, NULL, WNOHANG) > 0); 
        }
    }
    return 0;
}
