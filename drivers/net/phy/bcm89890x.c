// SPDX-License-Identifier: GPL-2.0
//
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

/*
 * Broadcom BCM89890 automotive 2500/5000/1000BASE-T1 PHY driver
 */

#include <linux/phy.h>
#include <linux/bcm89890x_phy.h>


static int bcm89890x_wait_init(struct phy_device *phydev)
{
	int val;

	return phy_read_mmd_poll_timeout(phydev, MDIO_MMD_PMAPMD, MDIO_CTRL1,
					 val, !(val & MDIO_CTRL1_RESET),
					 100000, 2000000, false);
}

/*
 * PMA_PMD reset function
 */
static int bcm89890x_soft_reset(struct phy_device *phydev)
{
	int ret;
	u16 data = 0U;
	int val = 0;

	/**
	* Read-modify-write operation for the PMD_IEEE_CTL1 register bit 15 (PMD reset bit)
	*/
	data = phy_read_mmd(phydev, MDIO_MMD_PMAPMD, MDIO_CTRL1);

	data |= MDIO_CTRL1_RESET;
	ret = phy_write_mmd(phydev, MDIO_MMD_PMAPMD, MDIO_CTRL1, data);

	if (ret < 0) {
		phydev_err(phydev, "Error: Writing to PMD_IEEE_CTL1 failed: %d\n", ret);
		return ret;
	}

	/* To check if the bit 15 reset bit of the PMD_cntrl1 register is reset indicating a successful reset happened witin.
	*  5ms sleep before read and 60ms time-out for the reset to happen.
	*/
	return phy_read_mmd_poll_timeout(phydev, MDIO_MMD_PMAPMD, MDIO_CTRL1,
					 val, !(val & MDIO_CTRL1_RESET), 50000,
					 600000, true);
}

/**
 * bcm89890x_cl45_pma_baset1_read_master_slave - read forced master/slave
 * configuration
 * @phydev: target phy_device struct
 */
int bcm89890x_cl45_pma_baset1_read_master_slave(struct phy_device *phydev)
{
	int ret;
	u16 data = 0U;

	/* Initializing master-slave state and configuration to unknown */
	phydev->master_slave_state = MASTER_SLAVE_STATE_UNKNOWN;
	phydev->master_slave_get = MASTER_SLAVE_CFG_UNKNOWN;

	/* Reading from PMAPMD Base-T1 Control Register */
	ret = phy_read_mmd(phydev, MDIO_MMD_PMAPMD,
			   MDIO_PMAPMD_BT1_CONTROL_REG_CL45_ADDR);
	if (ret < 0) {
		phydev_err(phydev, "Error: Read Master/Slave of MDIO_PMAPMD_BT1_CONTROL_REG_CL45 failed: %d\n",	ret);
		return ret;
	}

	data = (u16)ret;
	if (data & MDIO_PMA_PMD_BT1_CTRL_CFG_MST) {
		phydev->master_slave_get = MASTER_SLAVE_CFG_MASTER_FORCE;
		phydev->master_slave_state = MASTER_SLAVE_STATE_MASTER;
		phydev_dbg(phydev, "Master/Slave configuration set to Master\n");
	} else {
		phydev->master_slave_get = MASTER_SLAVE_CFG_SLAVE_FORCE;
		phydev->master_slave_state = MASTER_SLAVE_STATE_SLAVE;
		phydev_dbg(phydev, "Master/Slave configuration set to Slave\n");
	}

	return 0;
}

/**
 * bcm89890x_cl45_baset1_able - checks if the PMA has BASE-T1 extended abilities
 * @phydev: target phy_device struct
 */
static bool bcm89890x_cl45_baset1_able(struct phy_device *phydev)
{
	int val;

	if (phydev->pma_extable == -ENODATA) {
		val = phy_read_mmd(phydev, MDIO_MMD_PMAPMD, MDIO_PMA_EXTABLE);
		if (val < 0) {
			phydev_err(phydev, "Error: Read of MDIO_PMA_EXTABLE failed: %d\n", val);
			return false;
		}

		phydev->pma_extable = val;
		phydev_info(phydev, "Read PMA extended abilities: 0x%04x\n", phydev->pma_extable);
	}

	if (phydev->pma_extable & PMD_IEEE_EXT_AB_BT1) {
		phydev_dbg(phydev, "PMA supports BASE-T1 extended abilities\n");
	} else {
		phydev_dbg(phydev, "PMA does not support BASE-T1 extended abilities\n");
	}

	return !!(phydev->pma_extable & PMD_IEEE_EXT_AB_BT1);
}

