#ifndef __I2C_CMD_h__
#define __I2C_CMD_h__

struct i2c_write_cmd {
	u8 i2c_addr;
	u16 i2c_reg;
	u8 i2c_val;
	u16 i2c_delay_ms;
};

#define WD(addr, reg, val, delay)                               \
	{                                                       \
		(u8)(addr), (u16)(reg), (u8)(val), (u16)(delay) \
	}

#endif
