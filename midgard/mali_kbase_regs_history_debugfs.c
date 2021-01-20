/*
 *
 * (C) COPYRIGHT 2016, 2019 ARM Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#include "mali_kbase.h"

#include "mali_kbase_regs_history_debugfs.h"

#if defined(CONFIG_DEBUG_FS) && !defined(CONFIG_MALI_NO_MALI)

#include <linux/debugfs.h>



#if KBASE_TRACE_ENABLE

#ifndef DEBUG_MESSAGE_SIZE
#define DEBUG_MESSAGE_SIZE 256
#endif

static const char *kbasep_trace_code_string[] = {
	/* IMPORTANT: USE OF SPECIAL #INCLUDE OF NON-STANDARD HEADER FILE
	 * THIS MUST BE USED AT THE START OF THE ARRAY */
#define KBASE_TRACE_CODE_MAKE_CODE(X) # X
//#include "tl/mali_kbase_trace_defs.h"
#undef  KBASE_TRACE_CODE_MAKE_CODE
};

#endif	// KBASE_TRACE_ENABLE

static int regs_history_size_get(void *data, u64 *val)
{
	struct kbase_io_history *const h = data;

	*val = h->size;

	return 0;
}

static int regs_history_size_set(void *data, u64 val)
{
	struct kbase_io_history *const h = data;

	return kbase_io_history_resize(h, (u16)val);
}


DEFINE_SIMPLE_ATTRIBUTE(regs_history_size_fops,
		regs_history_size_get,
		regs_history_size_set,
		"%llu\n");


