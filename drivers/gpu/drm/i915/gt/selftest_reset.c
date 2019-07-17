// SPDX-License-Identifier: MIT
/*
 * Copyright © 2018 Intel Corporation
 */

#include "i915_selftest.h"
#include "selftests/igt_reset.h"
#include "selftests/igt_atomic.h"

static int igt_global_reset(void *arg)
{
<<<<<<< HEAD
	struct drm_i915_private *i915 = arg;
	unsigned int reset_count;
=======
	struct intel_gt *gt = arg;
	unsigned int reset_count;
	intel_wakeref_t wakeref;
>>>>>>> linux-next/akpm-base
	int err = 0;

	/* Check that we can issue a global GPU reset */

<<<<<<< HEAD
	igt_global_reset_lock(i915);

	reset_count = i915_reset_count(&i915->gpu_error);

	i915_reset(i915, ALL_ENGINES, NULL);

	if (i915_reset_count(&i915->gpu_error) == reset_count) {
=======
	igt_global_reset_lock(gt);
	wakeref = intel_runtime_pm_get(&gt->i915->runtime_pm);

	reset_count = i915_reset_count(&gt->i915->gpu_error);

	intel_gt_reset(gt, ALL_ENGINES, NULL);

	if (i915_reset_count(&gt->i915->gpu_error) == reset_count) {
>>>>>>> linux-next/akpm-base
		pr_err("No GPU reset recorded!\n");
		err = -EINVAL;
	}

<<<<<<< HEAD
	igt_global_reset_unlock(i915);

	if (i915_reset_failed(i915))
=======
	intel_runtime_pm_put(&gt->i915->runtime_pm, wakeref);
	igt_global_reset_unlock(gt);

	if (intel_gt_is_wedged(gt))
>>>>>>> linux-next/akpm-base
		err = -EIO;

	return err;
}

static int igt_wedged_reset(void *arg)
{
<<<<<<< HEAD
	struct drm_i915_private *i915 = arg;
=======
	struct intel_gt *gt = arg;
>>>>>>> linux-next/akpm-base
	intel_wakeref_t wakeref;

	/* Check that we can recover a wedged device with a GPU reset */

<<<<<<< HEAD
	igt_global_reset_lock(i915);
	wakeref = intel_runtime_pm_get(&i915->runtime_pm);

	i915_gem_set_wedged(i915);

	GEM_BUG_ON(!i915_reset_failed(i915));
	i915_reset(i915, ALL_ENGINES, NULL);

	intel_runtime_pm_put(&i915->runtime_pm, wakeref);
	igt_global_reset_unlock(i915);

	return i915_reset_failed(i915) ? -EIO : 0;
=======
	igt_global_reset_lock(gt);
	wakeref = intel_runtime_pm_get(&gt->i915->runtime_pm);

	intel_gt_set_wedged(gt);

	GEM_BUG_ON(!intel_gt_is_wedged(gt));
	intel_gt_reset(gt, ALL_ENGINES, NULL);

	intel_runtime_pm_put(&gt->i915->runtime_pm, wakeref);
	igt_global_reset_unlock(gt);

	return intel_gt_is_wedged(gt) ? -EIO : 0;
>>>>>>> linux-next/akpm-base
}

static int igt_atomic_reset(void *arg)
{
<<<<<<< HEAD
	struct drm_i915_private *i915 = arg;
=======
	struct intel_gt *gt = arg;
>>>>>>> linux-next/akpm-base
	const typeof(*igt_atomic_phases) *p;
	int err = 0;

	/* Check that the resets are usable from atomic context */

<<<<<<< HEAD
	igt_global_reset_lock(i915);
	mutex_lock(&i915->drm.struct_mutex);

	/* Flush any requests before we get started and check basics */
	if (!igt_force_reset(i915))
		goto unlock;

	for (p = igt_atomic_phases; p->name; p++) {
		GEM_TRACE("intel_gpu_reset under %s\n", p->name);

		p->critical_section_begin();
		reset_prepare(i915);
		err = intel_gpu_reset(i915, ALL_ENGINES);
		reset_finish(i915);
		p->critical_section_end();

		if (err) {
			pr_err("intel_gpu_reset failed under %s\n", p->name);
=======
	intel_gt_pm_get(gt);
	igt_global_reset_lock(gt);

	/* Flush any requests before we get started and check basics */
	if (!igt_force_reset(gt))
		goto unlock;

	for (p = igt_atomic_phases; p->name; p++) {
		intel_engine_mask_t awake;

		GEM_TRACE("__intel_gt_reset under %s\n", p->name);

		awake = reset_prepare(gt);
		p->critical_section_begin();

		err = __intel_gt_reset(gt, ALL_ENGINES);

		p->critical_section_end();
		reset_finish(gt, awake);

		if (err) {
			pr_err("__intel_gt_reset failed under %s\n", p->name);
>>>>>>> linux-next/akpm-base
			break;
		}
	}

	/* As we poke around the guts, do a full reset before continuing. */
<<<<<<< HEAD
	igt_force_reset(i915);

unlock:
	mutex_unlock(&i915->drm.struct_mutex);
	igt_global_reset_unlock(i915);
=======
	igt_force_reset(gt);

unlock:
	igt_global_reset_unlock(gt);
	intel_gt_pm_put(gt);

	return err;
}

static int igt_atomic_engine_reset(void *arg)
{
	struct intel_gt *gt = arg;
	const typeof(*igt_atomic_phases) *p;
	struct intel_engine_cs *engine;
	enum intel_engine_id id;
	int err = 0;

	/* Check that the resets are usable from atomic context */

	if (!intel_has_reset_engine(gt->i915))
		return 0;

	if (USES_GUC_SUBMISSION(gt->i915))
		return 0;

	intel_gt_pm_get(gt);
	igt_global_reset_lock(gt);

	/* Flush any requests before we get started and check basics */
	if (!igt_force_reset(gt))
		goto out_unlock;

	for_each_engine(engine, gt->i915, id) {
		tasklet_disable_nosync(&engine->execlists.tasklet);
		intel_engine_pm_get(engine);

		for (p = igt_atomic_phases; p->name; p++) {
			GEM_TRACE("intel_engine_reset(%s) under %s\n",
				  engine->name, p->name);

			p->critical_section_begin();
			err = intel_engine_reset(engine, NULL);
			p->critical_section_end();

			if (err) {
				pr_err("intel_engine_reset(%s) failed under %s\n",
				       engine->name, p->name);
				break;
			}
		}

		intel_engine_pm_put(engine);
		tasklet_enable(&engine->execlists.tasklet);
		if (err)
			break;
	}

	/* As we poke around the guts, do a full reset before continuing. */
	igt_force_reset(gt);

out_unlock:
	igt_global_reset_unlock(gt);
	intel_gt_pm_put(gt);
>>>>>>> linux-next/akpm-base

	return err;
}

int intel_reset_live_selftests(struct drm_i915_private *i915)
{
	static const struct i915_subtest tests[] = {
		SUBTEST(igt_global_reset), /* attempt to recover GPU first */
		SUBTEST(igt_wedged_reset),
		SUBTEST(igt_atomic_reset),
<<<<<<< HEAD
	};
	intel_wakeref_t wakeref;
	int err = 0;

	if (!intel_has_gpu_reset(i915))
		return 0;

	if (i915_terminally_wedged(i915))
		return -EIO; /* we're long past hope of a successful reset */

	with_intel_runtime_pm(&i915->runtime_pm, wakeref)
		err = i915_subtests(tests, i915);

	return err;
=======
		SUBTEST(igt_atomic_engine_reset),
	};
	struct intel_gt *gt = &i915->gt;

	if (!intel_has_gpu_reset(gt->i915))
		return 0;

	if (intel_gt_is_wedged(gt))
		return -EIO; /* we're long past hope of a successful reset */

	return intel_gt_live_subtests(tests, gt);
>>>>>>> linux-next/akpm-base
}
