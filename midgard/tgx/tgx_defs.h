#ifndef _TGX_DEFS_H
#define _TGX_DEFS_H

/* #include <linux/init.h> */
// #include <linux/module.h>
// #include <linux/kernel.h>
// #include <linux/list.h>
/* #include <linux/slab.h> */

#define TGX_FAIL	0
#define TGX_SUCCESS 1

struct tgx_as;

enum tgx_as_type {
	TGX_AS_TYPE_UNKNOWN,
	TGX_AS_TYPE_GPU_BINARY,
	TGX_AS_TYPE_INOUT,
	TGX_AS_TYPE_ALL_WO_COMPUTE_JOB,
	TGX_AS_TYPE_COMPUTE_JOB,
};

struct tgx_mmu {
	phys_addr_t pgd;
	u64 transtab;
	u64 memattr;
	u64 transcfg;
	atomic_t pgt_nr_pages; 	// # of valid pages used for page table
//	size_t pgt_nr_pages;	// # of valid pages used for page table
	size_t buf_size;
	char *buf;	// dump buffer
};

struct tgx_as {
	u64 vm_start;
	u64 vm_end;
	u64 start_pfn;			/* The PFN in GPU space */
	size_t nr_pages;		/* # of pages */
	size_t nents;
	unsigned int flags;		/* region flgas */
	uint8_t is_valid;

	uint32_t as_num;		/* as number */

	/* jin: tgx_flgas used to
	 * 		1) check the the region is valid or not
	 *		2) the region is used for either input or output
	 */
	uint8_t tgx_flags;		/* tgx specific flags used for special purpose */
	uint8_t is_dumped;		/* flag indicates if AS is dumped */

	enum kbase_memory_type cpu_type;
	enum kbase_memory_type gpu_type;

	struct kbase_va_region *reg;
	enum tgx_as_type as_type;

	struct list_head link;
};

struct tgx_sync_as {
	uint64_t vm_start;
	uint64_t vm_end;
	size_t size;
	u8 cpu_to_gpu;
	u8 gpu_to_cpu;

	struct list_head link;
};

struct tgx_context {
	struct tgx_as tas_tmp;
	struct list_head as_list;
	u32 as_cnt;
	struct tgx_mmu tmmu;
	struct kbase_context *kctx;

	// about dump sync_as
	struct list_head sync_as_list;
	u32 sync_as_cnt;
	struct file *sync_as_file;
	u64 sync_as_file_offset;

	// about dump va region memory
	struct file *mem_dump_file;
	u64 mem_dump_file_offset;

	struct mutex mutex;
};

#ifdef CONFIG_TGX

int tgx_context_init(struct kbase_context *kctx);
void tgx_context_term(struct kbase_context *kctx);

/* called by kbase_do_syncset
 * store synced AS into the list */
int tgx_as_add_sync(struct tgx_context *tgx_ctx, struct basep_syncset const *sset, size_t offet);

/* called by kbase_cpu_vm_fault.
 * mark region is valid after page fault. */
int tgx_as_add_valid(struct tgx_context *tgx_ctx, struct vm_area_struct *vma, size_t nents);

/* called by kbase_context_mmap.
 * store used AS into the list */
int tgx_as_add(struct tgx_context *tgx_ctx, struct vm_area_struct *vma, struct kbase_va_region *region);
int tgx_print_as(struct tgx_context *tgx_cnt, int is_forced);

void tgx_mmu_dump(struct tgx_context *tgx_ctx, struct kbase_context *kctx);

/* Wrapper to invoke kbasep_mmu_dump_level() which dumps current GPU page table */
size_t tgx_mmu_dump_level(struct kbase_context *kctx, phys_addr_t pgd,
		int level, char ** const buffer, size_t *size_left);

/* Dump physical pages
 * As memory is not flushed yet due to both cpu/gpu cache,
 * we should map physical pages to kernel space first using kbase_vmap which flush the cache
 * and synchronize the memory contents
 */
int tgx_dump_phys_pages(struct kbase_context *kctx, struct file *file, u64 *offset, struct tgx_as *tas, int is_mapped); 
void tgx_dump_mc(struct tgx_context *tctx, enum tgx_as_type as_type);
void tgx_dump_mc_compute_job(struct tgx_context *tctx);

void tgx_dump_memory_contents(struct tgx_context *tctx);
void tgx_dump_vm_region_memory(struct tgx_context *tctx, struct kbase_va_region *reg);
void tgx_dump_sync_as(struct tgx_context *tctx);

void tgx_print_reg_flags(unsigned int flags);

/* unused functions are the below */
void tgx_mem_file_open(struct tgx_context *tctx);
void tgx_mem_file_close(struct tgx_context *tctx);

#endif	// CONFIG_TGX


#endif // _TGX_DEFS_