#ifdef JIN_DUMP_TRACE
static char *jin_history_gpu_control(uint32_t reg, uint32_t value) {
	char *ret;

	/** switch ((uint32_t) offset & 0xFFF) { */
	switch (reg) {
		// jin: from "gpu/backend/mali_kbase_gpu_remap_jm.h"
		case CORE_FEATURES:
			ret = "CORE_FEATURES (RO)";	break;
		case JS_PRESENT:
			ret = "JS_PRESENT (RO)"; break;
		case LATEST_FLUSH:
			ret = "LATEST_FLUSH (RO)"; break;
		case PRFCNT_BASE_LO:
			ret = "PRFCNT_BASE_LO (RW)"; break;
		case PRFCNT_BASE_HI:
			ret = "PRFCNT_BASE_HI (RW)"; break;
		case PRFCNT_CONFIG:
			ret = "PRFCNT_CONFIG (RW)"; break;
		case PRFCNT_JM_EN:
			ret = "PRFCNT_JM_EN (RW)"; break;
		case PRFCNT_SHADER_EN:
			ret = "PRFCNT_SHADER_EN (RW)"; break;
		case PRFCNT_TILER_EN:
			ret = "PRFCNT_TILER_EN (RW)"; break;
		case PRFCNT_MMU_L2_EN:
			ret = "PRFCNT_MMU_L2_EN (RW)"; break;
		case JS0_FEATURES:
			ret = "JS0_FEATURES (RO)"; break;
		case JS1_FEATURES:
			ret = "JS1_FEATURES (RO)"; break;
		case JS2_FEATURES:
			ret = "JS2_FEATURES (RO)"; break;
		case JS3_FEATURES:
			ret = "JS3_FEATURES (RO)"; break;
		case JS4_FEATURES:
			ret = "JS4_FEATURES (RO)"; break;
		case JS5_FEATURES:
			ret = "JS5_FEATURES (RO)"; break;
		case JS6_FEATURES:
			ret = "JS6_FEATURES (RO)"; break;
		case JS7_FEATURES:
			ret = "JS7_FEATURES (RO)"; break;
		case JS8_FEATURES:
			ret = "JS8_FEATURES (RO)"; break;
		case JS9_FEATURES:
			ret = "JS9_FEATURES (RO)"; break;
		case JS10_FEATURES:
			ret = "JS10_FEATURES (RO)"; break;
		case JS11_FEATURES:
			ret = "JS11_FEATURES (RO)"; break;
		case JS12_FEATURES:
			ret = "JS12_FEATURES (RO)"; break;
		case JS13_FEATURES:
			ret = "JS13_FEATURES (RO)"; break;
		case JS14_FEATURES:
			ret = "JS14_FEATURES (RO)"; break;
		case JS15_FEATURES:
			ret = "JS15_FEATURES (RO)"; break;
		case JM_CONFIG:
			ret = "JM_CONFIG (RW)"; break;

			// jin: from "gpu/mali_kbase_gpu_remap.h"
		case GPU_ID:
			ret = "GPU_ID (RO)"; break;
		case L2_FEATURES:
			ret = "L2_FEATURES (RO)"; break;
		case TILER_FEATURES:
			ret = "TILER_FEATURES (RO)"; break;
		case MEM_FEATURES:
			ret = "MEM_FEATURES (RO)"; break;
		case MMU_FEATURES:
			ret = "MMU_FEATURES (RO)"; break;
		case AS_PRESENT:
			ret = "AS_PRESENT (RO)"; break;
		case GPU_IRQ_RAWSTAT:
			ret = "GPU_IRQ_RAWSTAT (RW)"; break;
		case GPU_IRQ_CLEAR:
			ret = "GPU_IRQ_CLEAR (WO)"; break;
		case GPU_IRQ_MASK:
			ret = "GPU_IRQ_MASK (RW)"; break;
		case GPU_IRQ_STATUS:
			ret = "GPU_IRQ_STATUS (RO)"; break;
		case GPU_COMMAND:
			switch (value) {
				case GPU_COMMAND_NOP:
					ret = "GPU_COMMAND (WO) | GPU_COMMAND_NOP"; break;
				case GPU_COMMAND_SOFT_RESET:
					ret = "GPU_COMMAND (WO) | GPU_COMMAND_SOFT_RESET"; break;
				case GPU_COMMAND_HARD_RESET:
					ret = "GPU_COMMAND (WO) | GPU_COMMAND_HARD_RESET"; break;
				case GPU_COMMAND_PRFCNT_CLEAR:
					ret = "GPU_COMMAND (WO) | GPU_COMMAND_PRFCNT_CLEAR"; break;
				case GPU_COMMAND_PRFCNT_SAMPLE:
					ret = "GPU_COMMAND (WO) | GPU_COMMAND_PRFCNT_SAMPLE"; break;
				case GPU_COMMAND_CYCLE_COUNT_START:
					ret = "GPU_COMMAND (WO) | GPU_COMMAND_CYCLE_COUNT_START"; break;
				case GPU_COMMAND_CYCLE_COUNT_STOP:
					ret = "GPU_COMMAND (WO) | GPU_COMMAND_CYCLE_COUNT_STOP"; break;
				case GPU_COMMAND_CLEAN_CACHES:
					ret = "GPU_COMMAND (WO) | GPU_COMMAND_CLEAN_CACHES"; break;
				case GPU_COMMAND_CLEAN_INV_CACHES:
					ret = "GPU_COMMAND (WO) | GPU_COMMAND_CLEAN_INV_CACHES"; break;
				case GPU_COMMAND_SET_PROTECTED_MODE:
					ret = "GPU_COMMAND (WO) | GPU_COMMAND_SET_PROTECTED_MODE"; break; 
				default:
					ret = "GPU_COMMAND (WO) | Unkown GPU_COMMAND"; 
			}
			break;
			/** ret = "GPU_COMMAND (WO)"; break; */
		case GPU_STATUS:	// 0x34 (RO)
			ret = "GPU_STATUS (RO)"; break;
			// GPU_DBGEN

		case GPU_FAULTSTATUS:
			ret = "GPU_FAULTSTATUS (RO)"; break;
		case GPU_FAULTADDRESS_LO:
			ret = "GPU_FAULTADDRESS_LO (RO)"; break;
		case GPU_FAULTADDRESS_HI:
			ret = "GPU_FAULTADDRESS_HI (RO)"; break;
//		case L2_CONFIG:
//			ret = "L2_CONFIG (RW)"; break;
			// GROUPS_L2_COHERENT
			// SUPER_L2_COHERENT

		case PWR_KEY:
			ret = "PWR_KEY (WO)"; break;
		case PWR_OVERRIDE0:
			ret = "PWR_OVERRIDE0 (RW)"; break;
		case PWR_OVERRIDE1:
			ret = "PWR_OVERRIDE1 (RW)"; break;

		case CYCLE_COUNT_LO:
			ret = "CYCLE_COUNT_LO (RO)"; break;
		case CYCLE_COUNT_HI:
			ret = "CYCLE_COUNT_HI (RO)"; break;
		case TIMESTAMP_LO:
			ret = "TIMESTAMP_LO (RO)"; break;
		case TIMESTAMP_HI:
			ret = "TIMESTAMP_HI (RO)"; break;

		case THREAD_MAX_THREADS:
			ret = "THREAD_MAX_THREAD (RO)"; break;
		case THREAD_MAX_WORKGROUP_SIZE:
			ret = "THREAD_MAX_WORKGROUP_SIZE (RO)"; break;
		case THREAD_MAX_BARRIER_SIZE:
			ret = "THREAD_MAX_BARRIER_SIZE (RO)"; break;
		case THREAD_FEATURES:
			ret = "THREAD_FEATURES (RO)"; break;
		case THREAD_TLS_ALLOC:
			ret = "THREAD_TLS_ALLOC (RO)"; break;

		case TEXTURE_FEATURES_0:
			ret = "TEXTURE_FEATURES_0 (RO)"; break;
		case TEXTURE_FEATURES_1:
			ret = "TEXTURE_FEATURES_1 (RO)"; break;
		case TEXTURE_FEATURES_2:
			ret = "TEXTURE_FEATURES_2 (RO)"; break;
		case TEXTURE_FEATURES_3:
			ret = "TEXTURE_FEATURES_3 (RO)"; break;

		case SHADER_PRESENT_LO:
			ret = "SHADER_PRESENT_LO (RO)"; break;
		case SHADER_PRESENT_HI:
			ret = "SHADER_PRESENT_HI (RO)"; break;

		case TILER_PRESENT_LO:
			ret = "TILER_PRESENT_LO (RO)"; break;
		case TILER_PRESENT_HI:
			ret = "TILER_PRESENT_HI (RO)"; break;

		case L2_PRESENT_LO:
			ret = "L2_PRESENT_LO (RO)"; break;
		case L2_PRESENT_HI:	
			ret = "L2_PRESENT_HI (RO)"; break;

		case STACK_PRESENT_LO:
			ret = "STACK_PRESENT_LO (RO)"; break;
		case STACK_PRESENT_HI:
			ret = "STACK_PRESENT_HI (RO)"; break;

		case SHADER_READY_LO:
			ret = "SHADER_READY_LO (RO)"; break;
		case SHADER_READY_HI:
			ret = "SHADER_READY_HI (RO)"; break;

		case TILER_READY_LO:
			ret = "TILER_READY_LO (RO)"; break;
		case TILER_READY_HI:	// 0x34 (RO)
			ret = "TILER_READY_HI (RO)"; break;

		case L2_READY_LO:
			ret = "L2_READY_LO (RO)"; break;
		case L2_READY_HI:
			ret = "L2_READY_HI (RO)"; break;

		case STACK_READY_LO:
			ret = "STACK_READY_LO (RO)"; break;
		case STACK_READY_HI:
			ret = "STACK_READY_HI (RO)"; break;

		case SHADER_PWRON_LO:
			ret = "SHADER_PWRON_LO (WO)"; break;
		case SHADER_PWRON_HI:
			ret = "SHADER_PWRON_HI (WO)"; break;

		case TILER_PWRON_LO:
			ret = "TILER_PWRON_LO (WO)"; break;
		case TILER_PWRON_HI:
			ret = "TILER_PWRON_HI (WO)"; break;

		case L2_PWRON_LO:
			ret = "L2_PWRON_LO (WO)"; break;
		case L2_PWRON_HI:
			ret = "L2_PWRON_HI (WO)"; break;

		case STACK_PWRON_LO:
			ret = "STACK_PWRON_LO (RO)"; break;
		case STACK_PWRON_HI:
			ret = "STACK_PWRON_HI (RO)"; break;

		case SHADER_PWROFF_LO:
			ret = "SHADER_PWROFF_LO (WO)"; break;
		case SHADER_PWROFF_HI:
			ret = "SHADER_PWROFF_HI (WO)"; break;

		case TILER_PWROFF_LO:
			ret = "TILER_PWROFF_LO (RO)"; break;
		case TILER_PWROFF_HI:
			ret = "TILER_PWROFF_HI (RO)"; break;

		case L2_PWROFF_LO:
			ret = "L2_PWROFF_LO (WO)"; break;
		case L2_PWROFF_HI:
			ret = "L2_PWROFF_HI (WO)"; break;

		case STACK_PWROFF_LO:
			ret = "STACK_PWROFF_LO (RO)"; break;
		case STACK_PWROFF_HI:
			ret = "STACK_PWROFF_HI (RO)"; break;

		case SHADER_PWRTRANS_LO:
			ret = "SHADER_PWRTRANS_LO (RO)"; break;
		case SHADER_PWRTRANS_HI:
			ret = "SHADER_PWRTRANS_HI (RO)"; break;

		case TILER_PWRTRANS_LO:
			ret = "TILER_PWRTRANS_LO (RO)"; break;
		case TILER_PWRTRANS_HI:
			ret = "TILER_PWRTRANS_HI (RO)"; break;

		case L2_PWRTRANS_LO:
			ret = "L2_PWRTRANS_LO (RO)"; break;
		case L2_PWRTRANS_HI:
			ret = "L2_PWRTRANS_HI (RO)"; break;

		case STACK_PWRTRANS_LO:
			ret = "STACK_PWRTRANS_LO (RO)"; break;
		case STACK_PWRTRANS_HI:
			ret = "STACK_PWRTRANS_LO (RO)"; break;

		case SHADER_PWRACTIVE_LO:
			ret = "SHADER_PWRACTIVE_LO (RO)"; break;
		case SHADER_PWRACTIVE_HI:
			ret = "SHADER_PWRACTIVE_HI (RO)"; break;

		case TILER_PWRACTIVE_LO:
			ret = "TILER_PWRACTIVE_LO (RO)"; break;
		case TILER_PWRACTIVE_HI:
			ret = "TILER_PWRACTIVE_HI (RO)"; break;

		case L2_PWRACTIVE_LO:
			ret = "L2_PWRACTIVE_LO (RO)"; break;
		case L2_PWRACTIVE_HI:
			ret = "L2_PWRACTIVE_HI (RO)"; break;

		case COHERENCY_FEATURES:
			ret = "COHERENCY_FEATURES (RO)"; break;
		case COHERENCY_ENABLE:
			ret = "COHERENCY_ENABLE (RW)"; break;

		case SHADER_CONFIG:
			ret = "SHADER_CONFIG (RW)"; break;
		case TILER_CONFIG:
			ret = "TILER_CONFIG (RW)"; break;
		case L2_MMU_CONFIG:
			ret = "L2_MMU_CONFIG (RW)"; break;

		default:
			ret = "[UNKOWN] not classified";
	}

	return ret;
}