/**
 * bcm89890x_cl45_read_pma - read link speed etc from PMA
 * @phydev: target phy_device struct
 */
int bcm89890x_cl45_read_pma(struct phy_device *phydev)
{
	int ret;
	u16 data = 0U;

	linkmode_zero(phydev->lp_advertising);
	ret = genphy_c45_read_pma(phydev);
	if (ret < 0) {
		phydev_err(phydev, "Error reading %d\n", ret);
		return ret;
	}

	/* Reading PMA/PMD Base-T1 control register */
	ret = phy_read_mmd(phydev, MDIO_MMD_PMAPMD,
			   MDIO_PMAPMD_BT1_CONTROL_REG_CL45_ADDR);
	if (ret < 0) {
		phydev_err(phydev, "Error reading link speed MDIO_PMAPMD_BT1_CONTROL_REG_CL45: %d\n", ret);
		return ret;
	}
	data = (u16)ret;
	data &= ~((uint16_t)MDIO_PMAPMD_BT1_CONTROL_REG_T1_BASE_SEL_MASK);

	/* Setting link speed */
	switch (phydev->speed) {
	case SPEED_100:
		data |= MDIO_PMAPMD_BT1_CONTROL_REG_T1_100BASE
			<< MDIO_PMAPMD_BT1_CONTROL_REG_T1_BASE_SEL_SHIFT;
		break;
	case SPEED_1000:
		data |= MDIO_PMAPMD_BT1_CONTROL_REG_T1_1000BASE
			<< MDIO_PMAPMD_BT1_CONTROL_REG_T1_BASE_SEL_SHIFT;
		break;
	case SPEED_2500:
		data |= MDIO_PMAPMD_BT1_CONTROL_REG_T1_BASE_2P5GBASE
			<< MDIO_PMAPMD_BT1_CONTROL_REG_T1_BASE_SEL_SHIFT;
		break;
	case SPEED_5000:
		data |= MDIO_PMAPMD_BT1_CONTROL_REG_T1_BASE_5GBASE
			<< MDIO_PMAPMD_BT1_CONTROL_REG_T1_BASE_SEL_SHIFT;
		break;
	case SPEED_10000:
		data |= MDIO_PMAPMD_BT1_CONTROL_REG_T1_BASE_10GBASE
			<< MDIO_PMAPMD_BT1_CONTROL_REG_T1_BASE_SEL_SHIFT;
		break;
	default:
		phydev_warn(phydev, "Unsupported or unknown device speed: %d\n", phydev->speed);
		return -EINVAL;
	}

	phydev_dbg(phydev, "Link speed set to %d Mbps, duplex mode to full\n", phydev->speed);

	/* Additional logic for BASE-T1 ability and reading master/slave configuration */
	if (bcm89890x_cl45_baset1_able(phydev)) {
		ret = bcm89890x_cl45_pma_baset1_read_master_slave(phydev);
		if (ret < 0) {
			phydev_err(phydev, "Error in reading master/slave configuration: %d\n",	ret);
			return ret;
		}
	}

	return 0;
}

static int bcm89890x_read_link(struct phy_device *phydev)
{
	int ret;
	bool link = false;

	/* Read vendor specific Auto-Negotiation status register to get local
	 * and remote receiver status according to software initialization
	 * guide.
	 */
	ret = phy_read_mmd(phydev, PCS_IEEE_REG, PCS_IEEE_ST1);
	if (ret < 0)
		return ret;
	else if (ret & PCS_RCV_LINK_ST)
		link = true;

	if (!link) {
		ret = phy_read_mmd(phydev, MDIO_MMD_PCS, MDIO_PCS_1000BT1_STAT);
		if (ret < 0)
			return ret;
		else if (ret & MDIO_PCS_1000BT1_STAT_LINK)
			link = true;
	}

	phydev->link = link;

	return 0;
}
/**
 * bcm89890x_cl45_read_status - read PHY status
 * @phydev: target phy_device struct
 *
 * Reads status from PHY and sets phy_device members accordingly.
 */
