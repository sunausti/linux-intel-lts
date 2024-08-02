// SPDX-License-Identifier: GPL-2.0-or-later
#include <linux/firmware.h>
#include <linux/slab.h>
#include <drm/drm_print.h>
#include <drm/drm_device.h>
#include <linux/iosys-map.h>

#include "drm_bootsplash_bitmap.h"

static char bitmap_firmware[PATH_MAX] = "bitmap/default.bmp";
module_param_string(bitmap_firmware, bitmap_firmware, sizeof(bitmap_firmware), 0644);
MODULE_PARM_DESC(bitmap_firmware, "Do not probe monitor, use specified EDID blob "
	"from built-in data or /lib/firmware instead. ");

int bitmap_load(struct drm_device *dev, bitmap_t* bitmap, const char *name)
{
	const struct firmware *fw = NULL;
	const u8 *fwdata;
	int fwsize, builtin;
	if (bitmap == NULL)
		return -1;

	{
		int err;

		err = request_firmware_direct(&fw, name, dev->dev);
		if (err) {
			drm_err(dev,
				"Requesting EDID firmware \"%s\" failed (err=%d)\n",
				name, err);
			return err;
		}

		fwdata = fw->data;
		fwsize = fw->size;
	}

	drm_dbg_kms(dev, "Loaded firmware EDID \"%s\"\n", name);

	bitmap->fw = (void*) fw;

	bitmap_create(bitmap, fwdata, fwsize);

	return 0;
}

void bitmap_release(bitmap_t* bitmap)
{
	bitmap_t *ret = bitmap;
	ret->width = 0;
	ret->height = 0;
	ret->buf = NULL;
	ret->image_bytes = NULL;
    if (ret->fw) {
	    release_firmware((struct firmware *)(ret->fw));
    }
}

/*
 * Simmple bitmap library used in kernel
 *
 **/
void bitmap_create(bitmap_t *bitmap, const uint8_t *buf, int size)
{
	bitmap_t *ret = bitmap;

	// Parse the bitmap
	bmp_fileheader_t * h = (bmp_fileheader_t *)buf;
	uint32_t offset = h->bfOffBits;
	//qemu_printf("bitmap size: %u\n", h->bfSize);
	//qemu_printf("bitmap offset: %u\n", offset);

	bmp_infoheader_t * info = 
		(bmp_infoheader_t*)(buf + sizeof(bmp_fileheader_t));

	ret->width = info->biWidth;
	ret->height = info->biHeight;
	ret->image_bytes= (uint8_t*) buf + offset;
	ret->buf = (uint8_t *) buf;
	ret->total_size= size;
	ret->bpp = info->biBitCount;

	DRM_DEBUG_KMS("image is here: %p\n", ret->image_bytes);
	DRM_DEBUG_KMS("image width %u height %u\n", ret->width, ret->height);
	DRM_DEBUG_KMS("image size %u\n", ret->total_size);
	DRM_DEBUG_KMS("image bpp %u\n", ret->bpp);
}

void bitmap_to_dmabuf(
		bitmap_t *bmp, struct iosys_map *map, uint32_t pitches_0)
{
	uint8_t * image;
	if (!bmp) return;
	if (!bmp->buf) return;
	if (iosys_map_is_null(map)) return;

	image = bmp->image_bytes;
	// Do copy
	// Copy the ith row of image to height - 1 - i row of frame buffer, 
	// each row is of length width * 3
	for(int i = 0; i < bmp->height; i++) {
		char * image_row = 
			image + (bmp->height - 1 - i) * bmp->width * (bmp->bpp / 8);
		uint32_t * framebuffer_row = (uint32_t*) map->vaddr;
		int j = 0;
		for(int k = 0; k < bmp->width; k++) {
			uint32_t b = image_row[j++] & 0xff;
			uint32_t g = image_row[j++] & 0xff;
			uint32_t r = image_row[j++] & 0xff;
			uint32_t rgb = ((r << 16) | (g << 8) | (b)) & 0x00ffffff;
			rgb = rgb | 0xff000000;
			framebuffer_row[k] = rgb;
		}
		iosys_map_incr(map, pitches_0);
	}
}