static char *jin_history_job_control(uint32_t reg, char *job_cmd_reg, uint32_t value) {
	char *ret = "";
	uint32_t job_slot;

	switch (reg) {
		// jin: from "gpu/mali_kbase_gpu_remap.h"
		case JOB_IRQ_RAWSTAT:
			ret = "JOB_IRQ_RAWSTAT"; break;
		case JOB_IRQ_CLEAR:
			ret = "JOB_IRQ_CLEAR"; break;
		case JOB_IRQ_MASK:
			ret = "JOB_IRQ_MASK"; break;
		case JOB_IRQ_STATUS:
			ret = "JOB_IRQ_STATUS"; break;

			// jin: from "gpu/backend/mali_kbase_gpu_remap_jm.h"
		case JOB_IRQ_JS_STATE:
			ret = "JOB_IRQ_JS_STATE"; break;
		case JOB_IRQ_THROTTLE:
			ret = "JOB_IRQ_THROTTLE"; break;

		default:
			/**          uint32_t job_slot; */
			job_slot = reg & 0xF80;
			/** EE("job slot : 0x%08x", job_slot); */
			switch(job_slot) {
				/** switch(reg & 0xFF0) { */
				case JOB_SLOT0:
					ret = "JOB_SLOT0"; break;
				case JOB_SLOT1:
					ret = "JOB_SLOT1"; break;
				case JOB_SLOT2:
					ret = "JOB_SLOT2"; break;
				case JOB_SLOT3:
					ret = "JOB_SLOT3"; break;
				case JOB_SLOT4:
					ret = "JOB_SLOT4"; break;
				case JOB_SLOT5:
					ret = "JOB_SLOT5"; break;
				case JOB_SLOT6:
					ret = "JOB_SLOT6"; break;
				case JOB_SLOT7:
					ret = "JOB_SLOT7"; break;
				default:
					ret = "[UNKOWN] JOB_SLOT";
			}
			/** EE("job_cmd_reg = 0x%08x", reg & ~job_slot); */
			switch(reg & ~job_slot) {
				case JS_HEAD_LO:
					strcpy(job_cmd_reg, "JS_HEAD_LO (RO)"); break;
				case JS_HEAD_HI:
					strcpy(job_cmd_reg, "JS_HEAD_HI (RO)"); break;
				case JS_TAIL_LO:
					strcpy(job_cmd_reg, "JS_TAIL_LO (RO)"); break;
				case JS_TAIL_HI:
					strcpy(job_cmd_reg, "JS_TAIL_HI (RO)"); break;
				case JS_AFFINITY_LO:
					strcpy(job_cmd_reg, "JS_AFFINITY_LO (RO)"); break;
				case JS_AFFINITY_HI:
					strcpy(job_cmd_reg, "JS_AFFINITY_HI (RO)"); break;				
				case JS_CONFIG:
					strcpy(job_cmd_reg, "JS_CONFIG (RO)"); break;
				case JS_XAFFINITY:
					strcpy(job_cmd_reg, "JS_XAFFINITY"); break;

				case JS_COMMAND:
					switch (value) {
						case JS_COMMAND_NOP:	
							strcpy(job_cmd_reg, "JS_COMMAND (WO) | JS_COMMAND_NOP"); break;
						case JS_COMMAND_START:	
							strcpy(job_cmd_reg, "JS_COMMAND (WO) | JS_COMMAND_START"); break;
						case JS_COMMAND_SOFT_STOP:	
							strcpy(job_cmd_reg, "JS_COMMAND (WO) | JS_COMMAND_SOFT_STOP"); break;
						case JS_COMMAND_HARD_STOP:	
							strcpy(job_cmd_reg, "JS_COMMAND (WO) | JS_COMMAND_HARD_STOP"); break;
						case JS_COMMAND_SOFT_STOP_0:	
							strcpy(job_cmd_reg, "JS_COMMAND (WO) | JS_COMMAND_SOFT_STOP_0"); break;
						case JS_COMMAND_HARD_STOP_0:	
							strcpy(job_cmd_reg, "JS_COMMAND (WO) | JS_COMMAND_HARD_STOP_0"); break;
						case JS_COMMAND_SOFT_STOP_1:	
							strcpy(job_cmd_reg, "JS_COMMAND (WO) | JS_COMMAND_SOFT_STOP_1"); break;
						case JS_COMMAND_HARD_STOP_1:	
							strcpy(job_cmd_reg, "JS_COMMAND (WO) | JS_COMMAND_HARD_STOP_1"); break;
						default:
							strcpy(job_cmd_reg, "JS_COMMAND (WO) | Unknown JS_COMMAND");
					}
					break;
					/** strcpy(job_cmd_reg, "JS_COMMAND (WO)"); break; */
				case JS_STATUS:
					strcpy(job_cmd_reg, "JS_STATUS (RO)"); break;

				case JS_HEAD_NEXT_LO:
					strcpy(job_cmd_reg, "JS_HEAD_NEXT_LO (RW)"); break;
				case JS_HEAD_NEXT_HI:
					strcpy(job_cmd_reg, "JS_HEAD_NEXT_HI (RW)"); break;

				case JS_AFFINITY_NEXT_LO:
					strcpy(job_cmd_reg, "JS_AFFINITY_NEXT_LO (RW)"); break;
				case JS_AFFINITY_NEXT_HI:
					strcpy(job_cmd_reg, "JS_AFFINITY_NEXT_HI (RW)"); break;
				case JS_CONFIG_NEXT:
					strcpy(job_cmd_reg, "JS_CONFIG_NEXT (RW)"); break;
				case JS_XAFFINITY_NEXT:
					strcpy(job_cmd_reg, "JS_XAFFINITY_NEXT (RW)"); break;

				case JS_COMMAND_NEXT:
					switch (value) {
						case JS_COMMAND_NOP:	
							strcpy(job_cmd_reg, "JS_COMMAND_NEXT (WO) | JS_COMMAND_NOP"); break;
						case JS_COMMAND_START:	
							strcpy(job_cmd_reg, "JS_COMMAND_NEXT (WO) | JS_COMMAND_START"); break;
						case JS_COMMAND_SOFT_STOP:	
							strcpy(job_cmd_reg, "JS_COMMAND_NEXT (WO) | JS_COMMAND_SOFT_STOP"); break;
						case JS_COMMAND_HARD_STOP:	
							strcpy(job_cmd_reg, "JS_COMMAND_NEXT (WO) | JS_COMMAND_HARD_STOP"); break;
						case JS_COMMAND_SOFT_STOP_0:	
							strcpy(job_cmd_reg, "JS_COMMAND_NEXT (WO) | JS_COMMAND_SOFT_STOP_0"); break;
						case JS_COMMAND_HARD_STOP_0:	
							strcpy(job_cmd_reg, "JS_COMMAND_NEXT (WO) | JS_COMMAND_HARD_STOP_0"); break;
						case JS_COMMAND_SOFT_STOP_1:	
							strcpy(job_cmd_reg, "JS_COMMAND_NEXT (WO) | JS_COMMAND_SOFT_STOP_1"); break;
						case JS_COMMAND_HARD_STOP_1:	
							strcpy(job_cmd_reg, "JS_COMMAND_NEXT (WO) | JS_COMMAND_HARD_STOP_1"); break;
						default:
							strcpy(job_cmd_reg, "JS_COMMAND_NEXT (WO) | Unknown JS_COMMAND"); 
					}
					break;
					/** strcpy(job_cmd_reg, "JS_COMMAND_NEXT (RW)"); break; */
				case JS_FLUSH_ID_NEXT:
					strcpy(job_cmd_reg, "JS_FLUSH_ID_NEXT (RW)"); break;

				default :
					strcpy(job_cmd_reg, "[UNKOWN_JOB_CMD_REG]");
			}
		}
	return ret;
}

