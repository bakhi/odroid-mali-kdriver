#include "mali_kbase.h"
#include "tgx_defs.h"
#include "jin/measure.h"

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <asm/uaccess.h>

#include <linux/delay.h>

#ifdef CONFIG_TGX

/** #include "jin/log.h" */

// While it is inadvisable, just try to dump driver instrument using file IO in user space
// sys_open doesn't work
// https://sonseungha.tistory.com/137
// use filp_open
// https://stackoverrun.com/ko/q/214960
static struct file *file_open(const char *path, int flags, int rights)
{
	struct file *filp = NULL;
	mm_segment_t oldfs;
	int err = 0;

	oldfs = get_fs();
	set_fs(get_ds());
	filp = filp_open(path, flags, rights);
	set_fs(oldfs);
	if (IS_ERR(filp)) {
		err = PTR_ERR(filp);
		return NULL;
	}
	return filp;
}

static int file_write(struct file *file, unsigned long long offset, void *data, unsigned int size)
{
	mm_segment_t oldfs;
	int ret;
	oldfs = get_fs();
	set_fs(get_ds());

	// Since kernel version 4.14, vfs_write is no longer exported 
	// https://stackoverflow.com/questions/53917041/unknown-symbol-vfs-write-err-2-in-kernel-module-in-kernel-4-20
	/** ret = vfs_write(file, data, size, &offset); */
//	ret = kernel_write(file, data, size, &offset);
	ret = kernel_write(file, data, size, offset);

	set_fs(oldfs);
	return ret;
}

static void file_close(struct file *file)
{
	filp_close(file, NULL);
}

int tgx_context_init(struct kbase_context *kctx)
{
	struct tgx_context *tgx_ctx;
	tgx_ctx = vzalloc(sizeof(*tgx_ctx));
	if (WARN_ON(!tgx_ctx))
		return -1;

	if (tgx_ctx != NULL) {
		VV("tgx context init success");
		VV("kctx : %p, tgx_ctx : %p", (void *)kctx, (void *)tgx_ctx);
		INIT_LIST_HEAD(&tgx_ctx->as_list);
		INIT_LIST_HEAD(&tgx_ctx->sync_as_list);
	}

	/** tgx_ctx->tmmu.pgt_nr_pages = 0; */
	atomic_set(&tgx_ctx->tmmu.pgt_nr_pages, 0);

	tgx_ctx->tmmu.buf_size 		= 0;
	tgx_ctx->kctx 				= kctx;
	kctx->tgx_ctx 				= tgx_ctx;
	tgx_ctx->as_cnt				= 0;

	// jin: memory dump file open
	tgx_ctx->mem_dump_file 			= file_open("/home/linaro/tgx_mem_contents", O_WRONLY | O_CREAT, 0644);
	tgx_ctx->mem_dump_file_offset	= 0;

	// jin: format = vm_start (u64), vm_end (u64), size (size_t)
	tgx_ctx->sync_as_file 			= file_open("/home/linaro/tgx_sync_as", O_WRONLY | O_CREAT, 0644);
	tgx_ctx->sync_as_file_offset 	= 0;
	tgx_ctx->sync_as_cnt 			= 0;

	/** EE("mem_dump_file: %p, sysn_as_file: %p", tgx_ctx->mem_dump_file, tgx_ctx->sync_as_file); */

	return 0;
}

void tgx_context_term(struct kbase_context *kctx)
{
	struct tgx_context *tgx_ctx = kctx->tgx_ctx;
	struct tgx_as *tas;
	struct tgx_sync_as *sync_as;

	VV("free TGX context");
	file_close(tgx_ctx->mem_dump_file);
	file_close(tgx_ctx->sync_as_file);

	list_for_each_entry(tas, &tgx_ctx->as_list, link) {
		kfree(tas);
	}

	list_for_each_entry(sync_as, &tgx_ctx->sync_as_list, link) {
		kfree(sync_as);
	}

	vfree(tgx_ctx);
}

// jin: assume that only single thread from userspace can access it.
// otherwise we should lock before add as
int tgx_as_add_sync(struct tgx_context *tgx_ctx, struct basep_syncset const *sset, size_t offset)
{
	size_t sync_size = (size_t) sset->size;
	u64 sync_vm_start = sset->user_addr;
	u64 sync_vm_end = sync_vm_start + sync_size;
	struct tgx_sync_as *sync_as;
	u8 is_exist = 0;

	list_for_each_entry(sync_as, &tgx_ctx->sync_as_list, link) {
		if (sync_as->vm_start == sync_vm_start && sync_as->vm_end == sync_vm_end) {
			is_exist = 1;
			WW("[exist] vm_start: 0x%llx, vm_end: 0x%llx, size: %lu", sync_vm_start, sync_vm_end, sync_size);
			break;
		}
	}
	
	if (!is_exist) {
		WW("[Not exist] vm_start: 0x%llx, vm_end: 0x%llx, size: %lu", sync_vm_start, sync_vm_end, sync_size);
		sync_as = kzalloc(sizeof(*sync_as), GFP_KERNEL);
		if (WARN_ON(!sync_as))
			return TGX_FAIL;

		sync_as->vm_start 	= sync_vm_start;
		sync_as->vm_end 	= sync_vm_end;
		sync_as->size 		= sync_size;

		tgx_ctx->sync_as_cnt++;

		INIT_LIST_HEAD(&sync_as->link);

		list_add_tail(&sync_as->link, &tgx_ctx->sync_as_list);
	}

	switch (sset->type) {
	case BASE_SYNCSET_OP_MSYNC:
		sync_as->cpu_to_gpu++;
		break;
	case BASE_SYNCSET_OP_CSYNC:
		sync_as->gpu_to_cpu++;
		break;
	default:
		EE("[ERROR] Unkown msync op %d", sset->type);
		return TGX_FAIL;
	}

	return TGX_SUCCESS;
}

