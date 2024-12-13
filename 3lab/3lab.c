#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>

static int __init core_init(void) {
    pr_info("Welcome to the Tomsk State University\n");
    return 0;
}

static void __exit core_exit(void) {
    pr_info("Tomsk State University forever!\n");
}

module_init(core_init);
module_exit(core_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Фёдор");
MODULE_DESCRIPTION("Какой-то модуль для TSU");