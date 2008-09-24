/*
 * AWDS kernel module for kernel 2.6
 *
 * Description:
 * 
 * Add a packet_type handler for AWDS packets
 * Specifically, we will register our packet_type to be
 * the first handler invoked by netif_receive_skb()
 *
 * Implement new AWDS socket type inside the kernel
 *
 * Author: 
 *   - Johannes Pfeiffer
 * 
 * Licence: GNU/GPL
 *
*/
#include "awds_module.h"

static inline struct awds_sock* awds_sk(const struct sock *sk)
{
    return (struct awds_sock *)sk;
}

static struct proto awds_proto_ops = {
    owner:         THIS_MODULE,
    name:          "AWDS",
    obj_size:      sizeof(struct awds_sock)
};

/*
 * The "packet_statistics" file where statistics are read from.
 */
static ssize_t awds_sysfs_show(struct kobject *kobj, struct kobj_attribute *attr,
			char *buf)
{
    int ret = 0;

    /* write statistics to buffer, we have only PAGE_SIZE byte available */
    ret = snprintf(buf, PAGE_SIZE, "total rx packets:\t%d\n"
                        "total rx unicasts:\t%d\n"
                        "total rx beacon:\t%d\n"
                        "total rx flood:\t\t%d\n"
                        "  topology:\t\t%d\n"
                        "  multicast:\t\t%d\n"
                        "  private:\t\t%d\n"
                        "total rx flow:\t\t%d\n\n"
                        "local unicast:\t\t%d (%d failed)\n"
                        "forwarded unicast:\t%d\n\n"
                        "total tx packets:\t%d\n",
                        awds_stats.rx_total_packets,
                        awds_stats.rx_total_unicast,
                        awds_stats.rx_total_beacon,
                        awds_stats.rx_total_flood,
                        awds_stats.rx_flood_topo,
                        awds_stats.rx_flood_mcast,
                        awds_stats.rx_flood_private,
                        awds_stats.rx_total_foreward,
                        awds_stats.rx_local_unicast,awds_stats.rx_local_unicast_failed,
                        awds_stats.rx_forwarded_unicast,
                        awds_stats.tx_total);

    /* the buffer is truncated at PAGE_SIZE bytes */
    if(ret > PAGE_SIZE)
        ret = PAGE_SIZE;
    return ret;
}

/*
 * sysfs statistics attribute
 */
static struct kobj_attribute awds_attribute =
	__ATTR(packet_stats, S_IRUGO, awds_sysfs_show, NULL);

static struct attribute *awds_attrs[] = {
	&awds_attribute.attr,
	NULL,	/* need to NULL terminate the list of attributes */
};

static struct attribute_group awds_attr_group = {
    .name = "statistics",
	.attrs = awds_attrs,
};

/* module configuration output */
static inline void awds_configuration_info(void)
{
    printk(KERN_INFO "packet forwarding configuration: %s\n",
           (configuration & AWDS_FORWARD_CONFIG)?"enabled":"disabled");
    printk(KERN_INFO "local delivery configuration: %s\n",
           (configuration & AWDS_LOCAL_CONFIG)?"enabled":"disabled");
}

/*
 * our global routing information
 * this is an array with multiple entries, this array must be sorted!
 */
static struct awds_routing_entry* awds_routing;
/*
 * the number of routing entries
 */
static UINT   awds_routing_count;

/*
 * struct with function pointers to the handling functions 
 */
static struct proto_ops awds_socket_ops = {
	family:		PF_AWDS,
	release:	awds_socket_release,
	bind:		awds_socket_bind,
	connect:	sock_no_connect,
	socketpair:	sock_no_socketpair,
	accept:		sock_no_accept,
	getname:	sock_no_getname,
	poll:		awds_socket_poll,
	ioctl:		awds_socket_ioctl,
	listen:		sock_no_listen,
	shutdown:	sock_no_shutdown,
	setsockopt:	sock_no_setsockopt,
	getsockopt:	sock_no_getsockopt,
	sendmsg:	awds_socket_sendmsg,
	recvmsg:	awds_socket_recvmsg,
	mmap:		sock_no_mmap,
	sendpage:	sock_no_sendpage
};

/*
 * struct with function pointers to the socket creation
 * function
 */ 
static struct net_proto_family awds_family_ops = {
	family:         PF_AWDS,
	create:         awds_create_socket,
    owner:          THIS_MODULE
};

/*
 * all awds instances
 * when all slots are filled, no more instances can connect
 */
static struct awds_instance*    awds_instances[MAX_AWDS_COUNT];

/*
 * awds_init_socket_handler - initialize socket handler for PF_AWDS sockets
 */ 
int awds_init_socket_handler(void)
{	
    int err = 0;    

    AWDS_FUNC_IN(5);

    AWDS_INFO("registering socket handler\n");
    
    err = sock_register(&awds_family_ops);

	AWDS_INFO("sock_register returned: %d\n",err);

	AWDS_RETURN err;
}

/*
 * awds_release_socket_handler - deinitializes the socket handler for PF_AWDS sockets
 */ 
void awds_release_socket_handler(void)
{
    AWDS_FUNC_IN(5);
	
    sock_unregister(awds_family_ops.family);

    AWDS_RETURN;
}

/*
 * awds_create_socket - creates a new AWDS socket
 */ 
int awds_create_socket(struct net *net, struct socket *sock, int protocol)
{
	int err = 0;
    struct sock *sk = 0;

    AWDS_FUNC_IN(5);
	
	sock->ops = &awds_socket_ops;
	sk = sk_alloc(net, PF_AWDS, GFP_KERNEL, &awds_proto_ops);

    if(sk == 0) {
        err = -ENOBUFS;
    }

    sock_init_data(sock, sk);

    /*
     *everything went fine?
     */
	AWDS_RETURN err;
}

/*
 * awds_socket_release - release an awds socket
 */
