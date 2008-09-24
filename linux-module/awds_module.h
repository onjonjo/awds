#include <linux/module.h>	    /* all modules */
#include <linux/kernel.h>	    /* KERN_ALERT */
#include <linux/init.h>		    /* __init and __exit macros? */

#include <linux/etherdevice.h>
#include <linux/netdevice.h>    /* struct packet_type */
#include <linux/if_tun.h>       /* TUN_FASYNC */
#include <linux/if_arp.h>       /* ARPHRD_ETHER */
#include <linux/if_ether.h>	    /* struct ethhdr */
#include <linux/ip.h>		    /* struct iphdr */

#include <linux/stat.h>         /* sysfs */

#include <linux/mutex.h>

#include <linux/poll.h>

#include <linux/ktime.h>

#include <linux/types.h>
#include <linux/net.h>
#include <linux/socket.h>
#include <net/sock.h>           /* all sock_no_* function pointers */
#include <net/tcp_states.h>     /* TCP Socket states */

/*
 * #define's used inside this kernel module
 *
 */
#include "awds_defines.h"

/*
 * debugging help-functions
 *
 */
#include "awds_debug.h"

#ifdef DEBUG
/*
 * This is the debug level used for defining different levels
 * of output priority
 *
 * 0: output nothing
 * 1: error
 * 2: warning
 * 3: normal information
 * 4: function entry points, function exit points
 * 5: debug information
 * 6: verbose
 * 7: extra verbose
 * 8: very frequent messages
 * 9: output everything
 *
 */
static short int debuglevel = 0;
#endif

/*
 * This is the current module configuration, per default everything is enabled
 * this settings can be overwritten with 'configuration' module parameter
 */
static short int configuration = AWDS_LOCAL_CONFIG | AWDS_FORWARD_CONFIG;

/*
 * The setting if packet statistics are collected
 * this setting can be changed with the 'statistics' module parameter
 * the statistics collection is enabled per default
 */
static short int statistics = 1;

/*
 * struct forward declarations
 */
static struct packet_type awds_packet_type;
struct awds_instance;
struct awds_packet_queue;
struct awds_sock;

/*
 * Kernel module loading and unloading
 */
int     awds_init_module(void);
void    awds_cleanup_module(void);

int     awds_packet_rcv(struct sk_buff *skb,
                        struct net_device *dev,
                        struct packet_type *pt,
                        struct net_device *real_dev);

/*
 * AWDS Socket Functions
 */
int     awds_init_socket_handler(void);
void    awds_release_socket_handler(void);

int     awds_create_socket(struct net *net,
                       struct socket *sock,
                       int protocol);

int     awds_socket_release(struct socket *sock);

int     awds_socket_bind(struct socket *sock,
                     struct sockaddr *umyaddr,
                     int sockaddr_len);

int     awds_socket_connect(struct socket *sock,
		                struct sockaddr *uservaddr,
		                int sockaddr_len, int flags);
		       
int     awds_socket_socketpair(struct socket *sock1, 
			               struct socket *sock2);
			  
int     awds_socket_accept(struct socket *sock, 
		               struct socket *newsock,
		               int flags);
		      
int     awds_socket_getname(struct socket *sock, 
		                struct sockaddr *uaddr,
		                int *usockaddr_len,
		                int peer);
		       
UINT    awds_socket_poll(struct file *file,
			                  struct socket *sock,
			                  struct poll_table_struct *wait);

int     awds_socket_ioctl(struct socket *sock, 
		              unsigned int cmd,
		              unsigned long arg);    


int     awds_socket_listen(struct socket *sock,
                       int backlog);

int     awds_socket_shutdown(struct socket *sock,
                         int flags);

int     awds_socket_setsockopt(struct socket *sock,
			               int   level,
			               int   optname,
			               char *optval,
			               int   optlen);

