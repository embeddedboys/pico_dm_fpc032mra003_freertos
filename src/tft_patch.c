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

static int tft_new_init_display(struct tft_priv *priv)
{
    pr_debug("%s, writing patched initial sequence...\n", __func__);
    priv->tftops->reset(priv);
    dm_gpio_set_value(priv->gpio.rd, 1);
    mdelay(150);

    write_reg(priv, 0x11);
    mdelay(120);

    write_reg(priv, 0x36, 0x00);

    write_reg(priv, 0x3A, 0x05);

    write_reg(priv, 0xB2, 0x0C, 0x0C, 0x00, 0x33, 0x33);

    write_reg(priv, 0xB7, 0x35);

    write_reg(priv, 0xBB, 0x37);

    write_reg(priv, 0xC0, 0x2C);

    write_reg(priv, 0xC2, 0x01);

    write_reg(priv, 0xC3, 0x12);

    //VDV, 0x20:0v
    write_reg(priv, 0xC4, 0x20);

    //0x0F:60Hz
    write_reg(priv, 0xC6, 0x0F);

    write_reg(priv, 0xD0, 0xA4, 0xA1);

    //sleep in后，gate输出为GND
    write_reg(priv, 0xD6, 0xA1);

    write_reg(priv, 0xE0, 0xD0, 0x08, 0x0E, 0x09, 0x09, 0x05, 0x31, 0x33, 0x48, 0x17, 0x14, 0x15, 0x31, 0x34);

    write_reg(priv, 0xE1, 0xD0, 0x08, 0x0E, 0x09, 0x09, 0x15, 0x31, 0x33, 0x48, 0x17, 0x14, 0x15, 0x31, 0x34);

    //使用240根gate  (N+1)*8
    //设定gate起点位置
    //当gate没有用完时，bit4(TMG)设为0
    // write_reg(priv, 0xE4, 0x25, 0x00, 0x00);

    write_reg(priv, 0x21);
    write_reg(priv, 0x29);

    return 0;
}

static void tft_new_set_addr_win(struct tft_priv *priv, int xs, int ys, int xe,
                                int ye)
{
    /* set column adddress */
    write_reg(priv, 0x2A, xs, xe);
    
    /* set row address */
    write_reg(priv, 0x2B, ys, ye);
    
    /* write start */
    write_reg(priv, 0x2C);
}

static void tft_clear(struct tft_priv *priv, u16 clear)
{

}

static void tft_new_video_sync(struct tft_priv *priv, int xs, int ys, int xe, int ye, void *vmem, size_t len)
{
    printf("video sync: xs=%d, ys=%d, xe=%d, ye=%d, len=%d\n", xs, ys, xe, ye, len);
    priv->tftops->set_addr_win(priv, xs, ys, xe, ye);
    write_buf_rs(priv, vmem, len * 2, 1);
}

static struct tft_operations new_tft_ops =  {
    .init_display = tft_new_init_display,
    // .set_addr_win = tft_new_set_addr_win,
    .video_sync = tft_new_video_sync,
};

void tft_apply_patch(struct tft_priv *priv)
{
    if (new_tft_ops.init_display)
        priv->tftops->init_display = new_tft_ops.init_display;

    if (new_tft_ops.reset)
        priv->tftops->reset = new_tft_ops.reset;

    if (new_tft_ops.clear)
        priv->tftops->clear = new_tft_ops.clear;

    if (new_tft_ops.blank)
        priv->tftops->blank = new_tft_ops.blank;

    if (new_tft_ops.sleep)
        priv->tftops->blank = new_tft_ops.blank;

    if (new_tft_ops.set_var)
        priv->tftops->set_var = new_tft_ops.set_var;

    if (new_tft_ops.set_addr_win)
        priv->tftops->set_addr_win = new_tft_ops.set_addr_win;

    if (new_tft_ops.set_cursor)
        priv->tftops->set_cursor = new_tft_ops.set_cursor;

    if (new_tft_ops.video_sync)
        priv->tftops->video_sync = new_tft_ops.video_sync;
}