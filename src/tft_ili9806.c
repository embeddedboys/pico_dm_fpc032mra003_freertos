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

#if LCD_DRV_USE_ILI9806

static int tft_ili9806_init_display(struct tft_priv *priv)
{
    pr_debug("%s, writing initial sequence...\n", __func__);
    priv->tftops->reset(priv);
    // dm_gpio_set_value(priv->gpio.rd, 1);
    // mdelay(120);

    write_reg(priv, 0xFF, 0xFF, 0x98, 0x06);

    write_reg(priv, 0xB1, 0x00, 0x13, 0x16);

    write_reg(priv, 0xB4, 0x00, 0x00, 0x00);

    write_reg(priv, 0xBC, 0x03, 0x0E, 0x63, 0x69, 0x01, 0x01, 0x1B, 0x10, 0x6F, 0x63, 0xFF,
              0xFF, 0x01, 0x01, 0x01, 0x01, 0xFF, 0xF2, 0xC1);

    write_reg(priv, 0xBD, 0x01, 0x23, 0x45, 0x67, 0x01, 0x23, 0x45, 0x67);

    write_reg(priv, 0xBE, 0x00, 0x22, 0x27, 0x6A, 0xBC, 0xD8, 0x92, 0x22, 0x22);

    // Power Control 1
    write_reg(priv, 0xC0, 0x03, 0x0B, 0x02);

    // Power Control 2
    write_reg(priv, 0xC1, 0x17, 0x50, 0x50);

    // VCOM Control 1
    write_reg(priv, 0xC7, 0x25);

    // Engineering Setting
    write_reg(priv, 0xDF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20);

    // Postive Gamma Control
    write_reg(priv, 0xE0, 0x00, 0x0E, 0x14, 0x0C, 0x0E, 0x0A, 0x06, 0x03, 0x09, 0x0C, 0x13, 0x10, 0x0F, 0x14, 0x0B, 0x00);

    // Negative Gamma Control
    write_reg(priv, 0xE1, 0x00, 0x08, 0x10, 0x0E, 0x0F, 0x0C, 0x08, 0x05, 0x07, 0x0B, 0x12, 0x10, 0x0E, 0x17, 0x0F, 0x00);

    // VGMP / VGMN /VGSP / VGSN Voltage Measurement Set
    write_reg(priv, 0xED, 0x7F, 0x0F, 0x00);

    // Panel Timing Control 1
    write_reg(priv, 0xF1, 0x29, 0x8A, 0x07);

    // Panel Timing Control 2
    write_reg(priv, 0xF2, 0x40, 0xD2, 0x50, 0x28);

    // DVDD Voltage Setting
    write_reg(priv, 0xF3, 0x74);

    // Panel Resolution Selection Set
    write_reg(priv, 0xF7, 0x81);    // 480x854

    // LVGL Voltage Setting
    write_reg(priv, 0xFC, 0x08);

    // Interface Pixel Format
    write_reg(priv, 0x3A, 0x55); // Data mode : RGB565

    // Memory Access Control
    // Exchange Row/Column order and Inverse Column Address Order
    write_reg(priv, 0x36, (1 << 6) | (1 << 5));

    // Tearing Effect Line OFF
    write_reg(priv, 0x34);
    // write_reg(priv, 0x35, 0x00);    // Tearing Effect Line ON

    write_reg(priv, 0x11);  // Sleep out
    mdelay(120);
    write_reg(priv, 0x20);  // Display inversion OFF
    write_reg(priv, 0x29);  // Display ON

    return 0;
}

#if LCD_PIN_DB_COUNT == 8
static void tft_video_sync(struct tft_priv *priv, int xs, int ys, int xe, int ye,
                           void *vmem, size_t len)
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

static struct tft_display ili9806 = {
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
        .init_display = tft_ili9806_init_display,
    },
};

int tft_driver_init(void)
{
    tft_probe(&ili9806);
    return 0;
}

#endif