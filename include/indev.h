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

#ifndef __INDEV_H
#define __INDEV_H

#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
// #include "hardware/spi.h"

#define TOUCH_X_RES LCD_HOR_RES
#define TOUCH_Y_RES LCD_VER_RES

#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof(arr[0]))
#define mdelay(v)   busy_wait_ms(v)
#define dm_gpio_set_value(p,v) gpio_put(p, v)

#define write_reg(priv, reg, val) \
    priv->ops->write_reg(priv, reg, val)

#define read_reg(priv, reg) \
    priv->ops->read_reg(priv, reg)

typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;

enum {
    INDEV_TYPE_POINTER = 0x00,
    INDEV_TYPE_KEYPAD  = 0x01,
};

typedef enum {
    INDEV_DIR_NOP       = 0x00,
    INDEV_DIR_REVERT_X  = 0x01,
    INDEV_DIR_REVERT_Y  = 0x02,
    INDEV_DIR_SWITCH_XY = 0x04,
} indev_direction_t;

struct indev_priv;

struct indev_ops {
    void    (*write_reg)(struct indev_priv *priv, u8 reg, u8 val);
    u8      (*read_reg)(struct indev_priv *priv, u8 reg);

    void    (*init)(struct indev_priv *priv);
    void    (*reset)(struct indev_priv *priv);
    void    (*set_dir)(struct indev_priv *priv, indev_direction_t dir);
    bool    (*is_pressed)(struct indev_priv *priv);
    u16     (*read_x)(struct indev_priv *priv);
    u16     (*read_y)(struct indev_priv *priv);
};

struct indev_spec {
    u8 type;

    struct {
        u8     addr;
        i2c_inst_t  *master;
        u32    speed;

        u8     pin_scl;
        u8     pin_sda;
    } i2c;

    struct {
        u32    speed;

        u8     pin_mosi;
        u8     pin_miso;
        u8     pin_cs;
        u8     pin_sck;
    } spi;

    u16            x_res;
    u16            y_res;

    /* for res-touch like */
    u8             resolution;

    u8             pin_irq;
    u8             pin_rst;

    struct indev_ops ops;
};

struct indev_priv {
    u16                 x_res;
    u16                 y_res;

    indev_direction_t   dir;
    bool                revert_x;
    bool                revert_y;

    struct indev_spec   *spec;
    struct indev_ops    *ops;
};

extern int indev_driver_init(void);
extern int indev_probe(struct indev_spec *spec);
extern void indev_set_dir(indev_direction_t dir);
extern bool indev_is_pressed(void);
extern u16 indev_read_x(void);
extern u16 indev_read_y(void);

#endif