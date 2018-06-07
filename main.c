// CTelnet
// This program provides a simple telnet server.
// This program is published under the terms of the GNU GPL Version 2.
// Copyright ahahn94 2018

#include <stdio.h>
#include "server.h"
#include "payload.h"

#define PORT 5678
#define CLIENTCOUNT 10

int main() {
    start_server(PORT, CLIENTCOUNT, &example_payload);
    return 0;
}