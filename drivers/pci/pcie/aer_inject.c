// SPDX-License-Identifier: GPL-2.0
/*
 * PCIe AER software error injection support.
 *
 * Debuging PCIe AER code is quite difficult because it is hard to
 * trigger various real hardware errors. Software based error
 * injection can fake almost all kinds of errors with the help of a
 * user space helper tool aer-inject, which can be gotten from:
 *   http://www.kernel.org/pub/linux/utils/pci/aer-inject/
 *
 * Copyright 2009 Intel Corporation.
 *     Huang Ying <ying.huang@intel.com>
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/ftrace.h>
#include <linux/irq.h>
#include <linux/kallsyms.h>
#include <linux/kernel.h>
#include <linux/linkage.h>
#include <linux/miscdevice.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/stddef.h>
#include <linux/device.h>

#include "portdrv.h"

/* Override the existing corrected and uncorrected error masks */
static bool aer_mask_override;
module_param(aer_mask_override, bool, 0);

struct aer_error_inj {
	u8 bus;
	u8 dev;
	u8 fn;
	u32 uncor_status;
	u32 cor_status;
	u32 header_log0;
	u32 header_log1;
	u32 header_log2;
	u32 header_log3;
	u32 domain;
};

struct aer_error {
	struct list_head list;
	u32 domain;
	unsigned int bus;
	unsigned int devfn;
	int pos_cap_err;

	u32 uncor_status;
	u32 cor_status;
	u32 header_log0;
	u32 header_log1;
	u32 header_log2;
	u32 header_log3;
	u32 root_status;
	u32 source_id;
};

static LIST_HEAD(einjected);

/* Protect einjected */
static DEFINE_SPINLOCK(inject_lock);

static void aer_error_init(struct aer_error *err, u32 domain,
			   unsigned int bus, unsigned int devfn,
			   int pos_cap_err)
{
	INIT_LIST_HEAD(&err->list);
	err->domain = domain;
	err->bus = bus;
	err->devfn = devfn;
	err->pos_cap_err = pos_cap_err;
}

static struct aer_error *__find_aer_error(u32 domain, unsigned int bus,
					  unsigned int devfn)
{
	struct aer_error *err;

	list_for_each_entry(err, &einjected, list) {
		if (domain == err->domain &&
		    bus == err->bus &&
		    devfn == err->devfn)
			return err;
	}
	return NULL;
}

static struct aer_error *__find_aer_error_by_dev(struct pci_dev *dev)
{
	int domain = pci_domain_nr(dev->bus);
	if (domain < 0)
		return NULL;
	return __find_aer_error(domain, dev->bus->number, dev->devfn);
}

static u32 *find_pci_config_dword(struct aer_error *err, int where,
				  int *prw1cs)
{
	int rw1cs = 0;
	u32 *target = NULL;

	if (err->pos_cap_err == -1)
		return NULL;

	switch (where - err->pos_cap_err) {
	case PCI_ERR_UNCOR_STATUS:
		target = &err->uncor_status;
		rw1cs = 1;
		break;
	case PCI_ERR_COR_STATUS:
		target = &err->cor_status;
		rw1cs = 1;
		break;
	case PCI_ERR_HEADER_LOG:
		target = &err->header_log0;
		break;
	case PCI_ERR_HEADER_LOG+4:
		target = &err->header_log1;
		break;
	case PCI_ERR_HEADER_LOG+8:
		target = &err->header_log2;
		break;
	case PCI_ERR_HEADER_LOG+12:
		target = &err->header_log3;
		break;
	case PCI_ERR_ROOT_STATUS:
		target = &err->root_status;
		rw1cs = 1;
		break;
	case PCI_ERR_ROOT_ERR_SRC:
		target = &err->source_id;
		break;
	}
	if (prw1cs)
		*prw1cs = rw1cs;
	return target;
}

static int aer_inj_read_config(struct pci_bus *bus, unsigned int devfn,
			       int where, u32 *val)
{
	u32 *sim;
	struct aer_error *err;
	unsigned long flags;
	int domain;

	spin_lock_irqsave(&inject_lock, flags);
	domain = pci_domain_nr(bus);
	if (domain < 0)
		goto out;
	err = __find_aer_error(domain, bus->number, devfn);
	if (!err)
		goto out;

	sim = find_pci_config_dword(err, where, NULL);
	if (sim) {
		*val = *sim;
		spin_unlock_irqrestore(&inject_lock, flags);
		return 0;
	}
out:
	spin_unlock_irqrestore(&inject_lock, flags);
	return -1;
}

