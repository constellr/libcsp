#!/usr/bin/python3

# Build required code:
# $ ./examples/buildall.py
#
# Run server
# $ LD_LIBRARY_PATH=build PYTHONPATH=build python3 examples/udp_server.py
#

import os
import time
import sys
import threading

import libcsp_py3 as libcsp


def csp_server():
    sock = libcsp.socket()
    libcsp.bind(sock, libcsp.CSP_ANY)
    libcsp.listen(sock, 5)
    while True:
        # wait for incoming connection
        conn = libcsp.accept(sock, libcsp.CSP_MAX_TIMEOUT)
        if not conn:
            continue

        print ("connection: source=%i:%i, dest=%i:%i" % (libcsp.conn_src(conn),
                                                         libcsp.conn_sport(conn),
                                                         libcsp.conn_dst(conn),
                                                         libcsp.conn_dport(conn)))

        while True:
            # Read all packets on the connection
            print("Reading packets \n")
            packet = libcsp.read(conn, 100)
            
            if packet is None:
                print("packet null")
                break

            if libcsp.conn_dport(conn) == 14:
                # print request
                data = bytearray(libcsp.packet_get_data(packet))
                length = libcsp.packet_get_length(packet)
                print ("got packet, len=" + str(length) + ", data=" + ''.join('{:02x}'.format(x) for x in data))
            else:
                # pass request on to service handler
                libcsp.service_handler(conn, packet)


if __name__ == "__main__":

    # init csp
    print(os.getpid())
    input("Press enter to continue")
    libcsp.init(27, "host", "model", "1.2.3", 10, 300)
    libcsp.udp_init("127.0.0.1",9601,9600)
    libcsp.rtable_set(0, 0, "UDP")
    libcsp.route_start_task()

    print("Hostname: %s" % libcsp.get_hostname())
    print("Model:    %s" % libcsp.get_model())
    print("Revision: %s" % libcsp.get_revision())

    print("Routes:")
    libcsp.print_routes()

    # start CSP server
    threading.Thread(target=csp_server).start()
