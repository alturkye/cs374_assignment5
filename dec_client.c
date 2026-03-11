#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

void send_all(int socket, char* buffer, int n) {
    int total = 0;
    while (total < n) {
        int bytes = send(socket, buffer + total, n - total, 0);
        if (bytes < 0) exit(1);
        total += bytes;
    }
}

void receive_all(int socket, char* buffer, int n) {
    int total = 0;
    while (total < n) {
        int bytes = recv(socket, buffer + total, n - total, 0);
        if (bytes <= 0) break;
        total += bytes;
    }
}

char* read_file(char* filename, int* length) {
    FILE* fp = fopen(filename, "r");
    if (!fp) exit(1);
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char* buffer = malloc(fsize + 1);
    fgets(buffer, fsize + 1, fp);
    fclose(fp);
    buffer[strcspn(buffer, "\n")] = '\0';
    *length = strlen(buffer);
    for (int i = 0; i < *length; i++) {
        if ((buffer[i] < 'A' || buffer[i] > 'Z') && buffer[i] != ' ') 
        {
            fprintf(stderr, "enc_client error: " 
                    "input contains bad characters\n");
            exit(1);
        }
    }
    return buffer;
}

int main(int argc, char *argv[])
{
    if (argc < 4) exit(1);
    int plainLen, keyLen;
    char* plaintext = read_file(argv[1], &plainLen);
    char* key = read_file(argv[2], &keyLen);
    if (keyLen < plainLen)
    {
        fprintf(stderr, "Error: key too short\n"); 
        exit(1);
    }

    int socketFD = socket(AF_INET, SOCK_STREAM, 0);
    struct hostent* serverHostInfo = gethostbyname("localhost");
    struct sockaddr_in serverAddress;
    memset((char*)&serverAddress, '\0', sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(atoi(argv[3]));
    memcpy((char*)&serverAddress.sin_addr.s_addr,
            (char*)serverHostInfo->h_addr, serverHostInfo->h_length);

    if (connect(socketFD, (struct sockaddr*)&serverAddress, 
                sizeof(serverAddress)) < 0)
    exit(2);

    char type = 'D';
    send(socketFD, &type, 1, 0);
    char auth;
    recv(socketFD, &auth, 1, 0);
    if (auth != 'Y') { 
        fprintf(stderr, "Error: could not contact dec_server on port %s\n",
                argv[3]); 
        exit(2); 
    }

    int netLen = htonl(plainLen);
    send_all(socketFD, (char*)&netLen, sizeof(netLen));
    send_all(socketFD, plaintext, plainLen);
    send_all(socketFD, key, plainLen);

    char* ciphertext = malloc(plainLen + 1);
    memset(ciphertext, '\0', plainLen + 1);
    receive_all(socketFD, ciphertext, plainLen);
    printf("%s\n", ciphertext);

    free(plaintext); free(key); free(ciphertext);
    close(socketFD);
    return 0;
}
