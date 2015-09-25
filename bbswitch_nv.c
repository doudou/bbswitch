#include <linux/pci.h>
#include <linux/acpi.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <linux/suspend.h>
#include <linux/seq_file.h>
#include <linux/pm_runtime.h>

static const char* nv_name;

static int
bbswitch_nv_probe(struct pci_dev *pdev, const struct pci_device_id *pent)
{
    pr_info("bbswitch_nv: got NVidia device %s\n", dev_name(&pdev->dev));
    nv_name = dev_name(&pdev->dev);
    pci_enable_device(pdev);
    pm_runtime_use_autosuspend(&pdev->dev);
    pm_runtime_set_autosuspend_delay(&pdev->dev, 5000);
    pm_runtime_set_active(&pdev->dev);
    pm_runtime_allow(&pdev->dev);
    pm_runtime_mark_last_busy(&pdev->dev);
    pm_runtime_put(&pdev->dev);
    return 0;
}

static int
bbswitch_nv_suspend(struct device* dev)
{
    pr_info("%s: suspending", dev->driver->name);
    return 0;
}

static int
bbswitch_nv_resume(struct device* dev)
{
    pr_info("%s: resuming", dev->driver->name);
    return 0;
}

static void bbswitch_nv_remove(struct pci_dev *pdev)
{
    pm_runtime_forbid(&pdev->dev);
    pm_runtime_get_noresume(&pdev->dev);
    pci_disable_device(pdev);
}

static const struct dev_pm_ops bbswitch_nv_pm_ops = {
    .suspend   = bbswitch_nv_suspend,
    .resume    = bbswitch_nv_resume,
    .runtime_suspend = bbswitch_nv_suspend,
    .runtime_resume  = bbswitch_nv_resume
};

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

static struct pci_driver nv_driver = {
    .name = "bbswitch_nv",
    .id_table  = bbswitch_nv_pci_table,
    .probe     = bbswitch_nv_probe,
    .remove    = bbswitch_nv_remove,
    .driver.pm = &bbswitch_nv_pm_ops
};

static bool is_registered = false;
extern bool bbswitch_nv_is_registered(void)
{
    return is_registered;
}

extern void bbswitch_nv_register(void)
{
    is_registered = true;
    pci_register_driver(&nv_driver);
}

extern void bbswitch_nv_unregister(void)
{
    is_registered = false;
    pci_unregister_driver(&nv_driver);
}

extern char const* bbswitch_nv_name(void)
{
    return nv_name;
}