static char *jin_history_memory_management(uint32_t reg, char *mmu_cmd_reg, uint32_t value) {
	char *ret = "";
	uint32_t mmu_slot;

	switch (reg) {
		case MMU_IRQ_RAWSTAT:
			ret = "MMU_IRQ_RAWSTAT (RW)"; break;
		case MMU_IRQ_CLEAR:
			ret = "MMU_IRQ_CLEAR (WO)"; break;
		case MMU_IRQ_MASK:
			ret = "MMU_IRQ_MASK (RW)"; break;
		case MMU_IRQ_STATUS:
			ret = "MMU_IRQ_STATUS (RO)"; break;
		default:
			mmu_slot = reg & 0xFC0;
			switch (mmu_slot) {
				case MMU_AS0:
					ret = "MMU_AS0"; break;
				case MMU_AS1:
					ret = "MMU_AS1"; break;
				case MMU_AS2:
					ret = "MMU_AS2"; break;
				case MMU_AS3:
					ret = "MMU_AS3"; break;
				case MMU_AS4:
					ret = "MMU_AS4"; break;
				case MMU_AS5:
					ret = "MMU_AS5"; break;
				case MMU_AS6:
					ret = "MMU_AS6"; break;
				case MMU_AS7:
					ret = "MMU_AS7"; break;
				case MMU_AS8:
					ret = "MMU_AS8"; break;
				case MMU_AS9:
					ret = "MMU_AS9"; break;
				case MMU_AS10:
					ret = "MMU_AS10"; break;
				case MMU_AS11:
					ret = "MMU_AS11"; break;
				case MMU_AS12:
					ret = "MMU_AS12"; break;
				case MMU_AS13:
					ret = "MMU_AS13"; break;
				case MMU_AS14:
					ret = "MMU_AS14"; break;
				case MMU_AS15:
					ret = "MMU_AS15"; break;
				default:
					ret = "[UNKOWN] not classified";
			}

			switch (reg & ~mmu_slot) {
				case AS_TRANSTAB_LO:
					strcpy(mmu_cmd_reg, "AS_TRANSTAB_LO (RW)"); break;
				case AS_TRANSTAB_HI:
					strcpy(mmu_cmd_reg, "AS_TRANSTAB_HI (RW)"); break;
				case AS_MEMATTR_LO:
					strcpy(mmu_cmd_reg, "AS_MEMATTR_LO (RW)"); break;
				case AS_MEMATTR_HI:
					strcpy(mmu_cmd_reg, "AS_MEMATTR_HI (RW)"); break;
				case AS_LOCKADDR_LO:
					strcpy(mmu_cmd_reg, "AS_LOCKADDR_LO (RW)"); break;
				case AS_LOCKADDR_HI:
					strcpy(mmu_cmd_reg, "AS_LOCKADDR_HI (RW)"); break;
				case AS_COMMAND:
					switch(value) {
						case AS_COMMAND_NOP:
							strcpy(mmu_cmd_reg, "AS_COMMAND (WO) | AS_COMMAND_NOP"); break;
						case AS_COMMAND_UPDATE:
							strcpy(mmu_cmd_reg, "AS_COMMAND (WO) | AS_COMMAND_UPDATE"); break;
						case AS_COMMAND_LOCK:
							strcpy(mmu_cmd_reg, "AS_COMMAND (WO) | AS_COMMAND_LOCK"); break;
						case AS_COMMAND_UNLOCK:
							strcpy(mmu_cmd_reg, "AS_COMMAND (WO) | AS_COMMAND_UNLOCK"); break;
							/** case AS_COMMAND_FLUSH: */	// deprecated - only for use with T60X
							/** strcpy(mmu_cmd_reg, "AS_COMMAND (WO) | AS_COMMAND_FLUSH"); break; */
						case AS_COMMAND_FLUSH_PT:
							strcpy(mmu_cmd_reg, "AS_COMMAND (WO) | AS_COMMAND_FLUSH_PT"); break;
						case AS_COMMAND_FLUSH_MEM:
							strcpy(mmu_cmd_reg, "AS_COMMAND (WO) | AS_COMMAND_FLUSH_MEM"); break;
						default :
							strcpy(mmu_cmd_reg, "AS_COMMAND (WO) | Unkonwn AS_COMMAND"); 
					}
					break;
					/** strcpy(mmu_cmd_reg, "AS_COMMAND (WO)"); break; */
				case AS_FAULTSTATUS:
					strcpy(mmu_cmd_reg, "AS_FAULTSTATUS (RO)"); break;
				case AS_FAULTADDRESS_LO:
					strcpy(mmu_cmd_reg, "AS_FAULTADDRESS_LO (RO)"); break;
				case AS_FAULTADDRESS_HI:
					strcpy(mmu_cmd_reg, "AS_FAULTADDRESS_HI (HI)"); break;
				case AS_STATUS:
					strcpy(mmu_cmd_reg, "AS_STATUS (RO)"); break;

				case AS_TRANSCFG_LO:
					strcpy(mmu_cmd_reg, "AS_TRANSCFG_LO"); break;
				case AS_TRANSCFG_HI:
					strcpy(mmu_cmd_reg, "AS_TRANSCFG_HI"); break;
				case AS_FAULTEXTRA_LO:
					strcpy(mmu_cmd_reg, "AS_FAULTEXTRA_LO"); break;
				case AS_FAULTEXTRA_HI:
					strcpy(mmu_cmd_reg, "AS_FAULTEXTRA_HI"); break;
				default:
					strcpy(mmu_cmd_reg, "[UNKNOWN MMU_CMD_REG]");
			}
	}
	return ret;
}



