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

#if LCD_DRV_USE_1P5623

/* This 1p5623 panel is based on R61581 but need some specifical settings */
static int tft_1p5623_init_display(struct tft_priv *priv)
{
    pr_debug("%s, writing initial sequence...\n", __func__);
    priv->tftops->reset(priv);
    dm_gpio_set_value(priv->gpio.rd, 1);
    mdelay(120);

    write_reg(priv, 0x11);
    mdelay(20);

    // VCI1  VCL  VGH  VGL DDVDH VREG1OUT power amplitude setting
    write_reg(priv, 0xD0, 0x07, 0x42, 0x1D);

    // VCOMH VCOM_AC amplitude setting
    write_reg(priv, 0xD1, 0x00, 0x1A, 0x09);

    // Operational Amplifier Circuit Constant Current Adjust , charge pump frequency setting
    write_reg(priv, 0xD2, 0x01, 0x22);

    // REV SM GS
    write_reg(priv, 0xC0, 0x10, 0x3B, 0x00, 0x02, 0x11);

    // Frame rate setting = 72HZ  when setting 0x03
    write_reg(priv, 0xC5, 0x03);

    // Gamma setting
    write_reg(priv, 0xC8, 0x00, 0x25, 0x21, 0x05, 0x00, 0x0A, 0x65, 0x25, 0x77, 0x50, 0x0F, 0x00);

    // Get_display_mode (0Dh)
    write_reg(priv, 0x0D, 0x00, 0x00);

    // LSI Test Registers
    write_reg(priv, 0xF8, 0x01);
    write_reg(priv, 0xFE, 0x00, 0x02);

    // Exit invert mode
    write_reg(priv, 0x20);

    /* 
     * Switch Page/Column and Set BGR order
     * As I said before, this panel is based on R61581,
     * and according to the manual, the RGB/BGR order
     * not supported to change, but here we can set it.
     * I don't know why, but it works.
     */
    write_reg(priv, 0x36, (1 << 5) | (1 << 3));

    write_reg(priv, 0x3A, 0x55);

    write_reg(priv, 0x29);
    write_reg(priv, 0x21);

    return 0;
}

#if LCD_PIN_DB_COUNT == 8
static void tft_video_sync(struct tft_priv *priv, int xs, int ys, int xe, int ye, void *vmem, size_t len)
{
    // printf("video sync: xs=%d, ys=%d, xe=%d, ye=%d, len=%d\n", xs, ys, xe, ye, len);
    priv->tftops->set_addr_win(priv, xs, ys, xe, ye);

    /* 
     * 8080 8-bit Data Bus for 16-bit/pixel (RGB 5-6-5 Bits Input)
     *      DB 7     R4  G2
     *      DB 6     R3  G1
     *      DB 5     R2  G0
     *      DB 4     R1  B4
     *      DB 3     R0  B3
     *      DB 2     G5  B2
     *      DB 1     G4  B1
     *      DB 0     G3  B0
     * But a 16-bit Data Bus RGB565 order like this:
     * B0 - B4, G0 - G5, R0 - R4 from DB0 to DB16
     * So simply swap 2 bytes here from pixel buffer.
     */
    u16 *p = (u16 *)vmem;
    for (size_t i = 0; i < len; i++)
        p[i] = (p[i] << 8) | (p[i] >> 8);

    write_buf_rs(priv, vmem, len * 2, 1);
}
#endif

static struct tft_display tft_1p5623 = {
    .xres   = TFT_X_RES,
    .yres   = TFT_Y_RES,
    .bpp    = 16,
    .backlight = 100,
    .tftops = {
#if LCD_PIN_DB_COUNT == 8
        .write_reg = tft_write_reg8,
        .video_sync = tft_video_sync,
#else
        .write_reg = tft_write_reg16,
#endif
        .init_display = tft_1p5623_init_display,
    },
};

int tft_driver_init(void)
{
    tft_probe(&tft_1p5623);
    return 0;
}

#endif