int tgx_as_add_valid(struct tgx_context *tgx_ctx, struct vm_area_struct *vma, size_t nents)
{
	struct tgx_as *tas;

	list_for_each_entry(tas, &tgx_ctx->as_list, link) {
		if (tas->vm_start == vma->vm_start && tas->vm_end == vma->vm_end) {
			tas->is_valid = 1;
			tas->nents = nents;
			break;
		}
	}
	return TGX_SUCCESS;
}

int tgx_as_add(struct tgx_context *tgx_ctx, struct vm_area_struct *vma, struct kbase_va_region *region)
{
	struct tgx_as *tas;

	tas = kzalloc(sizeof(*tas), GFP_KERNEL);
	if (WARN_ON(!tas))
		return TGX_FAIL;

   /**  tas->cpu_va = cpu_va; */
	/** tas->gpu_va = gpu_va; */
	tas->vm_start = vma->vm_start;
	tas->vm_end = vma->vm_end;
	tas->start_pfn = region->start_pfn;
	tas->nr_pages = region->nr_pages;
	tas->flags = region->flags;
	tas->is_valid = 0;
	tas->cpu_type = region->cpu_alloc->type;
	tas->gpu_type = region->gpu_alloc->type;

	tas->reg = region;

	/** NOTE:
	  * reg->initial_commit: initial commit when kbase_mem_alloc is invoked
	  * va_pages: size of vm region in pages
	  * alloc->nents: actual allocated pages which can be updated when ioctl_commit runs -> vm_page_fault() */
	tas->nents = region->cpu_alloc->nents;
	tas->is_dumped = 0;

	/** EE("region->start_pfn : %llx", region->start_pfn); */

#if 0
	struct kbase_mem_phy_alloc *cpu_alloc = tas->reg->cpu_alloc;
	struct kbase_mem_phy_alloc *gpu_alloc = tas->reg->gpu_alloc;

	phys_addr_t cpu_pa = as_phys_addr_t(cpu_alloc->pages[0]);
	phys_addr_t gpu_pa = as_phys_addr_t(gpu_alloc->pages[0]);

	EE("addr: %llx cpu_pa : %llx gpu_pa : %llx", tas->vm_start, cpu_pa, gpu_pa);
	EE("[tgx] vm_start : %llx, vm_end : %llx", tas->vm_start, tas->vm_end);
#endif
	/** EE("[tgx] as_cnt: %d, vm_start : %llx, vm_end : %llx", tgx_ctx->as_cnt, tas->vm_start, tas->vm_end); */

	INIT_LIST_HEAD(&tas->link);

	if (!(tas->flags & KBASE_REG_GPU_NX)) {
		// jin: GPU_BINARY region does not have GPU_NX flag
		tas->as_type = TGX_AS_TYPE_GPU_BINARY;
	} else if (tgx_ctx->as_cnt > 5 && tas->nr_pages == 1 && !(tas->flags & KBASE_REG_CPU_CACHED)) {
		// jin: compute job region has no CPU_CACHE flag and its nr_pages is 1
		tas->as_type = TGX_AS_TYPE_COMPUTE_JOB;
	} else if (tas->flags & KBASE_REG_CPU_CACHED && tas->flags & KBASE_REG_GPU_CACHED) {
		// jin: in/output region has both CPU and GPU CACHED flags
		tas->as_type = TGX_AS_TYPE_INOUT;
	} else {
		tas->as_type = TGX_AS_TYPE_UNKNOWN;
	}

	if (tgx_ctx) {
		/** EE("[tgx] list add tail"); */
		tas->as_num = tgx_ctx->as_cnt++;
		list_add_tail(&tas->link, &tgx_ctx->as_list);
	}

	return TGX_SUCCESS;
}

