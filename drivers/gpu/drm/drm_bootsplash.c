/* DRM internal client example */

#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/keyboard.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/workqueue.h>

#include <drm/drm_framebuffer.h>
#include <drm/drm_client.h>
#include <drm/drm_drv.h>
#include <drm/drm_fb_helper.h>
#include <drm/drm_fourcc.h>
#include <drm/drm_modes.h>
#include <drm/drm_print.h>
#include <drm/drm_gem.h>
#include <linux/dma-buf.h>
#include "i915/display/intel_fbdev.h"
#include "drm_bootsplash_bitmap.h"

// drm_lastclose()
#include "drm_internal.h"

struct drm_bootsplash {
	struct drm_client_dev client;
	struct mutex lock;
	struct work_struct worker;
	struct drm_client_buffer *buffers[2];
	bitmap_t bitmap;
	bool is_bitmap_load;
	bool started;
	bool stop;
};

static bool drm_bootsplash_key_init = false;
static bool drm_bootsplash_key_pressed;

int register_keyboard_notifier_one(struct notifier_block *nb)
{
	int ret = 0;
	if(drm_bootsplash_key_init == false) {
		ret = register_keyboard_notifier(nb);
		drm_bootsplash_key_init = true;
	}
	return ret;
}
int unregister_keyboard_notifier_one(struct notifier_block *nb)
{
	int ret = 0;
	if(drm_bootsplash_key_init == true) {
		ret = unregister_keyboard_notifier(nb);
		drm_bootsplash_key_init = false;
	}
	return ret;

}

static void drm_mode_print(struct drm_client_dev *client);

static int drm_bootsplash_keyboard_notifier_call(struct notifier_block *blk,
						 unsigned long code, void *_param)
{
	/* Any key is good */
	drm_bootsplash_key_pressed = true;

	return NOTIFY_OK;
}

static struct notifier_block drm_bootsplash_keyboard_notifier_block = {
	.notifier_call = drm_bootsplash_keyboard_notifier_call,
};

static void drm_bootsplash_buffer_delete(struct drm_bootsplash *splash)
{
	unsigned int i;

	for (i = 0; i < 2; i++) {
		if (!IS_ERR_OR_NULL(splash->buffers[i]))
			drm_client_framebuffer_delete(splash->buffers[i]);
		splash->buffers[i] = NULL;
	}
}

static int drm_bootsplash_buffer_create(struct drm_bootsplash *splash, u32 width, u32 height)
{
	unsigned int i;

	for (i = 0; i < 2; i++) {
		splash->buffers[i] = drm_client_framebuffer_create(&splash->client, width, height, DRM_FORMAT_XRGB8888);
		if (IS_ERR(splash->buffers[i])) {
			drm_bootsplash_buffer_delete(splash);
			return PTR_ERR(splash->buffers[i]);
		}
	}

	return 0;
}