int awds_socket_release(struct socket *sock)
{
    int ret = 0;    
    struct awds_sock*      sk = awds_sk(sock->sk);    
    struct awds_instance*  instance = 0;

    AWDS_FUNC_IN(5);
    AWDS_INFO("sock: %ld\n", (long)sock->sk);

    mutex_lock(&awds_instance_mutex);
    /*
     * check if we have a bind for this socket
     */
    instance = __awds_find_instance_by_sock(sk);

    if(instance) {
        AWDS_INFO("release instance number %d\n", instance->index);
        /*
         * release the given instance
         */
        ret = __awds_release_instance(instance);
    }
    else {
        AWDS_INFO("no instance found for socket\n");
        sock_put(&sk->sk);
        ret = -EAWDSINVAL;
    }

    mutex_unlock(&awds_instance_mutex);

    /*
     *socket closed successfully
     */
    AWDS_RETURN 0;
}

int awds_socket_bind(struct socket *sock, 
		             struct sockaddr *umyaddr,
		             int sockaddr_len)
{
    int res = 0;    
    struct awds_sock*       sk = awds_sk(sock->sk);
    struct sockaddr_awds*   addr = 0;
    /*
     * this is our instance, the bind belongs to
     * (identified through the struct sock pointer)
     */
    struct awds_instance*  instance = 0;

    AWDS_FUNC_IN(5);

    addr = (struct sockaddr_awds*)umyaddr;
    
    mutex_lock(&awds_instance_mutex);

    instance = __awds_find_instance_by_sock(sk);

    if(instance) {
        /*
         * add a new interface to the existing instance
         * this call will fail if this instance is already bound to this address
         */
        res = awds_create_interface(instance, addr);
    }
    else {   
        /*
         * create a new instance and add the new interface
         */
        res = __awds_create_instance(sk, addr);
    }
    mutex_unlock(&awds_instance_mutex);

    AWDS_RETURN res;
}

int awds_socket_ioctl(struct socket *sock, 
		     unsigned int cmd,
		     unsigned long arg)
{
    int data = 0;
    int err = 0;

    AWDS_FUNC_IN(5);

    /*
     * we need this checks, since other drivers may want to use some other
     * bit ranges for AWDS_IOC_MAGIC type
     * extract the type and number bitfields, and don't decode
     * wrong cmds: return ENOTTY (inappropriate ioctl)
     */
    if ((_IOC_TYPE(cmd) == AWDS_IOC_MAGIC) && (_IOC_NR(cmd) > AWDS_IOC_MAX_NR)) {
        err = -ENOTTY;
        goto out;
    }

    /*
     * the direction is a bitmask, and VERIFY_WRITE catches R/W
     * transfers. `Type' is user-oriented, while
     * access_ok is kernel-oriented, so the concept of "read" and
     * "write" is reversed
     */
    if (_IOC_DIR(cmd) & _IOC_READ) {
        err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
    }
    else if (_IOC_DIR(cmd) & _IOC_WRITE) {
        err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
    }
    if(err) {
        err = -EFAULT;
        goto out;
    }

    /* everything ok, we decode the ioctl */
    switch (cmd) {
        case AIOCSCFGNR:
        {
            err = copy_from_user(&data, (void __user *)arg, sizeof(int));
            if (err)
                goto out;

            if(data != configuration) {
                printk(KERN_INFO "new configuration set:\n");                
                configuration = data;
                awds_configuration_info();
            }
            else
                printk(KERN_INFO "configuration unchanged\n");
    
            goto out;
        }
        case AIOCGCFGNR:
        {
            err = copy_to_user((void __user *)arg, &configuration, sizeof(short int));
            goto out;
        }
        case AIOCSRTTBL:
        {
            err = copy_from_user(&data, (void __user *)arg, sizeof(int));
            if (err)
                goto out;

            AWDS_INFO("new routing table: %d bytes\n",data);

            if((data > 0) && ((data % sizeof(struct awds_routing_entry)) == 0)) {
            
                mutex_lock(&awds_instance_mutex);
                if(awds_routing != 0) {
                    kfree(awds_routing);
                    awds_routing = 0;
                }
                awds_routing = (struct awds_routing_entry*)kmalloc(data, GFP_KERNEL);

                if(!awds_routing) {
                    err = -ENOMEM;
                    goto out_unlock;
                }

                if(copy_from_user((void*)awds_routing, (void __user *)(arg+sizeof(int)), data)) {                    
                    goto out_unlock;
                }

                awds_routing_count = (data/sizeof(struct awds_routing_entry));
                AWDS_INFO("number of entries: %d\n", awds_routing_count);
                mutex_unlock(&awds_instance_mutex);            
            }
            goto out;
        }
        default:
        {
            err = -ENOIOCTLCMD;
            break;
        }
    }
    
out_unlock:
    mutex_unlock(&awds_instance_mutex);
out:
    AWDS_RETURN err;
}

/*
 * Wait for a packet..
 */
static int wait_for_packet(struct sock *sk, int *err, long *timeo_p)
{
        int error;
        DEFINE_WAIT(wait);

        prepare_to_wait_exclusive(sk->sk_sleep, &wait, TASK_INTERRUPTIBLE);

        /* Socket errors? */
        error = sock_error(sk);
        if (error)
                goto out_err;

        if (!skb_queue_empty(&sk->sk_receive_queue))
                goto out;

        /* Socket shut down? */
        if (sk->sk_shutdown & RCV_SHUTDOWN)
                goto out_noerr;

        /* handle signals */
        if (signal_pending(current))
                goto interrupted;

        error = 0;
        *timeo_p = schedule_timeout(*timeo_p);
out:
        finish_wait(sk->sk_sleep, &wait);
        return error;
interrupted:
        error = sock_intr_errno(*timeo_p);
out_err:
        *err = error;
        goto out;
out_noerr:
        *err = 0;
        error = 1;
        goto out;
}