void tgx_print_reg_flags(unsigned int flags)
{
#if 0	// in: not supported from current version
	switch (flags & KBASE_REG_ZONE_MASK) {
		case KBASE_REG_ZONE_CUSTOM_VA:
			printk(KERN_CONT "ZONE_CUSTOME_VA | ");
			break;
		case KBASE_REG_ZONE_SAME_VA:
			printk(KERN_CONT "ZONE_SAME_VA | ");
			break;
		case KBASE_REG_ZONE_EXEC_VA:
			printk(KERN_CONT "ZONE_EXEC_VA | ");
			break;
		default:
			break;
	}
#endif

	if (flags & KBASE_REG_FREE)
		printk(KERN_CONT "FREE | ");
	if (flags & KBASE_REG_CPU_WR)
		printk(KERN_CONT "CPU_WR | ");
	if (flags & KBASE_REG_CPU_RD)
		printk(KERN_CONT "CPU_RD | ");
	if (flags & KBASE_REG_GPU_WR)
		printk(KERN_CONT "GPU_WR | ");
	if (flags & KBASE_REG_GPU_RD)
		printk(KERN_CONT "GPU_RD | ");
	if (flags & KBASE_REG_GPU_NX)
		printk(KERN_CONT "GPU_NX | ");
//	if (flags & KBASE_REG_NO_USER_FREE)
//		printk(KERN_CONT "NO_USER_FREE | ");
	if (flags & KBASE_REG_CPU_CACHED)
		printk(KERN_CONT "CPU_CACHED | ");
	if (flags & KBASE_REG_GPU_CACHED)
		printk(KERN_CONT "GPU_CACHED | ");
}

int tgx_print_as(struct tgx_context *tgx_ctx, int is_forced) {
	struct tgx_as *tas;
//	int i;
	int cnt = 0;
	static int is_done = 0;

	
	// jin: note that reg->cpu_alloc would be freed so that it no longer contains paddrs used for that region
	// This is why vmap failed...
	if (is_done < 1 || is_forced) {	// for va, conv
	/** if (is_done < 2) {	// for sgemm */
		is_done++;

		if (!tgx_ctx)
			return TGX_FAIL;

		list_for_each_entry(tas, &tgx_ctx->as_list, link) {
			EE("[AS:%d type: %d] start: %llx, end: %llx, flags: %x, valid: %u, nr_pages: %lu, nents: %lu", \
					cnt, tas->as_type, tas->vm_start, tas->vm_end, tas->flags, tas->is_valid, tas->nr_pages, tas->nents);
			tgx_print_reg_flags(tas->flags);
			/** EE("cpu_type : %d gpu_type : %d", (int)tas->cpu_type, (int)tas->gpu_type); */

#if 0
			struct kbase_mem_phy_alloc *cpu_alloc = tas->reg->cpu_alloc;
			struct kbase_mem_phy_alloc *gpu_alloc = tas->reg->gpu_alloc;
			phys_addr_t cpu_pa;
			phys_addr_t gpu_pa;

			if (tas->is_valid) {
				struct page *p;
				u64 *mapped_page;

				// jin: print last page in case of this address space
				// Sometimes the last page of this vma space contains in/output data, if in/output size are small enought to fit into a single page
				if (tas->nents == 513) {
					cpu_pa = as_phys_addr_t(cpu_alloc->pages[512]);
					EE("[USE_VA | last page among 513] vma: %llx pa: %llx", tas->vm_start + PAGE_SIZE * 512, cpu_pa);
					hex_dump((void *)(tas->vm_start + PAGE_SIZE * 512), 64);
				/** } else if (cnt == 8 || cnt == 9) {	// for conv */
				} else if (cnt == 5) {	// for va
					for (i = 0; i < min(tas->nents, 2UL); i++) {
						// jin: cpu paddr is same with gpu paddr
						cpu_pa = as_phys_addr_t(cpu_alloc->pages[i]);
						gpu_pa = as_phys_addr_t(gpu_alloc->pages[i]);

#if 0 // jin: print using kmap to map phys to kernel vma
						/** p = pfn_to_page(PFN_DOWN(cpu_pa)); */
						p = as_page(cpu_alloc->pages[i]);

						mapped_page = kmap(p);
						EE("mapped page : %p", mapped_page);
						EE("[USE_PA][offset:%d] mapped_va: %p pa : %llx cpu_pa : %llx gpu_pa : %llx", \
								i, mapped_page, virt_to_phys(mapped_page), cpu_pa, gpu_pa);
						/** hex_dump((void *)(mapped_page + PAGE_SIZE * i), 64); */
						hex_dump(mapped_page, 64);
						kunmap(p);
#endif

						EE("[USE_VA][offset:%d] va: %llx pa: %llx", \
								i, tas->vm_start + PAGE_SIZE * i, cpu_pa);
						hex_dump((void *)(tas->vm_start + PAGE_SIZE * i), 64);
					}
				} else if (cnt == 27) {
					struct kbase_vmap_struct map = { 0 };
					/** u64 *vmapped_pages; */
					/** struct page *p; */
					/** u64 *mapped_page; */

					/** vmapped_pages = kbase_vmap(tgx_ctx->kctx, tas->vm_start, tas->nents, &map); */

					for (i = 0; i < min(tas->nents, 999UL); i++) {
						cpu_pa = as_phys_addr_t(cpu_alloc->pages[i]);
						gpu_pa = as_phys_addr_t(gpu_alloc->pages[i]);

						/**                      p = as_page(cpu_alloc->pages[i]); */
						/** mapped_page = kmap(p); */
						/** EE("mapped page : %p", mapped_page); */
						/** EE("[USE_PA][offset:%d] mapped_va: %p pa : %llx cpu_pa : %llx gpu_pa : %llx", \ */
						/**         i, mapped_page, virt_to_phys(mapped_page), cpu_pa, gpu_pa); */
						/** hex_dump(mapped_page, 64); */
						/** kunmap(p); */

#if 1
   /**                      EE("map->cpu_pages : %llx", map.cpu_pages[i]); */
						/** EE("[USE_VMAP] vmapped_addr: %llx pages : %llx cpu_pa : %llx gpu_pa : %llx", \ */
						/**         vmapped_pages[i], cpu_alloc->pages[i], cpu_pa, gpu_pa); */
						/** hex_dump((void *)vmapped_pages[i], 64); */
#endif
					}

					/* vmap */
					/** kbase_vunmap(tgx_ctx->kctx, &map); */
				}

			} else {
#if 0
				/* vmap */
				if (is_done == 27) {	// output as
					struct kbase_vmap_struct map = { 0 };
					u64 *vmapped_page;
					/** struct page *p; */
					/** u64 *mapped_page; */

					vmapped_page = kbase_vmap(tgx_ctx->kctx, tas->vm_start, tas->nents, &map);

					for (i = 0; i < min(tas->nents, 999UL); i++) {
						cpu_pa = as_phys_addr_t(cpu_alloc->pages[i]);
						gpu_pa = as_phys_addr_t(gpu_alloc->pages[i]);

   /**                      p = as_page(cpu_alloc->pages[i]); */
						/** mapped_page = kmap(p); */
						/** EE("mapped page : %p", mapped_page); */
						/** EE("[USE_PA][offset:%d] mapped_va: %p pa : %llx cpu_pa : %llx gpu_pa : %llx", \ */
						/**         i, mapped_page, virt_to_phys(mapped_page), cpu_pa, gpu_pa); */
						/** hex_dump(mapped_page, 64); */
						/** kunmap(p); */

#if 1
						EE("map->cpu_pages : %llx", map.cpu_pages[i]);
						EE("[USE_VMAP] vmapped_addr: %llx pages : %llx cpu_pa : %llx gpu_pa : %llx", \
								vmapped_page[i], cpu_alloc->pages[i], cpu_pa, gpu_pa);
						hex_dump((void *)vmapped_page[i], 64);
#endif
					}

					/* vmap */
					kbase_vunmap(tgx_ctx->kctx, &map);
				}
#endif
			}
#endif
			cnt++;
			}
		} else {

		}
	return TGX_SUCCESS;

}

