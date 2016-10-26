/*
 * ppc64 code to implement the kexec_file_load syscall
 *
 * Copyright (C) 2004  Adam Litke (agl@us.ibm.com)
 * Copyright (C) 2004  IBM Corp.
 * Copyright (C) 2004,2005  Milton D Miller II, IBM Corporation
 * Copyright (C) 2005  R Sharada (sharada@in.ibm.com)
 * Copyright (C) 2006  Mohan Kumar M (mohan@in.ibm.com)
 * Copyright (C) 2016  IBM Corporation
 *
 * Based on kexec-tools' kexec-elf-ppc64.c, fs2dt.c.
 * Heavily modified for the kernel by
 * Thiago Jung Bauermann <bauerman@linux.vnet.ibm.com>.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation (version 2 of the License).
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/slab.h>
#include <linux/kexec.h>
#include <linux/memblock.h>
#include <linux/of_fdt.h>
#include <linux/libfdt.h>
#include <asm/elf_util.h>
#include <asm/ima.h>

#define SLAVE_CODE_SIZE		256

static struct kexec_file_ops *kexec_file_loaders[] = {
	&kexec_elf64_ops,
};

int arch_kexec_kernel_image_probe(struct kimage *image, void *buf,
				  unsigned long buf_len)
{
	int i, ret = -ENOEXEC;
	struct kexec_file_ops *fops;

	/* We don't support crash kernels yet. */
	if (image->type == KEXEC_TYPE_CRASH)
		return -ENOTSUPP;

	for (i = 0; i < ARRAY_SIZE(kexec_file_loaders); i++) {
		fops = kexec_file_loaders[i];
		if (!fops || !fops->probe)
			continue;

		ret = fops->probe(buf, buf_len);
		if (!ret) {
			image->fops = fops;
			return ret;
		}
	}

	return ret;
}

void *arch_kexec_kernel_image_load(struct kimage *image)
{
	if (!image->fops || !image->fops->load)
		return ERR_PTR(-ENOEXEC);

	return image->fops->load(image, image->kernel_buf,
				 image->kernel_buf_len, image->initrd_buf,
				 image->initrd_buf_len, image->cmdline_buf,
				 image->cmdline_buf_len);
}

int arch_kimage_file_post_load_cleanup(struct kimage *image)
{
	if (!image->fops || !image->fops->cleanup)
		return 0;

	return image->fops->cleanup(image->image_loader_data);
}

/**
 * arch_kexec_walk_mem - call func(data) for each unreserved memory block
 * @kbuf:	Context info for the search. Also passed to @func.
 * @func:	Function to call for each memory block.
 *
 * This function is used by kexec_add_buffer and kexec_locate_mem_hole
 * to find unreserved memory to load kexec segments into.
 *
 * Return: The memory walk will stop when func returns a non-zero value
 * and that value will be returned. If all free regions are visited without
 * func returning non-zero, then zero will be returned.
 */
int arch_kexec_walk_mem(struct kexec_buf *kbuf, int (*func)(u64, u64, void *))
{
	int ret = 0;
	u64 i;
	phys_addr_t mstart, mend;

	if (kbuf->top_down) {
		for_each_free_mem_range_reverse(i, NUMA_NO_NODE, 0,
						&mstart, &mend, NULL) {
			/*
			 * In memblock, end points to the first byte after the
			 * range while in kexec, end points to the last byte
			 * in the range.
			 */
			ret = func(mstart, mend - 1, kbuf);
			if (ret)
				break;
		}
	} else {
		for_each_free_mem_range(i, NUMA_NO_NODE, 0, &mstart, &mend,
					NULL) {
			/*
			 * In memblock, end points to the first byte after the
			 * range while in kexec, end points to the last byte
			 * in the range.
			 */
			ret = func(mstart, mend - 1, kbuf);
			if (ret)
				break;
		}
	}

	return ret;
}

