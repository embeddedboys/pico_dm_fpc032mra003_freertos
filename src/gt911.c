// Copyright (c) 2024 embeddedboys developers

// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:

// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <stdio.h>
#include <string.h>

#include "indev.h"
#include "debug.h"

#if INDEV_DRV_USE_GT911

#define GT911_PIN_SCL   27
#define GT911_PIN_SDA   26
#define GT911_PIN_RST   18
#define GT911_PIN_IRQ   21

#define GT911_CMD_WR    0x28
#define GT911_CMD_RD    0x29

#define GT911_REG_CTRL  0x8040  // GT911 Control Register
#define GT911_REG_CFGS  0x8047  // GT911 Configuration Start Address Regsiter
#define GT911_REG_CHCK  0x80FF  // GT911 Checksum Register
#define GT911_REG_PID   0x8140  // GT911 Product ID Register

#define GT911_REG_GSTID   0x814E  // GT911 Touch state
#define GT911_REG_TP1_X   0x8150  // Touch Point 1 X Data start Address
#define GT911_REG_TP1_Y   0x8152  // Touch Point 1 Y Data start Address
// #define GT911_REG_TP2   0x8158  // Touch Point 2 Data Address
// #define GT911_REG_TP3   0x8160  // Touch Point 3 Data Address
// #define GT911_REG_TP4   0x8168  // Touch Point 4 Data Address
// #define GT911_REG_TP5   0x8170  // Touch Point 5 Data Address

#define GT911_X_RES     LCD_HOR_RES
#define GT911_Y_RES     LCD_VER_RES

#define GT911_ADDR      0x14
// #define GT911_ADDR      0x5D
#define GT911_DEF_SPEED 400000

extern int i2c_bus_scan(i2c_inst_t *i2c);

// rp2040 i2c write byte
#define rp_i2c_wb(v) \
    i2c_write_blocking(priv->spec->i2c.master, priv->spec->i2c.addr, (uint8_t []){v}, 1, false);

// rp2040 i2c write byte non stop
#define rp_i2c_wb_ns(v) \
    i2c_write_blocking(priv->spec->i2c.master, priv->spec->i2c.addr, (uint8_t []){v}, 1, true);

#define rp_i2c_waddr(a, l) \
    i2c_write_blocking(priv->spec->i2c.master, priv->spec->i2c.addr, a, l, false);

#define rp_i2c_raddr(a, l) \
    i2c_read_blocking(priv->spec->i2c.master, priv->spec->i2c.addr, a, l, false);

static void gt911_write_addr16(struct indev_priv *priv, u16 reg, u8 *txbuf, u8 len)
{
    u8 buf[32] = {0};

    buf[0] = reg >> 8;
    buf[1] = reg & 0xFF;

    memcpy(buf + 2, txbuf, len);
    len+=2;

    // for (int i = 0; i < len; i++)
    //     printf("buf[%d] : 0x%x\n", i, buf[i]);

    i2c_write_blocking(priv->spec->i2c.master, priv->spec->i2c.addr, buf, len, false);
}

// static u8 gt911_read_reg16(struct indev_priv *priv, u16 reg)
// {
//     u8 buf[2];

//     buf[0] = reg >> 8;
//     buf[1] = reg & 0xFF;

//     i2c_write_blocking(priv->spec->i2c.master, priv->spec->i2c.addr, buf, 2, false);
//     i2c_read_blocking(priv->spec->i2c.master, priv->spec->i2c.addr, buf, 1, false);
    
//     return buf[0];
// }

static void gt911_read_addr16(struct indev_priv *priv, u16 reg, u8 *rxbuf, u8 len)
{
    u8 buf[2];

    buf[0] = reg >> 8;
    buf[1] = reg & 0xFF;

    i2c_write_blocking(priv->spec->i2c.master, priv->spec->i2c.addr, buf, 2, false);
    i2c_read_blocking(priv->spec->i2c.master, priv->spec->i2c.addr, rxbuf, len, false);
}

static uint16_t gt911_read_x(struct indev_priv *priv)
{
    u16 this_x = 0;

    read_addr16(priv, GT911_REG_TP1_X, (u8  *)&this_x, sizeof(this_x));
    pr_debug("this_x : %d\n", this_x);

    return this_x;
}