static int drm_bootsplash_display_probe(struct drm_bootsplash *splash)
{
	struct drm_client_dev *client = &splash->client;
	unsigned int width = 0, height = 0;
	unsigned int num_non_tiled = 0, i;
	unsigned int modeset_mask = 0;
	struct drm_mode_set *modeset;
	bool tiled = false;
	int ret;

	ret = drm_client_modeset_probe(client, 0, 0);
	if (ret)
		return ret;

	mutex_lock(&client->modeset_mutex);

	drm_client_for_each_modeset(modeset, client) {
		if (!modeset->mode)
			continue;

		if (modeset->connectors[0]->has_tile)
			tiled = true;
		else
			num_non_tiled++;
	}

	if (!tiled && !num_non_tiled) {
		drm_bootsplash_buffer_delete(splash);
		ret = -ENOENT;
		goto out;
	}

	/* Assume only one tiled monitor is possible */
	if (tiled) {
		int hdisplay = 0, vdisplay = 0;

		i = 0;
		drm_client_for_each_modeset(modeset, client) {
			i++;
			if (!modeset->connectors[0]->has_tile)
				continue;

			if (!modeset->y)
				hdisplay += modeset->mode->hdisplay;
			if (!modeset->x)
				vdisplay += modeset->mode->vdisplay;
			modeset_mask |= BIT(i - 1);
		}

		width = hdisplay;
		height = vdisplay;

		goto trim;
	}

	/* The rest have one display (maybe cloned) per modeset, pick the largest */
	i = 0;
	drm_client_for_each_modeset(modeset, client) {
		i++;
		if (!modeset->mode || modeset->connectors[0]->has_tile)
			continue;

		if (modeset->mode->hdisplay * modeset->mode->vdisplay > width * height) {
			width = modeset->mode->hdisplay;
			height = modeset->mode->vdisplay;
			modeset_mask = BIT(i - 1);
		}
	}

trim:
	drm_mode_print(client);
	/* Remove unused modesets (warning in __drm_atomic_helper_set_config()) */
#if 0
	i = 0;
	drm_client_for_each_modeset(modeset, client) {
		unsigned int j;

		if (modeset_mask & BIT(i++))
			continue;
		drm_mode_destroy(client->dev, modeset->mode);
		modeset->mode = NULL;

		for (j = 0; j < modeset->num_connectors; j++) {
			drm_connector_put(modeset->connectors[j]);
			modeset->connectors[j] = NULL;
		}
		modeset->num_connectors = 0;
	}
#else 
	i = 0;
	drm_client_for_each_modeset(modeset, client) {
		unsigned int j;
		if (modeset && modeset->mode) {
			if (modeset->mode->hdisplay >=1920) {
				continue;
			}
		} else {
			continue;
		}
		drm_mode_destroy(client->dev, modeset->mode);
		modeset->mode = NULL;

		for (j = 0; j < modeset->num_connectors; j++) {
			drm_connector_put(modeset->connectors[j]);
			modeset->connectors[j] = NULL;
		}
		modeset->num_connectors = 0;
	}
#endif

	if (!splash->buffers[0] ||
	    splash->buffers[0]->fb->width != width ||
	    splash->buffers[0]->fb->height != height) {
		drm_bootsplash_buffer_delete(splash);
		DRM_DEBUG_KMS("drm_bootsplash_buffer_create width=%u height=%u\n",width, height);
		ret = drm_bootsplash_buffer_create(splash, width, height);
	}

out:
	mutex_unlock(&client->modeset_mutex);

	return ret;
}

static int drm_bootsplash_display_commit_buffer(struct drm_bootsplash *splash, unsigned int num)
{
	struct drm_client_dev *client = &splash->client;
	struct drm_mode_set *modeset;

	mutex_lock(&client->modeset_mutex);

	drm_client_for_each_modeset(modeset, client) {
		if (modeset->mode)
			modeset->fb = splash->buffers[num]->fb;
	}
	mutex_unlock(&client->modeset_mutex);

	return drm_client_modeset_commit(client);
}

static struct dma_buf *export_and_register_object_internal(struct drm_device *dev,
						  struct drm_gem_object *obj,
						  uint32_t flags)
{
	struct dma_buf *dmabuf = ERR_PTR(-ENOENT);
	/**
	 * i915_gem_prime_export
	*/
	if (obj->funcs && obj->funcs->export)
		dmabuf = obj->funcs->export(obj, flags);

	if (IS_ERR(dmabuf)) {
		/* normally the created dma-buf takes ownership of the ref,
		 * but if that fails then drop the ref
		 */
		return dmabuf;
	}


	return dmabuf;
}
static int i915_dma_buf_vmap_internal(struct dma_buf *dma_buf, struct iosys_map *map) {
	/**
	 * i915_gem_dmabuf_vmap
	*/
	if (dma_buf &&  dma_buf->ops && dma_buf->ops->vmap) {
		dma_buf->ops->vmap(dma_buf, map);
		return 0;
	} else {
		return -1;
	}
}

static int i915_dma_buf_vunmap_internal(struct dma_buf *dma_buf, struct iosys_map *map) {
	if (dma_buf &&  dma_buf->ops && dma_buf->ops->vunmap) {
		dma_buf->ops->vunmap(dma_buf, map);
		return 0;
	} else {
		return -1;
	}
}