int     awds_socket_sendmsg(struct kiocb *iocb,
		                struct socket *sock, 
		                struct msghdr *m,
		                size_t total_len);

int     awds_socket_recvmsg(struct kiocb *iocb,
		                struct socket *sock,
		                struct msghdr *m,
		                size_t total_len,
		                int    flags);
		          
int     awds_socket_mmap(struct file *file,
		             struct socket *sock,
		             struct vm_area_struct *vma);
		    
ssize_t awds_socket_sendpage(struct socket *sock,
			                 struct page *page,
			                 int    offset,
			                 size_t size,
			                 int    flags);

int     awds_socket_getsockopt(struct socket *sock,
			               int   level,
			               int   optname,
			               char *optval,
			               int  *optlen);

struct sockaddr_awds {
	sa_family_t	sa_family;	              /* address family, PF_AWDS       */
    int         sa_ifindex;               /* the network interface index   */
    char        sa_addr[ETH_ALEN];        /* the network interface address */
	char		sa_data[14 - sizeof(int) - ETH_ALEN];	  /* additional information   	   */
};

/*
 * This functions add a new interface to the interface list
 * of the given awds instance
 *
 * return:
 *  0: if the interface is in use of another instance
 *  interf: if the interface is new for this intance and not registerd in another instance
 *
 *  when the interface is already registered, the old registered struct awds_interface pointer
 *  is returned 
 *
 */
struct awds_interface* awds_add_interf(struct awds_instance* awds, struct awds_interface *interf);

/*
 * mutex for locking
 *
 */
static DEFINE_MUTEX(awds_instance_mutex);

/*
 * the number of awds instances
 * currently attached to the kernel module
 */
static atomic_t awds_instance_index;

/*
 * unicast destination timestamp table
 */
struct awds_uc_dest_table
{
    struct awds_uc_dest_table*  prev;
    struct awds_uc_dest_table*  next;

    char                        mac[ETH_ALEN];
    /* readable string */
    char                        mac_string[MAC_BUF_SIZE];

    long                        time;
};

/*
 * this structure represents one awds user daemon registered
 * with this kernel module (through bind)
 */
struct awds_instance
{
    /*
     * the index of this instance inside all
     * attached awds instances
     */
    int                         index;

    /*
     * the sock structure used for identifying this
     * instance
     */
    struct awds_sock*           sock;

    struct mutex                lock;

};

/*
 * this structure represents one
 * interface, one awds can be bound to
 */
struct awds_interface
{
    /*
     * list navigation
     */
    struct awds_interface*      next;
    struct awds_interface*      prev;

    /*
     * the current awds interface index
     * this is the interface index inside the kernel!
     */
    int                         index;
    /*
     * the real netinterface
     */
    struct net_device*          dev;
    /*
     * the address this interface is bound to
     */
    struct sockaddr_awds*       addr;
};

struct awds_sock
{
    struct sock                 sk;
    /*
     * the number of interfaces this socket is bound to
     */
    int                         ifcount;
    /*
     * all interfaces this instance is boud to
     * every call to bind, may add an interface to this list
     */
    struct awds_interface*      interf;

    /*
     * other awds tap interface addresses
     * every time we receive an unknown unicast packet:
     * 
     * - if we don't find the dest in this list:
     *   we add an entry to this list, and remove too old entries
     * 
     * - we find the dest in our list:
     *   we check the timestamp and may let the packet pass through to the
     *   tap interface, or if the timestamp is too old, update timestamp
     *   and pass packet to the awds instance, for tapiface mac-table updates
     */
    struct awds_uc_dest_table*  table;
};

/*
 * one routing entry
 */
struct awds_routing_entry
{
    char   dest[ETH_ALEN];
    char   nexthop[ETH_ALEN];
};

/* packet statistics layout */
struct awds_packet_statistic
{
    __u32   rx_total_packets;
    __u32   rx_total_unicast;
    __u32   rx_total_beacon;
    __u32   rx_total_flood;
    __u32   rx_total_foreward;

