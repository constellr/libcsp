/*
 * csp_id.c
 *
 *  Created on: 30. jun. 2020
 *      Author: johan
 */

#include <endian.h>
#include <csp/csp.h>

/**
 * CSP 1.x
 *
 * +-----------+-------------------------+-------------------------+-----------------------------+---------------------------+----------------------------------------+
 * |           |                         |                         |                             |                           |                                        |
 * | 2 PRIO    |       5 SOURCE          |     5 DESTINATION       |   6 DESTINATION PORT        |    6 SOURCE PORT          |           8 FLAGS                      |
 * |           |                         |                         |                             |                           |                                        |
 * +-----------+-------------------------+-------------------------+-----------------------------+---------------------------+----------------------------------------+
 *
 */

#define CSP_ID1_HOST_SIZE 5
#define CSP_ID1_PORT_SIZE 6

#define CSP_ID1_PRIO_MASK 0x3
#define CSP_ID1_PRIO_OFFSET 30
#define CSP_ID1_SRC_MASK 0x1F
#define CSP_ID1_SRC_OFFSET 25
#define CSP_ID1_DST_MASK 0x1F
#define CSP_ID1_DST_OFFSET 20
#define CSP_ID1_DPORT_MASK 0x3F
#define CSP_ID1_DPORT_OFFSET 14
#define CSP_ID1_SPORT_MASK 0x3F
#define CSP_ID1_SPORT_OFFSET 8
#define CSP_ID1_FLAGS_MASK 0xFF
#define CSP_ID1_FLAGS_OFFSET 0

#define CSP_ID1_HEADER_SIZE 4

static void csp_id1_prepend(csp_packet_t *packet)
{

    /* Pack into 32-bit using host endian */
    uint32_t id1 = ((packet->id.pri << CSP_ID1_PRIO_OFFSET) |
                    (packet->id.dst << CSP_ID1_DST_OFFSET) |
                    (packet->id.src << CSP_ID1_SRC_OFFSET) |
                    (packet->id.dport << CSP_ID1_DPORT_OFFSET) |
                    (packet->id.sport << CSP_ID1_SPORT_OFFSET) |
                    (packet->id.flags << CSP_ID1_FLAGS_OFFSET));

    /* Convert to big / network endian */
    id1 = htobe32(id1);

    packet->frame_begin = packet->data - CSP_ID1_HEADER_SIZE;
    packet->frame_length = packet->length + CSP_ID1_HEADER_SIZE;

    memcpy(packet->frame_begin, &id1, CSP_ID1_HEADER_SIZE);
}

static int csp_id1_strip(csp_packet_t *packet)
{

    if (packet->frame_length < CSP_ID1_HEADER_SIZE)
    {
        return -1;
    }

    /* Get 32 bit in network byte order */
    uint32_t id1 = 0;
    memcpy(&id1, packet->frame_begin, CSP_ID1_HEADER_SIZE);
    packet->length = packet->frame_length - CSP_ID1_HEADER_SIZE;

    /* Convert to host order */
    id1 = be32toh(id1);

    /* Parse header:
     * Now in easy to work with in 32 bit register */
    packet->id.pri = (id1 >> CSP_ID1_PRIO_OFFSET) & CSP_ID1_PRIO_MASK;
    packet->id.dst = (id1 >> CSP_ID1_DST_OFFSET) & CSP_ID1_DST_MASK;
    packet->id.src = (id1 >> CSP_ID1_SRC_OFFSET) & CSP_ID1_SRC_MASK;
    packet->id.dport = (id1 >> CSP_ID1_DPORT_OFFSET) & CSP_ID1_DPORT_MASK;
    packet->id.sport = (id1 >> CSP_ID1_SPORT_OFFSET) & CSP_ID1_SPORT_MASK;
    packet->id.flags = (id1 >> CSP_ID1_FLAGS_OFFSET) & CSP_ID1_FLAGS_MASK;

    return 0;
}

static void csp_id1_setup_rx(csp_packet_t *packet)
{
    packet->frame_begin = packet->data - CSP_ID1_HEADER_SIZE;
    packet->frame_length = 0;
}

/**
 * CSP 2.x
 *
 * +--------+-----------------------------------+-----------------------------------------+---------------------+-------------------+----------------------+
 * |        |                                   |                                         |                     |                   |                      |
 * | 2 PRIO |      14 DESTINATION               |       14 SOURCE                         | 6 DESTINATION PORT  | 6 SOURCE PORT     | 6 FLAGS              |
 * |        |                                   |                                         |                     |                   |                      |
 * +--------+-----------------------------------+-----------------------------------------+---------------------+-------------------+----------------------+
 *
 */

#define CSP_ID2_HOST_SIZE 14
#define CSP_ID2_PORT_SIZE 6

#define CSP_ID2_PRIO_MASK 0x3
#define CSP_ID2_PRIO_OFFSET 46
#define CSP_ID2_DST_MASK 0x3FFF
#define CSP_ID2_DST_OFFSET 32
#define CSP_ID2_SRC_MASK 0x3FFF
#define CSP_ID2_SRC_OFFSET 18
#define CSP_ID2_DPORT_MASK 0x3F
#define CSP_ID2_DPORT_OFFSET 12
#define CSP_ID2_SPORT_MASK 0x3F
#define CSP_ID2_SPORT_OFFSET 6
#define CSP_ID2_FLAGS_MASK 0x3F
#define CSP_ID2_FLAGS_OFFSET 0

#define CSP_ID2_HEADER_SIZE 6

/**
 * Simple runtime dispatch between version 1 and 2:
 * This could have been done other ways, by registering functions into the csp config structure
 * to avoid a runtime comparison. But that would also clutter the configuration and expose this
 * to the user. An alternative would be a set of global but non exported function pointers.
 * That would actually be nicer, but it can be done later, it works for now.
 */

void csp_id_prepend(csp_packet_t *packet)
{
    csp_id1_prepend(packet);
}

int csp_id_strip(csp_packet_t *packet)
{
    packet->timestamp_rx = 0;

    return csp_id1_strip(packet);
}

int csp_id_setup_rx(csp_packet_t *packet)
{

    csp_id1_setup_rx(packet);
    return CSP_ID1_HEADER_SIZE;
}

unsigned int csp_id_get_host_bits(void)
{

    return CSP_ID1_HOST_SIZE;
}

unsigned int csp_id_get_max_nodeid(void)
{

    return (1 << CSP_ID1_HOST_SIZE) - 1;
}

unsigned int csp_id_get_max_port(void)
{

    return ((1 << (CSP_ID1_PORT_SIZE)) - 1);
}

int csp_id_is_broadcast(uint16_t addr, csp_iface_t *iface)
{
    uint16_t hostmask = (1 << (csp_id_get_host_bits() - iface->netmask)) - 1;
    uint16_t netmask = (1 << csp_id_get_host_bits()) - 1 - hostmask;
    if (((addr & hostmask) == hostmask) && ((addr & netmask) == (iface->addr & netmask)))
    {
        return 1;
    }

    if (addr == csp_id_get_max_nodeid())
    {
        return 1;
    }
    return 0;
}
