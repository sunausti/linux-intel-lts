/*
 * Copyright © 2023 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/version.h>
#include <linux/gpio.h> /* For Legacy integer based GPIO */
#include <linux/interrupt.h> /* For IRQ */
#include <asm/uaccess.h> /* for copy_from_user */
#include <linux/proc_fs.h> /* for proc fs */
#include <linux/seq_file.h>
#include <linux/backlight.h>
#include <linux/fb.h> /* for FB_BLANK_UNBLANK */


#include "i2c_cmd.h"
#include "i2c_cmd_igpu_phud_3840_720.h"
#include "i2c_cmd_igpu_phud_1920_720.h"
#include "i2c_cmd_igpu_laser_3840_720.h"
#include "i2c_cmd_igpu_holo_1920_1080.h"
#include "i2c_cmd_dgpu_central_3840_720.h"
#include "i2c_cmd_dgpu_trans_3840_1080.h"

#include "max_ser_drv.h"
#include "max_ser_drv_init.h"

#define MAX_BRIGHTNESS_VAL 100

struct max_device {
	uint32_t device_id;
	uint32_t chip;
	char pci_bus[32];
	uint16_t ser_addr;
	uint16_t des_addr;
	uint16_t eeprom_addr;
	uint32_t pwm_reg;
	struct i2c_board_info i2c_board;
};

int max_dp_ser_read_eeprom_reg(struct device *dev, struct i2c_adapter *adapter,
			       uint16_t addr, unsigned int reg_addr, u8 *val)
{
	u8 buf[1];
	int ret = 0;

	struct i2c_msg msg[2];

	buf[0] = reg_addr & 0xff;

	msg[0].addr = addr;
	msg[0].flags = 0;
	msg[0].buf = buf;
	msg[0].len = sizeof(buf);

	msg[1].addr = addr;
	msg[1].flags = 0 | I2C_M_RD;
	msg[1].buf = val;
	msg[1].len = 1;

	ret = i2c_transfer(adapter, msg, 2);
	if (ret < 0) {
		LOG_INFO(dev, "fail read eeprom reg_addr=0x%04x val=0x%02x\n",
			 reg_addr, *val);
	}
	return ret;
}

char max_dp_ser_read_reg(struct i2c_client *client, unsigned int reg_addr,
			 u8 *val)
{
	struct device *dev = &client->dev;
	u8 buf[2];
	int ret = 0;

	struct i2c_msg msg[2];

	buf[0] = reg_addr >> 8;
	buf[1] = reg_addr & 0xff;

	msg[0].addr = client->addr;
	msg[0].flags = client->flags;
	msg[0].buf = buf;
	msg[0].len = sizeof(buf);

	msg[1].addr = client->addr;
	msg[1].flags = client->flags | I2C_M_RD;
	msg[1].buf = val;
	msg[1].len = 1;

	ret = i2c_transfer(client->adapter, msg, 2);
	if (ret < 0) {
		LOG_INFO(dev, "fail read 0x%04x reg_addr=0x%04x val=0x%02x\n",
			 client->addr, reg_addr, *val);
	}
	return ret;
}

int max_dp_ser_write_reg(struct i2c_client *client, unsigned int reg_addr,
			 unsigned int val)
{
	struct device *dev = &client->dev;
	int ret = 0;
	struct i2c_msg msg;
	u8 buf[3];
	u8 read_val;

	buf[0] = (reg_addr & 0xff00) >> 8;
	buf[1] = reg_addr & 0xff;
	buf[2] = val;

	msg.addr = client->addr;
	msg.flags = client->flags;
	msg.buf = buf;
	msg.len = sizeof(buf);

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret < 0) {
		LOG_INFO(dev,
			 "fail write 0x%04x, reg_addr=0x%04x, val=0x%02x\n",
			 client->addr, reg_addr, val);
	} else {
		LOG_DEBUG(dev, "0x%04x,0x%02x\n", reg_addr, val);
	}
	return ret;
}

static int max_read_lock(struct i2c_client *client, unsigned int reg_addr,
			 u32 mask, u32 expected_value)
{
	struct device *dev = &client->dev;
	u8 reg_data;

	max_dp_ser_read_reg(client, reg_addr, &reg_data);
	if ((reg_data & mask) == expected_value)
		return 0;

	LOG_INFO(dev, "fail read 0x%02x, expect 0x%02x\n", reg_data,
		  expected_value);
	return -1;
}