static void regs_history_with_info(struct seq_file *sfile, const struct kbase_io_history *h)
{
	u16 i;
	size_t iters;

	iters = (h->size > h->count) ? h->count : h->size;
	for (i = 0; i < iters; ++i) {
		struct kbase_io_access *io =
			&h->buf[(h->count - iters + i) % h->size];

		char const access = (io->addr & 1) ? 'W' : 'R';
		uintptr_t offset = io->addr & ~0x1;
		char *type = "";
		char *tmp = "";
		char cmd_reg[50] = "";

		seq_printf(sfile, "%5llu\t%5s\t%6d\t%4d: %c ", io->timestamp, "IO", io->thread_id, i, access);
		switch ((offset >> 12) & 0xF) {
			case 0x0:
				type = "GPU_CTRL";
				tmp = jin_history_gpu_control(offset & 0xFFF, io->value);
				seq_printf(sfile, "reg 0x%08x val %08x [ %10s | %s ]\n", (uint32_t)(io->addr & ~0x1), io->value, type, tmp); 
   /**              seq_printf(sfile, "%5llu\tIO\t%d\t%6i: %c: reg 0x%08x val %08x [ %10s | %s ]\n", io->timestamp, io->thread_id, i, */
						/** access, (uint32_t)(io->addr & ~0x1), io->value, type, tmp); */
				break;
			case 0x1:
				type = "JOB_CTRL";
				tmp = jin_history_job_control(offset & 0xFFF, cmd_reg, io->value);
				seq_printf(sfile, "reg 0x%08x val %08x [ %10s | %10s | %s]\n", (uint32_t)(io->addr & ~0x1), io->value, type, tmp, cmd_reg);
   /**              seq_printf(sfile, "%5llu\tIO\t%d%6i: %c: reg 0x%08x val %08x [ %10s | %10s | %s]\n", io->timestamp, io->thread_id, i, */
						/** access, (uint32_t)(io->addr & ~0x1), io->value, type, tmp, cmd_reg); */
				break;
			case 0x2:
				type = "MEM_MGMT";
				tmp = jin_history_memory_management(offset & 0xFFF, cmd_reg, io->value);
				seq_printf(sfile, "reg 0x%08x val %08x [ %10s | %10s | %s]\n", (uint32_t)(io->addr & ~0x1), io->value, type, tmp, cmd_reg);
   /**              seq_printf(sfile, "%5llu\tIO\t%d\t%6i: %c: reg 0x%08x val %08x [ %10s | %10s | %s]\n", io->timestamp, io->thread_id, i, */
						/** access, (uint32_t)(io->addr & ~0x1), io->value, type, tmp, cmd_reg); */
				break;
			default:
				type = "UNKNOWN";
				tmp = "UNKNWON";
		}
	}
}
#endif	// JIN_DUMP_TRACE