static uint16_t gt911_read_y(struct indev_priv *priv)
{
    u16 this_y = 0;

    read_addr16(priv, GT911_REG_TP1_Y, (u8  *)&this_y, sizeof(this_y));
    pr_debug("this_y : %d\n", this_y);

    return this_y;
}

static bool gt911_is_pressed(struct indev_priv *priv)
{
    u8 state = 0;
    gt911_read_addr16(priv, GT911_REG_GSTID, &state, 1);
    // printf("state : %d\n", state);
    gt911_write_addr16(priv, GT911_REG_GSTID, (u8 []){0x00}, 1);
    if (state & 0x80)
        return true;
    
    return false;
}

static void gt911_hw_init(struct indev_priv *priv)
{
    pr_debug("%s\n", __func__);

    pr_debug("initialzing i2c controller\n");
    i2c_init(priv->spec->i2c.master, GT911_DEF_SPEED);

    pr_debug("set gpio i2c function\n");
    gpio_init(priv->spec->i2c.pin_scl);
    gpio_init(priv->spec->i2c.pin_sda);
    gpio_set_function(priv->spec->i2c.pin_scl, GPIO_FUNC_I2C);
    gpio_set_function(priv->spec->i2c.pin_sda, GPIO_FUNC_I2C);

    pr_debug("pull up i2c gpio\n");
    gpio_pull_up(priv->spec->i2c.pin_scl);
    gpio_pull_up(priv->spec->i2c.pin_sda);

    pr_debug("initialzing reset pin\n");
    gpio_init(priv->spec->pin_rst);
    gpio_set_dir(priv->spec->pin_rst, GPIO_OUT);
    // gpio_pull_up(priv->spec->pin_rst);
    gpio_put(priv->spec->pin_rst, 0);

    pr_debug("set irq pin as output\n");
    gpio_init(priv->spec->pin_irq);
    gpio_set_dir(priv->spec->pin_irq, GPIO_OUT);
    gpio_pull_up(priv->spec->pin_irq);
    gpio_put(priv->spec->pin_irq, 1);
    udelay(150);

    pr_debug("chip reset\n");
    priv->ops->reset(priv);

    pr_debug("set irq pin as input\n");
    gpio_init(priv->spec->pin_irq);
    gpio_set_dir(priv->spec->pin_irq, GPIO_IN);
    gpio_pull_down(priv->spec->pin_irq);

    i2c_bus_scan(priv->spec->i2c.master);

    u8 temp[5];
    read_addr16(priv, GT911_REG_PID, temp, 4);
    pr_debug("Product ID : %s\n", (char *)temp);
    if (0 == strcmp((char *)temp, "911"))  {
        pr_debug("GT911 found\n");
        temp[0] = 0x02;
        write_addr16(priv, GT911_REG_CTRL, temp, 1);
        read_addr16(priv, GT911_REG_CFGS, temp, 1);
        pr_debug("Config reg : 0x%02x\n", temp[0]);
        if (temp[0] < 0x60)
            pr_debug("Default Ver : %d\n", temp[0]);

        mdelay(10);
        temp[0] = 0x00;
        write_addr16(priv, GT911_REG_CTRL, temp, 1);
    }

    // priv->ops->set_dir(priv, INDEV_DIR_SWITCH_XY | INDEV_DIR_INVERT_Y);
}

static struct indev_spec gt911 = {
    .type = INDEV_TYPE_POINTER,

    .i2c = {
        .addr = GT911_ADDR,
        .master = i2c1,
        .speed = GT911_DEF_SPEED,
        .pin_scl = GT911_PIN_SCL,
        .pin_sda = GT911_PIN_SDA,
    },

    .x_res = TOUCH_X_RES,
    .y_res = TOUCH_Y_RES,

    .pin_irq = GT911_PIN_IRQ,
    .pin_rst = GT911_PIN_RST,

    .ops = {
        .write_addr16 = gt911_write_addr16,
        // .read_reg16 = gt911_read_reg16,
        .read_addr16 = gt911_read_addr16,
        .init       = gt911_hw_init,
        .is_pressed = gt911_is_pressed,
        .read_x     = gt911_read_x,
        .read_y     = gt911_read_y,
    }
};

int indev_driver_init(void)
{
    printf("%s\n", __func__);
    indev_probe(&gt911);
    return 0;
}

#endif