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

int start_server(int port_number, int client_count, int (*payload)(int));

void handleSigint(int sig);

void send_message(int file_descriptor, char message[]);

void send_inputline(int file_descriptor);