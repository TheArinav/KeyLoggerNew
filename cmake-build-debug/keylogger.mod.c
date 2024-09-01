#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x367fcc51, "module_layout" },
	{ 0xd3588bc1, "netlink_kernel_release" },
	{ 0xd0da656b, "__stack_chk_fail" },
	{ 0x9ed554b3, "unregister_keyboard_notifier" },
	{ 0xce98707e, "__netlink_kernel_create" },
	{ 0x1b4d5951, "init_net" },
	{ 0x96554810, "register_keyboard_notifier" },
	{ 0x8be5aa02, "netlink_unicast" },
	{ 0xcfb519cf, "__nlmsg_put" },
	{ 0x61259993, "__alloc_skb" },
	{ 0x5b8239ca, "__x86_return_thunk" },
	{ 0x92997ed8, "_printk" },
	{ 0xbdfb6dbb, "__fentry__" },
};

MODULE_INFO(depends, "");


MODULE_INFO(srcversion, "9DC6B7B464F748478BD3BDB");