/* statistics update */
void awds_count_packet(struct sk_buff* skb)
{
    /* minimum packet length required */
    if(unlikely(skb->len < (ETH_ALEN + 3))) return;

    awds_stats.rx_total_packets++;
    switch((int)skb->data[0]) {
        case PACKET_BEACON:
        {
            awds_stats.rx_total_beacon++;
            break;
        }        
        case PACKET_FLOOD:
        {
            awds_stats.rx_total_flood++;
            /* flood sub type */
            if(skb->len >= AWDS_FLOODTYPE_OFFSET) {
                switch((int)skb->data[AWDS_FLOODTYPE_OFFSET]) {
                    case FLOOD_PACKET_TOPO:
                        awds_stats.rx_flood_topo++;
                        break;
                    case FLOOD_PACKET_MCAST:
                        awds_stats.rx_flood_mcast++;
                        break;
                    case FLOOD_PACKET_PRIV:
                        awds_stats.rx_flood_private++;
                        break;
                    default:
                        break;
                }
            }
            break;
        }
        case PACKET_UNICAST:
        {
            awds_stats.rx_total_unicast++;
            break;
        }
        case PACKET_FOREWARD:
        {
            awds_stats.rx_total_foreward++;
            break;
        }
        default:
            break;
    }
}

int awds_get_packet_type(struct sk_buff* skb)
{
    int type = -1;

    AWDS_FUNC_IN(7);

    /*
     * we need at least 1 * ethernet address, 1 byte data and 2 byte sequence number
     */
    if(unlikely(skb->len < (ETH_ALEN + 3))) {
        AWDS_INFO("len error: %d < %d\n", skb->len, (ETH_ALEN + 3));
        AWDS_RETURN type;
    }
    
    type = (int)skb->data[0];

    AWDS_INFO("packet type: %d\n", type);

    AWDS_RETURN type;
}

struct awds_routing_entry* awds_check_routing(struct sk_buff* skb)
{
    struct awds_routing_entry* entry = 0;

    mutex_lock(&awds_instance_mutex);
    
    entry = __awds_check_routing(skb);

    mutex_unlock(&awds_instance_mutex);

    return entry;
}

struct awds_routing_entry* __awds_check_routing(struct sk_buff* skb)
{
    int cmp = 0;
    int head = 0;
    int mid = 0;
    int end = awds_routing_count;
    struct awds_routing_entry entry;

    if(awds_routing_count == 0) {
        return 0;
    }

    /*
     * extract destination address from skb
     * 1 byte type
     * 6 byte dest
     * 2 byte ttl
     */
    memcpy(entry.dest, skb->data + AWDS_DEST_OFFSET, ETH_ALEN);

    /* do a binary search */
    while(head != end) {
        /* prevent integer overflow */
        mid = head + (end - head) / 2;

        cmp = memcmp(awds_routing[mid].dest, entry.dest, ETH_ALEN);

        if(cmp == 0) {
            /* found */
            return &awds_routing[mid];
        }
        else if(cmp < 0) {
            /* between mid and end */
            head = mid +1;
        }
        else if(cmp > 0) {
            /* between head and mid */
            end = mid;
        }
    }

    return 0;
}

int awds_do_retransmit(struct sk_buff* skb, struct awds_routing_entry* entry)
{
    int err = 0;

    AWDS_FUNC_IN(7);

    mutex_lock(&awds_instance_mutex);

    err = __awds_do_retransmit(skb, entry);    

    mutex_unlock(&awds_instance_mutex);

    AWDS_RETURN err;
}

int __awds_do_retransmit(struct sk_buff* skb, struct awds_routing_entry* entry)
{
    int err = -EINVAL;
    unsigned char ttl = 0;

    AWDS_FUNC_IN(6);

    /* get current ttl */
    memcpy(&ttl, skb->data + AWDS_TTL_OFFSET, sizeof(unsigned char));
    
    if(--ttl == 0) {
        AWDS_RETURN err;    
    }
    
    /* copy ttl back into packet buffer */
    memcpy(skb->data + AWDS_TTL_OFFSET, &ttl, sizeof(unsigned char));

    /* copy dest address to new src addr in ethernet header */
    memcpy(skb->data - ETH_ALEN - 2, skb->data - ETH_HLEN, ETH_ALEN);

    /* update dest in ethernet header to the given nexthop */
    memcpy(skb->data - ETH_HLEN, entry->nexthop, ETH_ALEN);

    /* update nexthop information inside awds header */
    memcpy(skb->data + AWDS_NEXTHOP_OFFSET, entry->nexthop, ETH_ALEN);

    /*
     * readd ethernet header as payload
     */
    skb_push(skb, ETH_HLEN);

    err = awds_send(skb);

    AWDS_RETURN err;
}

static inline void awds_instert_unicast_table(struct awds_sock* sk, struct sk_buff* skb)
{
    if(sk->table) {
        /* find first, normally no while iteration necessary */
        while(sk->table->prev) {    
            sk->table = sk->table->prev;
        };
        sk->table->prev = (struct awds_uc_dest_table*)kmalloc(sizeof(struct awds_uc_dest_table), GFP_KERNEL);
        if(sk->table->prev) {        
            sk->table->prev->next = sk->table;
            sk->table = sk->table->prev;
        }
        else
            return;
    }
    else {
        sk->table = (struct awds_uc_dest_table*)kmalloc(sizeof(struct awds_uc_dest_table), GFP_KERNEL);
        if(sk->table)
            sk->table->next = 0;
        else
            return;
    }

    if(sk->table) {
        sk->table->prev = 0;
        memcpy(sk->table->mac, &skb->data[AWDS_HLEN], ETH_ALEN);
        print_mac(sk->table->mac_string, sk->table->mac);
        sk->table->time = CURRENT_TIME_SEC.tv_sec;
        printk(KERN_INFO "insert unicast entry: %s - %ld\n", sk->table->mac_string, sk->table->time);
    }
}

