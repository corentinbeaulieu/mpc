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

#include <mpc_config.h>
#include <mpc_thread_accelerator.h>
#include <sctk_alloc.h>
#include <mpc_common_debug.h>
#include <mpc_topology_device.h>
#include <mpc_common_spinlock.h>
#include <mpc_topology.h>
#include <mpc_common_flags.h>
#include <mpc_thread_cuda.h>

#define MPC_MODULE "Threads/Accel/CUDA"

extern __thread void *sctk_cuda_ctx;


static int sctk_accl_cuda_check_devices()
{
	int num_devices = sctk_accl_get_nb_cuda_devices();

	// if CUDA support is loaded but the current configuration does not
	// provide a GPU: stop
	if (num_devices <= 0)
	{
		mpc_common_nodebug("CUDA: support enabled but no GPU found !");
		return 0;
	}

	// sanity check
	int nb_check;
	safe_cudadv(cuDeviceGetCount(&nb_check));
	assert(num_devices == nb_check);

	return num_devices;
}

/**
 * Retrieve the best device to select for the thread cpu_id.
 *
 * This function mainly uses two sctk_device_topology calls:
 *   - <b>sctk_device_matrix_get_list_closest_from_pu(...)</b>:
 *     Returns a list of devices, each device being at the same distance from
 *     the PU than others
 *   - <b>sctk_device_attach_freest_device_from</b>: From the previously
 *     generated list, selects the GPU with the smallest number of attached
 *     resources to it (ie. PU). This call must be thread-safe.
 *
 * @param[in] cpu_id the CPU for whom we pick up a device
 * @return           the device_id for the elected device
 */
static int sctk_accl_cuda_get_closest_device(int cpu_id)
{
	int num_devices;

	if ((num_devices = sctk_accl_cuda_check_devices()) == 0)
	{
		return 0;
	}

	// else, we try to find the closest device for the current thread
	// to use, num_devices contains the number of minimum distance device
	mpc_topology_device_t **closest_devices = NULL;
	closest_devices = mpc_topology_device_matrix_get_list_closest_from_pu(cpu_id, "cuda-enabled-card*", &num_devices);
	// once the list is filtered with the nearest ones,
	// we need to elected the one with the minimum number of attached resources
	mpc_topology_device_t *elected = mpc_topology_device_attach_freest_device_from(closest_devices, num_devices);
	assert(elected != NULL);
	int nearest_id = elected->device_id;

	mpc_common_nodebug("CUDA: (DETECTION) elected device %d", nearest_id);

	sctk_free(closest_devices);
	return nearest_id;
}

/**
 * Initialize a CUDA context for the calling thread.
 * @retval  0 if the init succeeded
 * @retval  1 otherwise
 */
void sctk_accl_cuda_init_context()
{
	mpc_thread_yield();
	int num_devices;
	if ((num_devices = sctk_accl_cuda_check_devices()) == 0)
	{
		return;
	}

	/* we init CUDA TLS context */
	cuda_ctx_t *cuda = (cuda_ctx_t *)sctk_cuda_ctx;
	cuda         = (cuda_ctx_t *)sctk_malloc(sizeof(cuda_ctx_t));
	cuda->pushed = 0;
	cuda->cpu_id = mpc_topology_get_current_cpu();

	mpc_common_nodebug("CUDA: (MALLOC) pushed?%d, cpu_id=%d, address=%p", cuda->pushed, cuda->cpu_id, cuda);

	// cast from int -> CUdevice (which is a typedef to a int)
	CUdevice nearest_device =
		(CUdevice)sctk_accl_cuda_get_closest_device(cuda->cpu_id);

	// Use v3 for right context API version since cuda 12.0
	safe_cudadv(cuCtxCreate_v3(&cuda->context, NULL, 0, CU_CTX_SCHED_YIELD, nearest_device));
	// cuCtxCreate_v3 automatically attaches the ctx to the GPU
	cuda->pushed = 1;



	mpc_common_debug("CUDA: (INIT) PU %d bound to device %d", cuda->cpu_id, nearest_device);

	// Set the current pointer as default one for the current thread
	sctk_cuda_ctx = cuda;
	return;
}

/**
 * Popping the CUDA context for the calling thread from the associated GPU.
 *
 * Popping a context means we request CUDA to detach the current ctx from the
 * calling thread. This is done when saving the thread, before yielding it with
 * another one (save())
 *
 * @retval 0 if the popping succeeded
 * @retval 1 otherwise
 */
