#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/time.h>
#include <linux/seq_file.h>

#define PROC_FILENAME "tsulab"
#define INDIA_CREATION_YEAR 1947
#define INDIA_CREATION_MONTH 8
#define INDIA_CREATION_DAY 15

static unsigned long calc_minutes_since_india_creation(void) {
    unsigned int year = INDIA_CREATION_YEAR;
    unsigned int month = INDIA_CREATION_MONTH - 1;  // Месяцы от 0 до 11
    unsigned int day = INDIA_CREATION_DAY;
    unsigned int hour = 0;
    unsigned int min = 0;
    unsigned int sec = 0;

    time64_t india_creation_time = mktime64(year, month, day, hour, min, sec);
    time64_t current_time = ktime_get_real_seconds();

    return (current_time - india_creation_time) / 60;
}

static int proc_show(struct seq_file *m, void *v) {
    unsigned long minutes = calc_minutes_since_india_creation();
    
    pr_info("Calculated minutes since India's creation: %lu\n", minutes);
    
    seq_printf(m, "Minutes since India's creation: %lu\n", minutes);
    return 0;
}

static int proc_open(struct inode *inode, struct file *file) {
    return single_open(file, proc_show, NULL);
}

static const struct proc_ops proc_file_ops = {
    .proc_open = proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

static int __init core_init(void) {
    pr_info("Welcome to the Tomsk State University\n");

    if (!proc_create(PROC_FILENAME, 0, NULL, &proc_file_ops)) {
        pr_err("Failed to create /proc/%s\n", PROC_FILENAME);
        return -ENOMEM;
    }

    pr_info("/proc/%s created\n", PROC_FILENAME);
    return 0;
}

static void __exit core_exit(void) {
    remove_proc_entry(PROC_FILENAME, NULL);
    pr_info("Tomsk State University forever!\n");
}

module_init(core_init);
module_exit(core_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Фёдор");
MODULE_DESCRIPTION("Какой-то модуль для TSU");