static int aer_inj_write_config(struct pci_bus *bus, unsigned int devfn,
				int where, u32 val)
{
	u32 *sim;
	struct aer_error *err;
	unsigned long flags;
	int rw1cs;
	int domain;

	spin_lock_irqsave(&inject_lock, flags);
	domain = pci_domain_nr(bus);
	if (domain < 0)
		goto out;
	err = __find_aer_error(domain, bus->number, devfn);
	if (!err)
		goto out;

	sim = find_pci_config_dword(err, where, &rw1cs);
	if (sim) {
		if (rw1cs)
			*sim ^= val;
		else
			*sim = val;
		spin_unlock_irqrestore(&inject_lock, flags);
		return 0;
	}
out:
	spin_unlock_irqrestore(&inject_lock, flags);
	return -1;
}

static int aer_inject(struct aer_error_inj *einj)
{
	struct aer_error *err, *rperr;
	struct aer_error *err_alloc = NULL, *rperr_alloc = NULL;
	struct pci_dev *dev, *rpdev;
	struct pcie_device *edev;
	struct device *device;
	unsigned long flags;
	unsigned int devfn = PCI_DEVFN(einj->dev, einj->fn);
	int pos_cap_err, rp_pos_cap_err;
	u32 sever, cor_mask, uncor_mask, cor_mask_orig = 0, uncor_mask_orig = 0;
	int ret = 0;

	dev = pci_get_domain_bus_and_slot(einj->domain, einj->bus, devfn);
	if (!dev)
		return -ENODEV;
	rpdev = pcie_find_root_port(dev);
	if (!rpdev) {
		pci_err(dev, "aer_inject: Root port not found\n");
		ret = -ENODEV;
		goto out_put;
	}

	pos_cap_err = dev->aer_cap;
	if (!pos_cap_err) {
		pci_err(dev, "aer_inject: Device doesn't support AER\n");
		ret = -EPROTONOSUPPORT;
		goto out_put;
	}
	pci_read_config_dword(dev, pos_cap_err + PCI_ERR_UNCOR_SEVER, &sever);
	pci_read_config_dword(dev, pos_cap_err + PCI_ERR_COR_MASK, &cor_mask);
	pci_read_config_dword(dev, pos_cap_err + PCI_ERR_UNCOR_MASK,
			      &uncor_mask);

	rp_pos_cap_err = rpdev->aer_cap;
	if (!rp_pos_cap_err) {
		pci_err(rpdev, "aer_inject: Root port doesn't support AER\n");
		ret = -EPROTONOSUPPORT;
		goto out_put;
	}

	err_alloc =  kzalloc(sizeof(struct aer_error), GFP_KERNEL);
	if (!err_alloc) {
		ret = -ENOMEM;
		goto out_put;
	}
	rperr_alloc =  kzalloc(sizeof(struct aer_error), GFP_KERNEL);
	if (!rperr_alloc) {
		ret = -ENOMEM;
		goto out_put;
	}

	if (aer_mask_override) {
		cor_mask_orig = cor_mask;
		cor_mask &= !(einj->cor_status);
		pci_write_config_dword(dev, pos_cap_err + PCI_ERR_COR_MASK,
				       cor_mask);

		uncor_mask_orig = uncor_mask;
		uncor_mask &= !(einj->uncor_status);
		pci_write_config_dword(dev, pos_cap_err + PCI_ERR_UNCOR_MASK,
				       uncor_mask);
	}

	spin_lock_irqsave(&inject_lock, flags);

	err = __find_aer_error_by_dev(dev);
	if (!err) {
		err = err_alloc;
		err_alloc = NULL;
		aer_error_init(err, einj->domain, einj->bus, devfn,
			       pos_cap_err);
		list_add(&err->list, &einjected);
	}
	err->uncor_status |= einj->uncor_status;
	err->cor_status |= einj->cor_status;
	err->header_log0 = einj->header_log0;
	err->header_log1 = einj->header_log1;
	err->header_log2 = einj->header_log2;
	err->header_log3 = einj->header_log3;

	if (!aer_mask_override && einj->cor_status &&
	    !(einj->cor_status & ~cor_mask)) {
		ret = -EINVAL;
		pci_warn(dev, "aer_inject: The correctable error(s) is masked by device\n");
		spin_unlock_irqrestore(&inject_lock, flags);
		goto out_put;
	}
	if (!aer_mask_override && einj->uncor_status &&
	    !(einj->uncor_status & ~uncor_mask)) {
		ret = -EINVAL;
		pci_warn(dev, "aer_inject: The uncorrectable error(s) is masked by device\n");
		spin_unlock_irqrestore(&inject_lock, flags);
		goto out_put;
	}

	rperr = __find_aer_error_by_dev(rpdev);
	if (!rperr) {
		rperr = rperr_alloc;
		rperr_alloc = NULL;
		aer_error_init(rperr, pci_domain_nr(rpdev->bus),
			       rpdev->bus->number, rpdev->devfn,
			       rp_pos_cap_err);
		list_add(&rperr->list, &einjected);
	}
	if (einj->cor_status) {
		if (rperr->root_status & PCI_ERR_ROOT_COR_RCV)
			rperr->root_status |= PCI_ERR_ROOT_MULTI_COR_RCV;
		else
			rperr->root_status |= PCI_ERR_ROOT_COR_RCV;
		rperr->source_id &= 0xffff0000;
		rperr->source_id |= (einj->bus << 8) | devfn;
	}
	if (einj->uncor_status) {
		if (rperr->root_status & PCI_ERR_ROOT_UNCOR_RCV)
			rperr->root_status |= PCI_ERR_ROOT_MULTI_UNCOR_RCV;
		if (sever & einj->uncor_status) {
			rperr->root_status |= PCI_ERR_ROOT_FATAL_RCV;
			if (!(rperr->root_status & PCI_ERR_ROOT_UNCOR_RCV))
				rperr->root_status |= PCI_ERR_ROOT_FIRST_FATAL;
		} else
			rperr->root_status |= PCI_ERR_ROOT_NONFATAL_RCV;
		rperr->root_status |= PCI_ERR_ROOT_UNCOR_RCV;
		rperr->source_id &= 0x0000ffff;
		rperr->source_id |= ((einj->bus << 8) | devfn) << 16;
	}
	spin_unlock_irqrestore(&inject_lock, flags);

	if (aer_mask_override) {
		pci_write_config_dword(dev, pos_cap_err + PCI_ERR_COR_MASK,
				       cor_mask_orig);
		pci_write_config_dword(dev, pos_cap_err + PCI_ERR_UNCOR_MASK,
				       uncor_mask_orig);
	}

	device = pcie_port_find_device(rpdev, PCIE_PORT_SERVICE_AER);
	if (device) {
		edev = to_pcie_device(device);
		if (!get_service_data(edev)) {
			dev_warn(&edev->device,
				 "aer_inject: AER service is not initialized\n");
			ret = -EPROTONOSUPPORT;
			goto out_put;
		}
		dev_info(&edev->device,
			 "aer_inject: Injecting errors %08x/%08x into device %s\n",
			 einj->cor_status, einj->uncor_status, pci_name(dev));
		local_irq_disable();
		generic_handle_irq(edev->irq);
		local_irq_enable();
	} else {
		pci_err(rpdev, "aer_inject: AER device not found\n");
		ret = -ENODEV;
	}
out_put:
	kfree(err_alloc);
	kfree(rperr_alloc);
	pci_dev_put(dev);
	return ret;
}

