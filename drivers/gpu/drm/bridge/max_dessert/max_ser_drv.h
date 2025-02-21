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

#ifndef __MAX_SER_DEV_h__
#define __MAX_SER_DEV_h__

#define USB_DEVICE 0
#define IGPU_PHUD_3840_720_DEVICE 1
#define IGPU_PHUD_1920_720_DEVICE 2
#define IGPU_LASER_3840_720_DEVICE 3
#define IGPU_HOLO_1920_1080_DEVICE 4
#define DGPU_CENTRAL_3840_720_DEVICE 5
#define DGPU_TRANS_3840_1080_DEVICE 6


enum max_ser_current_mode {
	MAX_MODE_IGPU_PHUD_3840_720,
	MAX_MODE_IGPU_PHUD_1920_720,
	MAX_MODE_IGPU_LASER_3840_720,
	MAX_MODE_IGPU_HOLO_1920_1080,
	MAX_MODE_DGPU_CENTRAL_3840_720,
	MAX_MODE_DGPU_TRANS_3840_1080,
};
enum max_ser_current_chip {
	MAX_96745,
};


#define MAX_DP_SER_CTRL3 0x13
#define MAX_DP_SER_CTRL3_LOCK_MASK (1 << 3)
#define MAX_DP_SER_CTRL3_LOCK_VAL (1 << 3)

#define MAX_DP_SER_CTRL3_ERROR_MASK (1 << 2)
#define MAX_DP_SER_CTRL3_ERROR_VAL (1 << 2)

#define MAX_DP_DEV_ID16_LSB 0x24
#define MAX_DP_DEV_ID16_LSB_96749_MASK 0x60
#define MAX_DP_DEV_ID16_LSB_96749_VAL 0x60

#define MAX_DP_SER_LCTRL1_A 0x29
#define MAX_DP_SER_LCTRL1_B 0x33
#define MAX_DP_SER_LCTRL1_RESET_ONESHOT 0x02

#define MAX_DP_SER_LCTRL2_A 0x2A
#define MAX_DP_SER_LCTRL2_B 0x34
#define MAX_DP_SER_LCTRL2_LOCK_MASK (1 << 0)
#define MAX_DP_SER_LCTRL2_LOCK_VAL 0x1

#define MAX_DP_SER_VID_TX_MASK (1 << 0)
#define MAX_DP_SER_VID_TX_LINK_MASK (3 << 1)
#define MAX_DP_SER_LINK_SEL_SHIFT_VAL 0x1

#define MAX_DP_SER_DPRX_TRAIN 0x641A
#define MAX_DP_SER_DPRX_TRAIN_STATE_MASK (0xF << 4)
#define MAX_DP_SER_DPRX_TRAIN_STATE_VAL 0xF0

#define MAX_DP_SER_VID_TX2_PCLK 0x0102
#define MAX_DP_SER_VID_TX2_PCLK_STATE_MASK (0x8 << 4)
#define MAX_DP_SER_VID_TX2_PCLK_STATE_VAL 0x80

#define MAX_DP_SER_VID_TX_X 0x100
#define MAX_DP_SER_VID_TX_Y 0x110
#define MAX_DP_SER_VID_TX_Z 0x120
#define MAX_DP_SER_VID_TX_U 0x130

#define MAX_GMSL_DP_SER_ENABLE_LINK_A 0x0
#define MAX_GMSL_DP_SER_ENABLE_LINK_B 0x1
#define MAX_GMSL_DP_SER_ENABLE_LINK_AB 0x2

enum max_state {
	MAX_STATE_UNLOCK,
	MAX_STATE_LOCK,
	MAX_STATE_TRAINING_SUCCESS,
};

int __init max_dp_ser_module_init(void);
void __exit max_dp_ser_module_exit(void);

#define LOG_DEBUG(dev, fmt, ...)                              \
do {                                                          \
	dev_dbg(dev, "[%s:%d] " fmt, __FUNCTION__, __LINE__,      \
		##__VA_ARGS__);                                       \
} while (0)

#define LOG_INFO(dev, fmt, ...)                               \
do {                                                          \
	dev_info(dev, "[%s:%d] " fmt, __FUNCTION__, __LINE__,     \
		##__VA_ARGS__);                                       \
} while (0)


#endif /* __MAX_SER_DRV__ */
