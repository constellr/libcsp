## Installation instructions

We will use WAF build sytstem to build CSP library and to generate the python embeddings.

All of the required settings are set as default in the [script](/wscript). Use the following commands to install libcsp with python bindings enabled.
```
./waf configure --prefix=INSTALL_PATH
./waf build install
```
The first command configures the build environment. Replace `INSTALL_PATH` with your preferred installation location. The second command builds the library and copies the required binaries to the `INSTALL_PATH` if given.

## Using libcsp python bindings

To use the python bindings of libcsp within a script/terminal, we need to point `LD_LIBRARY_PATH` and `PYTHON_PATH` to the libcsp build directory.  

```
LD_LIBRARY_PATH=LIBCSP_DIR/build PYTHON_PATH=LIBCSP_DIR/build python
```

You can import the library as shown below in the python interpreter/script.
```python
import libcsp_py3 as libcsp
```

## Using the UDP interface

```python
import libcsp_py3 as libcsp

#initialises a CSP instance with an ID 27 and creates 10 buffers of size 300 each
libcsp.init(27, "host", "model", "1.2.3", 10, 300)
#initialised UDP interface with NET_ADDR as local host, listening port 9601, transmitting port 9600
libcsp.udp_init("127.0.0.1",9601,9600)
```

## Running the example

Start the server in one instance of a terminal.
```
LD_LIBRARY_PATH=LIBCSP_DIR/build PYTHON_PATH=LIBCSP_DIR/build python LIBCSP_DIR/examples/udp_server.py
```
Start the client in another instance of a terminal.
```
LD_LIBRARY_PATH=LIBCSP_DIR/build PYTHON_PATH=LIBCSP_DIR/build python LIBCSP_DIR/examples/udp_client.py
```

The client sends a fixed data 10 times over the UDP interface.  In the server terminal, the recieved data is printed out 10 times similarly.

## Changes made to backport UDP interface

- Files added from V2.0
  - csp_if_udp.c 
    - Changes from V2.0 - Modified transmission function signature to match it to V1.6 transmission function signature.
  - cso_if_udp.h
    - Changes from V2.0 - Modified transmission function signature to match it to V1.6 transmission function signature.
  - csp_id.c
    - Changes from V2.0 - Removed V2.0 functions.
  - csp_id.h 
    - Changes from V2.0 - Removed V2.0 functions.

- Files modified
  - csp_interface.h
    - Modified interface structure to support UDP - `csp_iface_s`.
  - csp_types.h
    - Modified packet structure to support UDP - `csp_packet_s`.
  - pyscp.c
    - Added entrypoint for UDP initialisation - `pycsp_udp_init`.

## Control flow

- Initialise CSP with ID, number of buffers and buffer size.
- Initialise UDP with net address, listening port, transmission port.
  - Setup transmission function to be invoked when a packet of data should be sent.
  - Creates a new socket and binds the listening port to the socket.
  - Starts a thread for recieving data on the listening port.
  - Use the same socket for transmission.
- Server specific flow
  - Setup routing tables.
  - Depending on the interface mentioned in the routing table the interface specific functions are called for transmission and recieving.
    - Example: `0/0 UDP` - UDP interface functions are invoked.
- Client specific flow
  - Load routing tables setup by the server. (Can also setup routing table)
- To send data
  - CSP preallocates buffers when initialised.
  - To send data, request a buffer from the buffer pool for the packet length.
  - Copy data to the acquired buffer.
  - Use the sendto function to send data.
    - Will call the transmission function setup during UDP initialisation.
    - Use operating system based `sendto` to send the packet of data over UDP.
  - Free up the buffer.
- To recieve data
  - On UDP initialisation a recieving thread is created.
  - The thread actively listens on the port with operating system based `recvfrom` in blocking state.
  - Once the packet is recieved the packet of data is put into a fifo queue.
  - By using the library functions `accept` and later `read` the queue is polled for new data.
  - Once new data comes in the first time the connection is accepted. Later using `read` function packets are read and sent for further processing.