static u32 drm_bootsplash_color_table[3] = {
	0x00ff0000, 0x0000ff00, 0x000000ff,
};

static void color_table_to_dmabuf(u32 *table, int index, struct iosys_map *dst, uint32_t pitches_0)
{
	unsigned int x, y;
	int len;
	static u32 buf[128];

	for (x = 0; x < 128; x ++) {
		buf[x] =  table[index];
	}

	if(!iosys_map_is_null(dst)) {
		len = 128 * 4;
		for (y = 0; y < 512; y++) {
			iosys_map_memcpy_to(dst, 0, buf, len);
			iosys_map_memcpy_to(dst, 2*len, buf, len);
			iosys_map_memcpy_to(dst, 4*len, buf, len);
			iosys_map_incr(dst, pitches_0);
		}
	} else {
		DRM_DEBUG_KMS("%s: dst is NULL\n",__func__);
	}
}

/* Draw a box with changing colors */
static void drm_bootsplash_draw_box(struct drm_bootsplash *splash, struct drm_client_buffer *buffer, unsigned int sequence)
{
	unsigned int width = buffer->fb->width;
	unsigned int height = buffer->fb->height;
	unsigned int pitches_0 = buffer->fb->pitches[0];

	int ret;

	struct dma_buf *dma_buf;
	struct iosys_map map;

	dma_buf = export_and_register_object_internal(buffer->client->dev, buffer->gem, 0);
	if (IS_ERR(dma_buf)) {
		DRM_DEBUG_KMS("%s: export_and_register_object_internal error\n", __func__);
		goto out;
	}

	ret = i915_dma_buf_vmap_internal(dma_buf, &map);

	if (ret) {
		DRM_DEBUG_KMS("%s: drm_gem_dmabuf_vmap ret=%d\n", __func__, ret);
		goto out;
	}
	DRM_DEBUG_KMS("%s: pitches_0=%u width=%u height=%u\n", __func__, pitches_0, width, height);
	if (splash->is_bitmap_load) {
		bitmap_to_dmabuf(&splash->bitmap, &map, pitches_0);
	} else {
		color_table_to_dmabuf(drm_bootsplash_color_table, sequence, &map, pitches_0);
	}
	i915_dma_buf_vunmap_internal(dma_buf, &map);
out:
	return;
}

static int drm_bootsplash_draw(struct drm_bootsplash *splash, unsigned int sequence, unsigned int buffer_num)
{
	if (!splash->buffers[buffer_num])
		return -ENOENT;

	DRM_DEBUG_KMS("draw: buffer_num=%u, sequence=%u\n", buffer_num, sequence);

	drm_bootsplash_draw_box(splash, splash->buffers[buffer_num], sequence);

	return drm_bootsplash_display_commit_buffer(splash, buffer_num);
}

static void drm_bootsplash_worker(struct work_struct *work)
{
	struct drm_bootsplash *splash = container_of(work, struct drm_bootsplash, worker);
	struct drm_client_dev *client = &splash->client;
	struct drm_device *dev = client->dev;
	unsigned int buffer_num = 0, sequence = 0;
	bool stop = false;
	int ret = 0;

    while (!drm_bootsplash_key_pressed) {
		mutex_lock(&splash->lock);

		stop = splash->stop;

		buffer_num = !buffer_num;

		ret = drm_bootsplash_draw(splash, sequence, buffer_num);

		DRM_DEV_DEBUG_KMS(dev->dev, "Bootsplash show pic ok ret %d\n",
				ret);

		mutex_unlock(&splash->lock);

		if (stop || ret == -ENOENT || ret == -EBUSY)
			break;

		if (++sequence == 2)
			break;

		msleep(500);
	}

	/* Restore fbdev (or other) on key press. */
	/* TODO: Check if it's OK to call drm_lastclose here. */
	if (drm_bootsplash_key_pressed && !splash->stop)
		drm_lastclose(dev);

	drm_bootsplash_buffer_delete(splash);

	DRM_DEV_DEBUG_KMS(dev->dev, "Bootsplash has stopped (key=%u stop=%u, ret=%d).\n",
			  drm_bootsplash_key_pressed, splash->stop, ret);
}

