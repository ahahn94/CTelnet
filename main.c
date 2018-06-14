// CTelnet
// This program provides a simple telnet server.
// This program is published under the terms of the GNU GPL Version 2.
// Copyright ahahn94 2018

#include "main.h"

#define PORT 5678
#define CLIENTCOUNT 10
#define INPUTLENGTH 15

#define SERVERLINEHEADER "[MAIN]\t"

#define RED "\033[31;1m"
#define YELLOW "\033[33;1m"
#define GREEN "\033[32;1m"
#define BOLD "\033[1m"
#define RESETFORMAT "\033[0;m"

int pid_server; // PID of the serverpart that waits for new connections.

int main() {

    int peers_shared_memory_id, peers_semaphore_id;
    struct peer *peers;

    unsigned short markers[CLIENTCOUNT];

    /**
     * Prepare shared memory.
     */

    // Allocate shared memory for CLIENTCOUNT peers.
    if ((peers_shared_memory_id = shmget(IPC_PRIVATE, sizeof(struct peer) * CLIENTCOUNT, IPC_CREAT | 0644)) == -1) {
        perror("Error creating shared memory segment!\n");
    }

    peers = (struct peer *) shmat(peers_shared_memory_id, 0, 0);

    // Init peers with default values.
    for (int i = 0; i < CLIENTCOUNT; ++i) {
        peers[i].peer_id = 0;
        peers[i].pid = 0;
        strcpy(peers[i].ip_address, "");
        peers[i].connection = 0;
    }

    /**
     * Prepare semaphoras.
     */

    if ((peers_semaphore_id = semget(IPC_PRIVATE, CLIENTCOUNT, IPC_CREAT | 0644)) == -1) {
        perror("Error creating semaphore group!\n");
        exit(1);
    }

    for (int i = 0; i < CLIENTCOUNT; ++i) {
        markers[i] = 1;
    }
    semctl(peers_semaphore_id, CLIENTCOUNT, SETALL, markers);

    // Set handler for SIGINT to prevent hard exit of the program.
    signal(SIGINT, handle_sigint_main);

    // Start server in a new process, save pid into pid_server.
    if ((pid_server = fork()) == 0) {
        start_server(PORT, CLIENTCOUNT, &example_payload, peers, peers_semaphore_id);
    } else {
        /**
         * Handle commands on stdin.
         */
        int running = 1;
        while (running) {
            char input[INPUTLENGTH];
            memset(input, '\0', INPUTLENGTH);
            fgets(input, INPUTLENGTH, stdin);

            if (strncmp(input, "exit", strlen("exit")) == 0) {
                // exit.
                printf("\n");
                running = 0;
                killpg(0, SIGINT); // Send SIGINT to main process and all childs.
            } else if (strncmp(input, "peers", strlen("peers")) == 0) {
                // peers.
                char separation_line[] = "----------------------------------------------------------------------";
                printf("\nConnected peers:\n\n"
                       "ID:\t|\tPID:\t|\tIP-Address:\n"
                       "%s\n", separation_line);
                for (int i = 0; i < CLIENTCOUNT; ++i) {
                    if (peers[i].pid != 0) {
                        printf("%d\t|\t%d\t|\t%s\n", peers[i].peer_id, peers[i].pid, peers[i].ip_address);
                        printf("%s\n", separation_line);
                    }
                }
                printf("\n");
            } else if (strstr(input, "disconnect") != NULL) {
                char input_copy[strlen(input)];
                char peer_id_char[4];
                int peer_id;
                strcpy(input_copy, input);  // Working copy of input as strtok may alter the string.
                char *part;
                part = strtok(input_copy, " "); // Get disconnect.
                part = strtok(NULL, " "); // Get peer_id.
                strcpy(peer_id_char, part);
                peer_id = (int) strtol(part, NULL, 10); // Parse port number to int.
                printf("\nDisconnecting peer with ID %d...\n\n", peer_id);

                int peer_pid = 0;
                int found = 0;

                if (peer_id > 0) {
                    for (int i = 0; i < CLIENTCOUNT; ++i) {
                        if (peers[i].peer_id == peer_id) {
                            found = 1;
                            int pid = peers[i].pid;

                            // Kill connection and fork.
                            kill(pid, SIGINT);

                            break;
                        }
                    }

                    if (found == 0) {
                        printf(RED SERVERLINEHEADER "Error! There is no peer matching the given ID!" RESETFORMAT "\n\n");
                    }
                } else {
                    printf(RED SERVERLINEHEADER "Error! The ID must be bigger than 0!" RESETFORMAT "\n\n");
                }

            } else {
                // Unknown command.
                printf("\nUnknown command!\n\n");
            }
        }
    }

    /**
     * Release shared memory and semaphoras.
     */
    shmdt(peers);
    shmctl(peers_shared_memory_id, IPC_RMID, 0);
    semctl(peers_semaphore_id, 0, IPC_RMID);

    return 0;
}

/**
 * Close connections and stop server.
 * @param sig
 */
void handle_sigint_main(int sig) {
    // Just wait for the server to shutdown, which it will do after receiving SIGINT.
    wait((int *) pid_server);
    exit(0);
}