void tgx_mmu_dump(struct tgx_context *tgx_ctx, struct kbase_context *kctx) {
	struct tgx_mmu *tmmu = &tgx_ctx->tmmu;
	char *buffer;
	char *mmu_dump_buffer;
	u64 *tmp_buffer;
	struct kbase_mmu_setup as_setup;

	struct file* file;
//	uint64_t pos = 0;
//	uint64_t written = 0;
	size_t size_left = 0, dump_size = 0;
//	int i;
	u64 end_marker = 0xFFFFFFFFFFULL;

	size_t pgt_nr_pages = 0;

	static int is_done = 0;

	if (!is_done) {

		kctx->kbdev->mmu_mode->get_as_setup(kctx, &as_setup);

		pgt_nr_pages = atomic_read(&tmmu->pgt_nr_pages);
		//	pgt_nr_pages += 1; 	// add one for level 0 pgd
		//	tmmu->pgt_nr_pages += 1;	// add one for level 0 pgd

		/** calculate buffer size*/
		size_left += sizeof(u64);					// size
		size_left += sizeof(u64) * 3;					// for config
		size_left += sizeof(u64) * pgt_nr_pages;	// for pgd, pgd, pmd
		size_left += pgt_nr_pages * PAGE_SIZE;	// for entries
		size_left += sizeof(u64);						// for end_marker

		EE("size_left : %lu", size_left);
		EE("[%s] pgd : 0x%0llx", __func__, tmmu->pgd);
		EE("[%s] nr_pages for pgt : %lu", __func__, pgt_nr_pages);

		tmmu->transtab = as_setup.transtab;
		tmmu->memattr = as_setup.memattr;
		tmmu->transcfg = as_setup.transcfg;

		buffer = (char *) vzalloc(size_left);
		mmu_dump_buffer = buffer;
		tmp_buffer = (u64 *) buffer;

		memcpy(mmu_dump_buffer, &size_left, sizeof(u64));
		mmu_dump_buffer += sizeof(size_t);
		memcpy(mmu_dump_buffer, &as_setup.transtab, sizeof(u64));
		mmu_dump_buffer += sizeof(u64);
		memcpy(mmu_dump_buffer, &as_setup.memattr, sizeof(u64));
		mmu_dump_buffer += sizeof(u64);
		memcpy(mmu_dump_buffer, &as_setup.transcfg, sizeof(u64));
		mmu_dump_buffer += sizeof(u64);

		size_left -= sizeof(size_t) + sizeof(u64) * 3;
		dump_size += sizeof(size_t) + sizeof(u64) * 3;		// confg

		dump_size += tgx_mmu_dump_level(kctx,
				kctx->pgd,
				MIDGARD_MMU_TOPLEVEL,
				&mmu_dump_buffer,
				&size_left);

		if (size_left != sizeof(u64)) {
			EE("size left (%lu) is not euqal to config and endmarker", size_left);
		}

		memcpy(mmu_dump_buffer, &end_marker, sizeof(u64));
		mmu_dump_buffer += sizeof(u64);
		size_left -= sizeof(u64);
		dump_size += sizeof(u64);		// end marker

		EE("dump_size : %lu", dump_size);

		// jin: check the pte is correctly stored
		/** EE("transtab : %016llx", tmmu->transtab); */
		/** EE("memattr : %016llx", tmmu->memattr); */
		/** EE("transcfg : %016llx", tmmu->transcfg); */
		/**  for (i = 0; i < 516; i++) { */
		/** EE("%llx %llx", (void *) tmp_buffer, (void *) buffer); */
		/**  for (i = 0; i < 3; i++) { */
		/**     EE("[%d] tmp_buffer 0x%llx", i, tmp_buffer[i]); */
		/**     EE("[%d] %c %c %c %c %c %c %c %c", i,  */
		/**         buffer[i*8], buffer[i*8 + 1], buffer[i*8 + 2], buffer[i*8] + 3, */
		/**         buffer[i*8] + 4, buffer[i*8 + 5], buffer[i*8 + 6], buffer[i*8] + 7); */
		/** } */

		// open file to dump page table
		file = file_open("/home/linaro/tgx_pgt", O_WRONLY | O_CREAT, 0644);

		file_write(file, 0, buffer, dump_size);

		file_close(file);
		vfree(buffer);

		is_done++;
	}
}

