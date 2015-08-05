#include <linux/pci.h>
#include <linux/acpi.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <linux/suspend.h>
#include <linux/seq_file.h>
#include <linux/pm_runtime.h>

//#include "bbswitch_dsm.c"

static int pcie_root_slot = 0;
MODULE_PARM_DESC(pcie_root_slot, "Slot number of the PCIe root port on which the card is attached");
module_param(pcie_root_slot, int, 0400);
static int pcie_root_fun = 0;
MODULE_PARM_DESC(pcie_root_fun, "Function number of the PCIe root port on which the card is attached");
module_param(pcie_root_fun, int, 0400);

static int
bbswitch_suspend(struct device* dev)
{
    struct pci_dev* pdev = to_pci_dev(dev);
    pr_info("%s: attempting to suspend", dev->driver->name);
    pci_save_state(pdev);
    pci_disable_device(pdev);
    pci_set_power_state(pdev, PCI_D3hot);
    return 0;
}

static int
bbswitch_nv_suspend(struct device* dev)
{
    return bbswitch_suspend(dev);
}

static int
bbswitch_pcie_suspend(struct device* dev)
{
    return bbswitch_suspend(dev);
}

static int
bbswitch_resume(struct device* dev)
{
    struct pci_dev* pdev = to_pci_dev(dev);
    pr_info("%s: attempting to resume", dev->driver->name);
    pci_set_power_state(pdev, PCI_D0);
    pci_restore_state(pdev);
    pci_enable_device(pdev);
    return 0;
}

static int
bbswitch_nv_resume(struct device* dev)
{
    return bbswitch_resume(dev);
}

static int
bbswitch_pcie_resume(struct device* dev)
{
    return bbswitch_resume(dev);
}

static void bbswitch_remove(struct pci_dev *pdev)
{
    pm_runtime_forbid(&pdev->dev);
    pm_runtime_get_noresume(&pdev->dev);
    pci_disable_device(pdev);
}

static void bbswitch_nv_remove(struct pci_dev *pdev)
{
    return bbswitch_remove(pdev);
}

static void bbswitch_pcie_remove(struct pci_dev *pdev)
{
    return bbswitch_remove(pdev);
}

static int bbswitch_nv_probe(struct pci_dev *pdev, const struct pci_device_id *pent)
{
    pr_info("bbswitch_nv: got NVidia device %s\n", dev_name(&pdev->dev));
    pci_enable_device(pdev);
    pm_runtime_put_noidle(&pdev->dev);
    pm_runtime_allow(&pdev->dev);
    pm_runtime_use_autosuspend(&pdev->dev);
    pm_request_idle(&pdev->dev);
    return 0;
}

static int bbswitch_pcie_probe(struct pci_dev *pdev, const struct pci_device_id *pent)
{
    pr_info("bbswitch_pcie: got PCIe root port %s\n", dev_name(&pdev->dev));
    if (PCI_SLOT(pdev->devfn) != pcie_root_slot || PCI_FUNC(pdev->devfn) != pcie_root_fun)
    {
        pr_info("bbswitch_pcie: %i.%i not matching expected dev.fn %i.%i\n",
                PCI_SLOT(pdev->devfn), PCI_FUNC(pdev->devfn),
                pcie_root_slot, pcie_root_fun);
        return -1;
    }

    pci_enable_device(pdev);
    pm_runtime_put_noidle(&pdev->dev);
    pm_runtime_allow(&pdev->dev);
    pm_runtime_use_autosuspend(&pdev->dev);
    pm_request_idle(&pdev->dev);
    return 0;
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

static const struct dev_pm_ops bbswitch_pcie_pm_ops = {
    .suspend   = bbswitch_pcie_suspend,
    .resume    = bbswitch_pcie_resume,
    .runtime_suspend = bbswitch_pcie_suspend,
    .runtime_resume  = bbswitch_pcie_resume
};

static struct pci_device_id
bbswitch_pcie_pci_table[] = {
	{
            PCI_DEVICE_CLASS(((PCI_CLASS_BRIDGE_PCI << 8) | 0x00), ~0)
	},
	{}
};

static struct pci_driver pcie_driver = {
    .name = "bbswitch_pcie",
    .id_table  = bbswitch_pcie_pci_table,
    .probe     = bbswitch_pcie_probe,
    .remove    = bbswitch_pcie_remove,
    .driver.pm = &bbswitch_pcie_pm_ops
};

static int __init bbswitch_nv_init(void)
{
    pci_register_driver(&pcie_driver);
    pci_register_driver(&nv_driver);
    return 0;
}

static void __exit
bbswitch_nv_exit(void)
{
    pci_unregister_driver(&nv_driver);
    pci_unregister_driver(&pcie_driver);
}

module_init(bbswitch_nv_init);
module_exit(bbswitch_nv_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Minimal power management for an NV card");
MODULE_AUTHOR("Sylvain Joyeux <sylvain.joyeux@m4x.org>");
MODULE_VERSION("0.1");