static int bcm89890x_read_status(struct phy_device *phydev)
{
	int ret;

	phydev_dbg(phydev, "Starting to read link status\n");

	ret = bcm89890x_read_link(phydev);
	if (ret < 0) {
		phydev_err(phydev, "Error in reading link status: %d\n", ret);
		return ret;
	}

	phydev_dbg(phydev, "Link status read successfully, proceeding to read PMA\n");
	ret = bcm89890x_cl45_read_pma(phydev);
	if (ret < 0) {
		phydev_err(phydev, "Error in reading PMA status: %d\n", ret);
		return ret;
	}

	bcm89890x_read_link(phydev);
	phydev->speed = SPEED_2500;
	phydev->duplex = DUPLEX_FULL;

	phydev_dbg(phydev, "PMA read successfully\n");
	return 0;
}

/**
 * bcm89890x_cl45_an_config_aneg - configure PMA/PMD AN Control register
 * @phydev: target phy_device struct
 *
 *
 * Returns negative errno code on failure, 0 if PMA/PMD AN Control register didn't change,
 * or 1 if PMA/PMD AN Control register modes changed.
 */
int bcm89890x_cl45_an_config_aneg(struct phy_device *phydev)
{
	int changed = 0, ret;
	u16 reg0 = 0U;
	u16 reg1 = 0U;

	reg0 = phy_read_mmd(phydev, MDIO_MMD_PMAPMD,
			    MDIO_AN_ADVERTISEMENT_REG0_REG_CL45_ADDR);
	if (reg0 < 0) {
		phydev_err(phydev, "Error reading MDIO_AN_ADVERTISEMENT_REG0: %d\n", reg0);
		return reg0;
	}

	reg1 = phy_read_mmd(phydev, MDIO_MMD_PMAPMD,
			    MDIO_AN_ADVERTISEMENT_REG1_REG_CL45_ADDR);
	if (reg1 < 0) {
		phydev_err(phydev, "Error reading MDIO_AN_ADVERTISEMENT_REG1: %d\n", reg1);
		return reg1;
	}

	/* Handle master/slave configuration */
	switch (phydev->master_slave_set) {
	case MASTER_SLAVE_CFG_MASTER_FORCE:
		reg0 |= (uint16_t)MDIO_AN_T1_ADV_L_FORCE_MS;
		reg1 |= (uint16_t)MDIO_AN_T1_ADV_M_MST;
		break;
	case MASTER_SLAVE_CFG_SLAVE_FORCE:
		reg0 |= (uint16_t)MDIO_AN_T1_ADV_L_FORCE_MS;
		reg1 &= ~((uint16_t)MDIO_AN_T1_ADV_M_MST);
		break;
	case MASTER_SLAVE_CFG_MASTER_PREFERRED:
		reg0 &= (uint16_t)~MDIO_AN_T1_ADV_L_FORCE_MS;
		reg1 |= (uint16_t)MDIO_AN_T1_ADV_M_MST;
		break;
	case MASTER_SLAVE_CFG_SLAVE_PREFERRED:
		reg0 &= ~((uint16_t)MDIO_AN_T1_ADV_L_FORCE_MS);
		reg1 &= ~((uint16_t)MDIO_AN_T1_ADV_M_MST);
		break;
	default:
		phydev_warn(phydev, "Unsupported Master/Slave mode\n");
		return -EOPNOTSUPP;
	}

	/* Handle speed settings */
	if (phydev->speed == SPEED_100) {
		reg1 |= (uint16_t)MDIO_AN_ADVERTISEMENT_REG1_REG_ADVERTISE_100BASE_T1_MASK;
	} else {
		reg1 &= ~((uint16_t)MDIO_AN_ADVERTISEMENT_REG1_REG_ADVERTISE_100BASE_T1_MASK);
	}

	if (phydev->speed == SPEED_1000) {
		reg1 |= (uint16_t)MDIO_AN_ADVERTISEMENT_REG1_REG_ADVERTISE_1000BASE_T1_MASK;
	} else {
		reg1 &= ~((uint16_t)MDIO_AN_ADVERTISEMENT_REG1_REG_ADVERTISE_1000BASE_T1_MASK);
	}

	if (phydev->speed == SPEED_2500) {
		reg1 |= (uint16_t)MDIO_AN_ADVERTISEMENT_REG1_REG_ADVERTISE_2P5GBASE_T1_MASK;
	} else {
		reg1 &= ~((uint16_t)MDIO_AN_ADVERTISEMENT_REG1_REG_ADVERTISE_2P5GBASE_T1_MASK);
	}

	if (phydev->speed == SPEED_5000) {
		reg1 |= (uint16_t)MDIO_AN_ADVERTISEMENT_REG1_REG_ADVERTISE_5GBASE_T1_MASK;
	} else {
		reg1 &= ~((uint16_t)MDIO_AN_ADVERTISEMENT_REG1_REG_ADVERTISE_5GBASE_T1_MASK);
	}

	if (phydev->speed == SPEED_10000) {
		reg1 |= (uint16_t)MDIO_AN_ADVERTISEMENT_REG1_REG_ADVERTISE_10GBASE_T1_MASK;
	} else {
		reg1 &= ~((uint16_t)MDIO_AN_ADVERTISEMENT_REG1_REG_ADVERTISE_10GBASE_T1_MASK);
	}

	/* Writing updated values to the PHY's AN Advertisement registers*/
	ret = phy_write_mmd(phydev, MDIO_MMD_PMAPMD, MDIO_AN_ADVERTISEMENT_REG1_REG_CL45_ADDR, reg0);
	if (ret < 0) {
		phydev_err(phydev, "Error writing MDIO_AN_ADVERTISEMENT_REG1: %d\n", ret);
		return ret;
	}

	reg0 |= 0x1U;
	ret = phy_write_mmd(phydev, MDIO_MMD_PMAPMD, MDIO_AN_ADVERTISEMENT_REG0_REG_CL45_ADDR, reg0);
	if (ret < 0) {
		phydev_err(phydev, "Error writing MDIO_AN_ADVERTISEMENT_REG0: %d\n", ret);
		return ret;
	}

	phydev_info(phydev, "AN Advertisement registers updated successfully\n");

	/* enable autoneg and restart it */
	reg0 = MDIO_AN_CTRL1_RESTART;
	reg0 |= MDIO_AN_CTRL1_ENABLE;

	phydev_info(phydev, "Enabling and restarting autonegotiation\n");

	ret = phy_write_mmd(phydev, MDIO_MMD_PMAPMD, MDIO_AN_BT1_AN_CONTROL_REG_CL45_ADDR, reg0);

	if (ret < 0) {
		phydev_err(phydev, "Error writing to AN control register after enabling and restarting autonegotiation: %d\n", ret);
		return ret;
	}

	if (ret > 0) {
		changed = 1;
		phydev_info(phydev, "PMA/PMD AN Control register modes changed\n");
	}

	return changed;
}