// not used yet
#if 0
void tgx_dump_single_phys_page(uint64_t vaddr, uint64_t paddr, uint8_t is_mapped, uint8_t is_print)
{
	if (!is_mapped) {
		uint64_t *mapped_page;
		struct page *p;

		p = phys_to_page(paddr);

		// sync cpu to mem
		if (likely(cpu_alloc == gpu_alloc)) {
			dma_addr_t dma_addr;

			dma_addr = kbase_dma_addr(pfn_to_page(PFN_DOWN(paddr)));
			/** dma_sync_single_for_device(kctx->kbdev->dev, dma_addr, PAGE_SIZE, DMA_BIDIRECTIONAL); */
			/** dma_sync_single_for_cpu(kctx->kbdev->dev, dma_addr, PAGE_SIZE, DMA_BIDIRECTIONAL); */
		} else {
			/** TODO: handle cpu_pa != gpu_pa */
		}

		mapped_page = kmap(p);
		file_write(file, *offset, mapped_page, PAGE_SIZE);

		if (is_print == 1) {
			EE("----------- mapped_page (%llx)------------ ", mapped_page);
			/** hex_dump((void *) mapped_page, 128); */
		}
		kunmap(p);

	} else {
		uint64_t *buffer = (uint64_t *) vaddr;
		file_write(file, *offset, buffer, PAGE_SIZE);

		if (is_print) {
			EE("----------- buffer (%llx) ------------ ", buffer);
			/** hex_dump((void *) buffer, 128); */
		}
	}

	*offset += PAGE_SIZE;
}
#endif

