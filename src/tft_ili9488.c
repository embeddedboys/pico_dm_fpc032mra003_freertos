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

#include "tft.h"
#include "debug.h"

#if LCD_DRV_USE_ILI9488

static int tft_ili9488_init_display(struct tft_priv *priv)
{
    pr_debug("%s, writing initial sequence...\n", __func__);
    priv->tftops->reset(priv);
    dm_gpio_set_value(priv->gpio.rd, 1);
    mdelay(150);

    write_reg(priv, 0xf7, 0xa9, 0x51, 0x2c, 0x82);

    write_reg(priv, 0xc0, 0x11, 0x09);

    write_reg(priv, 0xc1, 0x41);

    write_reg(priv, 0xc5, 0x00, 0x28, 0x80);

    write_reg(priv, 0xb1, 0xb0, 0x11);

    write_reg(priv, 0xb4, 0x02);

    write_reg(priv, 0xb6, 0x02, 0x22);

    write_reg(priv, 0xb7, 0xc6);

    write_reg(priv, 0xbe, 0x00, 0x04);

    write_reg(priv, 0xe9, 0x00);

    write_reg(priv, 0x36, 0x8 | (1 << 5) | (1 << 6));

    write_reg(priv, 0x3a, 0x55);

    write_reg(priv, 0xe0, 0x00, 0x07, 0x10, 0x09, 0x17, 0x0b, 0x41, 0x89, 0x4b, 0x0a, 0x0c, 0x0e, 0x18, 0x1b, 0x0f);

    write_reg(priv, 0xe1, 0x00, 0x17, 0x1a, 0x04, 0x0e, 0x06, 0x2f, 0x45, 0x43, 0x02, 0x0a, 0x09, 0x32, 0x36, 0x0f);

    write_reg(priv, 0x11);
    mdelay(60);
    write_reg(priv, 0x29);

    return 0;
}

static struct tft_display ili9488 = {
    .xres   = TFT_X_RES,
    .yres   = TFT_Y_RES,
    .bpp    = 16,
    .backlight = 100,
    .tftops = {
        .write_reg = tft_write_reg16,
        .init_display = tft_ili9488_init_display,
    },
};

int tft_driver_init(void)
{
    tft_probe(&ili9488);
    return 0;
}

#endif