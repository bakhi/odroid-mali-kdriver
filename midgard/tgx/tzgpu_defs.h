/* written by jin to configure TZGPU with Mali Bifrost */

#ifndef _TZGPU_DEFS_H_
#define _TZGPU_DEFS_H_

#include "jin/log.h"
#include "jin/measure.h"

#ifndef CONFIG_TGX
#define CONFIG_TGX
#endif

// comment this on to get trace related information
#ifndef JIN
#define JIN
#endif

/* #ifdef CONFIG_TGX */
// #include "tgx/tgx_defs.h"
/* #endif */

#define JIN_MEASURE_CONTEXT_EXEC_TIME	1
#define JIN_MEASURE_PM_TIME				2
#define JIN_MEASURE_JOB_TIME			3
#define JIN_CONTEXT_TIME				4

// comment out this: no msg / no measurement ==> orig driver

/* #define JIN_MEASURE_TIME	JIN_MEASURE_CONTEXT_EXEC_TIME */
// #define JIN_MEASURE_TIME	JIN_MEASURE_PM_TIME
// #define JIN_MEASURE_TIME	JIN_MEASURE_JOB_TIME
// #define JIN_MEASURE_TIME	JIN_CONTEXT_TIME

/******************/
#ifdef JIN

	#ifndef CONFIG_MALI_GATOR_SUPPORT
	// #define CONFIG_MALI_GATOR_SUPPORT
	#endif

	/**** DEBUG FS ****/
	#ifndef DEBUG_FS
	#define DEBUG_FS
	#endif

	// jin: for IO reg trace
	#ifndef CONFIG_DEBUG_FS
	#define CONFIG_DEBUG_FS
	#endif

	// jin: should define it at "backend/gpu/mali_kbase_pm_defs.h"
   /*  #ifndef CONFIG_MALI_DEBUG */
	// #define CONFIG_MALI_DEBUG
	/* #endif */

	#ifndef CONFIG_MALI_MIDGARD_ENABLE_TRACE
	// jin: it defines KBASE_TRACE_ENABLE
	// Enables tracing in kbase. Trace log available through the "mali_trace" debugfs file
	#define CONFIG_MALI_DEBUG // It works when the CONFIG_DEBUG_FS is enabled
	#define CONFIG_MALI_MIDGARD_ENABLE_TRACE
	#endif

	// jin: [custom] dump trace
	#ifndef JIN_DUMP_TRACE
	#define JIN_DUMP_TRACE
	#endif


	// jin: [custom] measure interrupt latency
	/* #ifndef JIN_MEASURE_IRQ */
	// #define JIN_MEASURE_IRQ
	/* #endif */

	/* dump io history with DEBUG_FS */
	/* CONFIG_DEBUG_FS should be defined in advance */
	#ifndef JIN_IO_HISTORY
	#define JIN_IO_HISTORY
	#endif

	// jin: does not work 
	// This may be used for ftrace.
	// If kernel is not compiled with relevant flags like CONFIG_FTRACE, it may not work.
	// If it is not defined, it uses driver internal trace buffer.
	// In such case, use KBASE_DUMP_TRACE to dump collected trace
	/* #ifndef CONFIG_MALI_SYSTEM_TRACE */
	// #define CONFIG_MALI_SYSTEM_TRACE
	/* #endif */
	
	// jin: does not work for hikey linux kenrel
	/* #ifndef CONFIG_GPU_TRACEPOINTS */
	// #define CONFIG_GPU_TRACEPOINTS
	/* #endif */

#endif // End of JIN
/*************************************************************************/

#endif // End of _TZGPU_DEFS_H_
