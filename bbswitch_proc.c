#include <linux/module.h>
#include <asm/uaccess.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>

#define BBSWITCH_VERSION "0.9"

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Toggle the discrete graphics card");
MODULE_AUTHOR("Peter Wu <lekensteyn@gmail.com>");
MODULE_VERSION(BBSWITCH_VERSION);

enum {
    CARD_UNCHANGED = -1,
    CARD_OFF = 0,
    CARD_ON = 1,
};

extern void bbswitch_nv_register(void);
extern void bbswitch_nv_unregister(void);
extern bool bbswitch_nv_is_registered(void);
extern const char* bbswitch_nv_name(void);

static struct proc_dir_entry* proc_entry;

static int load_state = CARD_UNCHANGED;
MODULE_PARM_DESC(load_state, "Card state on unload (0 = off, 1 = on, -1 = unchanged)");
module_param(load_state, int, 0600);

extern struct proc_dir_entry *acpi_root_dir;

static ssize_t bbswitch_proc_write(struct file *fp, const char __user *buff,
        size_t len, loff_t *off) {
    char cmd[8];

    if (len >= sizeof(cmd))
        len = sizeof(cmd) - 1;

    if (copy_from_user(cmd, buff, len))
        return -EFAULT;

    if (strncmp(cmd, "OFF", 3) == 0)
        bbswitch_nv_register();
    else if (strncmp(cmd, "ON", 2) == 0)
        bbswitch_nv_unregister();

    return len;
}

static int bbswitch_proc_show(struct seq_file *seqfp, void *p) {
    seq_printf(seqfp, "%s %s\n", bbswitch_nv_name(),
            bbswitch_nv_is_registered() ? "OFF" : "ON");
    return 0;
}
static int bbswitch_proc_open(struct inode *inode, struct file *file) {
    return single_open(file, bbswitch_proc_show, NULL);
}

static struct file_operations bbswitch_fops = {
    .open   = bbswitch_proc_open,
    .read   = seq_read,
    .write  = bbswitch_proc_write,
    .llseek = seq_lseek,
    .release= single_release
};

static int __init bbswitch_init(void) {
    pr_info("version %s\n", BBSWITCH_VERSION);

    proc_entry = proc_create("bbswitch", 0664, acpi_root_dir, &bbswitch_fops);
    if (proc_entry == NULL) {
        pr_err("Couldn't create proc entry\n");
        return -ENOMEM;
    }

    if (load_state == CARD_OFF)
        bbswitch_nv_register();

    pr_info("Succesfully loaded");
    return 0;
}

static void __exit bbswitch_exit(void) {
    remove_proc_entry("bbswitch", acpi_root_dir);
    if (bbswitch_nv_is_registered())
        bbswitch_nv_unregister();
}

module_init(bbswitch_init);
module_exit(bbswitch_exit);

/* vim: set sw=4 ts=4 et: */