static ssize_t aer_inject_write(struct file *filp, const char __user *ubuf,
				size_t usize, loff_t *off)
{
	struct aer_error_inj einj;
	int ret;

	if (!capable(CAP_SYS_ADMIN))
		return -EPERM;
	if (usize < offsetof(struct aer_error_inj, domain) ||
	    usize > sizeof(einj))
		return -EINVAL;

	memset(&einj, 0, sizeof(einj));
	if (copy_from_user(&einj, ubuf, usize))
		return -EFAULT;

	ret = aer_inject(&einj);
	return ret ? ret : usize;
}

static const struct file_operations aer_inject_fops = {
	.write = aer_inject_write,
	.owner = THIS_MODULE,
	.llseek = noop_llseek,
};

static struct miscdevice aer_inject_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "aer_inject",
	.fops = &aer_inject_fops,
};

static asmlinkage int (*read_config_dword)(struct pci_bus *bus,
					   unsigned int devfn,
					   int where, u32 *val);
static asmlinkage int (*write_config_dword)(struct pci_bus *bus,
					    unsigned int devfn,
					    int where, u32 val);
struct aer_hook {
	struct ftrace_ops ops;
	const char *name;
	void *function;
	void *original;
	unsigned long address;
};

static int asmlinkage ainj_read_config_dword(struct pci_bus *bus,
			unsigned int devfn, int where, u32 *val)
{
	if (!aer_inj_read_config(bus, devfn, where, (u32 *)val))
		return 0;
	return read_config_dword(bus, devfn, where, val);
}

