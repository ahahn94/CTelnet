// Part of CTelnet
// This program is published under the terms of the GNU GPL Version 2.
// Copyright ahahn94 2018

#include "payload.h"
#include "server.h"

#define BUFFERSIZE 2000

#define RED "\033[31;1m"
#define YELLOW "\033[33;1m"
#define GREEN "\033[32;1m"
#define BOLD "\033[1m"
#define RESETFORMAT "\033[0;m"

/**
 * Example payload to run on a connection.
 * @param file_descriptor File descriptor of the connected client.
 * @return 0 if ok.
 */
int example_payload(int file_descriptor) {

    char receiving[BUFFERSIZE];
    char out[BUFFERSIZE];
    memset(receiving, '\0', BUFFERSIZE); // Turn receiving into a null terminated string.

    printf(GREEN "[%d]\tConnected!" RESETFORMAT "\n\n", getpid());

    char help_text[] = "Available commands:\n\n"
                       BOLD "Hello" RESETFORMAT "\t\tReturn \"World\"\n\n"
                       BOLD "exit" RESETFORMAT "\t\tDisconnect from server\n\n"
                       "Everything else will be echoed.\n\n";

    send_message(file_descriptor, GREEN "\nConnection established!\n\n\tWelcome!" RESETFORMAT "\n\n");
    send_message(file_descriptor, help_text);
    send_inputline(file_descriptor);

    char typed[2];

    int connection_open = 1;

    while (connection_open) {

        while (read(file_descriptor, typed, 1) > 0) {
            // Append the read char to receiving.
            strcat(receiving, typed);

            if (typed[0] == '\n') {
                // Clear typed.
                memset(typed, 0, 2);

                if (strncmp(receiving, "Hello", 5) == 0) {
                    send_message(file_descriptor, "\nWorld\n\n");
                    send_inputline(file_descriptor);
                } else if (strncmp(receiving, "exit", 4) == 0) {
                    send_message(file_descriptor, GREEN "\n\tBye!" RESETFORMAT "\n\n");
                    printf(GREEN "[%d]\tDisconnected!" RESETFORMAT "\n\n", getpid());
                    close(file_descriptor);
                    connection_open = 0;
                    break;
                } else {
                    send_message(file_descriptor, "\n");
                    send_message(file_descriptor, receiving);
                    send_message(file_descriptor, "\n");
                    send_inputline(file_descriptor);
                }

                // Reset receiving.
                memset(receiving, '\0', BUFFERSIZE);
            }
        }
    }
    return 0;
}