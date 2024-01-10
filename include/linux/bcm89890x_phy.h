// SPDX-License-Identifier: GPL-2.0
// INTEL CONFIDENTIAL
// Copyright 2023-2024 Intel Corporation.
// This software and the related documents are Intel copyrighted materials, and
// your use of them is governed by the express license under which they were
// provided to you ("License"). Unless the License provides otherwise, you may
// not use, modify, copy, publish, distribute, disclose or transmit this
// software or the related documents without Intel's prior written permission.
// This software and the related documents are provided as is, with no express
// or implied warranties, other than those that are expressly stated in the
// License.

#ifndef _BCM89890x_PHY_H
#define _BCM89890x_PHY_H

#include <linux/types.h>


/* Mask used for ID comparisons */
#define BCM_PHY_ID_MASK			0xfffffff0

/* Known PHY IDs */
#define BCM_PHY_ID_BCM89890x		0x35905216

/* Speed register. */
#define SPEED_10MBPS       (0x01U) /* Select 10Mbps               */
#define SPEED_100MBPS      (0x02U) /* Select 100Mbps               */
#define SPEED_1000MBPS     (0x04U) /* Select 1000Mbps               */
#define SPEED_2500MBPS     (0x08U) /* Select 2.5G-X               */
#define SPEED_2500MBPS_X   (0x08U) /* Select 2.5G-X               */
#define SPEED_2500MBPS_R   (0x09U) /* Select 2.5G-R               */
#define SPEED_5000MBPS     (0x10U) /* Select 5G-X               */
#define SPEED_5000MBPS_X   (0x10U) /* Select 5G-X               */
#define SPEED_5000MBPS_R   (0x11U) /* Select 5G-R               */
#define SPEED_5000MBPS_KR  (0x12U) /* Select 5G-KR               */
#define SPEED_10000MBPS    (0x20U) /* Select 10G               */
#define SPEED_10000MBPS_R  (0x21U) /* Select 10G-R               */
#define SPEED_10000MBPS_KR (0x21U) /* Select 10G-KR               */

/* Duplex, half or full. */
#define DUPLEXMODE_HALF (0x1U)
#define DUPLEXMODE_FULL (0x2U)

/* Extended abilities register. */
#define PMD_IEEE_EXT_AB_BT1		0x000B	/* BASE-T1 ability */

#define MDIO_PMAPMD_ST1_REG_RECEIVE_LINK_STATUS_MASK (0x4U)
#define MDIO_AN_BT1_AN_STATUS_REG_LINK_STATUS_MASK (0x4U)

#define MDIO_PCS_CONTROL_REG_CL45_ADDR          (0x0U)

#define MDIO_AN_BT1_AN_CONTROL_REG_CL45_ADDR                      (0x200U)
#define MDIO_AN_ADVERTISEMENT_REG0_REG_CL45_ADDR                     (0x202U)
#define MDIO_AN_BT1_AN_STATUS_REG_CL45_ADDR                       (0x201U)
#define MDIO_AN_ADVERTISEMENT_REG1_REG_CL45_ADDR                     (0x203U)


#define MDIO_PMAPMD_PHYCONTROL_SQI_REG_CL45_ADDR    (0x8052U)

#define MDIO_AN_ADVERTISEMENT_REG1_REG_ADVERTISE_10GBASE_T1_MASK (0x400U)
#define MDIO_AN_ADVERTISEMENT_REG1_REG_ADVERTISE_5GBASE_T1_MASK (0x200U)
#define MDIO_AN_ADVERTISEMENT_REG1_REG_ADVERTISE_2P5GBASE_T1_MASK (0x100U)
#define MDIO_AN_ADVERTISEMENT_REG1_REG_ADVERTISE_1000BASE_T1_MASK (0x80U)
#define MDIO_AN_ADVERTISEMENT_REG1_REG_ADVERTISE_100BASE_T1_MASK (0x20U)


#define MDIO_PMA_CTRL1_REMOTE		0x0002
#define MDIO_PMAPMD_BT1_CONTROL_REG_T1_BASE_SEL_MASK (0xfU)
#define MDIO_PMAPMD_BT1_CONTROL_REG_T1_BASE_SEL_SHIFT (0U)
#define MDIO_PMAPMD_BT1_CONTROL_REG_T1_100BASE        (0x0U)
#define MDIO_PMAPMD_BT1_CONTROL_REG_T1_1000BASE       (0x1U)
#define MDIO_PMAPMD_BT1_CONTROL_REG_T1_BASE_2P5GBASE  (0x4U)
#define MDIO_PMAPMD_BT1_CONTROL_REG_T1_BASE_5GBASE  (0x5U)
#define MDIO_PMAPMD_BT1_CONTROL_REG_T1_BASE_10GBASE (0x6U)
#define MDIO_PMAPMD_BT1_CONTROL_REG_CL45_ADDR         (0x834U)

#define BCM_TC15_DCQ_SQI		0x813F


#define  PCS_IEEE_REG			0x0003
#define  PCS_IEEE_ST1			0x0001
#define  PCS_RCV_LINK_ST		BIT(2)

enum LoopbackModeType {
    LOOPBACK_MODE_NONE = 0x0U,          // No loopback
    LOOPBACK_MODE_REMOTE = 0x1U,        // Remote loopback   
    LOOPBACK_MODE_EXTERNAL = 0x2U,      // External loopback
    LOOPBACK_MODE_INTERNAL = 0x3U       // Internal loopback
};

#endif /* _BCM89890x_PHY_H */

