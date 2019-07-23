// SPDX-License-Identifier: GPL-2.0

#include <linux/mm.h>
#include <linux/ptdump.h>
#include <linux/kasan.h>

static int ptdump_pgd_entry(pgd_t *pgd, unsigned long addr,
			    unsigned long next, struct mm_walk *walk)
{
	struct ptdump_state *st = walk->private;
	pgd_t val = READ_ONCE(*pgd);

	if (pgd_leaf(val))
		st->note_page(st, addr, 1, pgd_val(val));

	return 0;
}

static int ptdump_p4d_entry(p4d_t *p4d, unsigned long addr,
			    unsigned long next, struct mm_walk *walk)
{
	struct ptdump_state *st = walk->private;
	p4d_t val = READ_ONCE(*p4d);

	if (p4d_leaf(val))
		st->note_page(st, addr, 2, p4d_val(val));

	return 0;
}

static int ptdump_pud_entry(pud_t *pud, unsigned long addr,
			    unsigned long next, struct mm_walk *walk)
{
	struct ptdump_state *st = walk->private;
	pud_t val = READ_ONCE(*pud);

	if (pud_leaf(val))
		st->note_page(st, addr, 3, pud_val(val));

	return 0;
}

static int ptdump_pmd_entry(pmd_t *pmd, unsigned long addr,
			    unsigned long next, struct mm_walk *walk)
{
	struct ptdump_state *st = walk->private;
	pmd_t val = READ_ONCE(*pmd);

	if (pmd_leaf(val))
		st->note_page(st, addr, 4, pmd_val(val));

	return 0;
}

static int ptdump_pte_entry(pte_t *pte, unsigned long addr,
			    unsigned long next, struct mm_walk *walk)
{
	struct ptdump_state *st = walk->private;

	st->note_page(st, addr, 5, pte_val(READ_ONCE(*pte)));

	return 0;
}

#ifdef CONFIG_KASAN
/*
 * This is an optimization for KASAN=y case. Since all kasan page tables
 * eventually point to the kasan_early_shadow_page we could call note_page()
 * right away without walking through lower level page tables. This saves
 * us dozens of seconds (minutes for 5-level config) while checking for
 * W+X mapping or reading kernel_page_tables debugfs file.
 */
static inline bool kasan_page_table(struct ptdump_state *st, void *pt,
				    unsigned long addr)
{
	if (__pa(pt) == __pa(kasan_early_shadow_pmd) ||
#ifdef CONFIG_X86
	    (pgtable_l5_enabled() &&
			__pa(pt) == __pa(kasan_early_shadow_p4d)) ||
#endif
	    __pa(pt) == __pa(kasan_early_shadow_pud)) {
		st->note_page(st, addr, 5, pte_val(kasan_early_shadow_pte[0]));
		return true;
	}
	return false;
}
#else
static inline bool kasan_page_table(struct ptdump_state *st, void *pt,
				    unsigned long addr)
{
	return false;
}
#endif

static int ptdump_test_p4d(unsigned long addr, unsigned long next,
			   p4d_t *p4d, struct mm_walk *walk)
{
	struct ptdump_state *st = walk->private;

	if (kasan_page_table(st, p4d, addr))
		return 1;
	return 0;
}

static int ptdump_test_pud(unsigned long addr, unsigned long next,
			   pud_t *pud, struct mm_walk *walk)
{
	struct ptdump_state *st = walk->private;

	if (kasan_page_table(st, pud, addr))
		return 1;
	return 0;
}

static int ptdump_test_pmd(unsigned long addr, unsigned long next,
			   pmd_t *pmd, struct mm_walk *walk)
{
	struct ptdump_state *st = walk->private;

	if (kasan_page_table(st, pmd, addr))
		return 1;
	return 0;
}

static int ptdump_hole(unsigned long addr, unsigned long next,
		       struct mm_walk *walk)
{
	struct ptdump_state *st = walk->private;

	st->note_page(st, addr, -1, 0);

	return 0;
}

void ptdump_walk_pgd(struct ptdump_state *st, struct mm_struct *mm)
{
	struct mm_walk walk = {
		.mm		= mm,
		.pgd_entry	= ptdump_pgd_entry,
		.p4d_entry	= ptdump_p4d_entry,
		.pud_entry	= ptdump_pud_entry,
		.pmd_entry	= ptdump_pmd_entry,
		.pte_entry	= ptdump_pte_entry,
		.test_p4d	= ptdump_test_p4d,
		.test_pud	= ptdump_test_pud,
		.test_pmd	= ptdump_test_pmd,
		.pte_hole	= ptdump_hole,
		.private	= st
	};
	const struct ptdump_range *range = st->range;

	down_read(&mm->mmap_sem);
	while (range->start != range->end) {
		walk_page_range(range->start, range->end, &walk);
		range++;
	}
	up_read(&mm->mmap_sem);

	/* Flush out the last page */
	st->note_page(st, 0, 0, 0);
}
