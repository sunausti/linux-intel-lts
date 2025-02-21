#ifndef __I2C_CMD_IGPU_PHUD_1920_720_H__
#define __I2C_CMD_IGPU_PHUD_1920_720_H__

#include "i2c_cmd.h"

#define I2C_SER_ADDR 0x80
#define I2C_DES_ADDR 0x90

/*
 * 1920x720
 */

struct i2c_write_cmd dp_igpu_phud_1920_720_reset[] = {
};

struct i2c_write_cmd dp_igpu_phud_1920_720_ser[] = {
	//#reset HPD_CTRL0
	WD(I2C_SER_ADDR, 0x6444, 0x00, 0),
	//#Ensure the VID Disable,,
	WD(I2C_SER_ADDR, 0x100, 0x60, 0),
	//#Disable LINK_ENABLE,,
	WD(I2C_SER_ADDR, 0x7000, 0x00, 0),
	//##Disable reporting MST Capability,,
	WD(I2C_SER_ADDR, 0x7019, 0x00, 0),
	//#Set AUX_RD_INTERVAL to 16ms,,
	WD(I2C_SER_ADDR, 0x70A0, 0x04, 0),
	//#Set MAX_LINK_RATE to 5.4Gb/s,,
	WD(I2C_SER_ADDR, 0x7074, 0x14, 0),
	//#Set MAX_LINK_COUNT to 4,,
	WD(I2C_SER_ADDR, 0x7070, 0x04, 0),
	//#Set PCLK_RATE as 461MHz,,
	WD(I2C_SER_ADDR, 0x6424, 0xAA, 0),
	//#Enable LINK_ENABLE,,
	WD(I2C_SER_ADDR, 0x7000, 0x01, 0),
	//#Disable FEC,,
	WD(I2C_SER_ADDR, 0x50, 0x64, 0),
	//#Ensure the VID enabled,,
	WD(I2C_SER_ADDR, 0x100, 0x61, 0),
	//#Set GMSL rate to 3Gbps,,
	WD(I2C_SER_ADDR, 0x28, 0x84, 0),
	WD(I2C_SER_ADDR, 0x29, 0x02, 100),
};

struct i2c_write_cmd dp_igpu_phud_1920_720_des[] = {
};
#endif