/**
 * regs_history_show - show callback for the register access history file.
 *
 * @sfile: The debugfs entry
 * @data: Data associated with the entry
 *
 * This function is called to dump all recent accesses to the GPU registers.
 *
 * @return 0 if successfully prints data in debugfs entry file, failure
 * otherwise
 */
static int regs_history_show(struct seq_file *sfile, void *data)
{
	struct kbase_io_history *const h = sfile->private;
	size_t iters;
	unsigned long flags;
#ifndef JIN_DUMP_TRACE
	u16 i;
#endif

	if (!h->enabled) {
		seq_puts(sfile, "The register access history is disabled\n");
		goto out;
	}

	spin_lock_irqsave(&h->lock, flags);

#ifdef JIN_DUMP_TRACE
	iters = (h->size > h->count) ? h->count : h->size;
   /**  seq_printf(sfile, "Last %zu register accesses of %zu total:\n", iters, */
			/** h->count); */
	regs_history_with_info(sfile, h);
#else	// jin: original implementation
	iters = (h->size > h->count) ? h->count : h->size;
	seq_printf(sfile, "Last %zu register accesses of %zu total:\n", iters,
			h->count);

	for (i = 0; i < iters; ++i) {
		struct kbase_io_access *io =
			&h->buf[(h->count - iters + i) % h->size];

		char const access = (io->addr & 1) ? 'w' : 'r';

			 seq_printf(sfile, "%6i: %c: reg 0x%016lx val %08x\n", i, access,
		(unsigned long)(io->addr & ~0x1), io->value);
   /**      seq_printf(sfile, "[%5llu] %6i: %c: reg 0x%016lx val %08x\n", io->timestamp, i, access, */
				/** (unsigned long)(io->addr & ~0x1), io->value); */
	}
#endif

	spin_unlock_irqrestore(&h->lock, flags);

out:
	return 0;
}


