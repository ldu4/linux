/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright © 2008-2018 Intel Corporation
 */

#ifndef I915_RESET_H
#define I915_RESET_H

#include <linux/compiler.h>
#include <linux/types.h>
#include <linux/srcu.h>

<<<<<<< HEAD
#include "gt/intel_engine_types.h"
=======
#include "intel_engine_types.h"
#include "intel_reset_types.h"
>>>>>>> linux-next/akpm-base

struct drm_i915_private;
struct i915_request;
struct intel_engine_cs;
<<<<<<< HEAD
struct intel_guc;

__printf(4, 5)
void i915_handle_error(struct drm_i915_private *i915,
		       intel_engine_mask_t engine_mask,
		       unsigned long flags,
		       const char *fmt, ...);
#define I915_ERROR_CAPTURE BIT(0)

void i915_check_and_clear_faults(struct drm_i915_private *i915);

void i915_reset(struct drm_i915_private *i915,
		intel_engine_mask_t stalled_mask,
		const char *reason);
int i915_reset_engine(struct intel_engine_cs *engine,
		      const char *reason);

void i915_reset_request(struct i915_request *rq, bool guilty);

int __must_check i915_reset_trylock(struct drm_i915_private *i915);
void i915_reset_unlock(struct drm_i915_private *i915, int tag);

int i915_terminally_wedged(struct drm_i915_private *i915);

bool intel_has_gpu_reset(struct drm_i915_private *i915);
bool intel_has_reset_engine(struct drm_i915_private *i915);

int intel_gpu_reset(struct drm_i915_private *i915,
		    intel_engine_mask_t engine_mask);

int intel_reset_guc(struct drm_i915_private *i915);

struct i915_wedge_me {
	struct delayed_work work;
	struct drm_i915_private *i915;
	const char *name;
};

void __i915_init_wedge(struct i915_wedge_me *w,
		       struct drm_i915_private *i915,
		       long timeout,
		       const char *name);
void __i915_fini_wedge(struct i915_wedge_me *w);

#define i915_wedge_on_timeout(W, DEV, TIMEOUT)				\
	for (__i915_init_wedge((W), (DEV), (TIMEOUT), __func__);	\
	     (W)->i915;							\
	     __i915_fini_wedge((W)))
=======
struct intel_gt;
struct intel_guc;

void intel_gt_init_reset(struct intel_gt *gt);
void intel_gt_fini_reset(struct intel_gt *gt);

__printf(4, 5)
void intel_gt_handle_error(struct intel_gt *gt,
			   intel_engine_mask_t engine_mask,
			   unsigned long flags,
			   const char *fmt, ...);
#define I915_ERROR_CAPTURE BIT(0)

void intel_gt_reset(struct intel_gt *gt,
		    intel_engine_mask_t stalled_mask,
		    const char *reason);
int intel_engine_reset(struct intel_engine_cs *engine,
		       const char *reason);

void __i915_request_reset(struct i915_request *rq, bool guilty);

int __must_check intel_gt_reset_trylock(struct intel_gt *gt);
void intel_gt_reset_unlock(struct intel_gt *gt, int tag);

void intel_gt_set_wedged(struct intel_gt *gt);
bool intel_gt_unset_wedged(struct intel_gt *gt);
int intel_gt_terminally_wedged(struct intel_gt *gt);

int __intel_gt_reset(struct intel_gt *gt, intel_engine_mask_t engine_mask);

int intel_reset_guc(struct intel_gt *gt);

struct intel_wedge_me {
	struct delayed_work work;
	struct intel_gt *gt;
	const char *name;
};

void __intel_init_wedge(struct intel_wedge_me *w,
			struct intel_gt *gt,
			long timeout,
			const char *name);
void __intel_fini_wedge(struct intel_wedge_me *w);

#define intel_wedge_on_timeout(W, GT, TIMEOUT)				\
	for (__intel_init_wedge((W), (GT), (TIMEOUT), __func__);	\
	     (W)->gt;							\
	     __intel_fini_wedge((W)))

static inline bool __intel_reset_failed(const struct intel_reset *reset)
{
	return unlikely(test_bit(I915_WEDGED, &reset->flags));
}

bool intel_has_gpu_reset(struct drm_i915_private *i915);
bool intel_has_reset_engine(struct drm_i915_private *i915);
>>>>>>> linux-next/akpm-base

#endif /* I915_RESET_H */
