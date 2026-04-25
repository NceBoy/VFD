#include "string.h"
#include "eeprom.h"
#include "i2c.h"

uint8_t eeprom_init(void)
{
	I2C_Init();
	return EEPROM_ERR_NONE;
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))

uint8_t eeprom_write(uint32_t addr, const void* buf, uint16_t len)
{
    if ((buf == NULL) || (addr + len) > EEPROM_SIZE)
    {
        return EEPROM_ERR_ARGS;
    }

    uint8_t* p_data_buf = (uint8_t*)buf;
    uint16_t need_len = len;
    uint32_t w_addr = addr;

	uint8_t  tx_buf[EEPROM_ADDR_BYTES + EEPROM_PAGE_SIZE];	//临时的页面缓冲
	
	while (need_len > 0)
	{
        if(EEPROM_TYPE > EEPROM_TYPE_AT24C16) //采用2个字节寻址，直接填写写入地址即可
        {
            // fill addr
            for (uint8_t i = 0; i < EEPROM_ADDR_BYTES; i++)
            {
                tx_buf[EEPROM_ADDR_BYTES - i - 1] = (uint8_t)(w_addr >> (8 * i));
            }
        }
        else //采用一个字节寻址，高位存放在设备地址中
        {
            tx_buf[EEPROM_ADDR_BYTES - 1] = (uint8_t)(w_addr % 256);
        }

        uint16_t tx_data_len;
        if (w_addr % EEPROM_PAGE_SIZE)
        {
            tx_data_len = MIN(need_len, EEPROM_PAGE_SIZE - (w_addr % EEPROM_PAGE_SIZE));
        }
        else
        {
            tx_data_len = MIN(need_len, EEPROM_PAGE_SIZE);
        }

        memcpy(tx_buf + EEPROM_ADDR_BYTES, p_data_buf, tx_data_len);

        uint8_t err;
        if (EEPROM_TYPE > EEPROM_TYPE_AT24C16)
        {
            err = I2C_Trans(EEPROM_I2C_INDEX, EEPROM_I2C_ADDR, tx_buf, EEPROM_ADDR_BYTES + tx_data_len, NULL, 0, 3);
        }
        else
        {
            err = I2C_Trans(EEPROM_I2C_INDEX, EEPROM_I2C_ADDR + ((w_addr / 256) << 1), tx_buf, EEPROM_ADDR_BYTES + tx_data_len, NULL, 0, 3);
        }

        if (err != I2C_ERR_NONE)
        {
            return EEPROM_ERR_I2C;
        }

        p_data_buf += tx_data_len;
        need_len -= tx_data_len;
        w_addr += tx_data_len;
    }

    return EEPROM_ERR_NONE;
}

uint8_t eeprom_read(uint32_t addr, void* buf, uint16_t len)
{
    if ((buf == NULL) || (addr + len) > EEPROM_SIZE)
    {
        return EEPROM_ERR_ARGS;
    }

    uint8_t tx_buf[EEPROM_ADDR_BYTES];

    if (EEPROM_TYPE > EEPROM_TYPE_AT24C16)
    {
        // fill addr
        for (uint8_t i = 0; i < EEPROM_ADDR_BYTES; i++)
        {
            tx_buf[EEPROM_ADDR_BYTES - i - 1] = (uint8_t)(addr >> (8 * i));
        }
    }
    else
    {
        tx_buf[EEPROM_ADDR_BYTES - 1] = (uint8_t)(addr % 256);
    }

    uint8_t err;
    if (EEPROM_TYPE > EEPROM_TYPE_AT24C16)
    {
        err = I2C_Trans(EEPROM_I2C_INDEX, EEPROM_I2C_ADDR, tx_buf, EEPROM_ADDR_BYTES, (uint8_t*)buf, len, 3);
    }
    else
    {
        err = I2C_Trans(EEPROM_I2C_INDEX, EEPROM_I2C_ADDR + ((addr / 256) << 1), tx_buf, EEPROM_ADDR_BYTES, (uint8_t*)buf, len, 3);
    }

    return (err == I2C_ERR_NONE) ? EEPROM_ERR_NONE : EEPROM_ERR_I2C;
}