int awds_check_unicast_table(struct awds_sock* sk, struct sk_buff* skb)
{
    /* per default, no local transmitting */
    struct awds_uc_dest_table* tbl = sk->table;
    struct awds_uc_dest_table* tmp = 0;
    /* hope the best, we can transmit it the optimized way */
    int local = 1;
    
    if(!sk || !skb || (skb->len < (AWDS_HLEN + ETH_ALEN))) {
        local = 0;
        goto out;
    }

    while(tbl) {
        /* try to find our destination */
        if(likely(!memcmp(tbl->mac, &skb->data[AWDS_HLEN], ETH_ALEN))) {
            /* found */
            break;
        }
        /* remove old entries */
        else {
            if(unlikely((CURRENT_TIME_SEC.tv_sec - tbl->time - MAX_AWDS_UC_INTERVAL) > 0)) {
                /* too old, remove */
                printk(KERN_INFO "too old, remove unicast entry: %s - %ld\n", tbl->mac_string, tbl->time);
                if(tbl->prev)
                    tbl->prev->next = tbl->next;
                if(tbl->next)
                    tbl->next->prev = tbl->prev;
                tmp = tbl->next;
                kfree(tbl);
                tbl = tmp;
            }
            else {
                tbl = tbl->next;
            }
        }
    };

    /* entry found */
    if(likely(tbl)) {
        /* too old, force update in user space */
        if(unlikely((CURRENT_TIME_SEC.tv_sec - tbl->time - MAX_AWDS_UC_INTERVAL) > 0)) {
            local = 0;
            tbl->time = CURRENT_TIME_SEC.tv_sec;
            printk(KERN_INFO "flush unicast entry: %s - %ld\n", tbl->mac_string, tbl->time);
        }
        else {
            local = 1;
        }
    }
    /* missing, or first */
    else {
        awds_instert_unicast_table(sk, skb);
        /* force update in user space */
        local = 0;
    }

out:
    return local;
}

int awds_do_local_retransmit(struct sk_buff* skb)
{
    int err = 0;

    AWDS_FUNC_IN(7);

    mutex_lock(&awds_instance_mutex);

    err = __awds_do_local_retransmit(skb);

    mutex_unlock(&awds_instance_mutex);

    AWDS_RETURN err;
}

int __awds_do_local_retransmit(struct sk_buff* skb)
{
    int err = -ENODEV;
    struct net_device *dev = 0;
    struct net_device *old_dev = 0;
    __u16 protocol = 0;

    AWDS_FUNC_IN(7);

    rtnl_lock();
    dev = dev_getbyhwaddr(&init_net, ARPHRD_ETHER, skb->data + AWDS_HLEN);
    /* grab a reference */
    if (dev)
        dev_hold(dev);
    rtnl_unlock();

    if(dev) {
        /*
         * we can possibly do a local retransmit
         */
        AWDS_INFO("local packet for: %s device\n", dev->name);

        /*
         * remove AWDS header from packet
         */
        skb_pull(skb, AWDS_HLEN);
        skb_reset_network_header(skb);

        old_dev = skb->dev;
        protocol = skb->protocol;

        /* copy the protocol from the ethernet packet */
        skb->protocol = eth_type_trans(skb, dev);        
        skb->dev = dev;

        /*
         * TODO: we must assure that this is a tun device!!
         */
        err = netif_rx_ni(skb);
        dev_put(dev);

        /*
         * sending failed, undo changes
         */
        if(err) {
            AWDS_INFO("local packet delivery failed\n");
            if(likely(statistics==1)) awds_stats.rx_local_unicast_failed++;
            skb_push(skb, AWDS_HLEN);
            skb_reset_network_header(skb);
            skb->dev = old_dev;
            skb->protocol = protocol;
        }
        else {
            if(likely(statistics==1)) awds_stats.rx_local_unicast++;
        }
    }

    AWDS_RETURN err;
}


/*
 * The packet handler
 */
int awds_packet_rcv(struct sk_buff *skb,
                    struct net_device *dev,
                    struct packet_type *pt,
                    struct net_device *real_dev)
{
    int self = 0;
    struct sock* sk = 0;
    struct awds_instance* instance = 0;
    struct awds_interface* interface = 0;
    struct awds_routing_entry* entry = 0;

    AWDS_FUNC_IN(8);

    if (skb->protocol == htons(ETH_P_AWDS)) {

        AWDS_INFO("awds packet from %s\n", skb->dev->name);

        //mutex_lock(&awds_instance_mutex);

        instance = __awds_find_instance_by_dev(skb->dev);
        
        if(instance) {
            if(likely(statistics==1)) awds_count_packet(skb);
            AWDS_INFO("packet for instance %d\n", instance->index);

            sk = &instance->sock->sk;

            /*
             * check if we can route this packet by ourselve
             * and we are not the packet's destination!
             */
            if(awds_get_packet_type(skb) == PACKET_UNICAST) {                                
                interface = awds_find_interface(instance, dev);
                /* check if we are the AWDS destination of this packet */
                self = memcmp(interface->addr->sa_addr, skb->data + AWDS_DEST_OFFSET, ETH_ALEN);
                /* configuration must include local delivery of packets */
                if((self == 0) && (configuration & AWDS_LOCAL_CONFIG)) {
                    
                    AWDS_INFO("local packet found:\n");

                    /* try to deliver the packet to the local destination */        
                    if(awds_check_unicast_table(awds_sk(sk), skb) &&
                       __awds_do_local_retransmit(skb) == 0) {
                        /* we are finished with this skb */
                        goto return_ok;
                    }
                }

                /*
                 * check if we can retransmit this packet, with
                 * our current routing information
                 */
                entry = __awds_check_routing(skb);

                /*
                 * we route only non local packets
                 * check if we have the correct configuration
                 */
                if((self != 0) && (entry!=0)  && (configuration & AWDS_FORWARD_CONFIG)) {
                    
                    AWDS_INFO("entry found: %ld\n",
                              ((long)entry - (long)awds_routing)/(sizeof(struct awds_routing_entry)));
        
                    /* try to resend the packet over the wireless medium */
                    if(__awds_do_retransmit(skb, entry) == 0) {
                        if(likely(statistics==1)) awds_stats.rx_forwarded_unicast++;
                        /*
                         * we are finished with this skb
                         */
                        goto return_ok;
                    }
                }
            }

            /*
             * the packet could not be delivered directly to the local device,
             * or it could not be successful relayed
             *
             * or more simply it is a packet which must be copied to awds in user space
             * (Beacon, Topology)
             */
            if(sock_queue_rcv_skb(sk, skb)) {   
                goto skb_free;
            }

            /*
             * we have packets, wake up wait-queue
             */
            wake_up(sk->sk_sleep);

            goto return_ok;
        }
        else {
            AWDS_INFO("no instance bound to %s\n", dev->name);
            goto skb_free;
        }		
	}
    else {
        goto return_nolock;
    }

skb_free:
	kfree_skb(skb);
return_ok:
    //mutex_unlock(&awds_instance_mutex);
return_nolock:
	AWDS_RETURN 0;
}