static int max_read_lock_new(struct i2c_client *client, unsigned int reg_addr,
			     u32 mask, u32 expected_value, u8 *out_reg_data)
{
	struct device *dev = &client->dev;
	u8 reg_data;
	max_dp_ser_read_reg(client, reg_addr, &reg_data);
	if (out_reg_data) {
		*out_reg_data = reg_data;
	}
	if ((reg_data & mask) == expected_value)
		return 0;

	LOG_INFO(dev, "fail read 0x%02x, expect 0x%02x\n", reg_data,
		  expected_value);
	return -1;
}

static int max_dp_ser_detect(struct max_dp_ser_priv *priv)
{
	struct device *dev = priv->dev;
	int ret = 0;
	int retry = 0;

	LOG_INFO(dev, "MAXIM Serializer detected ok\n");
	return 0;
}

#define SINGLE_RESET 1
#define DUAL_RESET 2
static int max_dp_ser_prepare(struct max_dp_ser_priv *priv)
{
	struct device *dev = priv->dev;
	int i;
	int ret = 0;
	int reset_mode = SINGLE_RESET;

	switch (priv->current_mode) {
	case MAX_MODE_IGPU_PHUD_3840_720:
		reset_mode = SINGLE_RESET;
		break;
	case MAX_MODE_IGPU_PHUD_1920_720:
		reset_mode = SINGLE_RESET;
		break;
	default:
		break;
	}

	if (reset_mode == SINGLE_RESET) {
		// single link
		// igpu reset
		if (priv && priv->priv_client[0]) {
			max_dp_ser_write_reg(priv->priv_client[0], 0x10, 0x80);
			usleep_range(20000, 22000);
			max_dp_ser_write_reg(priv->priv_client[0], 0x45, 0x00);
		}
	} 
	#if 0
	else if (reset_mode == DUAL_RESET) {
		// dual link
		if (priv && priv->priv_client[1] && priv->priv_client[0]) {
			max_dp_ser_write_reg(priv->priv_client[1], 0x10, 0x00);
			max_dp_ser_write_reg(priv->priv_client[0], 0x10, 0x80);
			usleep_range(20000, 22000);
			max_dp_ser_write_reg(priv->priv_client[0], 0x45, 0x02);
		}
	}
	#endif
	/* Wait ~2ms for powerup to complete */
	usleep_range(2000, 2200);

	return ret;
}

static int max_dp_ser_setup_with_i2c_write_cmd(struct i2c_client *client,
					struct i2c_write_cmd *i2c_cmd, u32 size)
{
	struct i2c_write_cmd *pc = NULL;
	u32 i = 0;
	for (i = 0; i < size; i++) {
		pc = i2c_cmd + i;
		if (pc != NULL) {
			max_dp_ser_write_reg(client, pc->i2c_reg, pc->i2c_val);
			if (pc->i2c_delay_ms > 0) {
				msleep(pc->i2c_delay_ms);
			}
		}
	}
	return 0;
}

static int max_dp_ser_setup_phud_1920_720(struct i2c_client *client)
{
	return max_dp_ser_setup_with_i2c_write_cmd(client, dp_igpu_phud_1920_720_ser,
						   ARRAY_SIZE(dp_igpu_phud_1920_720_ser));
}

static int max_dp_des_setup_phud_1920_720(struct i2c_client *client)
{
	return max_dp_ser_setup_with_i2c_write_cmd(client, dp_igpu_phud_1920_720_des,
						   ARRAY_SIZE(dp_igpu_phud_1920_720_des));
}

static int max_dp_ser_setup_phud_3840_720(struct i2c_client *client)
{
	return max_dp_ser_setup_with_i2c_write_cmd(client, dp_igpu_phud_3840_720_ser,
						   ARRAY_SIZE(dp_igpu_phud_3840_720_ser));
}

static int max_dp_des_setup_phud_3840_720(struct i2c_client *client)
{
	return max_dp_ser_setup_with_i2c_write_cmd(client, dp_igpu_phud_3840_720_des,
						   ARRAY_SIZE(dp_igpu_phud_3840_720_des));
}

