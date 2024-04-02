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

#if LCD_DRV_USE_ST6201

static int tft_st6201_init_display(struct tft_priv *priv)
{
    pr_debug("%s, writing initial sequence...\n", __func__);
    priv->tftops->reset(priv);
    // dm_gpio_set_value(priv->gpio.rd, 1);
    // mdelay(120);

    write_reg(priv, 0xFF, 0xA5);
    write_reg(priv, 0xE7, 0x10);    // TE output EN
    write_reg(priv, 0x35, 0x00);    // TE interface EN
    write_reg(priv, 0x36, 0xC0);
    write_reg(priv, 0x3A, 0x01);    // 01---RGB565 / 00---RGB666
    write_reg(priv, 0x40, 0x01);    // 01: IPS / 00 : TN
    write_reg(priv, 0x41, 0x03);    // 01 8Bit // 03 -- 16Bit
    write_reg(priv, 0x44, 0x15);    // VBP
    write_reg(priv, 0x45, 0x15);    // VFP
    write_reg(priv, 0x7D, 0x03);    // vdds_trim[2:0]

    write_reg(priv, 0xC1, 0xBB);
    write_reg(priv, 0xC2, 0x05);
    write_reg(priv, 0xC3, 0x10);
    write_reg(priv, 0xC6, 0x3E);
    write_reg(priv, 0xC7, 0x25);
    write_reg(priv, 0xC8, 0x21);
    write_reg(priv, 0x7A, 0x51);
    write_reg(priv, 0x6F, 0x49);
    write_reg(priv, 0x78, 0x57);
    write_reg(priv, 0xC9, 0x00);
    write_reg(priv, 0x67, 0x11);

    write_reg(priv, 0x51, 0x0A);
    write_reg(priv, 0x52, 0x7D);
    write_reg(priv, 0x53, 0x0A);
    write_reg(priv, 0x54, 0x7D);

    write_reg(priv, 0x46, 0x0A);
    write_reg(priv, 0x47, 0x2A);
    write_reg(priv, 0x48, 0x0A);
    write_reg(priv, 0x49, 0x1A);
    write_reg(priv, 0x44, 0x15);
    write_reg(priv, 0x45, 0x15);
    write_reg(priv, 0x73, 0x08);
    write_reg(priv, 0x74, 0x10);

    // test mode
    // write_reg(priv, 0xF8, 0x16);
    // write_reg(priv, 0xF9, 0x20);

    write_reg(priv, 0x56, 0x43);
    write_reg(priv, 0x57, 0x42);
    write_reg(priv, 0x58, 0x3C);
    write_reg(priv, 0x59, 0x64);
    write_reg(priv, 0x5A, 0x41);
    write_reg(priv, 0x5B, 0x3C);
    write_reg(priv, 0x5C, 0x3C);
    write_reg(priv, 0x5E, 0x1F);
    write_reg(priv, 0x60, 0x80);
    write_reg(priv, 0x61, 0x3F);
    write_reg(priv, 0x62, 0x21);
    write_reg(priv, 0x63, 0x07);
    write_reg(priv, 0x64, 0xE0);
    write_reg(priv, 0x65, 0x02);
    write_reg(priv, 0xCA, 0x20);
    write_reg(priv, 0xCB, 0x52);
    write_reg(priv, 0xCC, 0x10);
    write_reg(priv, 0xCD, 0x42);
    write_reg(priv, 0xD0, 0x20);
    write_reg(priv, 0xD1, 0x10);
    write_reg(priv, 0xD2, 0x10);
    write_reg(priv, 0xD3, 0x42);
    write_reg(priv, 0xD4, 0x0A);
    write_reg(priv, 0xD5, 0x32);

    write_reg(priv, 0x80, 0x00);
    write_reg(priv, 0xA0, 0x00);
    write_reg(priv, 0x81, 0x06);
    write_reg(priv, 0xA1, 0x08);
    write_reg(priv, 0x82, 0x03);
    write_reg(priv, 0xA2, 0x03);
    write_reg(priv, 0x86, 0x14);
    write_reg(priv, 0xA6, 0x14);
    write_reg(priv, 0x87, 0x2C);
    write_reg(priv, 0xA7, 0x26);
    write_reg(priv, 0x83, 0x37);
    write_reg(priv, 0xA3, 0x37);
    write_reg(priv, 0x84, 0x35);
    write_reg(priv, 0xA4, 0x35);
    write_reg(priv, 0x85, 0x3F);
    write_reg(priv, 0xA5, 0x3F);
    write_reg(priv, 0x88, 0x0A);
    write_reg(priv, 0xA8, 0x0A);
    write_reg(priv, 0x89, 0x13);
    write_reg(priv, 0xA9, 0x12);
    write_reg(priv, 0x8A, 0x18);
    write_reg(priv, 0xAA, 0x19);
    write_reg(priv, 0x8B, 0x0A);
    write_reg(priv, 0xAB, 0x0A);
    write_reg(priv, 0x8C, 0x17);
    write_reg(priv, 0xAC, 0x0B);
    write_reg(priv, 0x8D, 0x1A);
    write_reg(priv, 0xAD, 0x09);
    write_reg(priv, 0x8E, 0x1A);
    write_reg(priv, 0xAE, 0x08);
    write_reg(priv, 0x8F, 0x1F);
    write_reg(priv, 0xAF, 0x00);
    write_reg(priv, 0x90, 0x08);
    write_reg(priv, 0xB0, 0x00);
    write_reg(priv, 0x91, 0x10);
    write_reg(priv, 0xB1, 0x06);

    write_reg(priv, 0x92, 0x19);
    write_reg(priv, 0xB2, 0x15);
    write_reg(priv, 0xFF, 0x00);
    write_reg(priv, 0x11);
    mdelay(120);

    write_reg(priv, 0x29);
    mdelay(20);

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

static struct tft_display st6201 = {
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
        .init_display = tft_st6201_init_display,
    },
};

int tft_driver_init(void)
{
    tft_probe(&st6201);
    return 0;
}

#endif