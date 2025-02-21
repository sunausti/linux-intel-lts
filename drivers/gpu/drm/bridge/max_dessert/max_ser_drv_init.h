#ifndef __MAX_SER_DRV_INIT_H__
#define __MAX_SER_DRV_INIT_H__

#include <linux/workqueue.h>
#define MAX_ARRAY_SIZE 4

struct max_dp_ser_priv {
	struct device *dev;
	struct max_device *device_info;
	struct drm_privacy_screen *privacy_screen;
	u8 dprx_lane_count;
	u8 dprx_link_rate;
	int ser_lock_pin;
	int ser_pwrdn_pin;
	int lock_irq;
	bool enable_mst;
	u8 mst_payload_ids[MAX_ARRAY_SIZE];
	u8 gmsl_stream_ids[MAX_ARRAY_SIZE];
	u8 gmsl_link_select[MAX_ARRAY_SIZE];
	bool link_a_is_enabled;
	bool link_b_is_enabled;
	int current_mode;
	int current_chip;
	int backlight;
	int brightness;
	struct i2c_client *priv_client[2];
	struct backlight_device *bl_dev;
	struct proc_dir_entry *proc_file_entry;
	volatile int state;
	volatile int ctrl13;
	volatile int err_count;
	atomic_t ok_count;
	struct workqueue_struct *wq;
	struct delayed_work delay_work;
	struct work_struct oneshot_work;
	struct work_struct resume_work;
};
#endif