static int max_dp_des_setup(struct max_dp_ser_priv *priv)
{
	struct device *dev = priv->dev;
	struct i2c_client *client = priv->priv_client[1];
	if (client != NULL) {
		switch (priv->current_mode) {
			case MAX_MODE_IGPU_PHUD_3840_720:
				max_dp_des_setup_phud_3840_720(client);
				break;
			case MAX_MODE_IGPU_PHUD_1920_720:
				max_dp_des_setup_phud_1920_720(client);
				break;
			default:
				LOG_INFO(dev, "invalid mode =%d \n", priv->current_mode);
				return -ENODEV;
		}
	}

	LOG_INFO(dev, "Serdes training OK\n");
	return 0;
}

static int max_dp_des_setup_once(struct max_dp_ser_priv *priv)
{
	int ret = 0;
	/*
	 * if ok_count == 0 set ok_count = 1 and return 0
	 * if ok_count != 0 return 1
	 */
	if (atomic_cmpxchg(&priv->ok_count, 0, 1) == 0) {
		ret = max_dp_des_setup(priv);
	} else {
		atomic_inc(&priv->ok_count);
	}
	return ret;
}

/* static api to update given value */
static inline void max_dp_ser_update(struct i2c_client *client,
				     unsigned int reg, u32 mask, u8 val)
{
	u8 update_val;

	max_dp_ser_read_reg(client, reg, &update_val);
	update_val = ((update_val & (~mask)) | (val & mask));
	max_dp_ser_write_reg(client, reg, update_val);
}

static int max_dp_ser_setup(struct max_dp_ser_priv *priv)
{
	struct device *dev = priv->dev;
	struct i2c_client *client = priv->priv_client[0];
	int ret = 0;
	switch (priv->current_mode) {
	case MAX_MODE_IGPU_PHUD_3840_720:
		ret = max_dp_ser_setup_phud_3840_720(client);
		break;
	case MAX_MODE_IGPU_PHUD_1920_720:
		ret = max_dp_ser_setup_phud_1920_720(client);
		break;
	default:
		LOG_INFO(dev, "invalid mode =%d \n", priv->current_mode);
		return -ENODEV;
	}

	return 0;
}

static int max_dp_ser_enable(struct max_dp_ser_priv *priv)
{
	int ret = 0;

	atomic_set(&priv->ok_count, 0);

	ret = max_dp_ser_prepare(priv);
	if (ret < 0) {
		return -ENODEV;
	}

	ret = max_dp_ser_setup(priv);
	if (ret < 0) {
		return -ENODEV;
	}
	
	if (priv->device_info->des_addr != 0x00 && priv->priv_client[1] == NULL ) {
		priv->priv_client[1] = i2c_new_dummy_device(
			priv->priv_client[0]->adapter, priv->device_info->des_addr);
	}


	queue_delayed_work(priv->wq, &priv->delay_work, msecs_to_jiffies(500));
	return 0;
}

static int max_gmsl_training_check(struct max_dp_ser_priv *priv)
{
	int ret = 0;
	u8 value = 0;
	struct device *dev = priv->dev;

	ret = max_read_lock_new(priv->priv_client[0], MAX_DP_SER_CTRL3,
				MAX_DP_SER_CTRL3_LOCK_MASK,
				MAX_DP_SER_CTRL3_LOCK_VAL, &value);

	priv->ctrl13 = value;

	if (ret < 0) {
		priv->state = MAX_STATE_UNLOCK;
	}

	if (ret < 0) {
		LOG_INFO(dev, "Serdes GMSL Lock is not set\n");
		goto reschedule;
	}

	priv->state = MAX_STATE_LOCK;

	ret = max_read_lock(priv->priv_client[0], MAX_DP_SER_DPRX_TRAIN,
			    MAX_DP_SER_DPRX_TRAIN_STATE_MASK,
			    MAX_DP_SER_DPRX_TRAIN_STATE_VAL);
	if (ret < 0) {
		LOG_INFO(dev, "Serdes Link tranining hasn't completed\n");
		goto reschedule;
	}

	ret = max_read_lock(priv->priv_client[0], MAX_DP_SER_VID_TX2_PCLK,
			    MAX_DP_SER_VID_TX2_PCLK_STATE_MASK,
			    MAX_DP_SER_VID_TX2_PCLK_STATE_VAL);
	if (ret < 0) {
		LOG_INFO(dev, "Serdes PCLK hasn't completed\n");
		goto reschedule;
	}

	/*
       max_dp_ser_update(priv->priv_client[0], MAX_DP_SER_VID_TX_X,
       MAX_DP_SER_VID_TX_MASK, 0x1);
       max_dp_ser_update(priv->priv_client[0], MAX_DP_SER_VID_TX_Y,
       MAX_DP_SER_VID_TX_MASK, 0x1);
       max_dp_ser_update(priv->priv_client[0], MAX_DP_SER_VID_TX_Z,
       MAX_DP_SER_VID_TX_MASK, 0x1);
       max_dp_ser_update(priv->priv_client[0], MAX_DP_SER_VID_TX_U,
       MAX_DP_SER_VID_TX_MASK, 0x1);
       */

	return 0;
reschedule:
	return -1;
}