/**
 * bcm89890x_cl45_pma_setup_forced - configures a forced speed
 * @phydev: target phy_device struct
 */
int bcm89890x_cl45_pma_setup_forced(struct phy_device *phydev)
{
	int ret;
	u16 data = 0U;

	/* Half duplex is not supported */
	if (phydev->duplex != DUPLEX_FULL) {
		phydev_warn(phydev, "Half duplex mode is not supported\n");
		return -EINVAL;
	}

	data = phy_read_mmd(phydev, MDIO_MMD_PMAPMD,
			    MDIO_PMAPMD_BT1_CONTROL_REG_CL45_ADDR);
	if (data < 0) {
		phydev_err(phydev, "Error reading PMAPMD setup force Base-T1 control register: %d\n", data);
		return data;
	}
	data &= ~((uint16_t)MDIO_PMAPMD_BT1_CONTROL_REG_T1_BASE_SEL_MASK);

	switch (phydev->speed) {
	case SPEED_100:
		data |= MDIO_PMAPMD_BT1_CONTROL_REG_T1_100BASE << MDIO_PMAPMD_BT1_CONTROL_REG_T1_BASE_SEL_SHIFT;
		break;
	case SPEED_1000:
		data |= MDIO_PMAPMD_BT1_CONTROL_REG_T1_1000BASE	<< MDIO_PMAPMD_BT1_CONTROL_REG_T1_BASE_SEL_SHIFT;
		break;
	case SPEED_2500:
		data |= MDIO_PMAPMD_BT1_CONTROL_REG_T1_BASE_2P5GBASE << MDIO_PMAPMD_BT1_CONTROL_REG_T1_BASE_SEL_SHIFT;
		break;
	case SPEED_5000:
		data |= MDIO_PMAPMD_BT1_CONTROL_REG_T1_BASE_5GBASE << MDIO_PMAPMD_BT1_CONTROL_REG_T1_BASE_SEL_SHIFT;
		break;
	case SPEED_10000:
		data |= MDIO_PMAPMD_BT1_CONTROL_REG_T1_BASE_10GBASE << MDIO_PMAPMD_BT1_CONTROL_REG_T1_BASE_SEL_SHIFT;
		break;
	default:
		phydev_warn(phydev, "Unsupported or unknown device speed: %d Mbps\n", phydev->speed);
		return -EINVAL;
	}

	/* Configure Master/Slave settings */
	if (data & MDIO_PMA_PMD_BT1_CTRL_CFG_MST)
		data |= MDIO_PMA_PMD_BT1_CTRL_CFG_MST;
	else
		data &= (uint16_t)~MDIO_PMA_PMD_BT1_CTRL_CFG_MST;

	ret = phy_write_mmd(phydev, MDIO_MMD_PMAPMD, MDIO_PMAPMD_BT1_CONTROL_REG_CL45_ADDR, data);

	if (ret < 0) {
		phydev_err(phydev, "Error writing in setup force PMAPMD Base-T1 control register: %d\n", ret);
		return ret;
	}

	/* stop autoneg mode and disable it */
	ret = phy_read_mmd(phydev, MDIO_MMD_PMAPMD, MDIO_AN_BT1_AN_CONTROL_REG_CL45_ADDR);
	if (ret < 0) {
		phydev_err(phydev, "Error in stop autoneg mode while reading AN control register: %d\n", data);
		return ret;
	}

	data &= (uint16_t)~MDIO_AN_CTRL1_RESTART;
	data &= (uint16_t)~MDIO_AN_CTRL1_ENABLE;
	ret = phy_write_mmd(phydev, MDIO_MMD_PMAPMD, MDIO_AN_BT1_AN_CONTROL_REG_CL45_ADDR, data);
	if (ret < 0) {
		phydev_err(phydev, "Error writing AN control register: %d\n", ret);
		return ret;
	}

	phydev_info(phydev, "Forced PMA setup completed successfully\n");

	return 0;
}

