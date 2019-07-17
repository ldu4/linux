/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright © 2018 Intel Corporation
 */

#ifndef __I915_SELFTESTS_IGT_RESET_H__
#define __I915_SELFTESTS_IGT_RESET_H__

#include <linux/types.h>

<<<<<<< HEAD
void igt_global_reset_lock(struct drm_i915_private *i915);
void igt_global_reset_unlock(struct drm_i915_private *i915);
bool igt_force_reset(struct drm_i915_private *i915);
=======
struct intel_gt;

void igt_global_reset_lock(struct intel_gt *gt);
void igt_global_reset_unlock(struct intel_gt *gt);
bool igt_force_reset(struct intel_gt *gt);
>>>>>>> linux-next/akpm-base

#endif