static int max_gmsl_training(struct max_dp_ser_priv *priv) {
	int ret = 0;
	u8 value = 0;
	struct device *dev = priv->dev;

	ret = max_gmsl_training_check(priv);
	if (ret < 0) {
		return -1;
	}
	//ok:
	priv->state = MAX_STATE_TRAINING_SUCCESS;
	priv->err_count = 0;
	max_dp_des_setup_once(priv);
	return 0;
}

static void max_poll_gmsl_training_lock(struct work_struct *work)
{
	int ret = 0;
	u8 value = 0;

	struct max_dp_ser_priv *priv = container_of((struct delayed_work *)work,
						    struct max_dp_ser_priv,
						    delay_work);
	struct device *dev = priv->dev;

	ret = max_gmsl_training(priv);
	if (ret < 0) {
		goto reschedule;
	}
	//ok:
	if (atomic_read(&priv->ok_count) < 2) {
		queue_delayed_work(priv->wq, &priv->delay_work,
				   msecs_to_jiffies(700));
	} else {
		LOG_DEBUG(dev, "Serdes double check OK\n");
	}
	return;

reschedule:
	priv->err_count++;
	if (priv->state == MAX_STATE_LOCK || priv->err_count < 4) {
		if (priv->err_count < 16) {
			queue_delayed_work(priv->wq, &priv->delay_work,
					   msecs_to_jiffies(700));
		} else {
#if 1
			priv->err_count = 0;
			max_dp_ser_enable(priv);
#endif
		}
	} else {
		LOG_DEBUG(dev, "Serdes check fail MAX_STATE_UNLOCK\n");
	}
	return;
}

static void max_poll_gmsl_training_oneshot(struct work_struct *work)
{
	int ret = 0;
	struct max_dp_ser_priv *priv = container_of(work,
						    struct max_dp_ser_priv,
						    oneshot_work);
	struct device *dev = priv->dev;
	max_dp_des_setup_once(priv);
	return;
}

static int max_backlight_proc_show(struct seq_file *seq, void *offset)
{
	char buf[80];
	char state_buf[32];
	char state_error_buf[32];
	struct max_dp_ser_priv *priv = (struct max_dp_ser_priv *)seq->private;

	int backlight = priv->backlight;
	int state = priv->state;
	u8 ctrl13 = priv->ctrl13;

	switch (state) {
	case MAX_STATE_UNLOCK:
		snprintf(state_buf, 32, " [STATE_UNLOCK]");
		break;
	case MAX_STATE_LOCK:
		snprintf(state_buf, 32, " [STATE_LOCKED]");
		break;
	case MAX_STATE_TRAINING_SUCCESS:
		snprintf(state_buf, 32, " [STATE_TRAINING_SUCCESS]");
		break;
	default:
		snprintf(state_buf, 32, " [STATE_LOCKED]");
		break;
	}

	if ((ctrl13 & MAX_DP_SER_CTRL3_ERROR_MASK) ==
	    MAX_DP_SER_CTRL3_ERROR_VAL) {
		snprintf(state_error_buf, 32, " [ERROR]");
	} else {
		snprintf(state_error_buf, 32, "");
	}

	snprintf(buf, 80, "%s%s%s\n", backlight ? "on" : "off", state_error_buf,
		 state_buf);

	seq_puts(seq, buf);

	return 0;
}

