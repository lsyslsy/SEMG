#include <linux/module.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

MODULE_INFO(vermagic, VERMAGIC_STRING);

struct module __this_module
__attribute__((section(".gnu.linkonce.this_module"))) = {
 .name = KBUILD_MODNAME,
 .init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
 .exit = cleanup_module,
#endif
 .arch = MODULE_ARCH_INIT,
};

static const struct modversion_info ____versions[]
__used
__attribute__((section("__versions"))) = {
	{ 0x680d4942, "module_layout" },
	{ 0x49a43dff, "clk_get_rate" },
	{ 0x572934a, "clk_get" },
	{ 0x6d4d6df5, "s3c_gpio_cfgpin" },
	{ 0x38dd2bca, "kill_fasync" },
	{ 0xfaef0ed, "__tasklet_schedule" },
	{ 0xfda85a7d, "request_threaded_irq" },
	{ 0x47feb10d, "misc_register" },
	{ 0x40a6f522, "__arm_ioremap" },
	{ 0xadf42bd5, "__request_region" },
	{ 0xea147363, "printk" },
	{ 0xcdc55935, "fasync_helper" },
	{ 0x788fe103, "iomem_resource" },
	{ 0x72440f08, "misc_deregister" },
	{ 0x9bce482f, "__release_region" },
	{ 0x45a55ec8, "__iounmap" },
	{ 0xf20dabd8, "free_irq" },
	{ 0x2e5810c6, "__aeabi_unwind_cpp_pr1" },
};

static const char __module_depends[]
__used
__attribute__((section(".modinfo"))) =
"depends=";


MODULE_INFO(srcversion, "E53D07704F94D26E7573C99");