struct sk_buff* awds_socket_peek_skb(struct awds_sock* awds_sk, int flags, int* err)
{
    struct sock* sk = &awds_sk->sk;
    struct sk_buff *skb = 0;
    long timeo = 0;
    int error = sock_error(sk);

    AWDS_FUNC_IN(8);

    if (error) {
        goto no_packet;
    }

    timeo = sock_rcvtimeo(sk, flags & MSG_DONTWAIT);

    do {
            skb = skb_dequeue(&sk->sk_receive_queue);
            if (skb) {
                AWDS_RETURN skb;
            }

            /* User doesn't want to wait */
            error = -EAGAIN;
            if (!timeo) {
                goto no_packet;
            }

    } while (!wait_for_packet(sk, err, &timeo));

    AWDS_RETURN 0;

no_packet:
    *err = error;
    AWDS_RETURN 0;
}

int awds_send(struct sk_buff *skb)
{
    int err;
    
    AWDS_FUNC_IN(8);

    if (!(skb->dev->flags & IFF_UP)) {
            kfree_skb(skb);
            AWDS_RETURN -ENETDOWN;
    }

    /* send to netdevice */
    err = dev_queue_xmit(skb);
    if (err > 0) {
       err = net_xmit_errno(err);
    }

    if (err) {
       AWDS_INFO("sending of %d bytes failed (reason %d)\n", skb->len, err);
       AWDS_RETURN err;
    }

    AWDS_RETURN 0;
}

unsigned int awds_socket_poll(struct file *file, struct socket *sock,
                           poll_table *wait)
{
    struct sock *sk = sock->sk;
    unsigned int mask;

    AWDS_FUNC_IN(8);

    poll_wait(file, sk->sk_sleep, wait);
    mask = 0;

    /* exceptional events? */
    if (sk->sk_err || !skb_queue_empty(&sk->sk_error_queue)) {
        mask |= POLLERR;
    }

    /* readable? */
    if (!skb_queue_empty(&sk->sk_receive_queue) || (sk->sk_shutdown & RCV_SHUTDOWN)) {
        mask |= POLLIN | POLLRDNORM;
    }

    /* writable? */
    if (sock_writeable(sk)) {
        mask |= POLLOUT | POLLWRNORM | POLLWRBAND;
    }
    else {
        set_bit(SOCK_ASYNC_NOSPACE, &sk->sk_socket->flags);
    }

    AWDS_RETURN mask;
}

int awds_socket_sendmsg(struct kiocb *iocb,
		                struct socket *sock, 
		                struct msghdr *m,
		                size_t total_len)
{
    struct sock *sk = sock->sk;
    struct sockaddr_awds *addr = 0;
    struct sk_buff *skb;
    struct net_device *dev;
    int ifindex;
    int err;

    AWDS_FUNC_IN(7);

    if (m->msg_name) {
        addr = (struct sockaddr_awds *)m->msg_name;

        if (addr->sa_family != PF_AWDS) {
            AWDS_RETURN -EINVAL;
        }
        
        ifindex = addr->sa_ifindex;
    }
    else {
        AWDS_RETURN -EINVAL;
    }

    dev = dev_get_by_index(&init_net, ifindex);
    
    if (!dev) {
        AWDS_RETURN -ENXIO;
    }

    skb = sock_alloc_send_skb(sk, total_len + LL_RESERVED_SPACE(dev), m->msg_flags & MSG_DONTWAIT, &err);    

    if (!skb) {
        dev_put(dev);
        AWDS_RETURN err;
    }

    /*
     * make room for ethernet header
     */
    skb_reserve(skb, LL_RESERVED_SPACE(dev));
    skb_reset_network_header(skb);

    err = dev_hard_header(skb, dev, ETH_P_AWDS, addr->sa_addr, NULL, total_len);

    if(err < 0) {
        goto out_free;
    }

    err = memcpy_fromiovec(skb_put(skb, total_len), m->msg_iov, total_len);

    if (err < 0) {
out_free:
        kfree_skb(skb);
        dev_put(dev);
        AWDS_RETURN err;
    }

    skb->protocol = htons(ETH_P_AWDS);
    skb->dev = dev;
    skb->sk = sk;

    err = awds_send(skb);

    dev_put(dev);

    if (err) {
        AWDS_RETURN err;
    }

    if(likely(statistics==1)) awds_stats.tx_total++;
    AWDS_RETURN total_len;
}