/**
 * arch_kexec_apply_relocations_add - apply purgatory relocations
 * @ehdr:	Pointer to ELF headers.
 * @sechdrs:	Pointer to section headers.
 * @relsec:	Section index of SHT_RELA section.
 *
 * Elf64_Shdr.sh_offset has been modified to keep the pointer to the section
 * contents, while Elf64_Shdr.sh_addr points to the final address of the
 * section in memory.
 */
int arch_kexec_apply_relocations_add(const Elf64_Ehdr *ehdr,
				     Elf64_Shdr *sechdrs, unsigned int relsec)
{
	unsigned int i;
	int ret;
	int reloc_type;
	unsigned long *location;
	unsigned long address;
	unsigned long value;
	const char *name;
	Elf64_Sym *sym;
	/* Section containing the relocation entries. */
	Elf64_Shdr *rel_section = &sechdrs[relsec];
	const Elf64_Rela *rela = (const Elf64_Rela *) rel_section->sh_offset;
	/* Section to which relocations apply. */
	Elf64_Shdr *target_section = &sechdrs[rel_section->sh_info];
	/* Associated symbol table. */
	Elf64_Shdr *symtabsec = &sechdrs[rel_section->sh_link];
	void *syms_base = (void *) symtabsec->sh_offset;
	void *loc_base = (void *) target_section->sh_offset;
	Elf64_Addr addr_base = target_section->sh_addr;
	unsigned long sec_base;
	unsigned long r2;
	unsigned int toc;
	const char *strtab;

	if (symtabsec->sh_link >= ehdr->e_shnum) {
		/* Invalid strtab section number */
		pr_err("Invalid string table section index %d\n",
		       symtabsec->sh_link);
		return -ENOEXEC;
	}

	toc = elf_toc_section(ehdr, sechdrs);
	if (!toc) {
		pr_err("Purgatory TOC section not found.");
		return -ENOEXEC;
	}

	r2 = elf_my_r2(sechdrs, toc);

	/* String table for the associated symbol table. */
	strtab = (const char *) sechdrs[symtabsec->sh_link].sh_offset;

	for (i = 0; i < rel_section->sh_size / sizeof(Elf64_Rela); i++) {
		/*
		 * rels[i].r_offset contains the byte offset from the beginning
		 * of section to the storage unit affected.
		 *
		 * This is the location to update in the temporary buffer where
		 * the section is currently loaded. The section will finally
		 * be loaded to a different address later, pointed to by
		 * addr_base.
		 */
		location = loc_base + rela[i].r_offset;

		/* Final address of the location. */
		address = addr_base + rela[i].r_offset;

		/* This is the symbol the relocation is referring to. */
		sym = (Elf64_Sym *) syms_base + ELF64_R_SYM(rela[i].r_info);

		if (sym->st_name)
			name = strtab + sym->st_name;
		else
			name = "<unnamed symbol>";

		reloc_type = ELF64_R_TYPE(rela[i].r_info);

		pr_debug("RELOC at %p: %i-type as %s (0x%lx) + %li\n",
		       location, reloc_type, name, (unsigned long)sym->st_value,
		       (long)rela[i].r_addend);

		/*
		 * TOC symbols appear as undefined but should be
		 * resolved as well, so allow them to be processed.
		 */
		if (sym->st_shndx == SHN_UNDEF && strcmp(name, ".TOC.") != 0 &&
				reloc_type != R_PPC64_TOC) {
			pr_err("Undefined symbol: %s\n", name);
			return -ENOEXEC;
		} else if (sym->st_shndx == SHN_COMMON) {
			pr_err("Symbol '%s' in common section.\n",
			       name);
			return -ENOEXEC;
		}

		if (sym->st_shndx != SHN_ABS) {
			if (sym->st_shndx >= ehdr->e_shnum) {
				pr_err("Invalid section %d for symbol %s\n",
				       sym->st_shndx, name);
				return -ENOEXEC;
			}

			sec_base = sechdrs[sym->st_shndx].sh_addr;
		} else
			sec_base = 0;

		/* `Everything is relative'. */
		value = sym->st_value + sec_base + rela[i].r_addend;

		ret = elf64_apply_relocate_add_item(sechdrs, strtab, &rela[i],
						    sym, location, address,
						    value, r2,
						    "kexec purgatory", NULL);
		if (ret)
			return ret;
	}

	return 0;
}

