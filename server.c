// Part of CTelnet
// This program is published under the terms of the GNU GPL Version 2.
// Copyright ahahn94 2018

#include "server.h"

#define SERVERLINEHEADER "[MAIN]\t"

#define RED "\033[31;1m"
#define YELLOW "\033[33;1m"
#define GREEN "\033[32;1m"
#define BOLD "\033[1m"
#define RESETFORMAT "\033[0;m"

int broadcast;

struct peer *server_peers;
int server_client_count;
int server_semaphore_group_id;


/**
 * Start server.
 * @param port_number Port number of the new server.
 * @param client_count Maximum number of simultaneous connections.
 * @param payload Pointer to a function that takes a filedescriptor and returns an int (0 if all ok).
 * @return 0 if successful, 1 if not.
 */
int start_server(int port_number, int client_count, int (*payload)(int), struct peer *peers, int semaphore_group_id) {

    server_peers = peers;
    server_client_count = client_count;
    server_semaphore_group_id = semaphore_group_id;

    // File descriptor for broadcast on all active connections.
    broadcast = 0;

    // Connect signals.
    signal(SIGINT, handleSigint); // Connect SIGINT to the custom handler so ctrl + c causes a proper shutdown.
    signal(SIGCHLD, handle_sigchld_server); // Handle end of a child process.

    // from netinet/receiving.h; contains a socket address (ip_add, port_num), inet version (v4, v6), etc.
    struct sockaddr_in server;

    // File descriptors for data streams.
    int waiting_for_connections, client_connection;

    // Socket size receiving byte.
    int server_len = sizeof(server);

    int return_code = 0; // buffer for function return codes.

    char help_text[] = "\tCTelnet - a simple telnet server written in C.\n\n"
                       "Commands:\n\n"
                       BOLD "peers" RESETFORMAT "\t\t\tList connected clients\n\n"
                       BOLD "disconnect [ID]" RESETFORMAT "\t\tDisconnect the client with the given ID\n\n"
                       BOLD "exit" RESETFORMAT "\t\t\tExit the server after disconnecting all clients";

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
        printf("%s\n\n", help_text);
    }

    // Endless loop while waiting for connections.
    while (1) {

        // Accept new connection on waiting_for_connections and make it accessible under client_connection.
        client_connection = accept(waiting_for_connections, 0, 0);

        // Create child process for new connection.
        if (fork() == 0) {
            close(waiting_for_connections);
            broadcast = client_connection;


            // Close all copies of connections to other clients.
            for (int i = 0; i < client_count; ++i) {
                if (peers[i].connection != 0) {
                    close(peers[i].connection);
                }
            }



            // Get clients IP address.
            struct sockaddr_in client_address;
            socklen_t client_address_length = sizeof(client_address);
            getpeername(client_connection, (struct sockaddr *) &client_address, &client_address_length);
            char tmp_ip_address; // IP address as string
            strcpy(&tmp_ip_address, inet_ntoa(client_address.sin_addr));

            // Copy to new char array to prevent unwanted alteration.
            char ip_address[tmp_ip_address];
            strcpy(ip_address, &tmp_ip_address);

            int added_successfully = 0; // Set to 1 if added to peers.

            // Add to peers.
            for (int i = 0; i < client_count; ++i) {
                if (peers[i].pid == 0) {
                    struct sembuf enter, leave;

                    enter.sem_num = (unsigned short) i;
                    enter.sem_op = -1;
                    enter.sem_flg = SEM_UNDO;

                    leave.sem_num = (unsigned short) i;
                    leave.sem_op = 1;
                    leave.sem_flg = SEM_UNDO;


                    semop(semaphore_group_id, &enter, (size_t) client_count);
                    // Add client data to peers.
                    peers[i].peer_id = i + 1;
                    peers[i].pid = getpid();
                    strcpy(peers[i].ip_address, ip_address);
                    peers[i].connection = client_connection;
                    semop(semaphore_group_id, &leave, (size_t) client_count);

                    added_successfully = 1;

                    break;
                }
            }

            // Check if client has been added to peers, disconnect if not.
            if (added_successfully == 0) {
                send_message(client_connection,
                             RED "\nToo many peers! Please try again later! Disconnecting..." RESETFORMAT "\n\n");
                close(client_connection);
                exit(1);
            }

            return_code = payload(client_connection);

            // The client will be removed from the peers by the SIGCHLD handler.

            if (return_code == 0) {

                printf("Test Exit\n\n");

                exit(0);
            } else {
                fprintf(stderr, "[%d]\tError during execution of the payload!\n\n", getpid());
                exit(return_code);
            }
        } else {
            // If parent process, close filedescriptor.
//            close(client_connection);
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
                     "\n\n" RED "The server will disconnect your session in 5 seconds." RESETFORMAT "\n\n");
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


void handle_sigchld_server(int sig) {
    pid_t pid;
    int status;

    if((pid = waitpid(-1, &status, WNOHANG)) != -1) {

        for (int i = 0; i < server_client_count; i++){
            if (server_peers[i].pid == pid){
                send_message(server_peers[i].connection, "test");
                close(server_peers[i].connection);

                struct sembuf enter, leave;

                enter.sem_num = (unsigned short) i;
                enter.sem_op = -1;
                enter.sem_flg = SEM_UNDO;

                leave.sem_num = (unsigned short) i;
                leave.sem_op = 1;
                leave.sem_flg = SEM_UNDO;


                semop(server_semaphore_group_id, &enter, (size_t) server_client_count);
                server_peers[i].peer_id = 0;
                server_peers[i].pid = 0;
                strcpy(server_peers[i].ip_address, "");
                server_peers[i].connection = 0;
                semop(server_semaphore_group_id, &leave, (size_t) server_client_count);

                break;
            }
        }
    }
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