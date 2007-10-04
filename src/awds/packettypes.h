#ifndef _PACKETTYPES_H__
#define _PACKETTYPES_H__

/* broadcast types */

#define PACKET_TYPE_BC_CS_DETECT_INIT_AND_PROBE 43


#define PACKET_TYPE_BC_METRIC 97

#define PACKET_TYPE_BC_ETHERNET_ENCAP  98
#define PACKET_TYPE_BC_ETHERNET_CRYPT  99
#define PACKET_TYPE_BC_TOPO_LOCK      100




/* unicast types */

#define PACKET_TYPE_UC_PING                  42
#define PACKET_TYPE_UC_CS_DETECT_RESULTS     43
#define PACKET_TYPE_UC_CS_DETECT_REQ_RESULTS 44

#define PACKET_TYPE_TRAFFIC 96
#define PACKET_TYPE_UC_METRIC 97

#define PACKET_TYPE_UC_ETHERNET_ENCAP        98
#define PACKET_TYPE_UC_ETHERNET_CRYPT        99





#endif //PACKETTYPES_H__
/* This stuff is for emacs
 * Local variables:
 * mode:c++
 * c-basic-offset: 4
 * End:
 */
