// SPDX-License-Identifier: GPL-2.0
/*
 * Slim Bootloader(SBL) firmware update signaling driver
 *
 * Slim Bootloader is a small, open-source, non UEFI compliant, boot firmware
 * optimized for running on certain Intel platforms.
 *
 * SBL exposes an ACPI-WMI device via /sys/bus/wmi/<SBL_FWU_WMI_GUID>.
 * This driver further adds "firmware_update_request" device attribute.
 * This attribute normally has a value of 0 and userspace can signal SBL
 * to update firmware, on next reboot, by writing a value of 1.
 *
 * More details of SBL firmware update process is available at:
 * https://slimbootloader.github.io/security/firmware-update.html
 */

#include <linux/acpi.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sysfs.h>
#include <linux/wmi.h>

#define SBL_FWU_WMI_GUID  "44FADEB1-B204-40F2-8581-394BBDC1B651"

static int get_fwu_request(struct device *dev, u32 *out)
{
	struct acpi_buffer result = {ACPI_ALLOCATE_BUFFER, NULL};
	union acpi_object *obj;
	acpi_status status;

	status = wmi_query_block(SBL_FWU_WMI_GUID, 0, &result);
	if (ACPI_FAILURE(status)) {
		dev_err(dev, "wmi_query_block failed\n");
		return -ENODEV;
	}

	obj = (union acpi_object *)result.pointer;
	if (!obj || obj->type != ACPI_TYPE_INTEGER) {
		dev_warn(dev, "wmi_query_block returned invalid value\n");
		kfree(obj);
		return -EINVAL;
	}

	*out = obj->integer.value;
	kfree(obj);

	return 0;
}

static int set_fwu_request(struct device *dev, u32 in)
{
	struct acpi_buffer input;
	acpi_status status;
	u32 value;

	value = in;
	input.length = sizeof(u32);
	input.pointer = &value;

	status = wmi_set_block(SBL_FWU_WMI_GUID, 0, &input);
	if (ACPI_FAILURE(status)) {
		dev_err(dev, "wmi_set_block failed\n");
		return -ENODEV;
	}

	return 0;
}

static ssize_t firmware_update_request_show(struct device *dev,
					    struct device_attribute *attr,
					    char *buf)
{
	u32 val;
	int ret;

	ret = get_fwu_request(dev, &val);
	if (ret)
		return ret;

	return sprintf(buf, "%d\n", val);
}

static ssize_t firmware_update_request_store(struct device *dev,
					     struct device_attribute *attr,
					     const char *buf, size_t count)
{
	bool val;
	int ret;

	ret = kstrtobool(buf, &val);
	if (ret)
		return ret;

	ret = set_fwu_request(dev, val ? 1 : 0);
	if (ret)
		return ret;

	return count;
}

static DEVICE_ATTR_RW(firmware_update_request);

static struct attribute *firmware_update_attrs[] = {
	&dev_attr_firmware_update_request.attr,
	NULL
};

ATTRIBUTE_GROUPS(firmware_update);

static int sbl_fwu_wmi_probe(struct wmi_device *wdev, const void *context)
{
	dev_info(&wdev->dev, "Slim Bootloader signaling driver attached\n");
	return 0;
}

static void sbl_fwu_wmi_remove(struct wmi_device *wdev)
{
	dev_info(&wdev->dev, "Slim Bootloader signaling driver removed\n");
}

static const struct wmi_device_id sbl_fwu_wmi_id_table[] = {
	{ .guid_string = SBL_FWU_WMI_GUID },
	{}
};

static struct wmi_driver sbl_fwu_wmi_driver = {
	.driver = {
		.name = "sbl-fwu-wmi",
		.dev_groups = firmware_update_groups,
	},
	.probe = sbl_fwu_wmi_probe,
	.remove = sbl_fwu_wmi_remove,
	.id_table = sbl_fwu_wmi_id_table,
};

module_wmi_driver(sbl_fwu_wmi_driver);

MODULE_DEVICE_TABLE(wmi, sbl_fwu_wmi_id_table);
MODULE_AUTHOR("Jithu Joseph <jithu.joseph@intel.com>");
MODULE_DESCRIPTION("Slim Bootloader firmware update signaling driver");
MODULE_LICENSE("GPL v2");