/**
 * bcm89890x_cl45_restart_aneg - Enable and restart auto-negotiation
 * @phydev: target phy_device struct
 *
 * This assumes that the auto-negotiation MMD is present.
 *
 * Enable and restart auto-negotiation.
 */
int bcm89890x_cl45_restart_aneg(struct phy_device *phydev)
{
	int ret;
	u16 data = 0U;

	phydev_info(phydev, "Attempting to restart auto-negotiation\n");

	ret = phy_read_mmd(phydev, MDIO_MMD_AN,
			   MDIO_AN_BT1_AN_CONTROL_REG_CL45_ADDR);
	if (ret < 0) {
		phydev_err(phydev, "Error in restart auto-negotiatio while reading AN control register: %d\n", ret);
		return ret;
	}

	data = (u16)ret;
	if (data & MDIO_AN_CTRL1_ENABLE) {
		data |= MDIO_AN_CTRL1_RESTART;
		ret = phy_write_mmd(phydev, MDIO_MMD_AN, MDIO_AN_BT1_AN_CONTROL_REG_CL45_ADDR, data);
		if (ret < 0) {
			phydev_err(phydev, "Error writing AN control register to restart: %d\n", ret);
			return ret;
		}
		phydev_info(phydev, "Auto-negotiation restarted successfully\n");
	} else {
		phydev_warn(phydev, "Auto-negotiation is not enabled, cannot restart\n");
		return -EINVAL;
	}

	return ret;
}

/**
 * bcm89890x_cl45_check_and_restart_aneg - Enable and resgart auto-negotiation
 * @phydev: target phy_device struct
 * @restart: whether aneg restart is requested
 *
 * This assumes that the auto-negotiation MMD is present.
 *
 * Check, and restart auto-negotiation if needed.
 */
