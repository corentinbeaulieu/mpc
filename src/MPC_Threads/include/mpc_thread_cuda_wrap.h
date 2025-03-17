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

#ifndef SCTK_CUDA_WRAP_H
#define SCTK_CUDA_WRAP_H
#include <mpc_common_debug.h>

/** Global FATAL CUDA routines to catch weak symbol call */
#define sctk_cuFatal()                                                                     \
	do {                                                                               \
		mpc_common_debug_error("You reached fake CUDA %s() call inside MPC", __func__);        \
		mpc_common_debug_error("Please ensure a valid CUDA driver library is present and you " \
		           "provided --cuda option to mpc_* compilers");                   \
		mpc_common_debug_abort();                                                              \
	} while(0)

#ifdef MPC_USE_CUDA
#include <cuda.h>
#endif

#endif