/**
 * setup_purgatory - initialize the purgatory's global variables
 * @image:		kexec image.
 * @slave_code:		Slave code for the purgatory.
 * @fdt:		Flattened device tree for the next kernel.
 * @kernel_load_addr:	Address where the kernel is loaded.
 * @fdt_load_addr:	Address where the flattened device tree is loaded.
 * @stack_top:		Address where the purgatory can place its stack.
 * @debug:		Can the purgatory print messages to the console?
 *
 * Return: 0 on success, or negative errno on error.
 */
int setup_purgatory(struct kimage *image, const void *slave_code,
		    const void *fdt, unsigned long kernel_load_addr,
		    unsigned long fdt_load_addr, unsigned long stack_top,
		    int debug)
{
	int ret, tree_node;
	const void *prop;
	unsigned long opal_base, opal_entry;
	uint64_t toc;
	unsigned int *slave_code_buf, master_entry;
	unsigned int toc_section;

	slave_code_buf = kmalloc(SLAVE_CODE_SIZE, GFP_KERNEL);
	if (!slave_code_buf)
		return -ENOMEM;

	/* Get the slave code from the new kernel and put it in purgatory. */
	ret = kexec_purgatory_get_set_symbol(image, "purgatory_start",
					     slave_code_buf, SLAVE_CODE_SIZE,
					     true);
	if (ret) {
		kfree(slave_code_buf);
		return ret;
	}

	master_entry = slave_code_buf[0];
	memcpy(slave_code_buf, slave_code, SLAVE_CODE_SIZE);
	slave_code_buf[0] = master_entry;
	ret = kexec_purgatory_get_set_symbol(image, "purgatory_start",
					     slave_code_buf, SLAVE_CODE_SIZE,
					     false);
	kfree(slave_code_buf);

	ret = kexec_purgatory_get_set_symbol(image, "kernel", &kernel_load_addr,
					     sizeof(kernel_load_addr), false);
	if (ret)
		return ret;
	ret = kexec_purgatory_get_set_symbol(image, "dt_offset", &fdt_load_addr,
					     sizeof(fdt_load_addr), false);
	if (ret)
		return ret;

	tree_node = fdt_path_offset(fdt, "/ibm,opal");
	if (tree_node >= 0) {
		prop = fdt_getprop(fdt, tree_node, "opal-base-address", NULL);
		if (!prop) {
			pr_err("OPAL address not found in the device tree.\n");
			return -EINVAL;
		}
		opal_base = fdt64_to_cpu((const fdt64_t *) prop);

		prop = fdt_getprop(fdt, tree_node, "opal-entry-address", NULL);
		if (!prop) {
			pr_err("OPAL address not found in the device tree.\n");
			return -EINVAL;
		}
		opal_entry = fdt64_to_cpu((const fdt64_t *) prop);

		ret = kexec_purgatory_get_set_symbol(image, "opal_base",
						     &opal_base,
						     sizeof(opal_base), false);
		if (ret)
			return ret;
		ret = kexec_purgatory_get_set_symbol(image, "opal_entry",
						     &opal_entry,
						     sizeof(opal_entry), false);
		if (ret)
			return ret;
	}

	ret = kexec_purgatory_get_set_symbol(image, "stack", &stack_top,
					     sizeof(stack_top), false);
	if (ret)
		return ret;

	toc_section = elf_toc_section(image->purgatory_info.ehdr,
				      image->purgatory_info.sechdrs);
	if (!toc_section)
		return -ENOEXEC;

	toc = elf_my_r2(image->purgatory_info.sechdrs, toc_section);
	ret = kexec_purgatory_get_set_symbol(image, "my_toc", &toc, sizeof(toc),
					     false);
	if (ret)
		return ret;

	pr_debug("Purgatory TOC is at 0x%llx\n", toc);

	ret = kexec_purgatory_get_set_symbol(image, "debug", &debug,
					     sizeof(debug), false);
	if (ret)
		return ret;
	if (!debug)
		pr_debug("Disabling purgatory output.\n");

	return 0;
}