static int asmlinkage ainj_write_config_dword(struct pci_bus *bus,
			unsigned int devfn, int where, u32 val)
{
	if (!aer_inj_write_config(bus, devfn, where, val))
		return 0;
	return write_config_dword(bus, devfn, where, val);
}

static int aer_inject_resolve_hook_address(struct aer_hook *hook)
{
	hook->address = kallsyms_lookup_name(hook->name);

	if (!hook->address) {
		pr_warn("unresolved symbol: %s\n", hook->name);
		return -ENOENT;
	}
	*((unsigned long*) hook->original) = hook->address + MCOUNT_INSN_SIZE;
	return 0;
}

static void notrace aer_inject_ftrace_thunk(unsigned long ip, unsigned long parent_ip,
		struct ftrace_ops *ops, struct pt_regs *regs)
{
	struct aer_hook *hook = container_of(ops, struct aer_hook, ops);
	regs->ip = (unsigned long) hook->function;
}

static int aer_inject_install_hook(struct aer_hook *hook)
{
	int err;

	err = aer_inject_resolve_hook_address(hook);
	if (err)
		return err;

	hook->ops.func = aer_inject_ftrace_thunk;
	hook->ops.flags = FTRACE_OPS_FL_SAVE_REGS |
			  FTRACE_OPS_FL_RECURSION_SAFE |
			  FTRACE_OPS_FL_IPMODIFY;

	err = ftrace_set_filter_ip(&hook->ops, hook->address, 0, 0);
	if (err) {
		pr_warn("ftrace_set_filter_ip() failed: %d\n", err);
		return err;
	}

	err = register_ftrace_function(&hook->ops);
	if (err) {
		pr_warn("register_ftrace_function() failed: %d\n", err);
		ftrace_set_filter_ip(&hook->ops, hook->address, 1, 0);
		return err;
	}

	return 0;
}

static void aer_inject_remove_hook(struct aer_hook *hook)
{
	int err;

	err = unregister_ftrace_function(&hook->ops);
	if (err)
		pr_warn("unregister_ftrace_function() failed: %d\n", err);

	err = ftrace_set_filter_ip(&hook->ops, hook->address, 1, 0);
	if (err)
		pr_warn("ftrace_set_filter_ip() failed: %d\n", err);
}

static int aer_inject_install_hooks(struct aer_hook *hooks, size_t count)
{
	int err, i;

	for (i = 0; i < count; i++) {
		err = aer_inject_install_hook(&hooks[i]);
		if (err)
			goto error;
	}
	return 0;
error:
	while (i != 0)
		aer_inject_remove_hook(&hooks[--i]);
	return err;
}

static void aer_inject_remove_hooks(struct aer_hook *hooks, size_t count)
{
	int i;

	for (i = 0; i < count; i++)
		aer_inject_remove_hook(&hooks[i]);
}

static struct aer_hook aer_hooks[] = {
	{
		.name = "pci_bus_read_config_dword",
		.function = ainj_read_config_dword,
		.original = &read_config_dword,
	},
	{
		.name = "pci_bus_write_config_dword",
		.function = ainj_write_config_dword,
		.original = &write_config_dword,
	},
};

static int __init aer_inject_init(void)
{
	int err;

	err = misc_register(&aer_inject_device);
	if (err)
		return err;

	err = aer_inject_install_hooks(aer_hooks, ARRAY_SIZE(aer_hooks));
	if (err)
		goto out;
	return 0;
 out:
	misc_deregister(&aer_inject_device);
	return err;
}

static void __exit aer_inject_exit(void)
{
	struct aer_error *err, *err_next;
	unsigned long flags;

	aer_inject_remove_hooks(aer_hooks, ARRAY_SIZE(aer_hooks));
	misc_deregister(&aer_inject_device);

	spin_lock_irqsave(&inject_lock, flags);
	list_for_each_entry_safe(err, err_next, &einjected, list) {
		list_del(&err->list);
		kfree(err);
	}
	spin_unlock_irqrestore(&inject_lock, flags);
}

module_init(aer_inject_init);
module_exit(aer_inject_exit);

MODULE_DESCRIPTION("PCIe AER software error injector");
MODULE_LICENSE("GPL");
