/* SPDX-License-Identifier: MIT */
/*
 * Copyright © 2019 Intel Corporation
 */

#ifndef __INTEL_COMBO_PHY_H__
#define __INTEL_COMBO_PHY_H__

#include <linux/types.h>
<<<<<<< HEAD
#include <drm/i915_drm.h>

struct drm_i915_private;
=======

struct drm_i915_private;
enum phy;
>>>>>>> linux-next/akpm-base

void intel_combo_phy_init(struct drm_i915_private *dev_priv);
void intel_combo_phy_uninit(struct drm_i915_private *dev_priv);
void intel_combo_phy_power_up_lanes(struct drm_i915_private *dev_priv,
<<<<<<< HEAD
				    enum port port, bool is_dsi,
=======
				    enum phy phy, bool is_dsi,
>>>>>>> linux-next/akpm-base
				    int lane_count, bool lane_reversal);

#endif /* __INTEL_COMBO_PHY_H__ */