    __u32   rx_local_unicast;
    __u32   rx_local_unicast_failed;
    __u32   rx_forwarded_unicast;

    __u32   rx_flood_topo;
    __u32   rx_flood_mcast;
    __u32   rx_flood_private;

    __u32   tx_total;
};

/* packet stats object */
static struct awds_packet_statistic awds_stats = {
    rx_total_packets:   0,
    rx_total_unicast:   0,
    rx_total_beacon:    0,
    rx_total_flood:     0,
    rx_total_foreward:  0,

    rx_local_unicast:   0,
    rx_local_unicast_failed:    0,
    rx_forwarded_unicast:       0,

    rx_flood_topo:      0,
    rx_flood_mcast:     0,
    rx_flood_private:   0,

    tx_total:           0,
};

/*
 * cast any list entry to a routing table entry
 */
#define AWDS_ROUTING_ENTRY(list) ((struct awds_routing_entry*)list)

/*
 * sends an skb over the medium
 */
int awds_send(struct sk_buff *skb);

/*
 * Waits for a new packet on the socket receive queue for the given
 * socket, the timeout from the socket is used
 */
struct sk_buff* awds_socket_peek_skb(struct awds_sock* awds_sk, int flags, int* err);

/*
 * helper functions for manipulating the instances
 * array
 */

/*
 * searches the instance array for a given awds instance,
 * identified through its struct sock pointer !!
 *
 * returns the index if found, -EAWDSMISSING otherwise
 *
 * __* function can only be called with instance lock held
 */
int awds_find_instance_index_by_sock(struct awds_sock* sk);
int __awds_find_instance_index_by_sock(struct awds_sock* sk);

/*
 * searches the instance array for a given awds instance,
 * identified through its struct sock pointer !!
 *
 * returns the instance if found, 0 otherwise
 *
 * __* function can only be called with instance lock held
 */
struct awds_instance* awds_find_instance_by_sock(struct awds_sock* sk);
struct awds_instance* __awds_find_instance_by_sock(struct awds_sock* sk);

/*
 * searches an instance with a given network interface
 * returns the instance found, or 0 if no instance is bound to
 * the given network interface
 *
 * __* function can only be called with instance lock held
 */
struct awds_instance* awds_find_instance_by_dev(struct net_device *dev);
struct awds_instance* __awds_find_instance_by_dev(struct net_device *dev);

/*
 * searches inside an instance for a given network interface
 * returns the interface found, or 0 if no interface of this
 * instance is bound to the given network interface
 *
 * __* function can only be called with interface lock held
 */
struct awds_interface* awds_find_interface(struct awds_instance* instance, struct net_device *dev);
struct awds_interface* __awds_find_interface(struct awds_instance* instance, struct net_device *dev);

/*
 * creates a new awds instance
 * the instance is added to the interface list
 * 
 * return
 *   0: no error occured, new instance created 
 *   the appropriate error code otherwise
 *
 * __* function can only be called with interface lock held 
 */
int __awds_create_instance(struct awds_sock* sk, struct sockaddr_awds* addr);
/*
 * deletes an awds instance, identified through
 * a struct sock pointer
 *
 * __* function can only be called with interface lock held
 */
int __awds_release_instance(struct awds_instance* instance);

/*
 * deletes an existing awds instance
 * the instance is removed from the interface list
 * the structure memory if freed
 *
 * if the pointer does not exist in the
 * instance list, nothing happens! 
 */
int awds_add_instance(struct awds_instance* instance);
/*
 * deletes an existing awds instance
 * the instance is removed from the interface list
 * the structure memory if freed
 *
 * if the pointer does not exist in the
 * instance list, nothing happens! 
 */
int awds_del_instance(struct awds_instance* instance);