/* 
 * Format:
 * vaddr(u64)
 * paddr(u64)
 * contents(PAGE_SIZE) 512 entries
 *
 * Thought: Caller dump memory contents right before the job submission
 * 			It would be fine without sync (cache flush) as we access cpu vaddr.
 *			Even in case contents is cached, CPU knows the correct contents.
 *
*/
int tgx_dump_phys_pages(struct kbase_context *kctx, struct file *file, u64 *offset, struct tgx_as *tas, int is_mapped)
{
	struct kbase_mem_phy_alloc *cpu_alloc = tas->reg->cpu_alloc;
	struct kbase_mem_phy_alloc *gpu_alloc = tas->reg->gpu_alloc;
	/** phys_addr_t cpu_pa = as_phys_addr_t(cpu_alloc->pages[0]); */
	size_t i;
	u64 *buffer;
	u64 vaddr, paddr;
	
	buffer = (u64 *) tas->vm_start;

	if (!file || !offset) {
		EE("no file pointer or offset");
		return TGX_FAIL;
	}

	JJ("vm_start: 0x%llx vm_end: 0x%llx nents: %lu", tas->vm_start, tas->vm_end, tas->nents);
	for (i = 0; i < tas->nents; i++) {
		struct page *p;
		u64 *mapped_page;

		vaddr = tas->vm_start + (PAGE_SIZE * i);
		paddr = as_phys_addr_t(cpu_alloc->pages[i]);

		buffer = (u64 *) vaddr;
		/** EE("vaddr: %llx, paddr: %llx, nents: %llu buffer: %llx", vaddr, paddr, tas->nents, buffer); */

		//jin: must map paddr. Guess: at some point, cpu mapping is freed? cannot directly access cpu vaddr

		p = as_page(cpu_alloc->pages[i]);

		// sync cpu to mem
		if (likely(cpu_alloc == gpu_alloc)) {
			dma_addr_t dma_addr;

			dma_addr = kbase_dma_addr(pfn_to_page(PFN_DOWN(paddr)));
			dma_sync_single_for_device(kctx->kbdev->dev, dma_addr, PAGE_SIZE, DMA_BIDIRECTIONAL);
			/** dma_sync_single_for_cpu(kctx->kbdev->dev, dma_addr, PAGE_SIZE, DMA_BIDIRECTIONAL); */
		} else {
			/** TODO: handle cpu_pa != gpu_pa */
		}

		mapped_page = kmap(p);
		// XXX should sync page to cpu?
		// if needed, use kbase_sync_single()

		file_write(file, *offset, &vaddr, sizeof(u64));
		*offset += sizeof(u64);
		file_write(file, *offset, &paddr, sizeof(u64));
		*offset += sizeof(u64);

		/** file_write(file, *offset, buffer, PAGE_SIZE); */
		file_write(file, *offset, mapped_page, PAGE_SIZE);
		*offset += PAGE_SIZE;

		/** if (tas->nents != 513 && tas->nents != 64) { */
		if (tas->nents == 1 || tas->as_num == 0) {
			if(is_mapped == 1) {
				EE("----------- buffer (%lln) ------------ ", buffer);
				/** hex_dump((void *) buffer, 128); */
			} else if(is_mapped == 0) {
				EE("----------- mapped_page (%lln)------------ ", mapped_page);
				/** hex_dump((void *) mapped_page, 128); */
			} else {
				EE("----------- buffer (%lln) ------------ ", buffer);
				/** hex_dump((void *) mapped_page, 4096); */
			}
		}

		kunmap(p);
	}
	return TGX_SUCCESS;


// jin: use vmap to dump memory contents
// every allocated page by using kbase_mem_alloc is mapped to bus address (dma_addr_t) and not synchronized before we call dma_sync_single_For_cpu().
// calling vmap guarnatees that the memory is flushed by GPU and thus content is synchronized

/* XXX due to sync problem, 
 * 1) flush cache and map the page to the kernel space
 * 2) copy content with page unit
 */
#if 0
	struct kbase_vmap_struct map = { 0 };
	u64 *vmapped_page;
	size_t i;

	EE("vm_start: %llx, nents: %llu", tas->vm_start, tas->nents);
	vmapped_page = kbase_vmap(kctx, tas->vm_start, tas->nents * PAGE_SIZE, &map);
	if (!vmapped_page)
		return TGX_FAIL;

	u64 *buffer = vmapped_page;
	u64 paddr = tas->start_pfn;

	for (i = 0; i < tas->nents; i++) {
		file_write(file, *offset, &paddr, sizeof(u64));
		*offset += sizeof(u64);
		file_write(file, *offset, buffer, PAGE_SIZE);
		*offset += PAGE_SIZE;

		paddr += PAGE_SIZE;
		buffer += PAGE_SIZE;
	}

	kbase_vunmap(kctx, &map);
	return TGX_SUCCESS;
#endif
}

void tgx_mem_file_open(struct tgx_context *tctx) {
	tctx->mem_dump_file = file_open("/home/linaro/tgx_mem_contents", O_WRONLY | O_CREAT, 0644);
	tctx->mem_dump_file_offset = 0;
	/** WW("file open is done. offset = %llu", tctx->mem_dump_file_offset); */
}

void tgx_mem_file_close(struct tgx_context *tctx) {
	file_close(tctx->mem_dump_file);
}

void tgx_dump_sync_as(struct tgx_context *tctx) {
	struct file *file = tctx->sync_as_file;
	struct tgx_sync_as *sync_as;
	u64 *offset = &tctx->sync_as_file_offset;

	file_write(file, *offset, &tctx->sync_as_cnt, sizeof(u32));
	*offset += sizeof(u32);
	EE("[tgx_dump_sync_as] total count of sync_as: %u", tctx->sync_as_cnt);

	list_for_each_entry(sync_as, &tctx->sync_as_list, link) {
		WW("vm_start: 0x%016llx, vm_end: 0x%016llx, size: %lu", \
			sync_as->vm_start, sync_as->vm_end, sync_as->size);
		file_write(file, *offset, &sync_as->vm_start, sizeof(u64));
		*offset += sizeof(u64);
		file_write(file, *offset, &sync_as->vm_end, sizeof(u64));
		*offset += sizeof(u64);
		file_write(file, *offset, &sync_as->size, sizeof(size_t));
		*offset += sizeof(u64);
	}
}