int awds_socket_recvmsg(struct kiocb *iocb,
		                struct socket *sock,
		                struct msghdr *m,
		                size_t total_len,
		                int    flags)
{
    struct awds_sock*       sk = 0;
    struct awds_instance*   instance = 0;
    struct awds_interface*  interface = 0;
    struct sk_buff*         skb = 0;
    unsigned int len, copied;
    int err = 0;

    AWDS_FUNC_IN(7);

    sk = awds_sk(sock->sk);
    instance = awds_find_instance_by_sock(sk);

    /* not bound to any address */
    if(sk->ifcount == 0) {
        err = -EINVAL;
        goto out_inval;
    }

    skb = awds_socket_peek_skb(sk, flags, &err);
    
    if(skb && instance) {
        /*
         * copy address
         */
        m->msg_namelen = 0;
        interface = awds_find_interface(instance, skb->dev);
        if (interface) {
            m->msg_namelen = sizeof(*interface->addr);
            memcpy(m->msg_name, interface->addr, sizeof(*interface->addr));
        }

        len = skb->len;
        copied = total_len;
        if (copied > len) {
            copied = len;
        }
        else if (copied < len) {
            m->msg_flags |= MSG_TRUNC;
        }

        err = skb_copy_datagram_iovec(skb, 0, m->msg_iov, copied);

        /*
         * socket data accounting update
         */
        kfree_skb(skb);
        sk_mem_reclaim(sock->sk);

        if(err) {
            goto out_inval;
        }

        err = copied;

        if (flags & MSG_TRUNC) {
            err = len;
        }
    }

out_inval:    
    AWDS_RETURN err;
}

/*
 * Helper functions for manipulating data strctures
 */
int __awds_create_instance(struct awds_sock* sk,struct sockaddr_awds* addr)
{
    int ret = 0;
    struct awds_instance*  instance = 0;

    AWDS_FUNC_IN(5);
   
    if(sk && addr) {
        if(atomic_read(&awds_instance_index) >= MAX_AWDS_INDEX) {
            /*
             * maximum of awds instances reached
             */
            atomic_set(&awds_instance_index, MAX_AWDS_INDEX);
            AWDS_RETURN -ENOBUFS;
        }

        /*
         * get memory and initialize our instance object
         */    
        instance = kmalloc(sizeof(struct awds_instance), GFP_KERNEL);

        if(instance) {
            memset(instance, 0, sizeof(struct awds_instance));

            instance->index = -1;
            instance->sock = sk;
            instance->sock->interf = 0;
            instance->sock->table = 0;

            mutex_init(&instance->lock);

            ret = awds_create_interface(instance, addr);

            if(ret == 0) {
                __awds_add_instance(instance);
            }
            else {
                /*
                 * free the buffer, since no interface could be created
                 */
                kfree(instance);
                instance = 0;
            }

            AWDS_RETURN ret;
        }

        AWDS_RETURN -ENOBUFS;
    }

    AWDS_RETURN -EAWDSINVAL;
}

int __awds_release_instance(struct awds_instance* instance)
{
    struct awds_interface*  interface = 0;
    int ret = 0;

    AWDS_FUNC_IN(5);

    if(!mutex_is_locked(&awds_instance_mutex)) {
        AWDS_RETURN -EAWDSNOMUTEX;
    }
    
    /*
     * first release all device references, we are currently holding
     *
     */
    interface = instance->sock->interf;
    instance->sock->interf = 0;

    while(interface) {
        /*
         * not __awds_release_interface, since we are only under
         * awds_instance_mutex and not unter instance->lock
         */
        ret = awds_release_interface(instance, interface);

        if(ret) {
            /*
             * error occurred, not all interfaces could be deleted
             */
            break;
        }
        else {
            interface = interface->next;
        }
    }

    /*
     * only remove from list, if all interfaces could be removed
     */
    if(ret == 0) {
        /*
         * now remove the instance from the instance array
         */
        ret = __awds_del_instance(instance);

        /*
         * free the pointer
         */
        if(ret == 0) {
            kfree(instance);
            instance = 0;
        }        
    }
    AWDS_INFO("instance released: %d\n", ret);
    AWDS_RETURN ret;
}

int awds_add_instance(struct awds_instance* instance)
{
    int ret = 0;

    AWDS_FUNC_IN(5);
    
    /*
     * first remove the instance from the instance array
     */
    mutex_lock(&awds_instance_mutex);
    
    ret = __awds_add_instance(instance);

    mutex_unlock(&awds_instance_mutex);

    AWDS_RETURN ret;
}

int awds_del_instance(struct awds_instance* instance)
{
    int ret = 0;

    AWDS_FUNC_IN(5);
    
    /*
     * first remove the instance from the instance array
     */
    mutex_lock(&awds_instance_mutex);
    
    ret = __awds_del_instance(instance);

    mutex_unlock(&awds_instance_mutex);

    AWDS_RETURN ret;
}

int __awds_add_instance(struct awds_instance* instance)
{
    int i_count = 0;
    int cur_index = 0;
    int new_index = 0;

    AWDS_FUNC_IN(5);

    if(!mutex_is_locked(&awds_instance_mutex)) {
        AWDS_RETURN -EAWDSNOMUTEX;
    }

    if(instance) {
        cur_index = atomic_read(&awds_instance_index);
    
        for(i_count=0;i_count<=cur_index;i_count++) {
            if(awds_instances[cur_index] == instance) {
                /*
                 * this instance exist already in our list
                 */
                AWDS_RETURN -EAWDSEXIST;
            }
        }

        if(cur_index == MAX_AWDS_INDEX) {
            /*
             * no more space left
             */
            AWDS_RETURN -EAWDSFULL;
        }

        new_index = atomic_inc_return(&awds_instance_index);

        instance->index = new_index;
        awds_instances[new_index] = instance;
        /*
         * everything ok
         */
        AWDS_RETURN 0;
    }

    /*
     * null pointer
     */
    AWDS_RETURN -EAWDSINVAL;
}