int bcm89890x_cl45_check_and_restart_aneg(struct phy_device *phydev, bool restart)
{
	u16 reg = MDIO_CTRL1;
	int ret;

	phydev_info(phydev, "Checking auto-negotiation status and restarting if necessary\n");

	if (bcm89890x_cl45_baset1_able(phydev)) {
		reg = MDIO_AN_T1_CTRL;
		phydev_info(phydev, "Using Base-T1 control for auto-negotiation\n");
	}

	if (!restart) {
		/* Configure and restart aneg if it wasn't set before */
		ret = phy_read_mmd(phydev, MDIO_MMD_AN, reg);
		if (ret < 0) {
			phydev_err(phydev, "Error in configure and restart aneg while reading AN register: %d\n", ret);
			return ret;
		}

		if (!(ret & MDIO_AN_CTRL1_ENABLE)) {
			phydev_info( phydev, "Auto-negotiation is not enabled, enabling restart\n");
			restart = true;
		} else {
			phydev_info(phydev, "Auto-negotiation is already enabled\n");
		}
	}

	if (restart) {
		phydev_info(phydev, "Restarting auto-negotiation\n");
		return bcm89890x_cl45_restart_aneg(phydev);
	}

	phydev_info(phydev, "Auto-negotiation restart not required\n");

	return 0;
}

/**
 * bcm89890x_cl45_config_aneg - restart auto-negotiation or forced setup
 * @phydev: target phy_device struct
 *
 * Description: If auto-negotiation is enabled, we configure the
 *   PMA/PMD AN Control registers, and then restart auto-negotiation. If it is not
 *   enabled, then we force a configuration.
 */
int bcm89890x_cl45_config_aneg(struct phy_device *phydev)
{
	bool changed = false;
	int ret;

	phydev_info(phydev, "Configuring auto-negotiation\n");

	if (phydev->autoneg == AUTONEG_DISABLE) {
		phydev_info( phydev, "Auto-negotiation is disabled, setting up PMA forced mode\n");
		return bcm89890x_cl45_pma_setup_forced(phydev);
	}

	ret = bcm89890x_cl45_an_config_aneg(phydev);
	if (ret < 0) {
		phydev_err(phydev, "Error auto-negotiation configuring AN register: %d\n", ret);
		return ret;
	}
	if (ret > 0) {
		changed = true;
		phydev_info(phydev, "Auto-negotiation configuration changed\n");
	} else {
		phydev_info(phydev, "Auto-negotiation configuration did not change\n");
	}

	ret = bcm89890x_cl45_check_and_restart_aneg(phydev, changed);
	if (ret < 0) {
		phydev_err(phydev, "Error in checking and restarting auto-negotiation: %d\n", ret);
	}

	return ret;
}

static int bcm89890x_config_aneg(struct phy_device *phydev)
{
	int ret;
	int changed = false;

	/* Wait for the PHY to finish initialising, otherwise our
	 * advertisement may be overwritten.
	 */
	ret = bcm89890x_wait_init(phydev);
	if (ret)
		return ret;

	/* We only support autoneg off for now
	 */
	phydev->autoneg = AUTONEG_DISABLE;
	return genphy_c45_pma_setup_forced(phydev);
}

static int bcm89890x_config_init(struct phy_device *phydev)
{
	u8 rev = phydev->phy_id & ~phydev->drv->phy_id_mask;
	int ret;
	int addr, min_addr, max_addr;

	min_addr = phydev->mdio.addr;
	max_addr = phydev->mdio.addr;
	addr = phydev->mdio.addr;

	/* Get phy id and revision information from
	 * standard location in MII_PHYS_ID[23]
	 */
	pr_info_once("%s: Detected %s with revision: 0x%02x\n", phydev_name(phydev), phydev->drv->name, rev);
	dev_info(&phydev->mdio.dev, "Detected Broadcom PHY ID with mdio base_addr: %d min_addr: %d and max_addr: %d\n",
		addr, min_addr, max_addr);

	/* The 89890X PHYs do have the extended ability register available, but
	 * register MDIO_PMA_EXTABLE where they should signalize it does not
	 * work according to specification. Therefore, we force it here.
	 */
	phydev->pma_extable = PMD_IEEE_EXT_AB_BT1;
	phydev_info(phydev, "Forced PMA extended ability register to PMD_IEEE_EXT_AB_BT1\n");

	/* Read the current PHY configuration */
	ret = genphy_c45_read_pma(phydev);
	if (ret)
		return ret;

	/* Read the current PHY configuration */
	ret = bcm89890x_cl45_read_pma(phydev);
	if (ret < 0) {
		phydev_err(phydev, "Error reading PHY configuration: %d\n", ret);
		return ret;
	}

	phydev_info(phydev, "Successfully read PMA configuration\n");

	ret = phy_write_mmd(phydev, MDIO_MMD_AN, MDIO_AN_ADVERTISEMENT_REG1_REG_CL45_ADDR, 0x100);
	if (ret < 0)
		return ret;

	return bcm89890x_config_aneg(phydev);
}

