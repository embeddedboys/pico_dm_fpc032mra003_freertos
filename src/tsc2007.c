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

#if INDEV_DRV_USE_TSC2007

#define TSC2007_ADDR       0x48
#define TSC2007_DEF_SPEED  400000
#define TSC2007_PIN_SCL    27
#define TSC2007_PIN_SDA    26
#define TSC2007_PIN_IRQ    21
#define TSC2007_PIN_RST    29

#define TSC2007_CMD_READ_X 0xC0
#define TSC2007_CMD_READ_Y 0xD0

#define TSC2007_DISABLE_IRQ (1 << 2)

enum {
    TSC2007_RESOLUTION_8BIT = 8,
    TSC2007_RESOLUTION_12BIT = 12,
};

enum {
    TSC2007_POWER_MODE_NORMAL,
    TSC2007_POWER_MODE_LOW_POWER,
};

extern int i2c_bus_scan(i2c_inst_t *i2c);

static void tsc2007_write_reg(struct indev_priv *priv, uint8_t reg, uint8_t val)
{
    uint16_t buf = val << 8 | reg;
    i2c_write_blocking(priv->spec->i2c.master, priv->spec->i2c.addr, (uint8_t *)&buf, sizeof(buf), false);
}

// static uint16_t tsc2007_read_reg(struct indev_priv *priv, uint8_t reg)
// {
//     uint8_t data_out[2];
//     uint16_t val = 0;

//     i2c_write_blocking(priv->i2c.master, priv->i2c.addr, &cmd, 1, true);
//     i2c_read_blocking(priv->i2c.master, priv->i2c.addr, data_out, 2, false);

//     if (priv->resolution == TSC2007_RESOLUTION_12BIT)  {
//         val |= data_out[0] << 4;
//         val |= data_out[1] >> 4;
//     } else {
//         val = data_out[0];
//     }

//     return val;
// }

static uint8_t tsc2007_read_reg(struct indev_priv *priv, uint8_t reg)
{
    uint8_t val;
    i2c_write_blocking(priv->spec->i2c.master, priv->spec->i2c.addr, &reg, 1, true);
    i2c_read_blocking(priv->spec->i2c.master, priv->spec->i2c.addr, &val, 1, false);
    return val;
}

uint16_t tsc2007_read_x(struct indev_priv *priv)
{
    uint8_t val = read_reg(priv, TSC2007_CMD_READ_X);
    u16 this_x = 0;

    if (priv->invert_x)
        this_x = (priv->x_res - (val * priv->x_res) / (1 << priv->spec->resolution));
    else
        this_x = (val * priv->x_res) / (1 << priv->spec->resolution);

    pr_debug("x : %d, sc_x : %f\n", this_x, priv->sc_x);
    this_x += priv->spec->x_offs;
    this_x *= priv->sc_x;
    pr_debug("x : %d, sc_x : %f\n", this_x, priv->sc_x);

    return this_x;
}

uint16_t tsc2007_read_y(struct indev_priv *priv)
{
    uint8_t val = read_reg(priv, TSC2007_CMD_READ_Y);
    u16 this_y = 0;

    if (priv->invert_y)
        this_y = (priv->y_res - (val * priv->y_res) / (1 << priv->spec->resolution));
    else
        this_y = (val * priv->y_res) / (1 << priv->spec->resolution);

    pr_debug("y : %d, sc_y : %f\n", this_y, priv->sc_y);
    this_y += priv->spec->y_offs;
    this_y *= priv->sc_y;
    pr_debug("y : %d, sc_y : %f\n", this_y, priv->sc_y);

    return this_y;
}

// #define REAL_X(x) ((x * priv->y_res) / (1 << priv->spec->resolution))
// #define REAL_Y(y) ((y * priv->x_res) / (1 << priv->spec->resolution))
// #define TEST(v) (v | (0x1 << 2))
bool tsc2007_is_pressed(struct indev_priv *priv)
{
    // printf("X- : %d  Y- : %d\n", REAL_X(read_reg(priv, TEST(0xA0))), REAL_Y(read_reg(priv, TEST(0xB0))));
    // printf("Z1- : %d  Z2- : %d\n", REAL_Y(read_reg(priv, TEST(0xC0))), REAL_X(read_reg(priv, TEST(0xD0))));
    // return false;
    // printf("%d\n", gpio_get(priv->spec->pin_irq));
    return !gpio_get(priv->spec->pin_irq);
}

static void tsc2007_hw_init(struct indev_priv *priv)
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
    priv->ops->set_dir(priv, INDEV_DIR_SWITCH_XY | INDEV_DIR_INVERT_Y | INDEV_DIR_INVERT_X);
}

static struct indev_spec tsc2007 = {
    .name = "tsc2007",
    .type = INDEV_TYPE_POINTER,

    .i2c = {
        .addr = TSC2007_ADDR,
        .master = i2c1,
        .speed = TSC2007_DEF_SPEED,
        .pin_scl = TSC2007_PIN_SCL,
        .pin_sda = TSC2007_PIN_SDA,
    },

    .x_res  = 410,  // 25 ~ 435 --- 0 ~ 410
    .y_res  = 275,  // 25 ~ 300 --- 0 ~ 275
    .x_offs = -25,
    .y_offs = -25,
    .resolution = TSC2007_RESOLUTION_8BIT,

    .pin_irq = TSC2007_PIN_IRQ,
    .pin_rst = TSC2007_PIN_RST,

    .ops = {
        .write_reg  = tsc2007_write_reg,
        .read_reg   = tsc2007_read_reg,
        .init       = tsc2007_hw_init,
        .is_pressed = tsc2007_is_pressed,
        .read_x     = tsc2007_read_x,
        .read_y     = tsc2007_read_y,
    },
};

int indev_driver_init(void)
{
    pr_debug("%s\n", __func__);
    indev_probe(&tsc2007);
    return 0;
}

#endif