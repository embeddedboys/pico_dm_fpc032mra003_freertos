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

#include <stdint.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/types.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"

#include "indev.h"
#include "debug.h"

#if INDEV_DRV_USE_NS2009

#define NS2009_ADDR       0x48
#define NS2009_DEF_SPEED  400000
#define NS2009_PIN_SCL    27
#define NS2009_PIN_SDA    26
#define NS2009_PIN_IRQ    21
#define NS2009_PIN_RST    18

#define NS2009_CMD_READ_X 0xC0
#define NS2009_CMD_READ_Y 0xD0

#define NS2009_DISABLE_IRQ (1 << 2)

enum {
    NS2009_RESOLUTION_8BIT = 8,
    NS2009_RESOLUTION_12BIT = 12,
};

enum {
    NS2009_POWER_MODE_NORMAL,
    NS2009_POWER_MODE_LOW_POWER,
};

extern int i2c_bus_scan(i2c_inst_t *i2c);

static void ns2009_write_reg(struct indev_priv *priv, uint8_t reg, uint8_t val)
{
    uint16_t buf = val << 8 | reg;
    i2c_write_blocking(priv->spec->i2c.master, priv->spec->i2c.addr, (uint8_t *)&buf, sizeof(buf), false);
}

// static uint16_t ns2009_read_reg(struct indev_priv *priv, uint8_t reg)
// {
//     uint8_t data_out[2];
//     uint16_t val = 0;

//     i2c_write_blocking(priv->i2c.master, priv->i2c.addr, &cmd, 1, true);
//     i2c_read_blocking(priv->i2c.master, priv->i2c.addr, data_out, 2, false);

//     if (priv->resolution == NS2009_RESOLUTION_12BIT)  {
//         val |= data_out[0] << 4;
//         val |= data_out[1] >> 4;
//     } else {
//         val = data_out[0];
//     }

//     return val;
// }

static uint8_t ns2009_read_reg(struct indev_priv *priv, uint8_t reg)
{
    uint8_t val;
    i2c_write_blocking(priv->spec->i2c.master, priv->spec->i2c.addr, &reg, 1, true);
    i2c_read_blocking(priv->spec->i2c.master, priv->spec->i2c.addr, &val, 1, false);
    return val;
}

uint16_t ns2009_read_x(struct indev_priv *priv)
{
    uint8_t val = read_reg(priv, NS2009_CMD_READ_X);

    if (priv->revert_x)
        return (priv->x_res - (val * priv->x_res) / (1 << priv->spec->resolution));
    
    return (val * priv->x_res) / (1 << priv->spec->resolution);
}

uint16_t ns2009_read_y(struct indev_priv *priv)
{
    uint8_t val = read_reg(priv, NS2009_CMD_READ_Y);

    if (priv->revert_y)
        return (priv->y_res - (val * priv->y_res) / (1 << priv->spec->resolution));
    
    return (val * priv->y_res) / (1 << priv->spec->resolution);
}

#define REAL_X(x) ((x * priv->y_res) / (1 << priv->spec->resolution))
#define REAL_Y(y) ((y * priv->x_res) / (1 << priv->spec->resolution))
#define TEST(v) (v | (0x1 << 2))
bool ns2009_is_pressed(struct indev_priv *priv)
{
    // printf("X- : %d  Y- : %d\n", REAL_X(read_reg(priv, TEST(0xA0))), REAL_Y(read_reg(priv, TEST(0xB0))));
    // printf("Z1- : %d  Z2- : %d\n", REAL_Y(read_reg(priv, TEST(0xC0))), REAL_X(read_reg(priv, TEST(0xD0))));
    // return false;
    // printf("%d\n", gpio_get(priv->spec->pin_irq));
    return !gpio_get(priv->spec->pin_irq);
}

static void ns2009_hw_init(struct indev_priv *priv)
{
    i2c_init(priv->spec->i2c.master, priv->spec->i2c.speed);
    gpio_set_function(priv->spec->i2c.pin_scl, GPIO_FUNC_I2C);
    gpio_set_function(priv->spec->i2c.pin_sda, GPIO_FUNC_I2C);

    gpio_pull_up(priv->spec->i2c.pin_scl);
    gpio_pull_up(priv->spec->i2c.pin_sda);

    gpio_init(priv->spec->pin_rst);
    gpio_set_dir(priv->spec->pin_rst, GPIO_OUT);
    gpio_pull_up(priv->spec->pin_rst);

    gpio_init(priv->spec->pin_irq);
    gpio_pull_up(priv->spec->pin_irq);
    gpio_set_dir(priv->spec->pin_irq, GPIO_IN);

    i2c_bus_scan(priv->spec->i2c.master);

    priv->ops->reset(priv);
    priv->ops->set_dir(priv, INDEV_DIR_REVERT_Y);
}

static struct indev_spec ns2009 = {
    .type = INDEV_TYPE_POINTER,

    .i2c = {
        .addr = NS2009_ADDR,
        .master = i2c1,
        .speed = NS2009_DEF_SPEED,
        .pin_scl = NS2009_PIN_SCL,
        .pin_sda = NS2009_PIN_SDA,
    },

    .x_res = TOUCH_X_RES,
    .y_res = TOUCH_Y_RES,
    .resolution = NS2009_RESOLUTION_8BIT,

    .pin_irq = NS2009_PIN_IRQ,
    .pin_rst = NS2009_PIN_RST,

    .ops = {
        .write_reg  = ns2009_write_reg,
        .read_reg   = ns2009_read_reg,
        .init       = ns2009_hw_init,
        .is_pressed = ns2009_is_pressed,
        .read_x     = ns2009_read_x,
        .read_y     = ns2009_read_y,
    },
};

int indev_driver_init(void)
{
    pr_debug("%s\n", __func__);
    indev_probe(&ns2009);
    return 0;
}

#endif