static int bcm89890x_get_sqi(struct phy_device *phydev)
{
	int ret;

	if (phydev->speed == SPEED_100) {
		phydev_info(phydev, "Reading SQI for 100Mbps speed\n");
		/* Read the SQI from the vendor specific receiver status
		 * register
		 */
		ret = phy_read_mmd(phydev, MDIO_MMD_PMAPMD,
				   MDIO_PMAPMD_PHYCONTROL_SQI_REG_CL45_ADDR);
		if (ret < 0) {
			phydev_err(phydev, "Error reading SQI register: %d\n", ret);
			return ret;
		}

		ret = ret >> 12;
		phydev_dbg(phydev, "SQI read successfully, value: %d\n", ret & 0x0F);
	} else {
		ret = phy_read_mmd(phydev, 0x1e, BCM_TC15_DCQ_SQI);
		if (ret < 0) {
			phydev_err(phydev, "Error reading SQI register: %d\n", ret);
			return ret;
		}
		ret = ret >> 1;
	}

	return ret & 0x0F;
}

static int bcm89890x_get_sqi_max(struct phy_device *phydev)
{
	phydev_info(phydev, "Maximum SQI value is 7\n");
	return 7;
}

int bcm89890x_loopback(struct phy_device *phydev, bool enable)
{
	int ret;
	enum LoopbackModeType loopbackMode = LOOPBACK_MODE_NONE;
	u16 pmdCtl = 0;
	u16 pcsCtl = 0;

	phydev_info(phydev, "Configuring loopback mode, enable: %d\n", enable);

	/* Read the current control registers */
	pmdCtl = phy_read_mmd(phydev, MDIO_MMD_PMAPMD, MDIO_CTRL1);
	if (pmdCtl < 0) {
		phydev_err(phydev, "Error reading PMA/PMD control1 register: %d\n", pmdCtl);
		return pmdCtl;
	}

	pcsCtl = phy_read_mmd(phydev, MDIO_MMD_PCS, MDIO_PCS_CONTROL_REG_CL45_ADDR);
	if (pcsCtl < 0) {
		phydev_err(phydev, "Error reading PCS control register: %d\n", pcsCtl);
		return pcsCtl;
	}

	/* Determine the current loopback mode based on register settings */
	if (pcsCtl & MDIO_PCS_CTRL1_LOOPBACK) {
		loopbackMode = LOOPBACK_MODE_INTERNAL;
	} else if (pmdCtl & MDIO_PMA_CTRL1_REMOTE) {
		loopbackMode = LOOPBACK_MODE_REMOTE;
	} else if (pmdCtl & MDIO_PMA_CTRL1_LOOPBACK) {
		loopbackMode = LOOPBACK_MODE_EXTERNAL;
	} else {
		phydev_warn(phydev, "No specific loopback mode set\n");
	}

	phydev_info(phydev, "Current loopback mode: %d\n", loopbackMode);

	printk(KERN_DEBUG "Set loopback mode\n");
	switch (loopbackMode) {
	case LOOPBACK_MODE_NONE:
		pmdCtl &= ~MDIO_PMA_CTRL1_REMOTE;
		pmdCtl &= ~MDIO_PMA_CTRL1_LOOPBACK;
		pcsCtl &= ~MDIO_PCS_CTRL1_LOOPBACK;
		break;
	case LOOPBACK_MODE_REMOTE:
		pmdCtl |= MDIO_PMA_CTRL1_REMOTE;
		pmdCtl &= ~MDIO_PMA_CTRL1_LOOPBACK;
		pcsCtl &= ~MDIO_PCS_CTRL1_LOOPBACK;
		break;
	case LOOPBACK_MODE_EXTERNAL:
		pmdCtl &= ~MDIO_PMA_CTRL1_REMOTE;
		pmdCtl |= MDIO_PMA_CTRL1_LOOPBACK;
		pcsCtl &= ~MDIO_PCS_CTRL1_LOOPBACK;
		break;
	case LOOPBACK_MODE_INTERNAL:
		pmdCtl &= ~MDIO_PMA_CTRL1_REMOTE;
		pmdCtl &= ~MDIO_PMA_CTRL1_LOOPBACK;
		pcsCtl |= MDIO_PCS_CTRL1_LOOPBACK;
		break;
	default:
		phydev_warn(phydev, "Invalid loopback mode: %d\n", loopbackMode);
		return -EINVAL;
	}

	/* Write the new settings to the control registers */
	ret = phy_write_mmd(phydev, MDIO_MMD_PMAPMD, MDIO_CTRL1, pmdCtl);
	if (ret < 0) {
		phydev_err(phydev, "Error writing PMA/PMD control1 register: %d\n", ret);
		return ret;
	}

	ret = phy_write_mmd(phydev, MDIO_MMD_PCS, MDIO_PCS_CONTROL_REG_CL45_ADDR, pcsCtl);
	if (ret < 0) {
		phydev_err(phydev, "Error writing PCS control register: %d\n", ret);
		return ret;
	}

	phydev_info(phydev, "Loopback mode configuration completed, enable: %d\n", enable);

	ret = phy_modify_mmd(phydev, MDIO_MMD_PMAPMD, MDIO_CTRL1, MDIO_PCS_CTRL1_LOOPBACK,
			     enable ? MDIO_PCS_CTRL1_LOOPBACK : 0);

	if (ret < 0) {
		phydev_err(phydev, "Error modifying PMA/PMD control1 for loopback: %d\n", ret);
		return ret;
	}

	phydev_info(phydev, "Loopback mode set successfully, enable: %d\n", enable);

	return 0;
}

