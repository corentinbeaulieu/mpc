/* ############################# MPC License ############################## */
/* # Fri Apr 04 09:34:10 CET 2025                                         # */
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
#include <mpc_thread_rocm.h>

#define MPC_MODULE "Threads/Accel/HIP"

extern __thread void *sctk_hip_ctx;


static int sctk_accl_hip_check_devices()
{
	int num_devices = sctk_accl_get_nb_hip_devices();

	// if HIP support is loaded but the current configuration does not provide a GPU: stop
	if (num_devices <= 0)
	{
		mpc_common_nodebug("HIP: support enabled but no GPU found !");
		return 0;
	}

	// sanity check
	int nb_check;
	safe_hip(hipGetDeviceCount(&nb_check));
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
static int sctk_accl_hip_get_closest_device(int cpu_id)
{
	int num_devices;

	if ((num_devices = sctk_accl_hip_check_devices()) == 0)
	{
		return 0;
	}

	// else, we try to find the closest device for the current thread
	// to use, num_devices contains the number of minimum distance device
	mpc_topology_device_t **closest_devices = NULL;
	closest_devices = mpc_topology_device_matrix_get_list_closest_from_pu(cpu_id, "rocm-enabled-card*", &num_devices);

	// once the list is filtered with the nearest ones,
	// we need to elected the one with the minimum number of attached resources
	mpc_topology_device_t *elected = mpc_topology_device_attach_freest_device_from(closest_devices, num_devices);
	assert(elected != NULL);
	int nearest_id = elected->device_id;

	mpc_common_nodebug("HIP: (DETECTION) elected device %d", nearest_id);

	sctk_free(closest_devices);
	return nearest_id;
}

/**
 * Initialize a HIP context for the calling thread.
 * @retval 0 if the init succeeded
 * @retval 1 otherwise
 */
void sctk_accl_hip_init_context()
{
	mpc_thread_yield();
	int num_devices;
	if ((num_devices = sctk_accl_hip_check_devices()) == 0)
	{
		return;
	}

	/* we init HIP TLS context */
	hip_ctx_t *hip = (hip_ctx_t *)sctk_hip_ctx;
	hip         = (hip_ctx_t *)sctk_malloc(sizeof(hip_ctx_t));
	hip->pushed = 0;
	hip->cpu_id = mpc_topology_get_current_cpu();

	mpc_common_nodebug("HIP: (MALLOC) pushed?%d, cpu_id=%d, address=%p",
		hip->pushed, hip->cpu_id, hip);

	// cast from int -> hipDevice_t (which is a typedef to a int)
	hipDevice_t nearest_device =
		(hipDevice_t)sctk_accl_hip_get_closest_device(hip->cpu_id);


	safe_hip(
		hipCtxCreate(&hip->context, hipDeviceScheduleYield, nearest_device));


	hip->pushed = 1;

	hipDevice_t device;
	safe_hip(hipCtxGetDevice(&device));
	mpc_common_debug("HIP: (INIT) PU %d bound to device %d (%d)\n",
		hip->cpu_id, device, nearest_device);

	// Set the current pointer as default one for the current thread
	sctk_hip_ctx = hip;
	return;
}

/**
 * Popping the HIP context for the calling thread from the associated GPU.
 *
 * Popping a context means we request HIP to detach the current ctx from the
 * calling thread. This is done when saving the thread, before yielding it with another one (save())
 *
 * @retval 0 if the popping succeeded
 * @retval 1 otherwise
 */
int sctk_accl_hip_pop_context()
{
	int num_devices;

	if ((num_devices = sctk_accl_hip_check_devices()) == 0)
	{
		return 1;
	}

	// get The HIP context for the current thread
	hip_ctx_t *hip = (hip_ctx_t *)sctk_hip_ctx;

	// if the calling thread does not provide a HIP ctx, this thread is supposed to not execute GPU code
	if (hip == NULL)
	{
		return 1;
	}

	// if the associated HIP context is pushed on the GPU
	// This allow us to maintain save()/restore() operations independent from each other
	if (hip->pushed)
	{
		// hipDevice_t device;
		// safe_hip(hipCtxGetDevice(&device));
		// mpc_common_debug("HIP: (POP) PU %d bound to device %d\n", hip->cpu_id, device);

		safe_hip(hipCtxPopCurrent(&hip->context));
		hip->pushed = 0;
	}

	return 0;
}

/**
 * Pushing the HIP context for the calling thread to the associated GPU.
 *
 * Pushing a context means attaching the calling thread context to HIP.
 * This is done when a thread is restored, its context is pushed to the GPU
 *
 * @retval 0 if succeeded
 * @retval 1 otherwise
 */
int sctk_accl_hip_push_context()
{
	int num_devices;

	if ((num_devices = sctk_accl_hip_check_devices()) == 0)
	{
		return 1;
	}

	// else, we try to push a ctx in GPU queue
	hip_ctx_t *hip = (hip_ctx_t *)sctk_hip_ctx;

	// it the current thread does not have a HIP ctx, skip...
	if (hip == NULL)
	{
		return 1;
	}

	// if the context is not already on the GPU
	if (!hip->pushed)
	{
		safe_hip(hipCtxPushCurrent(hip->context));
		hip->pushed = 1;

		// hipDevice_t device;
		// safe_hip(hipCtxGetDevice(&device));
		// mpc_common_debug("HIP: (PUSH) PU %d bound to device %d\n", hip->cpu_id, device);
	}
	return 0;
}

/**
 * Initialize the HIP interface with MPC
 *
 * @return 0 if succeeded, 1 otherwise
 */
int sctk_accl_hip_init()
{
	if (mpc_common_get_flags()->enable_rocm)
	{
		safe_hip(hipInit(0));
		return 0;
	}
	return 1;
}

/**
 * Release the HIP contexts with MPC
 */
void sctk_accl_hip_release_context()
{
	// release context created by hip support
	if (mpc_common_get_flags()->enable_rocm)
	{
		int num_devices;
		if ((num_devices = sctk_accl_hip_check_devices()) == 0)
		{
			return;
		}

		hip_ctx_t *hip = (hip_ctx_t *)sctk_hip_ctx;

		hipDevice_t device;
		safe_hip(hipCtxGetDevice(&device));
		mpc_common_debug("HIP: (RELEASE) PU %d bound to device %d\n", hip->cpu_id, device);

		hipCtxDestroy(hip->context);
		sctk_hip_ctx = NULL;
	}
}

/*********************************
 * MPC HIP INIT FUNCTION *
 *********************************/

void mpc_accelerator_hip_register_function() __attribute__((constructor));

void mpc_accelerator_hip_register_function()
{
	MPC_INIT_CALL_ONLY_ONCE

	mpc_common_init_callback_register("VP Thread Start", "Init per VP HIP context", sctk_accl_hip_init_context, 26);

	mpc_common_init_callback_register("VP Thread End", "Release per VP HIP context", sctk_accl_hip_release_context, 21);
}
