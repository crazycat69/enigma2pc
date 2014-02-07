/*
 * ca_netlink.c: Generic netlink communication
 *
 * See the main source file 'dvb_softwareca.c' for copyright information.
 *
 */

#include <linux/kernel.h>
#include "ca_netlink.h"
#include <linux/version.h>

// attribute policy 
struct nla_policy ca_policy[ATTR_MAX + 1] = {
	[ATTR_CA_SIZE] = { .type = NLA_U32 },
};

struct genl_family ca_family = {
	.id = GENL_ID_GENERATE,
	.hdrsize = 0,
	.name = "CA_SEND",
	.version = 1,
	.maxattr = ATTR_MAX,
};

// commands: mapping between the command enumeration and the actual function
struct genl_ops ask_ca_size_ops = {
	.cmd = CMD_ASK_CA_SIZE,
	.flags = 0,
	.policy = ca_policy,
	.doit = reply_ca,
	.dumpit = NULL,
};

int processPid = 0;

int reply_ca(struct sk_buff *skb_2, struct genl_info *info)
{
        struct sk_buff *skb;
	void *msg_head;
	int ret;
	
#if LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0)
	printk("reply_ca %d\n", info->snd_pid);
#else
	printk("reply_ca %d\n", info->snd_portid);
#endif

        if (!info)
                goto out;
  
        skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
	if (!skb)
		goto out;

       	msg_head = genlmsg_put(skb, 0, info->snd_seq, &ca_family, 0, CMD_ASK_CA_SIZE);
	if (!msg_head) {
		goto out;
	}

	ret = nla_put_u32(skb, ATTR_CA_SIZE, devices_counter);
	if (ret)
		goto out;
	
	genlmsg_end(skb, msg_head);

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0)
	ret = genlmsg_unicast(&init_net, skb, info->snd_pid );
#else
	ret = genlmsg_unicast(&init_net, skb, info->snd_portid );
#endif
	if (ret)
		goto out;

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,7,0)
	processPid = info->snd_pid;
#else
	processPid = info->snd_portid;
#endif
	return 0;

 out:
        printk("dvbsoftwareca: reply_ca error\n");
	return 0;
}

int netlink_send_cw(unsigned char ca_num, ca_descr_t *ca_descr) {
        struct sk_buff *skb;
	void *msg_head;
	int ret;

        skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
	if (!skb)
		goto out;

       	msg_head = genlmsg_put(skb, 0, 0, &ca_family, 0, CMD_SET_CW);
	if (!msg_head) {
		goto out;
	}

	ret = nla_put_u16(skb, ATTR_CA_NUM, ca_num);
	if (ret)
		goto out;
	ret = nla_put(skb, ATTR_CA_DESCR, sizeof(ca_descr_t), ca_descr);
	if (ret)
		goto out;

	genlmsg_end(skb, msg_head);

	ret = genlmsg_unicast(&init_net, skb, processPid );
	if (ret)
		goto out;

	return 0;

 out:
        printk("dvbsoftwareca: send_cw error\n");
	return 0;
}

int netlink_send_pid(unsigned char ca_num, ca_pid_t *ca_pid) {
        struct sk_buff *skb;
	void *msg_head;
	int ret;

        skb = genlmsg_new(NLMSG_GOODSIZE, GFP_KERNEL);
	if (!skb)
		goto out;

       	msg_head = genlmsg_put(skb, 0, 0, &ca_family, 0, CMD_SET_CW);
	if (!msg_head) {
		goto out;
	}

	ret = nla_put_u16(skb, ATTR_CA_NUM, ca_num);
	if (ret)
		goto out;
	ret = nla_put(skb, ATTR_CA_PID, sizeof(ca_pid_t), ca_pid);
	if (ret)
		goto out;

	genlmsg_end(skb, msg_head);

	ret = genlmsg_unicast(&init_net, skb, processPid );
	if (ret)
		goto out;

	return 0;

 out:
        printk("dvbsoftwareca: send_pid error\n");
	return 0;
}

int register_netlink(void) {
	int ret;

	// register new family
	ret = genl_register_family(&ca_family);
	if (ret) {
		printk("dvbsoftwareca: genl_register_family error\n");
		return ret;
	}
	// register functions (commands) of the new family
	ret = genl_register_ops(&ca_family, &ask_ca_size_ops);
	if (ret) {
		genl_unregister_family(&ca_family);
		printk("dvbsoftwareca: genl_register_family error\n");
	}

	return ret;
}

void unregister_netlink(void) {
        // unregister the functions
	genl_unregister_ops(&ca_family, &ask_ca_size_ops);
        // unregister the family
	genl_unregister_family(&ca_family);
}

