//
// Created by ahahn94 on 13.06.18.
//

#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>

int main() {
    struct sockaddr_in server;
    unsigned long addr;

    int sock;

// Alternative zu memset() -> bzero()
    memset(&server, 0, sizeof(server));

    addr = inet_addr("127.0.0.1");
    memcpy((char *) &server.sin_addr, &addr, sizeof(addr));
    server.sin_family = AF_INET;
    server.sin_port = htons(5678);

    sock = socket(AF_INET, SOCK_STREAM, 0);

// Baue die Verbindung zum Server auf.
    if (connect(sock, (struct sockaddr *) &server, sizeof(server)) < 0) {
        printf("Can't connect!\n");
    }

    while (1) {
        char read_char[2];

        memset(read_char, '\0', 2);

        while (read(sock, read_char, 1) > 0) {
            printf("%s", read_char);
            memset(read_char, '\0', 2);
        }
    }
}