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

#if LCD_DRV_USE_R61581

#define R61581_HSPL         0       /*HSYNC signal polarity*/
#define R61581_HSL          10      /*HSYNC length (Not Implemented)*/
#define R61581_HFP          10      /*Horitontal Front poarch (Not Implemented)*/
#define R61581_HBP          10      /*Horitontal Back poarch (Not Implemented */
#define R61581_VSPL         0       /*VSYNC signal polarity*/
#define R61581_VSL          10      /*VSYNC length (Not Implemented)*/
#define R61581_VFP          8       /*Vertical Front poarch*/
#define R61581_VBP          8       /*Vertical Back poarch */
#define R61581_DPL          0       /*DCLK signal polarity*/
#define R61581_EPL          1       /*ENABLE signal polarity*/
#define R61581_ORI          0       /*0, 180*/
#define R61581_LV_COLOR_DEPTH 16    /*Fix 16 bit*/

#if 1
static int tft_r61581_init_display(struct tft_priv *priv)
{
    pr_debug("%s, writing initial sequence...\n", __func__);
    priv->tftops->reset(priv);
    dm_gpio_set_value(priv->gpio.rd, 1);
    mdelay(150);

    write_reg(priv, 0xB0, 0x00);
    write_reg(priv, 0xB3, 0x02, 0x00, 0x00, 0x00);

    /* Backlight control */

    write_reg(priv, 0xC0, 0x13, 0x3B, 0x00, 0x02, 0x00, 0x01, 0x00, 0x43);
    write_reg(priv, 0xC1, 0x08, 0x16, 0x08, 0x08);
    write_reg(priv, 0xC4, 0x11, 0x07, 0x03, 0x03);
    write_reg(priv, 0xC6, 0x00);
    write_reg(priv, 0xC8, 0x03, 0x03, 0x13, 0x5C, 0x03, 0x07, 0x14, 0x08, 0x00, 0x21, 0x08, 0x14, 0x07, 0x53, 0x0C, 0x13, 0x03, 0x03, 0x21, 0x00);
    write_reg(priv, 0x0C, 0x55);
    write_reg(priv, 0x38);
    write_reg(priv, 0x3A, 0x55);
    write_reg(priv, 0xD0, 0x07, 0x07, 0x1D, 0x03);
    write_reg(priv, 0xD1, 0x03, 0x30, 0x10);
    write_reg(priv, 0xD2, 0x03, 0x14, 0x04);

    write_reg(priv, 0x11);
    mdelay(10);
    write_reg(priv, 0x29);
}
#else
static int tft_r61581_init_display(struct tft_priv *priv)
{
    pr_debug("%s, writing initial sequence...\n", __func__);
    priv->tftops->reset(priv);
    dm_gpio_set_value(priv->gpio.rd, 1);
    mdelay(10);

    write_reg(priv, 0xB0, 0x00);

    write_reg(priv, 0xB3, 0x02, 0x00, 0x00, 0x10);

    write_reg(priv, 0xB4, 0x00);

    // Backlight PWM
    write_reg(priv, 0xB9, 0x01, 0xFF, 0xFF, 0x18);

    /*Panel Driving Setting*/
    write_reg(priv, 0xC0, 0x02, 0x3B, 0x00, 0x00, 0x00, 0x01, 0x00, 0x43);

    /*Display Timing Setting for Normal Mode */
    write_reg(priv, 0xC1, 0x08, 0x15, R61581_VFP, R61581_VBP);

    /*Source/VCOM/Gate Driving Timing Setting*/
    write_reg(priv, 0xC4, 0x15, 0x03, 0x03, 0x01);

    /* Interface setting */
    write_reg(priv, 0xC6, 0x00);

    /* Gamma set */
    write_reg(priv, 0xC8, 0x0C, 0x05, 0x0A, 0x6B, 0x04, 0x06, 0x15, 0x10, 0x00, 0x31);

    /* Rotation set */
    write_reg(priv, 0x36, 0x00);

    write_reg(priv, 0x0C, 0x55);

    write_reg(priv, 0x3A, 0x55);

    write_reg(priv, 0x38);

    write_reg(priv, 0xD0, 0x07, 0x07, 0x14, 0xA2);

    write_reg(priv, 0xD1, 0x03, 0x5A, 0x10);

    write_reg(priv, 0xD2, 0x03, 0x04, 0x04);

    /* Sleep out */
    write_reg(priv, 0x11);
    mdelay(10);

    /* Display on */
    write_reg(priv, 0x29);
    mdelay(5);
}
#endif

static int tft_clear(struct tft_priv *priv, u16 clear)
{
    u32 width =  priv->display->xres;
    u32 height = priv->display->yres;

    clear = (clear << 8) | (clear >> 8);

    pr_debug("clearing screen (%d x %d) with color 0x%x\n",
        width, height, clear);

    priv->tftops->set_addr_win(priv, 0, 0, width, height);
    
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            write_buf_rs(priv, &clear, sizeof(u16), 1);
        }
    }

    return 0;
}

static void tft_video_sync(struct tft_priv *priv, int xs, int ys, int xe, int ye, void *vmem, size_t len)
{
    //pr_debug("video sync: xs=%d, ys=%d, xe=%d, ye=%d, len=%d\n", xs, ys, xe, ye, len);
    priv->tftops->set_addr_win(priv, xs, ys, xe, ye);

    /* 
     * r61851 color order is always BGR and not supported for changing.
     * So simply reverse bit order here for the whole buffer.
     * This really Influence the performance, but it's the only way to do it for now.
     * May be we should check pico-sdk for a better way to do it.
     */
    u16 *p = (u16 *)vmem;
    for (size_t i = 0; i < len; i++)
        p[i] = (p[i] << 8) | (p[i] >> 8);

    write_buf_rs(priv, vmem, len * 2, 1);
}

static struct tft_display r61581 = {
    .xres   = TFT_X_RES,
    .yres   = TFT_Y_RES,
    .bpp    = 16,
    .backlight = 100,
    .tftops = {
        .write_reg = tft_write_reg8,
        .init_display = tft_r61581_init_display,
        .clear = tft_clear,
        .video_sync = tft_video_sync,
    },
};

int tft_driver_init(void)
{
    tft_probe(&r61581);
    return 0;
}

#endif