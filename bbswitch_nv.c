#include <linux/pci.h>
#include <linux/acpi.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <linux/suspend.h>
#include <linux/seq_file.h>
#include <linux/pm_runtime.h>

//#include "bbswitch_dsm.c"

static int
bbswitch_nv_suspend(struct device* dev)
{
    int ret;
    struct pci_dev* pdev = to_pci_dev(dev);

    pr_info("suspending, call DSM function");
    //bbswitch_optimus_dsm();

    pci_save_state(pdev);
    pci_clear_master(pdev);
    pci_disable_device(pdev);
    pci_ignore_hotplug(pdev);
    pr_info("attempting to set discrete card to D3");
    if ((ret = pci_set_power_state(pdev, PCI_D3hot)))
        pr_info("failed transition to D3: %i\n", ret);
    return 0;
}

static int
bbswitch_nv_resume(struct device* dev)
{
    struct pci_dev* pdev = to_pci_dev(dev);
    pr_info("attempting to set discrete card to D0");
    pci_set_power_state(pdev, PCI_D0);
    pci_restore_state(pdev);
    pci_enable_device(pdev);
    pci_set_master(pdev);
    return 0;
}

static struct pci_device_id
bbswitch_nv_pci_table[] = {
	{
		PCI_DEVICE(PCI_VENDOR_ID_NVIDIA, PCI_ANY_ID),
		.class = PCI_BASE_CLASS_DISPLAY << 16,
		.class_mask  = 0xff << 16,
	},
	{
		PCI_DEVICE(PCI_VENDOR_ID_NVIDIA_SGS, PCI_ANY_ID),
		.class = PCI_BASE_CLASS_DISPLAY << 16,
		.class_mask  = 0xff << 16,
	},
	{}
};

static void bbswitch_nv_remove(struct pci_dev *pdev)
{
    pm_runtime_forbid(&pdev->dev);
    pm_runtime_get_noresume(&pdev->dev);
    pci_disable_device(pdev);
    pr_info("REMOVED");
}

static int bbswitch_nv_probe(struct pci_dev *pdev,
			     const struct pci_device_id *pent)
{
    int ret;
    pr_info("bbswitch_nv: got device %s\n", dev_name(&pdev->dev));
    if ((ret = pci_enable_device(pdev)))
        return ret;

    //bbswitch_init_dsm(&pdev->dev);

    pm_runtime_put_noidle(&pdev->dev);
    pm_runtime_allow(&pdev->dev);
    pm_runtime_use_autosuspend(&pdev->dev);
    if ((ret = pm_request_idle(&pdev->dev)))
        pr_info("bbswitch_nv: failed idle %i\n", ret);
    return 0;
}

static const struct dev_pm_ops bbswitch_nv_pm_ops = {
    .suspend   = bbswitch_nv_suspend,
    .resume    = bbswitch_nv_resume,
    .runtime_suspend = bbswitch_nv_suspend,
    .runtime_resume  = bbswitch_nv_resume
};

static struct pci_driver nv_driver = {
    .name = "bbsitwch_nv",
    .id_table  = bbswitch_nv_pci_table,
    .probe     = bbswitch_nv_probe,
    .remove    = bbswitch_nv_remove,
    .driver.pm = &bbswitch_nv_pm_ops
};

static int __init bbswitch_nv_init(void)
{
    return pci_register_driver(&nv_driver);
}

static void __exit
bbswitch_nv_exit(void)
{
    pci_unregister_driver(&nv_driver);
}

module_init(bbswitch_nv_init);
module_exit(bbswitch_nv_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Minimal power management for an NV card");
MODULE_DEVICE_TABLE(pci, bbswitch_nv_pci_table);
MODULE_AUTHOR("Sylvain Joyeux <sylvain.joyeux@m4x.org>");
MODULE_VERSION("0.1");

