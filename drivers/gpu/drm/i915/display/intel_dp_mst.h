/* SPDX-License-Identifier: MIT */
/*
 * Copyright © 2019 Intel Corporation
 */

#ifndef __INTEL_DP_MST_H__
#define __INTEL_DP_MST_H__

<<<<<<< HEAD
struct intel_digital_port;

int intel_dp_mst_encoder_init(struct intel_digital_port *intel_dig_port, int conn_id);
void intel_dp_mst_encoder_cleanup(struct intel_digital_port *intel_dig_port);
=======
#include "intel_drv.h"

int intel_dp_mst_encoder_init(struct intel_digital_port *intel_dig_port, int conn_id);
void intel_dp_mst_encoder_cleanup(struct intel_digital_port *intel_dig_port);
static inline int
intel_dp_mst_encoder_active_links(struct intel_digital_port *intel_dig_port)
{
	return intel_dig_port->dp.active_mst_links;
}

>>>>>>> linux-next/akpm-base

#endif /* __INTEL_DP_MST_H__ */