/**
 * regs_history_open - open operation for regs_history debugfs file
 *
 * @in: &struct inode pointer
 * @file: &struct file pointer
 *
 * @return file descriptor
 */
static int regs_history_open(struct inode *in, struct file *file)
{
	return single_open(file, &regs_history_show, in->i_private);
}


static const struct file_operations regs_history_fops = {
	.owner = THIS_MODULE,
	.open = &regs_history_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

#ifdef JIN_DUMP_TRACE

static void rbuf_dump_msg(struct seq_file *sfile, struct kbase_trace *trace_msg)
{
	char buffer[DEBUG_MESSAGE_SIZE];
	int len = DEBUG_MESSAGE_SIZE;
	s32 written = 0;

#ifdef JIN_DUMP_TRACE
	/** written += MAX(snprintf(buffer + written, MAX(len - written, 0), "[%5llu][RBUF] ", trace_msg->ts), 0); */
	written += MAX(snprintf(buffer + written, MAX(len - written, 0), "%5llu\t%5s\t%6d", trace_msg->ts, "RBUF", trace_msg->thread_id), 0);

	/* Initial part of message */
   /**  written += MAX(snprintf(buffer + written, MAX(len - written, 0), "%s, ctx:%p, tid:%d, core:%d  ", kbasep_trace_code_string[trace_msg->code], trace_msg->ctx, trace_msg->thread_id, trace_msg->cpu), 0); */
	// jin: thread_id, cpu
	/** written += MAX(snprintf(buffer + written, MAX(len - written, 0), "%25s, tid:%d, core:%d  ", kbasep_trace_code_string[trace_msg->code], trace_msg->thread_id, trace_msg->cpu), 0); */
	// jin: no thread_id, cpu
	written += MAX(snprintf(buffer + written, MAX(len - written, 0), "\t%30s, ", kbasep_trace_code_string[trace_msg->code]), 0);
#else

	/* Initial part of message */
	written += MAX(snprintf(buffer + written, MAX(len - written, 0), "%d.%.6d,%d,%d,%s,%p,", (int)trace_msg->timestamp.tv_sec, (int)(trace_msg->timestamp.tv_nsec / 1000), trace_msg->thread_id, trace_msg->cpu, kbasep_trace_code_string[trace_msg->code], trace_msg->ctx), 0);
#endif

	if (trace_msg->katom)
		written += MAX(snprintf(buffer + written, MAX(len - written, 0), "atom %d (ud: 0x%llx 0x%llx)", trace_msg->atom_number, trace_msg->atom_udata[0], trace_msg->atom_udata[1]), 0);

	written += MAX(snprintf(buffer + written, MAX(len - written, 0), ",%.8llx,", trace_msg->gpu_addr), 0);

	/* NOTE: Could add function callbacks to handle different message types */
	/* Jobslot present */
	if (trace_msg->flags & KBASE_TRACE_FLAG_JOBSLOT)
		written += MAX(snprintf(buffer + written, MAX(len - written, 0), "%d", trace_msg->jobslot), 0);
	else
		written += MAX(snprintf(buffer + written, MAX(len - written, 0), ","), 0);

	/* Refcount present */
	if (trace_msg->flags & KBASE_TRACE_FLAG_REFCOUNT)
		written += MAX(snprintf(buffer + written, MAX(len - written, 0), "%d", trace_msg->refcount), 0);

	written += MAX(snprintf(buffer + written, MAX(len - written, 0), ","), 0);

	/* Rest of message */
	written += MAX(snprintf(buffer + written, MAX(len - written, 0), "0x%.8lx", trace_msg->info_val), 0);

#ifdef JIN_DUMP_TRACE
	/** written += MAX(snprintf(buffer + written, MAX(len - written, 0), " [tid:%d, core:%d] ", trace_msg->thread_id, trace_msg->cpu), 0); */
#endif

	seq_printf(sfile, "%s\n", buffer); 
	
}

static int rbuf_history_show(struct seq_file *sfile, void *data)
{
	struct kbase_device *const kbdev =  sfile->private;

	unsigned long flags;
	u32 start;
	u32 end;

	spin_lock_irqsave(&kbdev->trace_lock, flags);
	start = kbdev->trace_first_out;
	end = kbdev->trace_next_in;

	while (start != end) {
		struct kbase_trace *trace_msg = &kbdev->trace_rbuf[start];

		rbuf_dump_msg(sfile, trace_msg);
		/** kbasep_trace_format_msg(trace_msg, buffer, DEBUG_MESSAGE_SIZE); */

		start = (start + 1) & KBASE_TRACE_MASK;
	}
	spin_unlock_irqrestore(&kbdev->trace_lock, flags);

	/** KBASE_TRACE_CLEAR(kbdev); */
//out:
	return 0;
}

static int rbuf_history_open(struct inode *in, struct file *file)
{
	return single_open(file, &rbuf_history_show, in->i_private);
}

static const struct file_operations rbuf_history_fops = {
	.owner = THIS_MODULE,
	.open = &rbuf_history_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

#endif	// JIN_DUMP_TRACE

void kbasep_regs_history_debugfs_init(struct kbase_device *kbdev)
{
	debugfs_create_bool("regs_history_enabled", S_IRUGO | S_IWUSR,
			kbdev->mali_debugfs_directory,
			&kbdev->io_history.enabled);
	debugfs_create_file("regs_history_size", S_IRUGO | S_IWUSR,
			kbdev->mali_debugfs_directory,
			&kbdev->io_history, &regs_history_size_fops);
	debugfs_create_file("regs_history", S_IRUGO,
			kbdev->mali_debugfs_directory, &kbdev->io_history,
			&regs_history_fops);
#ifdef JIN_DUMP_TRACE
	

#ifdef KBASE_TRACE_ENABLE
	debugfs_create_file("rbuf_history", S_IRUGO,
			kbdev->mali_debugfs_directory, kbdev,
			&rbuf_history_fops);
#endif	// KBASE_TRACE_ENABLE
#endif	// JIN_DUMP_TRACE

}


#endif /* CONFIG_DEBUG_FS */
