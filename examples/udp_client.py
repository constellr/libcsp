#!/usr/bin/python3

# Build required code:
# $ ./examples/buildall.py
#
#
# Run client against server:
# $ LD_LIBRARY_PATH=build PYTHONPATH=build python3 examples/udp_client.py
#

import os
import time
import sys
import argparse

import libcsp_py3 as libcsp
import time

def getOptions():
    parser = argparse.ArgumentParser(description="Parses command.")
    parser.add_argument("-a", "--address", type=int, default=10, help="Local CSP address")
    parser.add_argument("-c", "--can", help="Add CAN interface")
    parser.add_argument("-z", "--zmq", help="Add ZMQ interface")
    parser.add_argument("-s", "--server-address", type=int, default=27, help="Server address")
    parser.add_argument("-R", "--routing-table", help="Routing table")
    return parser.parse_args(sys.argv[1:])


if __name__ == "__main__":

    options = getOptions()

    libcsp.init(17, "host", "model", "1.2.3", 10, 300)

    libcsp.udp_init('127.0.0.1', 9600,9601)
    libcsp.rtable_load("0/0 UDP")
    if options.routing_table:
        libcsp.rtable_load(options.routing_table)

    libcsp.route_start_task()
    time.sleep(0.2)  # allow router task startup

    print("Connections:")
    libcsp.print_connections()

    print("Routes:")
    libcsp.print_routes()

    print(os.getpid())

    input("Press enter to continue...")
    
    print ("Exchange data with server using csp_transaction ...")
    for i in range(10):
        data = bytearray("A1B2c3d4e5f6", 'utf-16')
        packet = libcsp.buffer_get(len(data))
        libcsp.packet_set_data(packet, data)
        libcsp.sendto(0, 27, 14, 14,0, packet)
        libcsp.buffer_free(packet)
        time.sleep(0.05)