static void _tgx_dump_mc(struct tgx_context *tctx, struct tgx_as *tas) {
	struct file *file = tctx->mem_dump_file;
	u64 *offset = &tctx->mem_dump_file_offset;

	file_write(file, *offset, &tas->vm_start, sizeof(u64));
	*offset += sizeof(u64);
	file_write(file, *offset, &tas->vm_end, sizeof(u64));
	*offset += sizeof(u64);
	file_write(file, *offset, &tas->nents, sizeof(size_t));
	*offset += sizeof(size_t);
	file_write(file, *offset, &tas->flags, sizeof(uint32_t));
	*offset += sizeof(uint32_t);
	file_write(file, *offset, &tas->is_valid, sizeof(uint8_t));
	*offset += sizeof(uint8_t);

	if (tas->is_valid) {
		if (tas->as_num == 7)
			tgx_dump_phys_pages(tctx->kctx, file, offset, tas, 2);
		else
			tgx_dump_phys_pages(tctx->kctx, file, offset, tas, 1);
		/** tgx_dump_single_phys_page(vaddr, paddr, 0); */
		EE("[TOUCHED][AS:%d SUCCESS] offset = %llu", tas->as_num, *offset);
	} else {
   /**      if (tas->as_num == 0) */
			/** tgx_dump_phys_pages(tctx->kctx, file, offset, tas, 0); */
		EE("[GPU INTERNAL][AS:%d SUCCESS] offset = %llu", tas->as_num, *offset);
	}

	tas->is_dumped = 1;
}

void tgx_dump_mc(struct tgx_context *tctx, enum tgx_as_type as_type) {
	struct tgx_as *tas;

	/** kbase_gpu_vm_lock(tctx->kctx); */
	/** mutex_lock(&tctx->mutex); */
	list_for_each_entry(tas, &tctx->as_list, link) {
		if (!tas->is_dumped) {
			switch (as_type) {
				case TGX_AS_TYPE_COMPUTE_JOB:
				case TGX_AS_TYPE_GPU_BINARY:
					if (tas->as_type == TGX_AS_TYPE_COMPUTE_JOB || tas->as_type == TGX_AS_TYPE_GPU_BINARY) {
						_tgx_dump_mc(tctx, tas);
					}
					break;
				case TGX_AS_TYPE_ALL_WO_COMPUTE_JOB:
				case TGX_AS_TYPE_UNKNOWN:
				case TGX_AS_TYPE_INOUT:
				default:
					if (tas->as_type != TGX_AS_TYPE_COMPUTE_JOB && tas->as_type != TGX_AS_TYPE_GPU_BINARY) {
						_tgx_dump_mc(tctx, tas);
					}
					break;
			}
		} else {
   /**          // to dump input of AlexNet */
			/** if (as_type == TGX_AS_TYPE_COMPUTE_JOB && tas->as_num == 26) { */
#if 0
			if (as_type == TGX_AS_TYPE_COMPUTE_JOB && tas->as_num == 24) {
	   /**      if (as_type == TGX_AS_TYPE_COMPUTE_JOB && !tas->is_valid && \ */
				/** tas->nents != 32 && tas->nents != 25) { */
			/** if (as_type == TGX_AS_TYPE_COMPUTE_JOB && tas->as_num == 35) { */

				struct kbase_mem_phy_alloc *cpu_alloc = tas->reg->cpu_alloc;
				struct kbase_mem_phy_alloc *gpu_alloc = tas->reg->gpu_alloc;

				struct page *p = as_page(cpu_alloc->pages[34]);
				u64 paddr = as_phys_addr_t(cpu_alloc->pages[34]);

				// sync cpu to mem
				if (likely(cpu_alloc == gpu_alloc)) {
					dma_addr_t dma_addr;

					dma_addr = kbase_dma_addr(pfn_to_page(PFN_DOWN(paddr)));
					/** dma_sync_single_for_device(kctx->kbdev->dev, dma_addr, PAGE_SIZE, DMA_BIDIRECTIONAL); */
					dma_sync_single_for_cpu(tctx->kctx->kbdev->dev, dma_addr, PAGE_SIZE, DMA_BIDIRECTIONAL);
				} else {
					/** TODO: handle cpu_pa != gpu_pa */
				}

				u64 *mapped_page = kmap(p);
				JJ("as_cnt: %u nents: %u", tas->as_num, tas->nents);
				hex_dump((void *) mapped_page, 256);
				kunmap(p);
			}
#endif
		}
#if 0
		/** if (as_type == TGX_AS_TYPE_COMPUTE_JOB && tas->as_num == 24) { */
		if (tas->as_num == 24) {
		/** if (tas->as_num == 41) { */
		/** if (tas->as_num == 38) { */
			struct kbase_mem_phy_alloc *cpu_alloc = tas->reg->cpu_alloc;
			struct kbase_mem_phy_alloc *gpu_alloc = tas->reg->gpu_alloc;

			struct page *p = as_page(cpu_alloc->pages[34]);
			u64 paddr = as_phys_addr_t(cpu_alloc->pages[34]);
   /**          struct page *p = as_page(cpu_alloc->pages[0]); */
			/** u64 paddr = as_phys_addr_t(cpu_alloc->pages[0]); */

			// sync cpu to mem
			if (likely(cpu_alloc == gpu_alloc)) {
				dma_addr_t dma_addr;

				dma_addr = kbase_dma_addr(pfn_to_page(PFN_DOWN(paddr)));
				/** dma_sync_single_for_device(kctx->kbdev->dev, dma_addr, PAGE_SIZE, DMA_BIDIRECTIONAL); */
				dma_sync_single_for_cpu(tctx->kctx->kbdev->dev, dma_addr, PAGE_SIZE, DMA_BIDIRECTIONAL);
			} else {
				/** TODO: handle cpu_pa != gpu_pa */
			}

			u64 *mapped_page = kmap(p);
			JJ("as_cnt: %u nents: %u", tas->as_num, tas->nents);
			hex_dump((void *) mapped_page, 256);
			kunmap(p);
		}
#endif
	}
	/** mutex_unlock(&tctx->mutex); */
	/** kbase_gpu_vm_unlock(tctx->kctx); */
}

