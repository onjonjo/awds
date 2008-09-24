#ifndef __AWDS_DEFINES_H_
#define __AWDS_DEFINES_H_

/*
 * The AWDS socket type identifier
 * this must be passed to the socket
 * creation function in userspace, to
 * get a socket from this kernel module
 */
#define PF_AWDS             32

/*
 * The AWDS ethernet type, all ethernet frames
 * with this type will be received inside
 * this kernel module
 */
#define ETH_P_AWDS          0x8334

#define UINT                unsigned int

/*
 * AWDS header length
 */
#define AWDS_HLEN       23

/*
 * Destination Offset inside AWDS header
 */
#define AWDS_DEST_OFFSET 9

/*
 * NextHop Offset inside AWDS header
 */
#define AWDS_NEXTHOP_OFFSET 15
/*
 * TTL Offset inside AWDS header
 */
#define AWDS_TTL_OFFSET 21

/*
 * The flood type inside flood packets
 */
#define AWDS_FLOODTYPE_OFFSET 17

/* we use 'k' as our ioctl magic number */
#define AWDS_IOC_MAGIC 'k'

/* set routing table */
#define AIOCSRTTBL _IOW(AWDS_IOC_MAGIC, 0, char *)
/* set configuration number */
#define AIOCSCFGNR _IOW(AWDS_IOC_MAGIC, 1, int *)
/* get configuration number */
#define AIOCGCFGNR _IOR(AWDS_IOC_MAGIC, 2, int *)

/* the maximum number inside the ioctl cmd */
#define AWDS_IOC_MAX_NR 2

/*
 * AWDS packet types
 */
#define PACKET_BEACON   0
#define PACKET_FLOOD    1
#define PACKET_UNICAST  2
#define PACKET_FOREWARD 3

/*
 * AWDS flood types
 */
#define FLOOD_PACKET_TOPO 0
#define FLOOD_PACKET_MCAST 1
#define FLOOD_PACKET_PRIV 2

/*
 * The maximum count of awds user daemons
 * registered with this kernel module
 * For one interface exactly one (1) awds
 * instance can be registered
 *
 * One awds user daemons can register for
 * multiple interfaces
 */
#define MAX_AWDS_COUNT      4
#define MAX_AWDS_INDEX      (MAX_AWDS_COUNT-1)

/*
 * the name for the awds instance
 * can have up to this number of characters
 */
#define MAX_AWDS_NAME       64

/*
 * The maximum timeintervall, unicast addresses
 * may stay non-updated in the unicast_dest_table
 * in seconds
 */
#define MAX_AWDS_UC_INTERVAL    15

/*
 * Return values
 *
 */
#define EAWDSINVAL          1   /* invalid arguments */
#define EAWDSEXIST          2   /* element exist already in the list */
#define EAWDSFULL           3   /* instance array is full */
#define EAWDSNOMUTEX        4   /* mutex is not hold inside function */
#define EAWDSMISSING        5   /* element not found in the list */

/*
 * Module Configuration
 *
 * We can dynamically enable or disable certain parts of the module
 * with the given configuration parameter
 */
#define AWDS_LOCAL_CONFIG (1<<0)
#define AWDS_FORWARD_CONFIG (1<<1)

#endif