static int max_backlight_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, max_backlight_proc_show, pde_data(inode));
}

void max_backlight_light_set(struct max_dp_ser_priv *priv, int value)
{
	struct device *dev = priv->dev;
	if (priv->backlight == value)
		return;

	priv->backlight = value;
}

static ssize_t max_backlight_mode_write(struct file *file,
					const char __user *buff, size_t len,
					loff_t *data)
{
	struct max_dp_ser_priv *priv =
		(struct max_dp_ser_priv *)pde_data(file_inode(file));
	char buf[10];
	memset(buf, 0, 10);

	if (len >= 8)
		len = 8;

	if (copy_from_user(buf, buff, len))
		return -EFAULT;

	if (!strncmp(buf, "on", 2)) {
		max_backlight_light_set(priv, 1);
	} else if (buf[0] == '1') {
		max_backlight_light_set(priv, 1);
	} else {
		max_backlight_light_set(priv, 0);
	}

	return len;
}
/**
 * GPIOMAP light pwm      [0..255]
 * EC GPA4      -> 96749 MFP0
 *                   |
 *                 96772 GPIO7 -> BL_PWM
 */
void max_backlight_pwm_set(int value)
{
}

static const struct proc_ops max_backlight_proc_fops = {
	.proc_open = max_backlight_proc_open,
	.proc_write = max_backlight_mode_write,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

//#define BACKLIGHT_PROCFS_NAME DEV_MODULE "_backlight"
//#define DEV_BL_NAME DEV_MODULE "_bl"

static int max_backlight_proc_init(struct max_dp_ser_priv *priv)
{
	char backlight_proc_fs_name[64];
	if (!priv) {
		return -1;
	}

	snprintf(backlight_proc_fs_name, 60, "%s_backlight",
		 priv->device_info->i2c_board.type);
	struct proc_dir_entry *proc_file_entry =
		proc_create_data(backlight_proc_fs_name, 0644, NULL,
				 &max_backlight_proc_fops, (void *)priv);

	priv->proc_file_entry = proc_file_entry;

	if (proc_file_entry == NULL) {
		return -ENOMEM;
	}

	return 0;
}

static void max_backlight_proc_exit(struct max_dp_ser_priv *priv)
{
	/* data */
	if (priv && priv->proc_file_entry) {
		proc_remove(priv->proc_file_entry);
	}
}

static int devmem_set_8(struct device *dev, unsigned int reg, unsigned char val)
{
	unsigned char __iomem *gpio_cfg;
	unsigned char data;

	/* Map  GPIO IO address to virtual address */
	gpio_cfg = (unsigned char *)devm_ioremap(dev, reg, 0x1);
	if (!gpio_cfg) {
		LOG_INFO(dev, "fail set reg %x val %x\n", reg, (int)val);
		return -ENOMEM;
	}

	data = (unsigned char)val;
	iowrite8(data, gpio_cfg);

	iounmap(gpio_cfg);
	return 0;
}

static int devmem_get_8(struct device *dev, unsigned int reg,
			unsigned char *val)
{
	unsigned char __iomem *gpio_cfg;
	unsigned char data;

	/* Map  GPIO IO address to virtual address */
	gpio_cfg = (unsigned char *)devm_ioremap(dev, reg, 0x1);
	if (!gpio_cfg) {
		LOG_INFO(dev, "fail get reg %x\n", reg);
		return -ENOMEM;
	}

	data = ioread8(gpio_cfg);

	if (val != NULL) {
		*val = data;
	}

	iounmap(gpio_cfg);
	return 0;
}

static int max_bl_update_status(struct backlight_device *bl_dev)
{
	struct max_dp_ser_priv *priv = bl_get_data(bl_dev);
	struct device *dev = priv->dev;
	unsigned int brightness = backlight_get_brightness(bl_dev);
	bool is_blank = backlight_is_blank(bl_dev);
	int ret = 0;

	if (!priv)
		return 0;

	uint32_t pwm_reg = priv->device_info->pwm_reg;

	if (brightness > MAX_BRIGHTNESS_VAL) {
		brightness = MAX_BRIGHTNESS_VAL;
	}

	priv->brightness = brightness;

	if (is_blank) {
		max_backlight_light_set(priv, 0);
	} else {
		max_backlight_light_set(priv, 1);
	}

	return 0;
}

static int max_bl_get_brightness(struct backlight_device *bl_dev)
{
	struct max_dp_ser_priv *priv = bl_get_data(bl_dev);

	return priv->brightness;
}

static const struct backlight_ops max96749_bl_ops = {
	.update_status = max_bl_update_status,
	.get_brightness = max_bl_get_brightness,
	.options = BL_CORE_SUSPENDRESUME,
};

static int max_backlight_dev_init(struct max_dp_ser_priv *priv)
{
	struct backlight_properties bl_props;
	char name[60];
	int ret = 0;

	snprintf(name, sizeof(name), "%s_bl",
		 priv->device_info->i2c_board.type);

	memset(&bl_props, 0, sizeof(bl_props));
	bl_props.type = BACKLIGHT_RAW;
	bl_props.max_brightness = MAX_BRIGHTNESS_VAL;
	bl_props.power = FB_BLANK_UNBLANK;

	priv->bl_dev = devm_backlight_device_register(
		priv->dev, name, priv->dev, priv, &max96749_bl_ops, &bl_props);

	if (IS_ERR(priv->bl_dev)) {
		ret = -1;
		goto fail;
	}

	priv->bl_dev->props.brightness = MAX_BRIGHTNESS_VAL;

	backlight_update_status(priv->bl_dev);

fail:
	return ret;
}

static void max_backlight_dev_exit(struct max_dp_ser_priv *priv)
{
}

#ifdef CONFIG_PM

static void max_poll_gmsl_resume(struct work_struct *work)
{
	int ret = 0;
	struct max_dp_ser_priv *priv = container_of((struct work_struct *)work,
						    struct max_dp_ser_priv,
						    resume_work);
	struct device *dev = priv->dev;

	LOG_INFO(dev, "\n");

	ret = max_dp_ser_detect(priv);
	if (ret < 0) {
		LOG_INFO(dev, "ERROR detect unlock\n");
		return;
	}
	max_dp_ser_enable(priv);
	priv->state = MAX_STATE_LOCK;
}

static void max_dp_ser_reset(struct max_dp_ser_priv *priv)
{
	struct device *dev = priv->dev;

	if (priv && priv->priv_client[0]) {
		max_dp_ser_write_reg(priv->priv_client[0], 0x10, 0x80);
		max_dp_ser_write_reg(priv->priv_client[0], 0x45, 0x00);
	}
	LOG_DEBUG(dev, "reset ok\n");
}

static int max_dp_ser_suspend(struct device *dev)
{
	struct max_dp_ser_priv *priv = dev_get_drvdata(dev);

	LOG_DEBUG(dev, "\n");

	max_backlight_light_set(priv, 0);

	if (priv != NULL) {
		cancel_delayed_work_sync(&priv->delay_work);
		cancel_work_sync(&priv->resume_work);
		cancel_work_sync(&priv->oneshot_work);
		flush_workqueue(priv->wq);
	}
	/* dual link reset to single link*/
	max_dp_ser_reset(priv);

	return 0;
}

static int max_dp_ser_resume(struct device *dev)
{
	struct max_dp_ser_priv *priv = dev_get_drvdata(dev);
	queue_work(priv->wq, &priv->resume_work);
	return 0;
}

const struct dev_pm_ops __maybe_unused max_dp_ser_pmops = {
	SET_LATE_SYSTEM_SLEEP_PM_OPS(max_dp_ser_suspend, max_dp_ser_resume)
};

#define MAX_DP_SER_PMOPS (&max_dp_ser_pmops)
#else
#define MAX_DP_SER_PMOPS NULL
#endif

static const struct i2c_device_id max_id_table[] = {
	{ "max_phud_lr", IGPU_PHUD_3840_720_DEVICE },
	{ "max_phud_m", IGPU_PHUD_1920_720_DEVICE },
	#if 0
	{ "max_laser_lr", IGPU_LASER_3840_720_DEVICE },
	{ "max_holo", IGPU_HOLO_1920_1080_DEVICE },
	{ "max_central", DGPU_CENTRAL_3840_720_DEVICE },
	{ "max_trans", DGPU_TRANS_3840_1080_DEVICE },
	#endif
	{}
};

static struct max_device device_infos[] = {
	{
		.device_id = IGPU_PHUD_3840_720_DEVICE,
		.chip = MAX_96745,
		.pci_bus = "0000:00:15.1",
		.ser_addr = 0x62,
		.des_addr = 0x4C,
		.eeprom_addr = 0x00,
		.pwm_reg = 0x00,
		.i2c_board =
			{
				I2C_BOARD_INFO("max_phud_lr", 0x62),
			},
	},
	{
		.device_id = IGPU_PHUD_1920_720_DEVICE,
		.chip = MAX_96745,
		.pci_bus = "0000:00:15.1",
		.ser_addr = 0x42,
		.des_addr = 0x00,
		.eeprom_addr = 0x00,
		.pwm_reg = 0x00,
		.i2c_board =
			{
				I2C_BOARD_INFO("max_phud_m", 0x42),
			},
	},
	#if 0
	{ .device_id = IGPU_LASER_3840_720_DEVICE,
	  .chip = MAX_96745,
	  .pci_bus = "0000:00:15.1",
	  .ser_addr = 0x40,
	  .des_addr = 0x00,
	  .eeprom_addr = 0x00,
	  .pwm_reg = 0x00,
	  .i2c_board =
		  {
			  I2C_BOARD_INFO("max_laser_lr", 0x60),
		  }
	},
	{ .device_id = IGPU_HOLO_1920_1080_DEVICE,
	  .chip = MAX_96745,
	  .pci_bus = "0000:00:15.1",
	  .ser_addr = 0x40,
	  .des_addr = 0x00,
	  .eeprom_addr = 0x00,
	  .pwm_reg = 0x00,
	  .i2c_board =
		  {
			  I2C_BOARD_INFO("max_holo", 0x60),
		  }
	},
	{
		.device_id = DGPU_CENTRAL_3840_720_DEVICE,
		.chip = MAX_96745,
		.pci_bus = "0000:00:15.3",
		.ser_addr = 0x40,
		.des_addr = 0x00,
		.eeprom_addr = 0x00,
		.pwm_reg = 0x00,
		.i2c_board =
			{
				I2C_BOARD_INFO("max_central", 0x40),
			},
	},
	{
		.device_id = DGPU_TRANS_3840_1080_DEVICE,
		.chip = MAX_96745,
		.pci_bus = "0000:00:15.3",
		.ser_addr = 0x40,
		.des_addr = 0x00,
		.eeprom_addr = 0x00,
		.pwm_reg = 0x00,
		.i2c_board =
			{
				I2C_BOARD_INFO("max_trans", 0x60),
			},
	},
	#endif
};

/**
 * read eeprom and detect the panel config
 * 
*/
static int max_dp_ser_i2c_initialize(struct max_dp_ser_priv *priv,
				     uint32_t device_id)
{
	struct device *dev = priv->dev;
	int err = 0;
	int i;

	for (i = 0; i < ARRAY_SIZE(device_infos); i++) {
		if (device_infos[i].device_id == device_id) {
			priv->device_info = &device_infos[i];
			dev_set_name(dev, "i2c-%s",
				     priv->device_info->i2c_board.type);
			break;
		}
	}

	return err;
}

static int max_dp_ser_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct max_dp_ser_priv *priv;
	int device_id = (int)id->driver_data;
	int ret;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		LOG_DEBUG(dev, "i2c_check_functionality error\n");
		return -EIO;
	}

	priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
	if (!priv)
		return -ENOMEM;

	priv->dev = dev;
	ret = max_dp_ser_i2c_initialize(priv, device_id);
	if (ret < 0) {
		goto fail;
	}
	switch(priv->device_info->device_id) {
		case IGPU_PHUD_3840_720_DEVICE:
			priv->current_mode = MAX_MODE_IGPU_PHUD_3840_720;
			break;
		case IGPU_PHUD_1920_720_DEVICE:
			priv->current_mode = MAX_MODE_IGPU_PHUD_1920_720;
			break;
	}
	priv->priv_client[0] = client;

	i2c_set_clientdata(client, priv);

	priv->backlight = 1;
	priv->wq =
		alloc_workqueue("max_poll_gmsl_training_lock", WQ_HIGHPRI, 0);

	INIT_DELAYED_WORK(&priv->delay_work, max_poll_gmsl_training_lock);
	INIT_WORK(&priv->oneshot_work, max_poll_gmsl_training_oneshot);
	INIT_WORK(&priv->resume_work, max_poll_gmsl_resume);

	ret = max_dp_ser_detect(priv);
	if (ret < 0) {
		goto fail;
	}

	ret = max_backlight_proc_init(priv);
	if (ret < 0) {
		goto fail;
	}

	ret = max_backlight_dev_init(priv);
	if (ret < 0) {
		goto fail;
	}

	max_dp_ser_enable(priv);
	priv->state = MAX_STATE_LOCK;

	return ret;