static int drm_bootsplash_client_hotplug(struct drm_client_dev *client)
{
	struct drm_bootsplash *splash = container_of(client, struct drm_bootsplash, client);
	int ret = 0;

	DRM_DEBUG_KMS("%s: key_pressed=%u, start=%u stop=%u\n", 
	__func__, drm_bootsplash_key_pressed, splash->started, splash->stop);
	drm_bootsplash_key_pressed = 0;
	splash->started = 0;
	splash->stop = 0;

	if (drm_bootsplash_key_pressed)
		return 0;

	mutex_lock(&splash->lock);

	if (splash->stop)
		goto out_unlock;

	ret = drm_bootsplash_display_probe(splash);
	if (ret < 0) {
		if (splash->started && ret == -ENOENT)
			splash->stop = true;
		goto out_unlock;
	}

	/*
	 * TODO: Deal with rotated panels, might have to sw rotate
	if (drm_client_panel_rotation(...))
	 */

	if (!splash->started) {
		splash->started = true;
		schedule_work(&splash->worker);
	}

out_unlock:
	mutex_unlock(&splash->lock);

	return ret;
}

static void drm_bootsplash_client_unregister(struct drm_client_dev *client)
{
	struct drm_bootsplash *splash = container_of(client, struct drm_bootsplash, client);

	DRM_DEBUG_KMS("%s: IN\n", __func__);

	mutex_lock(&splash->lock);
	splash->stop = true;
	mutex_unlock(&splash->lock);

	flush_work(&splash->worker);

	drm_client_release(client);

	if (splash->is_bitmap_load) {
		bitmap_release(&splash->bitmap);
	}
	splash->is_bitmap_load = 0;

	kfree(splash);

	unregister_keyboard_notifier_one(&drm_bootsplash_keyboard_notifier_block);

	DRM_DEBUG_KMS("%s: OUT\n", __func__);
}

static const struct drm_client_funcs drm_bootsplash_client_funcs = {
	.owner		= THIS_MODULE,
	.unregister	= drm_bootsplash_client_unregister,
	.hotplug	= drm_bootsplash_client_hotplug,
};

void drm_bootsplash_client_register(struct drm_device *dev)
{
	struct drm_bootsplash *splash;
	int ret;

	splash = kzalloc(sizeof(*splash), GFP_KERNEL);
	if (!splash)
		return;

	ret = drm_client_init(dev, &splash->client, "bootsplash", &drm_bootsplash_client_funcs);
	if (ret) {
		DRM_DEV_ERROR(dev->dev, "Failed to create client, ret=%d\n", ret);
		kfree(splash);
		return;
	}

	mutex_init(&splash->lock);
	
	DRM_DEBUG_KMS("%s before load firmware\n", __func__);
	ret = bitmap_load(dev, &splash->bitmap, "bitmap/default.bmp");
	if (ret < 0) {
		splash->is_bitmap_load = false;
	} else {
		splash->is_bitmap_load = true;
	}
	DRM_DEBUG_KMS("%s after load firmware is_bitmap_load: %d\n", __func__, splash->is_bitmap_load);

	INIT_WORK(&splash->worker, drm_bootsplash_worker);

	register_keyboard_notifier_one(&drm_bootsplash_keyboard_notifier_block);

	drm_client_register(&splash->client);
}

static void drm_mode_print(struct drm_client_dev *client)
{
	struct drm_mode_set *modeset;
	int i = 0;
	drm_client_for_each_modeset(modeset, client) {
		i++;
		if (!modeset->x&&!modeset->y){
			if (modeset->mode) {
				DRM_DEBUG_KMS("for all %s i=%d modeset->num_connectors=%ld width=%u height=%u\n",
						__func__, i, modeset->num_connectors, modeset->mode->hdisplay, modeset->mode->vdisplay);
			} else {
				DRM_DEBUG_KMS("for all %s i=%d modeset->num_connectors=%ld mode null\n",
						__func__, i, modeset->num_connectors);

			}
		}
	}
}
