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
/* #   - PERACHE Marc marc.perache@cea.fr                                 # */
/* #   - ADAM Julien adamj@paratools.com                                  # */
/* #   - BOUHROUR Stephane stephane.bouhrour@uvsq.fr                      # */
/* #                                                                      # */
/* ######################################################################## */

#include <mpc_thread_cuda_wrap.h>
#include <mpc_common_debug.h>


#ifdef MPC_USE_CUDA

/**
 * Weak symbols to let MPC know symbols without linking to it at compile time.
 * The real symbol will be found at run time (in cuda_lib/cuda_lib.c).
 * Functions core removed for compatibility as gcc-14 consider 
 * implicit functions declaration as errors.
 */

#pragma weak sctk_cuInit
CUresult sctk_cuInit(unsigned int flag) {
	UNUSED(flag);
	assert(0);
	return 0;
}

#pragma weak sctk_cuCtxCreate
CUresult sctk_cuCtxCreate(CUcontext *c, unsigned int f, CUdevice d) {
	UNUSED(c);
	UNUSED(f);
	UNUSED(d);
	assert(0);
	return 0;
}

#pragma weak sctk_cuCtxCreate_v3
CUresult sctk_cuCtxCreate_v3(CUcontext* c, CUexecAffinityParam* paramsArray, int  numParams, unsigned int f, CUdevice d) {
	UNUSED(c);
	UNUSED(paramsArray);
	UNUSED(numParams);
	UNUSED(f);
	UNUSED(d);
	assert(0);
	return 0;
}

#pragma weak sctk_cuCtxPopCurrent
CUresult sctk_cuCtxPopCurrent(CUcontext *c) {
	UNUSED(c);
	assert(0);
	return 0;
}

#pragma weak sctk_cuCtxPushCurrent
CUresult sctk_cuCtxPushCurrent(CUcontext c) {
	UNUSED(c);
	assert(0);
	return 0;
}

#pragma weak sctk_cuDevicePrimaryCtxGetState
CUresult sctk_cuDevicePrimaryCtxGetState(CUdevice dev, unsigned int* flags,
                                         int* active){
	UNUSED(dev);
	UNUSED(flags);
	UNUSED(active);
	assert(0);
	return 0;
}

#pragma weak sctk_cuDeviceGet
CUresult sctk_cuDeviceGet(CUdevice* device, int ordinal) {
	UNUSED(device);
	UNUSED(ordinal);
	assert(0);
	return 0;
}

#pragma weak sctk_cuDeviceGetByPCIBusId
CUresult sctk_cuDeviceGetByPCIBusId(CUdevice *d, const char *b) {
	UNUSED(d);
	UNUSED(b);
	assert(0);
	return 0;
}

#pragma weak sctk_cuCtxDestroy
CUresult sctk_cuCtxDestroy(CUcontext c) {
	UNUSED(c);
	assert(0);
	return 0;
}

#pragma weak sctk_cuDevicePrimaryCtxRelease
CUresult sctk_cuDevicePrimaryCtxRelease(CUdevice dev) {
	UNUSED(dev);
	assert(0);
	return 0;
}

#endif