int __awds_del_instance(struct awds_instance* instance)
{
    struct awds_sock*   sk = instance->sock;
    int                 i_count = 0;
    int                 cur_index = 0;

    AWDS_FUNC_IN(5);

    if(!mutex_is_locked(&awds_instance_mutex)) {       
        AWDS_RETURN -EAWDSNOMUTEX;
    }

    if(instance) {
        cur_index = atomic_read(&awds_instance_index);
    
        for(i_count=0;i_count<=cur_index;i_count++) {
            if(awds_instances[i_count] == instance) {
                awds_instances[i_count] = 0;
                instance->index = -1;

                if (sk) {
                    instance->sock = NULL;
                    sock_put(&sk->sk);
                }

                atomic_dec(&awds_instance_index);
                AWDS_RETURN 0;
            }
        }
    }

    /*
     * null pointer
     */
    AWDS_RETURN -EAWDSINVAL;
}

int awds_create_interface(struct awds_instance* instance, struct sockaddr_awds* addr)
{
    int ret = 0;
    struct awds_interface* interface = 0;
    struct net_device* dev = 0;

    AWDS_FUNC_IN(5);

    /*
     * NOTE: dev_get_by_index() adds a reference to the device pointer
     * So we must call dev_put() when we are finished with this
     * device
     *
     * This means when the user closes this socket, we have
     * to release this reference!
     */
    #ifdef CONFIG_NET_NS
        dev = dev_get_by_index(instance->sock->sk.sk_net, addr->sa_ifindex);
    #else
        dev = dev_get_by_index(&init_net, addr->sa_ifindex);
    #endif

    if(dev) {
        interface = awds_find_interface(instance, dev);

        /*
         * already bound to this instance
         */
        if(interface != 0) {
            AWDS_INFO("interface %d, already bound to instance %d\n", addr->sa_ifindex, instance->index);
            ret = -EADDRINUSE;
            goto out_put;
        }

        interface = kmalloc(sizeof(struct awds_interface), GFP_KERNEL);

        if(interface) {
            memset(interface, 0, sizeof(struct awds_interface));

            interface->dev = dev;
            interface->next = 0;
            interface->prev = 0;
            interface->index = addr->sa_ifindex;

            interface->addr = kmalloc(sizeof(*addr), GFP_KERNEL);
            if(!interface->addr) {
                kfree(interface);
                interface = 0;
                ret = -ENOBUFS;
                goto out_put;
            }
    
            memcpy(interface->addr, addr, sizeof(*addr));

            ret = awds_add_interface(instance, interface);
        
            if(ret) {
                awds_release_interface(instance, interface);
                /*
                 * no goto out_put here, reference is dropped
                 * in awds_release_interface
                 */
            }
            goto out;
        }

        ret = -ENOBUFS;
        goto out_put;
    }

    ret = -EAWDSINVAL;
    goto out;

out_put:
    dev_put(dev);
out:
    AWDS_RETURN ret;
}

int awds_release_interface(struct awds_instance* instance, struct awds_interface* iface)
{
    AWDS_FUNC_IN(5);

    mutex_lock(&instance->lock);

    if(instance && iface) {
        /*
         * free the interface, the linked list
         * is also updated
         */
        __awds_del_interface(instance, iface);

        AWDS_RETURN 0;
    }

    mutex_unlock(&instance->lock);

    /*
     * null pointer
     */
    AWDS_RETURN -EAWDSINVAL;
}

int awds_add_interface(struct awds_instance* instance, struct awds_interface* iface)
{
    int ret = 0;

    AWDS_FUNC_IN(5);

    mutex_lock(&instance->lock);
    
    ret = __awds_add_interface(instance, iface);

    mutex_unlock(&instance->lock);

    AWDS_RETURN ret;
}

int awds_del_interface(struct awds_instance* instance, struct awds_interface* iface)
{
    int ret = 0;

    AWDS_FUNC_IN(5);

    mutex_lock(&instance->lock);
    
    ret = __awds_del_interface(instance, iface);

    mutex_unlock(&instance->lock);

    AWDS_RETURN ret;
}

int __awds_add_interface(struct awds_instance* instance, struct awds_interface* interface)
{
    int number_if = 0;
    struct awds_interface* interface_iter = 0;

    AWDS_FUNC_IN(5);
    AWDS_INFO("instance: %ld,interface: %ld\n", (long)instance, (long)interface);

    if(!mutex_is_locked(&instance->lock)) {
        AWDS_RETURN -EAWDSNOMUTEX;
    }

    if(instance && interface) {
        interface_iter = instance->sock->interf;

        if(interface_iter) {
            /*
             * this interface exists already in our list
             */
            if(interface_iter == interface) {
                AWDS_RETURN -EAWDSINVAL;
            }

            while(interface_iter->next) {
                /*
                 * this interface exists already in our list
                 */
                if(interface_iter == interface) {
                    AWDS_RETURN -EAWDSINVAL;
                }

                interface_iter = interface_iter->next;
                number_if++;
            }
            interface_iter->next = interface;
            interface->prev = interface_iter;
        }
        else {
            instance->sock->interf = interface;
        }

        interface->index = number_if;
        instance->sock->ifcount++;
        AWDS_RETURN number_if;
    }

    /*
     * null pointer
     */
    AWDS_RETURN -EAWDSINVAL;
}

int __awds_del_interface(struct awds_instance* instance, struct awds_interface* interface)
{

    AWDS_FUNC_IN(5);

    if(!mutex_is_locked(&instance->lock)) {
        AWDS_RETURN -EAWDSNOMUTEX;
    }

    if(interface) {
        struct net_device* dev = interface->dev;

        /*
         * remove element from the linked list of interfaces
         */
        if(interface->prev) {
            interface->prev->next = interface->next;
        }

        if(interface->next) {
            interface->next->prev = interface->prev;
        }

        interface->prev = 0;
        interface->next = 0;
        interface->index = -1;
        interface->dev = 0;
        /*
         * release our reference to the netdevice,
         * grabbed with dev_get_by_index()
         */
        dev_put(dev);

        kfree(interface);
        interface = 0;
        instance->sock->ifcount--;
    }

    AWDS_RETURN 0;
}