int sctk_accl_cuda_pop_context()
{
	int num_devices;

	if ((num_devices = sctk_accl_cuda_check_devices()) == 0)
	{
		return 1;
	}

	// get The CUDA context for the current thread
	cuda_ctx_t *cuda = (cuda_ctx_t *)sctk_cuda_ctx;

	// if the calling thread does not provide a CUDA ctx, this thread is
	// supposed to not execute GPU code
	if (cuda == NULL)
	{
		return 1;
	}

	// if the associated CUDA context is pushed on the GPU
	// This allow us to maintain save()/restore() operations independent
	// from each other
	if (cuda->pushed)
	{
		safe_cudadv(cuCtxPopCurrent(&cuda->context));
		cuda->pushed = 0;
	}

	return 0;
}

/**
 * Pushing the CUDA context for the calling thread to the associated GPU.
 *
 * Pushing a context means attaching the calling thread context to CUDA.
 * This is done when a thread is restored, its context is pushed to the GPU
 *
 * @retval 0 if succeeded
 * @retval 1 otherwise
 */
int sctk_accl_cuda_push_context()
{
	int num_devices;

	if ((num_devices = sctk_accl_cuda_check_devices()) == 0)
	{
		return 1;
	}

	// else, we try to push a ctx in GPU queue
	cuda_ctx_t *cuda = (cuda_ctx_t *)sctk_cuda_ctx;

	// it the current thread does not have a CUDA ctx, skip...
	if (cuda == NULL)
	{
		return 1;
	}

	/* if the context is not already on the GPU */
	if (!cuda->pushed)
	{
		safe_cudadv(cuCtxPushCurrent(cuda->context));
		cuda->pushed = 1;
	}
	return 0;
}

/**
 * Initialize the CUDA interface with MPC
 *
 * @return 0 if succeeded, 1 otherwise
 */
int sctk_accl_cuda_init()
{
	if (mpc_common_get_flags()->enable_cuda)
	{
		safe_cudadv(cuInit(0));
		return 0;
	}
	return 1;
}

/**
 * Release Globals CUDA contexts
 */
void sctk_accl_release_global_cuda_context()
{
	// cuda runtime segfault after main if not release explicitly
	// one primary context per gpu device
	int      num_devices = 1;
	CUresult code        = cuDeviceGetCount(&num_devices);

	// Unknown Errors (if cuda driver is not started correctly).
	if (code == CUDA_ERROR_UNKNOWN)
	{
		return;
	}
	// Not initialized error if cuda is no cuinit.
	if (code == CUDA_ERROR_NOT_INITIALIZED)
	{
		return;
	}
	if (code != CUDA_SUCCESS)
	{
		const char *errorStr = NULL;
		// CUDA API does not specify if the string is user owned
		// and should be free
		cuGetErrorString(code, &errorStr);
		mpc_common_debug_warning(
			"CUDA: Release Context (cuDeviceGetCount): %s (code %d) %s:%dL\n", errorStr, code, __FILE__, __LINE__);
		return;
	}

	CUdevice     device;
	unsigned int flags;
	int          is_active;
	for (int i = 0; i < num_devices; i++)
	{
		safe_cudadv(cuDeviceGet(&device, i));

		// Get the primary context state
		safe_cudadv(cuDevicePrimaryCtxGetState(device, &flags, &is_active));
		if (is_active)
		{
			cuDevicePrimaryCtxRelease(device);
			mpc_common_debug("CUDA: (RELEASE-PRIMARY) Context of device %d", device);
		}
	}
}

/**
 * Release the CUDA contexts with MPC
 */
void sctk_accl_cuda_release_context()
{
	// release context created by cuda support
	if (mpc_common_get_flags()->enable_cuda)
	{
		int num_devices;
		if ((num_devices = sctk_accl_cuda_check_devices()) == 0)
		{
			return;
		}

		cuda_ctx_t *cuda = (cuda_ctx_t *)sctk_cuda_ctx;

		CUdevice device;
		safe_cudadv(cuCtxGetDevice(&device));
		mpc_common_debug("CUDA: (RELEASE) PU %d bound to device %d", cuda->cpu_id, device);

		cuCtxDestroy(cuda->context);
		sctk_cuda_ctx = NULL;
	}
	sctk_accl_release_global_cuda_context();
}

/*********************************
 * MPC CUDA INIT FUNCTION *
 *********************************/

void mpc_accelerator_cuda_register_function() __attribute__((constructor));

void mpc_accelerator_cuda_register_function()
{
	MPC_INIT_CALL_ONLY_ONCE

	mpc_common_init_callback_register("VP Thread Start", "Init per VP CUDA context", sctk_accl_cuda_init_context, 25);

	mpc_common_init_callback_register("VP Thread End", "Release per VP CUDA context",
		sctk_accl_cuda_release_context, 22);
}