void tgx_dump_mc_compute_job(struct tgx_context *tctx) {
	struct tgx_as *tas;
	int32_t cnt = 0;

	mutex_lock(&tctx->mutex);
	cnt = 0;
	list_for_each_entry(tas, &tctx->as_list, link) {
		if (!tas->is_dumped && tas->as_type == TGX_AS_TYPE_COMPUTE_JOB) {
			_tgx_dump_mc(tctx, tas);
		}
		cnt++;
	}
	mutex_unlock(&tctx->mutex);
}

void tgx_dump_memory_contents(struct tgx_context *tctx)
{
	struct tgx_as *tas;
	struct file *file;
	u64 *offset;
	int32_t cnt = 0;

//	static int is_done = 0;

	// acl_conv
   /**  uint8_t input_as = 8; */
	/** uint8_t output_as = 9; */

	// cl_va32k
   /**  uint8_t input_as = 6; */
	/** uint8_t output_as = 7; */

	// acl_sgemm
   /**  uint8_t input_as = 5; */
	/** uint8_t output_as = 5; */

	// alexNet
//	uint8_t input_as = 26;
//	uint8_t output_as = 33;

	file = tctx->mem_dump_file;
	offset = &tctx->mem_dump_file_offset;

	mutex_lock(&tctx->mutex);
	cnt = 0;
	list_for_each_entry(tas, &tctx->as_list, link) {
		/** if (!tas->is_dumped) { */
		/** if (!tas->is_dumped || cnt == 7) { */
		if (!tas->is_dumped && (tas->nents == 1 || tas->nents == 25)) {
			file_write(file, *offset, &tas->vm_start, sizeof(u64));
			*offset += sizeof(u64);
			file_write(file, *offset, &tas->vm_end, sizeof(u64));
			*offset += sizeof(u64);
			file_write(file, *offset, &tas->nents, sizeof(size_t));
			*offset += sizeof(size_t);
			file_write(file, *offset, &tas->flags, sizeof(uint32_t));
			*offset += sizeof(uint32_t);
			file_write(file, *offset, &tas->is_valid, sizeof(uint8_t));
			*offset += sizeof(uint8_t);

			if (tas->is_valid) {
				if(cnt == 7)
					tgx_dump_phys_pages(tctx->kctx, file, offset, tas, 2);
				else
					tgx_dump_phys_pages(tctx->kctx, file, offset, tas, 1);
				/** tgx_dump_single_phys_page(vaddr, paddr, 0); */
				EE("[TOUCHED][AS:%d SUCCESS] offset = %llu", cnt, *offset);
			} else {
				EE("[GPU INTERNAL][AS:%d SUCCESS] offset = %llu", cnt, *offset);
			}
			tas->is_dumped = 1;
		}
		cnt++;
		}
		mutex_unlock(&tctx->mutex);
}

// TODO: flush vm_region_memory when job submit happens or (periodically). Mark dumped region not to flush again
// @in: valid region
static void _tgx_dump_vm_region_memory(struct tgx_context *tctx, struct tgx_as *tas, uint32_t cnt) {
	struct file *file = tctx->mem_dump_file;
	u64 *offset = &tctx->mem_dump_file_offset;

	if (!tas->is_dumped) {
		WW("start_pfn : %llx, is_valid: %u, file_offset: %llu", tas->start_pfn, tas->is_valid, *offset);
		file_write(file, *offset, &tas->vm_start, sizeof(u64));
		*offset += sizeof(u64);
		file_write(file, *offset, &tas->vm_end, sizeof(u64));
		*offset += sizeof(u64);
		file_write(file, *offset, &tas->nents, sizeof(size_t));
		*offset += sizeof(size_t);
		file_write(file, *offset, &tas->flags, sizeof(uint32_t));
		*offset += sizeof(uint32_t);
		file_write(file, *offset, &tas->is_valid, sizeof(uint8_t));
		*offset += sizeof(uint8_t);

#if 0
		if (tas->is_valid) {
			if (!tgx_dump_phys_pages(tctx->kctx, file, offset, tas, 0)) {
				EE("[%s][AS:%d FAIL] offset = %llu", __func__, *offset);
			} else {
				EE("[%s][AS:%d SUCCESS] offset = %llu", __func__, *offset);
			}
		}
#endif
	}
}

void tgx_dump_vm_region_memory(struct tgx_context *tctx, struct kbase_va_region *reg) {
	struct tgx_as *tas;
	uint32_t cnt = 0;

	list_for_each_entry(tas, &tctx->as_list, link) {
		if (tas->start_pfn == reg->start_pfn && tas->is_valid) {
		/** if (tas->start_pfn == reg->start_pfn) { */
			_tgx_dump_vm_region_memory(tctx, tas, cnt);
			break;
		}
		cnt++;
	}
}

#endif	// CONFIG_TGX
