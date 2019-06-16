/*

  sap-client.c

  Copyright (C) 2019  Nicholas Humfrey
  License: MIT

*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "mast.h"


// Globals
mast_socket_t sock;


int main(int argc, char *argv[])
{
    const char *address = MAST_SAP_ADDRESS_LOCAL;
    const char *port = MAST_SAP_PORT;
    const char *ifname = NULL;
    int result;

    verbose++;

    setup_signal_hander();

    result = mast_socket_open(&sock, address, port, ifname);
    if (result) {
        return EXIT_FAILURE;
    }

    while(running) {
        uint8_t packet[2048];
        mast_sap_t sap;

        int packet_len = mast_socket_recv(&sock, packet, sizeof(packet));
        if (packet_len > 0) {
            mast_debug("Received: %d bytes", packet_len);

            int result = mast_sap_parse(packet, packet_len, &sap);
            if (result == 0) {
                printf("Got SAP: %s\n", sap.sdp);
            }
        }
    }

    mast_socket_close(&sock);

    return exit_code;
}