/**
 * delete_fdt_mem_rsv - delete memory reservation with given address and size
 *
 * Return: 0 on success, or negative errno on error.
 */
int delete_fdt_mem_rsv(void *fdt, unsigned long start, unsigned long size)
{
	int i, ret, num_rsvs = fdt_num_mem_rsv(fdt);

	for (i = 0; i < num_rsvs; i++) {
		uint64_t rsv_start, rsv_size;

		ret = fdt_get_mem_rsv(fdt, i, &rsv_start, &rsv_size);
		if (ret) {
			pr_err("Malformed device tree.\n");
			return -EINVAL;
		}

		if (rsv_start == start && rsv_size == size) {
			ret = fdt_del_mem_rsv(fdt, i);
			if (ret) {
				pr_err("Error deleting device tree reservation.\n");
				return -EINVAL;
			}

			return 0;
		}
	}

	return -ENOENT;
}

/*
 * setup_new_fdt - modify /chosen and memory reservation for the next kernel
 * @image:		kexec image being loaded.
 * @fdt:		Flattened device tree for the next kernel.
 * @initrd_load_addr:	Address where the next initrd will be loaded.
 * @initrd_len:		Size of the next initrd, or 0 if there will be none.
 * @cmdline:		Command line for the next kernel, or NULL if there will
 *			be none.
 *
 * Return: 0 on success, or negative errno on error.
 */