int awds_find_instance_index_by_sock(struct awds_sock* sk)
{
    int res = 0;

    AWDS_FUNC_IN(7);

    mutex_lock(&awds_instance_mutex);

    res = __awds_find_instance_index_by_sock(sk);

    mutex_unlock(&awds_instance_mutex);
    
    AWDS_RETURN res;
}

int __awds_find_instance_index_by_sock(struct awds_sock* sk)
{   
    int i = 0;

    AWDS_FUNC_IN(7);

    if(!mutex_is_locked(&awds_instance_mutex)) {
        AWDS_RETURN -EAWDSNOMUTEX;
    }

    for(i=0;i<=atomic_read(&awds_instance_index);i++) {
        if(sk == awds_instances[i]->sock) {
            AWDS_RETURN i;
        }
    }

    AWDS_RETURN -EAWDSMISSING;
}

struct awds_instance* awds_find_instance_by_sock(struct awds_sock* sk)
{    
    struct awds_instance* instance = 0;

    AWDS_FUNC_IN(7);

    mutex_lock(&awds_instance_mutex);

    instance = __awds_find_instance_by_sock(sk);

    mutex_unlock(&awds_instance_mutex);

    AWDS_RETURN instance;
}


struct awds_instance* __awds_find_instance_by_sock(struct awds_sock* sk)
{    
    int index = -EAWDSMISSING;

    AWDS_FUNC_IN(7);

    if(!mutex_is_locked(&awds_instance_mutex)) {
        AWDS_RETURN 0;
    }

    index = __awds_find_instance_index_by_sock(sk);

    if(index == -EAWDSMISSING) {
        AWDS_RETURN 0;
    } 

    AWDS_RETURN awds_instances[index];
}

struct awds_instance* awds_find_instance_by_dev(struct net_device *dev)
{
    struct awds_instance* instance = 0;

    AWDS_FUNC_IN(7);

    mutex_lock(&awds_instance_mutex);

    instance = __awds_find_instance_by_dev(dev);

    mutex_unlock(&awds_instance_mutex);

    AWDS_RETURN 0;
}

struct awds_instance* __awds_find_instance_by_dev(struct net_device *dev)
{
    int i = 0;
    struct awds_interface* interface = 0;

    AWDS_FUNC_IN(7);

    if(!mutex_is_locked(&awds_instance_mutex)) {
        AWDS_RETURN 0;
    }

    for(i=0;i<=atomic_read(&awds_instance_index);i++) {    
        interface = awds_find_interface(awds_instances[i], dev);

        if(interface) {
            AWDS_RETURN awds_instances[i];
        }
    }

    AWDS_RETURN 0;
}

struct awds_interface* awds_find_interface(struct awds_instance* instance, struct net_device *dev)
{
    struct awds_interface* interface = 0;    

    AWDS_FUNC_IN(7);

    mutex_lock(&instance->lock);

    interface = __awds_find_interface(instance, dev);

    mutex_unlock(&instance->lock);    

    AWDS_RETURN interface;
}

struct awds_interface* __awds_find_interface(struct awds_instance* instance, struct net_device *dev)
{
    struct awds_interface* interface = 0;    

    AWDS_FUNC_IN(7);

    if(!mutex_is_locked(&instance->lock)) {
        AWDS_RETURN 0;
    }

    if(instance) {
        interface = instance->sock->interf;

        while(interface) {

            if(interface->dev == dev) {
                AWDS_RETURN interface;   
            }
            interface = interface->next;
        }
    }    

    AWDS_RETURN interface;
}

/*
 *
 * Module stuff
 *
 */
#ifdef DEBUG
module_param(debuglevel, short, 0);
MODULE_PARM_DESC(debuglevel, "\n\t\tDefines the debuglevel, possible values:"
                             "\n\t\t[0=all,...,9=nothing]");
#endif
module_param(configuration, short, 0);
MODULE_PARM_DESC(configuration, "\n\t\tDefines the configuration stage, possible values:"
                                "\n\t\t[3=both,2=forwarding only,"
                                "1=local delivery only,0=nothing]");
module_param(statistics, short, 0);
MODULE_PARM_DESC(statistics, "\n\t\tDefines if statistics are collected:"
                             "\n\t\t[0=no,1=yes]");


int __init awds_init_module(void)
{
    int ret = 0;

    AWDS_FUNC_IN(5);

    #ifdef DEBUG
    debuglevel = (debuglevel < 0)?0:debuglevel;
    debuglevel = (debuglevel > 9)?9:debuglevel;
    printk(KERN_INFO "awds kernel module loaded (debuglevel: %d)\n", debuglevel);
    #elif
    printk(KERN_INFO "awds kernel module loaded\n");
    #endif
    awds_configuration_info();

    awds_routing = 0;

    mutex_init(&awds_instance_mutex);
    
    atomic_set(&awds_instance_index, -1);    

    awds_packet_type.type = htons(ETH_P_AWDS);
    awds_packet_type.func = awds_packet_rcv;

    /* Create the files associated with this kobject */
    ret = sysfs_create_group(&(THIS_MODULE->mkobj.kobj), &awds_attr_group);

    /*
     * register the awds packet type
     */
    dev_add_pack(&awds_packet_type);

    /*
     * register the PF_AWDS socket type
     */
    awds_init_socket_handler();

    AWDS_RETURN ret;
}

void __exit awds_cleanup_module(void)
{
 
    AWDS_FUNC_IN(5);

    /*
     * unregister socket type for
     * AWDS protocol family
     *
     */
    awds_release_socket_handler();

    /*
     * unregister packet type
     */
    dev_remove_pack(&awds_packet_type);

    if(awds_routing != 0) {
        kfree(awds_routing);
        awds_routing = 0;
    }

    AWDS_INFO("kernel module unloaded!\n");

    AWDS_RETURN;
}

module_init(awds_init_module);
module_exit(awds_cleanup_module);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Johannes Pfeiffer");
MODULE_DESCRIPTION("manages AWDS packets inside the kernel");

