// Part of CTelnet
// This program is published under the terms of the GNU GPL Version 2.
// Copyright ahahn94 2018

#include <stdio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/sem.h>

#include "peer.h"

int start_server(int port_number, int client_count, int (*payload)(int), struct peer *peers, int semaphore_group_id);

void handleSigint(int sig);

void send_message(int file_descriptor, char message[]);

void send_inputline(int file_descriptor);

void handle_sigchld_server(int sig) ;