int setup_new_fdt(const struct kimage *image, void *fdt,
		  unsigned long initrd_load_addr, unsigned long initrd_len,
		  const char *cmdline)
{
	int ret, chosen_node;
	const void *prop;

	/* Remove memory reservation for the current device tree. */
	ret = delete_fdt_mem_rsv(fdt, __pa(initial_boot_params),
				 fdt_totalsize(initial_boot_params));
	if (ret == 0)
		pr_debug("Removed old device tree reservation.\n");
	else if (ret != -ENOENT)
		return ret;

	chosen_node = fdt_path_offset(fdt, "/chosen");
	if (chosen_node == -FDT_ERR_NOTFOUND) {
		chosen_node = fdt_add_subnode(fdt, fdt_path_offset(fdt, "/"),
					      "chosen");
		if (chosen_node < 0) {
			pr_err("Error creating /chosen.\n");
			return -EINVAL;
		}
	} else if (chosen_node < 0) {
		pr_err("Malformed device tree: error reading /chosen.\n");
		return -EINVAL;
	}

	/* Did we boot using an initrd? */
	prop = fdt_getprop(fdt, chosen_node, "linux,initrd-start", NULL);
	if (prop) {
		uint64_t tmp_start, tmp_end, tmp_size;

		tmp_start = fdt64_to_cpu(*((const fdt64_t *) prop));

		prop = fdt_getprop(fdt, chosen_node, "linux,initrd-end", NULL);
		if (!prop) {
			pr_err("Malformed device tree.\n");
			return -EINVAL;
		}
		tmp_end = fdt64_to_cpu(*((const fdt64_t *) prop));

		/*
		 * kexec reserves exact initrd size, while firmware may
		 * reserve a multiple of PAGE_SIZE, so check for both.
		 */
		tmp_size = tmp_end - tmp_start;
		ret = delete_fdt_mem_rsv(fdt, tmp_start, tmp_size);
		if (ret == -ENOENT)
			ret = delete_fdt_mem_rsv(fdt, tmp_start,
						 round_up(tmp_size, PAGE_SIZE));
		if (ret == 0)
			pr_debug("Removed old initrd reservation.\n");
		else if (ret != -ENOENT)
			return ret;

		/* If there's no new initrd, delete the old initrd's info. */
		if (initrd_len == 0) {
			ret = fdt_delprop(fdt, chosen_node,
					  "linux,initrd-start");
			if (ret) {
				pr_err("Error deleting linux,initrd-start.\n");
				return -EINVAL;
			}

			ret = fdt_delprop(fdt, chosen_node, "linux,initrd-end");
			if (ret) {
				pr_err("Error deleting linux,initrd-end.\n");
				return -EINVAL;
			}
		}
	}

	if (initrd_len) {
		ret = fdt_setprop_u64(fdt, chosen_node,
				      "linux,initrd-start",
				      initrd_load_addr);
		if (ret < 0) {
			pr_err("Error setting up the new device tree.\n");
			return -EINVAL;
		}

		/* initrd-end is the first address after the initrd image. */
		ret = fdt_setprop_u64(fdt, chosen_node, "linux,initrd-end",
				      initrd_load_addr + initrd_len);
		if (ret < 0) {
			pr_err("Error setting up the new device tree.\n");
			return -EINVAL;
		}

		ret = fdt_add_mem_rsv(fdt, initrd_load_addr, initrd_len);
		if (ret) {
			pr_err("Error reserving initrd memory: %s\n",
			       fdt_strerror(ret));
			return -EINVAL;
		}
	}

	if (cmdline != NULL) {
		ret = fdt_setprop_string(fdt, chosen_node, "bootargs", cmdline);
		if (ret < 0) {
			pr_err("Error setting up the new device tree.\n");
			return -EINVAL;
		}
	} else {
		ret = fdt_delprop(fdt, chosen_node, "bootargs");
		if (ret && ret != -FDT_ERR_NOTFOUND) {
			pr_err("Error deleting bootargs.\n");
			return -EINVAL;
		}
	}

	ret = setup_ima_buffer(image, fdt, chosen_node);
	if (ret) {
		pr_err("Error setting up the new device tree.\n");
		return ret;
	}

	ret = fdt_setprop(fdt, chosen_node, "linux,booted-from-kexec", NULL, 0);
	if (ret) {
		pr_err("Error setting up the new device tree.\n");
		return -EINVAL;
	}

	return 0;
}

/**
 * find_debug_console - find out whether there is a console for the purgatory
 * @fdt:		Flattened device tree to search.
 */
bool find_debug_console(const void *fdt)
{
	int len;
	int console_node, chosen_node;
	const void *prop, *colon;

	chosen_node = fdt_path_offset(fdt, "/chosen");
	if (chosen_node < 0) {
		pr_err("Malformed device tree: /chosen not found.\n");
		return false;
	}

	prop = fdt_getprop(fdt, chosen_node, "stdout-path", &len);
	if (prop == NULL) {
		if (len == -FDT_ERR_NOTFOUND) {
			prop = fdt_getprop(fdt, chosen_node,
					   "linux,stdout-path", &len);
			if (prop == NULL) {
				pr_debug("Unable to find [linux,]stdout-path.\n");
				return false;
			}
		} else {
			pr_debug("Error finding console: %s\n",
				 fdt_strerror(len));
			return false;
		}
	}

	/*
	 * stdout-path can have a ':' separating the path from device-specific
	 * information, so we should only consider what's before it.
	 */
	colon = strchr(prop, ':');
	if (colon != NULL)
		len = colon - prop;
	else
		len -= 1;	/* Ignore the terminating NUL. */

	console_node = fdt_path_offset_namelen(fdt, prop, len);
	if (console_node < 0) {
		pr_debug("Error finding console: %s\n",
			 fdt_strerror(console_node));
		return false;
	}

	if (fdt_node_check_compatible(fdt, console_node, "hvterm1") == 0)
		return true;
	else if (fdt_node_check_compatible(fdt, console_node,
					   "hvterm-protocol") == 0)
		return true;

	return false;
}