static int bcm89890x_get_features(struct phy_device *phydev)
{
	int ret;

	phydev_info(phydev, "Reading general PHY features\n");
	ret = genphy_c45_pma_read_abilities(phydev);
	if (ret) {
		phydev_err(phydev, "Error reading general PMA abilities: %d\n", ret);
		return ret;
	}

	phydev_info(phydev, "Reading BASE-T1 extended abilities\n");
	/* We need to read the baset1 extended abilities manually because the
	 * PHY does not signalize it has the extended abilities register
	 * available.
	 */
	ret = genphy_c45_pma_baset1_read_abilities(phydev);
	if (ret) {
		phydev_err(phydev, "Error reading BASE-T1 extended abilities: %d\n", ret);
		return ret;
	}

	return 0;
}

static struct phy_driver bcm89890x_driver[] = {
	{
		.phy_id = BCM_PHY_ID_BCM89890x,
		.phy_id_mask = BCM_PHY_ID_MASK,
		.name = "bcm89890x",
		.get_features = bcm89890x_get_features,
		.config_aneg = bcm89890x_config_aneg,
		.config_init = bcm89890x_config_init,
		.read_status = bcm89890x_read_status,
		.soft_reset = bcm89890x_soft_reset,
		.set_loopback = bcm89890x_loopback,
		.get_sqi = bcm89890x_get_sqi,
		.get_sqi_max = bcm89890x_get_sqi_max,
	},
};

module_phy_driver(bcm89890x_driver);

static struct mdio_device_id __maybe_unused bcm89890x_tbl[] = {
	{ BCM_PHY_ID_BCM89890x, BCM_PHY_ID_MASK },
	{ /*Eiger*/ }
};

MODULE_DEVICE_TABLE(mdio, bcm89890x_tbl);
MODULE_DESCRIPTION("Broadcom BCM89890x Multi-Gigabit Automotive Ethernet PHY driver");
MODULE_LICENSE("GPL");
