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
 * Weak symbol to let MPC know the cuInit() symbol without linking to it at compile time.
 * The real symbol will be found at run time.
 * @param[in] flag not used in this wrapper
 */
#pragma weak sctk_cuInit
CUresult sctk_cuInit(unsigned int flag) { return cuInit(flag); }

/**
 * Weak symbol to let MPC know the cuCtxCreate() symbol without linking to it at compile time.
 * The real symbol will be found at run time.
 * @param[in] flag not used in this wrapper
 */
#pragma weak sctk_cuCtxCreate
CUresult sctk_cuCtxCreate(CUcontext *c, unsigned int f, CUdevice d) {
  return cuCtxCreate(c, f, d);
}

#pragma weak sctk_cuCtxCreate_v3
CUresult sctk_cuCtxCreate_v3(CUcontext* c, CUexecAffinityParam* paramsArray, int  numParams, unsigned int  f, CUdevice d) {
  return cuCtxCreate_v3(c, paramsArray, numParams, f, d);
}

/**
 * Weak symbol to let MPC know the cuCtxPopCurrent() symbol without linking to it at compile time.
 * @param[in] flag not used in this wrapper
 */
#pragma weak sctk_cuCtxPopCurrent
CUresult sctk_cuCtxPopCurrent(CUcontext *c) { return cuCtxPopCurrent(c); }
/**
 * Weak symbol to let MPC know the ctxPushCurrent() symbol without linking to it at compile time.
 * @param[in] flag not used in this wrapper
 */

#pragma weak sctk_cuCtxPushCurrent
CUresult sctk_cuCtxPushCurrent(CUcontext c) { return cuCtxPushCurrent(c); }
/**
 * Weak symbol to let MPC know the cuDeviceGetByPCIBusId() symbol without linking to it at compile time.
 * @param[in] flag not used in this wrapper
 */

#pragma weak sctk_cuDeviceGetByPCIBusId
CUresult sctk_cuDeviceGetByPCIBusId(CUdevice *d, const char *b) {
  return cuDeviceGetByPCIBusId(d, b);
}
#pragma weak sctk_cuCtxDestroy
CUresult sctk_cuCtxDestroy(CUcontext c) {
  return cuCtxDestroy(c);
}
#pragma weak sctk_cuDevicePrimaryCtxRelease
CUresult sctk_cuDevicePrimaryCtxRelease(CUdevice d) {
  return cuDevicePrimaryCtxRelease(d);
}
#endif