/*
 * adds a new instance to the instance array
 * return
 *   >0: the new instance position
 *   otherwise an error is returned
 *
 * mutex(awds_instance_mutex) must be locked inside this function
 */
int __awds_add_instance(struct awds_instance* instance);

/*
 * deletes a instance from the instance array
 * return
 *   0: the instance is removed from the array
 *   otherwise an error is returned
 *
 * mutex(awds_instance_mutex) must be locked inside this function
 */
int __awds_del_instance(struct awds_instance* instance);

/*
 * creates a new awds interface object
 * and adding the new interface to the instance
 */
int awds_create_interface(struct awds_instance* instance, struct sockaddr_awds* addr);

/*
 * removed an awds interface object from an awds instance
 * the overall instance interface count from instance
 * is decremented
 *
 */
int awds_release_interface(struct awds_instance* instance, struct awds_interface* iface);

/*
 * adds an interface to an instance
 * NOTE: only aquires the mutex and calls the __awds_dev_* function
 */
int awds_add_interface(struct awds_instance* instance, struct awds_interface* iface);

/*
 * removes an awds interface from an awds instance
 */
int awds_del_interface(struct awds_instance* instance, struct awds_interface* iface);

/*
 * adds a new awds interface to a given awds instance
 * (but only if the given device is not used by any other instance,
 *  including the given instance itself)
 *
 * return
 *   >0: the new interface index
 *   error id otherwise 
 *
 * mutex(awds_instance_mutex) must be locked inside this function
 */
int __awds_add_interface(struct awds_instance* instance, struct awds_interface* iface);

/*
 * releases a awds interface from a given awds instance
 * every time a awds instance calls bind inside
 * this kernel module, a new awds interface is added to the
 * interface list of the given awds instance
 * (but only if the given device is not used by any other instance,
 *  including the given instance itself)
 *
 * return
 *   >0: the removed interface index
 *   error id otherwise 
 *
 * mutex(awds_instance_mutex) must be locked inside this function
 */
int __awds_del_interface(struct awds_instance* instance, struct awds_interface* interface);

/*
 * returns the awds packet type for the given packet
 *
 * 0 Beacon
 * 1 Flood
 * 2 Unicast -> we route only these packets!
 * 3 Forward
 */
int awds_get_packet_type(struct sk_buff* skb);

/*
 * retrieves the awds packet type
 * checks for a minimum packet length, otherwise the packet is
 * discarded
 */
void awds_count_packet(struct sk_buff* skb);

/*
 * checks is the given packet, can be routed (we knew the nexthop)
 * of if we shall hand it to userspace
 * returns the routing entry containing the next hop
 * or 0 if the packet can not be routed in this kernel module
 */
struct awds_routing_entry* awds_check_routing(struct sk_buff* skb);

/*
 * performs the actual checking
 * mutex(awds_instance_mutex) must be locked inside this function
 */
struct awds_routing_entry* __awds_check_routing(struct sk_buff* skb);

/*
 * send a packet, to the given nexthop
 */
int awds_do_retransmit(struct sk_buff* skb, struct awds_routing_entry* entry);

/*
 * performs the actual sending
 * mutex(awds_instance_mutex) must be locked inside this function
 */
int __awds_do_retransmit(struct sk_buff* skb, struct awds_routing_entry* entry);

/*
 * relay a packet to the appropriate local network interface
 */
int awds_do_local_retransmit(struct sk_buff* skb);

/*
 * performs the actual sending
 * mutex(awds_instance_mutex) must be locked inside this function
 */
int __awds_do_local_retransmit(struct sk_buff* skb);

/*
 * checks the unicast mac table of the given awds socket
 * it removes too old entries, if found, insert new ones
 * and updates existing but not too old entries
 * 
 * return: 1 the packet can be transmitted optmized (direct into the tap device)
 * return: 0 the packet must be feed to the awds instance 
 */
int awds_check_unicast_table(struct awds_sock* sk, struct sk_buff* skb);
