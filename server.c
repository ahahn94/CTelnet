// Part of CTelnet
// This program is published under the terms of the GNU GPL Version 2.
// Copyright ahahn94 2018

#include "server.h"
#include "payload.h"

#define SERVERLINEHEADER "[MAIN]\t"

#define RED "\033[31;1m"
#define YELLOW "\033[33;1m"
#define GREEN "\033[32;1m"
#define BOLD "\033[1m"
#define RESETFORMAT "\033[0;m"

int broadcast;

/**
 * Start server.
 * @param port_number Port number of the new server.
 * @param client_count Maximum number of simultaneous connections.
 * @param payload Pointer to a function that takes a filedescriptor and returns an int (0 if all ok).
 * @return 0 if successful, 1 if not.
 */
int start_server(int port_number, int client_count, int (*payload)(int)) {

    // File descriptor for broadcast on all active connections.
    broadcast = 0;
    signal(SIGINT, handleSigint); // Connect SIGINT to the custom handler so ctrl + c causes a proper shutdown.


    // from netinet/receiving.h; contains a socket address (ip_add, port_num), inet version (v4, v6), etc.
    struct sockaddr_in server;

    // File descriptors for data streams.
    int waiting_for_connections, client_connection;

    // Socket size receiving byte.
    int server_len = sizeof(server);

    int return_code = 0; // buffer for function return codes.

    server.sin_family = AF_INET; // Set usage of IPv4.
    server.sin_port = htons(port_number); // Set server port.
    server.sin_addr.s_addr = INADDR_ANY; // Accept any address.

    printf(SERVERLINEHEADER "Starting server...\n\n");

    /*
     * Create socket.
     * AF_INET = Use IPv4.
     * SOCK_STREAM = Socket for TCP, UDP, IP and ICMP.
     * 0 = Use IP protocol.
     * socket() returns -1 if not successful!
     */
    if ((waiting_for_connections = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, SERVERLINEHEADER "Error setting socket options! Returncode: %d\n\n", waiting_for_connections);
        return 1;
    }

    // Make port reusable to prevent waiting time in the case of a restart shortly after
    // the program has ended, in which case the socket would not be available.
    int y = 1;
    if ((return_code = setsockopt(waiting_for_connections, SOL_SOCKET, SO_REUSEPORT, &y, sizeof(y))) < 0) {
        fprintf(stderr, SERVERLINEHEADER "Error setting socket options! Returncode: %d\n\n", return_code);
        exit(1);
    }

    /*
     * Bind socket.
     * waiting_for_connections = File descriptor of the socket.
     * &server = Pointer to server struct.
     * server_len = Memory size of server in byte.
     * bind() return -1 of not successful.
     */
    if ((return_code = bind(waiting_for_connections, (struct sockaddr *) &server, (socklen_t) server_len)) < 0) {
        fprintf(stderr, SERVERLINEHEADER "Error binding socket! Returncode: %d\n\n", return_code);
        return 1;
    }

    /*
     * Start listening on the socket.
     */
    if ((return_code = listen(waiting_for_connections, client_count)) > 0) {
        fprintf(stderr, "Error starting listening on socket! Returncode: %d\n\n", return_code);
        return 1;
    } else {
        printf(GREEN SERVERLINEHEADER "Server started successfully" RESETFORMAT "\n\n");
        printf(GREEN SERVERLINEHEADER "Press CTRL + C to disconnect all clients and stop the server." RESETFORMAT "\n\n");
    }

    // Endless loop while waiting for connections.
    while (1) {

        // Accept new connection on waiting_for_connections and make it accessible under client_connection.
        client_connection = accept(waiting_for_connections, 0, 0);

        // Create child process for new connection.
        if (fork() == 0) {
            broadcast = client_connection;
            close(waiting_for_connections);
            if ((return_code = payload(client_connection)) == 0) {
                return 0;
            } else {
                fprintf(stderr, "[%d]\tError during execution of the payload!\n\n", getpid());
                return return_code;
            }
        } else {
            // If parent process, close filedescriptor.
            close(client_connection);
        }
    }
    close(waiting_for_connections);
    return 0;
}

/**
 * Close connections and stop server.
 * @param sig
 */
void handleSigint(int sig) {
    // If broadcast is set, the current process has a client connection.
    if (broadcast != 0) {
        send_message(broadcast,
                     "\n\n" RED "The server is going down for halt. Closing your session in 5 seconds." RESETFORMAT "\n\n");
        sleep(5);
        send_message(broadcast, GREEN "\tBye!" RESETFORMAT "\n\n");
        close(broadcast);
        printf(GREEN "[%d]\tDisconnected!" RESETFORMAT "\n\n", getpid());
    } else {
        printf("\r" YELLOW SERVERLINEHEADER "The server is going down for halt now. Informing clients and closing connections." RESETFORMAT "\n\n");
        for (int i = 5; i > 0; --i) {
            printf(YELLOW SERVERLINEHEADER "Halting in %d..." RESETFORMAT "\n\n", i);
            sleep(1);
        }
        printf(GREEN SERVERLINEHEADER "Bye!" RESETFORMAT "\n\n");
    }
    exit(0);
}

/**
 * Send a message on the socket file_descriptor.
 * @param file_descriptor Filedescriptor of the socket.
 * @param message Message to send.
 */
void send_message(int file_descriptor, char message[]) {
    send(file_descriptor, message, strlen(message), 0);
}

/**
 * Send an input line.
 * @param file_descriptor Filedescriptor of the socket.
 */
void send_inputline(int file_descriptor) {
    send_message(file_descriptor, BOLD "input:" RESETFORMAT "\t");
}