fail:
	return ret;
}

static void max_dp_ser_i2c_remove(struct i2c_client *client)
{
	int i = 0;
	struct max_dp_ser_priv *priv = i2c_get_clientdata(client);
	struct device *dev = priv->dev;

	if (priv != NULL) {
		max_backlight_proc_exit(priv);
		max_backlight_dev_exit(priv);
		cancel_delayed_work_sync(&priv->delay_work);
		cancel_work_sync(&priv->oneshot_work);
		cancel_work_sync(&priv->resume_work);
		if (priv->priv_client[1] != NULL) {
			i2c_unregister_device(priv->priv_client[1]);
		}
		flush_workqueue(priv->wq);
		LOG_DEBUG(dev, "\n");
	}
}

struct i2c_client *dev_client[ARRAY_SIZE(device_infos)] = {};

int try_to_register_all_ser_device(void)
{
	struct i2c_adapter *adapter;
	struct device *parent;
	struct device *pp;
	struct i2c_client *client;
	int i = 0, j = 0;
	int ret = 0;

	while ((adapter = i2c_get_adapter(i)) != NULL) {
		parent = adapter->dev.parent;
		pp = parent->parent;
		int j = 0;

		printk("%s dev_name(pp) %s dev_name(parent) %s\n",__FUNCTION__,dev_name(pp), dev_name(parent));
		for (int j = 0; j < ARRAY_SIZE(device_infos); j++) {
			if (pp && !strncmp(device_infos[j].pci_bus,
					   dev_name(pp), 32)) {
				client = i2c_new_client_device(
					adapter, &device_infos[j].i2c_board);
				if (IS_ERR(client)) {
					LOG_DEBUG(
						&client->dev,
						"Failed to register %s\n",
						device_infos[j].i2c_board.type);
					dev_client[j] = NULL;
					ret = -EIO;
				} else {
					dev_client[j] = client;
				}
			}
		}
		i2c_put_adapter(adapter);
		i++;
	}
	return ret;
}

static struct i2c_driver max_dp_ser_i2c_driver = {
	.driver =
		{
			.name = "max_ser",
			.pm = MAX_DP_SER_PMOPS,
		},
	.probe = max_dp_ser_i2c_probe,
	.remove = max_dp_ser_i2c_remove,
	.id_table = max_id_table,
};

int __init max_dp_ser_module_init(void)
{
	int ret;
	ret = i2c_add_driver(&max_dp_ser_i2c_driver);
	if (ret < 0) {
		return ret;
	}
	ret = try_to_register_all_ser_device();
	if (ret < 0) {
		return ret;
	}

	return ret;
}

void __exit max_dp_ser_module_exit(void)
{
	i2c_del_driver(&max_dp_ser_i2c_driver);
}


MODULE_DESCRIPTION("MAXIM serdes 96745 driver");
MODULE_AUTHOR("Jia, Lin A <lin.a.jia@intel.com>");
MODULE_AUTHOR("Hu, Kanli <kanli.hu@intel.com>");
MODULE_LICENSE("GPL v2");

#if MODULE
module_init(max_dp_ser_module_init);
module_exit(max_dp_ser_module_exit);
#else
late_initcall(max_dp_ser_module_init);
#endif
