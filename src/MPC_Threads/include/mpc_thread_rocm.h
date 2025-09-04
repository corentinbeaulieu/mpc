/* ############################# MPC License ############################## */
/* # Wed Nov 19 15:19:19 CET 2008                                         # */
/* # Copyright or (C) or Copr. Commissariat a l'Energie Atomique          # */
/* #                                                                      # */
/* # IDDN.FR.001.230040.000.S.P.2007.000.10000                            # */
/* # This file is part of the MPC Runtime.                                # */
/* #                                                                      # */
/* # This software is governed by the CeCILL-C license under French law   # */
/* # and abiding by the rules of distribution of free software.  You can  # */
/* # use, modify and/ or redistribute the software under the terms of     # */
/* # the CeCILL-C license as circulated by CEA, CNRS and INRIA at the     # */
/* # following URL http://www.cecill.info.                                # */
/* #                                                                      # */
/* # The fact that you are presently reading this means that you have     # */
/* # had knowledge of the CeCILL-C license and that you accept its        # */
/* # terms.                                                               # */
/* #                                                                      # */
/* # Authors:                                                             # */
/* #   - MARIE Nicolas nicolas.marie@uvsq.fr                              # */
/* #                                                                      # */
/* #                                                                      # */
/* ######################################################################## */

#ifndef SCTK_ROCM_H
#define SCTK_ROCM_H

#ifdef MPC_USE_ROCM
#include <hip/hip_runtime.h>
#include <mpc_common_debug.h>

	/** in debug mode, check all HIP APIs return codes */
#ifndef NDEBUG
		#define safe_hip(u) \
				assume_m(((u) == hipSuccess), "HIP call failed with value %d: (%s)", u, hipGetErrorString(u))
#else
		#define safe_hip(u) u
#endif

	/**
	 * The HIP context structure as handled by MPC.
	 *
	 * This structure is part of TLS bundle handled internally by thread context.
	 */
	typedef struct hip_ctx_s
	{
		char     pushed;  /**< Set to 1 when the ctx is currently pushed */
		int      cpu_id;  /**< Register the cpu_id associated to the HIP ctx */
		hipCtx_t context; /**< THE HIP ctx */
	} hip_ctx_t;

	/* HIP libs init */
	int sctk_accl_hip_init();

	/** create a new HIP context for the current thread */
	void sctk_accl_hip_init_context();

	/** Push the HIP context of the current thread on the elected GPU */
	int sctk_accl_hip_push_context();

	/** Remove the current HIP context from the GPU */
	int sctk_accl_hip_pop_context();

#endif

#endif
