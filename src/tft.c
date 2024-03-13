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

#include "pico/platform.h"
#include "pico/time.h"
#define pr_fmt(fmt) "tft: " fmt

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>

#include "pico/stdio.h"
#include "pico/stdio_uart.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"

#include "tft.h"

#include "debug.h"

/*
 * tft Command Table
 */

#define DRV_NAME "tft"

static struct tft_priv g_priv;

static void fbtft_write_gpio16_wr(struct tft_priv *priv, void *buf, size_t len)
{
    u16 data;
    int i;
#ifndef DO_NOT_OPTIMIZE_FBTFT_WRITE_GPIO
    static u16 prev_data;
#endif

    /* Start writing by pulling down /WR */
    dm_gpio_set_value(priv->gpio.wr, 1);

    while (len) {
        data = *(u16 *)buf;
        
        /* Start writing by pulling down /WR */
        dm_gpio_set_value(priv->gpio.wr, 0);
   
        /* Set data */
#ifndef DO_NOT_OPTIMIZE_FBTFT_WRITE_GPIO
        if (data == prev_data) {
            dm_gpio_set_value(priv->gpio.wr, 1); /* used as delay */
        } else {
            for (i = 0; i < 16; i++) {
                if ((data & 1) != (prev_data & 1))
                    dm_gpio_set_value(priv->gpio.db[i],
                                      data & 1);
                data >>= 1;
                prev_data >>= 1;
            }
        }
#else
        for (i = 0; i < 16; i++) {
            dm_gpio_set_value(&priv->gpio.db[i], data & 1);
            data >>= 1;
        }
#endif
        
        /* Pullup /WR */
        dm_gpio_set_value(priv->gpio.wr, 1);
        
#ifndef DO_NOT_OPTIMIZE_FBTFT_WRITE_GPIO
        prev_data = *(u16 *)buf;
#endif
        buf += 2;
        len -= 2;
    }
}

void fbtft_write_gpio16_wr_rs(struct tft_priv *priv, void *buf, size_t len, bool rs)
{
    dm_gpio_set_value(priv->gpio.rs, rs);
    fbtft_write_gpio16_wr(priv, buf, len);
}

#define define_tft_write_reg(func, reg_type) \
void func(struct tft_priv *priv, int len, ...)  \
{   \
    reg_type *buf = (reg_type *)priv->buf; \
    va_list args;   \
    int i;  \
    \
    va_start(args, len);    \
    *buf = (reg_type)va_arg(args, unsigned int); \
    pr_debug("cmd: 0x%x\n", *buf); \
    write_buf_rs(priv, buf, sizeof(reg_type), 0); \
    len--;  \
    \
    /* if there no privams */  \
    if (len == 0)  \
        goto exit_no_param; \
    \
    pr_debug("  val:"); \
    for (i = 0; i < len; i++) { \
        pr_debug(" 0x%x", *buf); \
        *buf++ = (reg_type)va_arg(args, unsigned int); \
    }   \
    pr_debug("\n"); \
    \
    len *= sizeof(reg_type);    \
    write_buf_rs(priv, priv->buf, len, 1);  \
exit_no_param:  \
    va_end(args);   \
}

define_tft_write_reg(tft_write_reg8, uint8_t)
define_tft_write_reg(tft_write_reg16, uint16_t)

// void tft_write_reg(struct tft_priv *priv, int len, ...)
// {
//     u16 *buf = (u16 *)priv->buf;
//     va_list args;
//     int i;
    
//     va_start(args, len);
//     *buf = (u16)va_arg(args, unsigned int);
//     pr_debug("cmd: 0x%x\n", *buf);
//     write_buf_rs(priv, buf, sizeof(u16), 0);
//     len--;
    
//     /* if there no privams */
//     if (len == 0)
//         goto exit_no_param;
    
//     pr_debug("  val:");
//     for (i = 0; i < len; i++) {
//         pr_debug(" 0x%x", *buf);
//         *buf++ = (u16)va_arg(args, unsigned int);
//     }
//     pr_debug("\n");
    
//     len *= sizeof(u16);
//     write_buf_rs(priv, priv->buf, len, 1);
// exit_no_param:
//     va_end(args);
// }

static int tft_reset(struct tft_priv *priv)
{
    pr_debug("%s\n", __func__);
    dm_gpio_set_value(priv->gpio.reset, 1);
    mdelay(10);
    dm_gpio_set_value(priv->gpio.reset, 0);
    mdelay(10);
    dm_gpio_set_value(priv->gpio.reset, 1);
    mdelay(10);
    return 0;
}

static int tft_set_var(struct tft_priv *priv)
{
    pr_debug("%s\n", __func__);
    return 0;
}

static int tft_init_display(struct tft_priv *priv)
{
    pr_debug("%s, writing initial sequence...\n", __func__);
    tft_reset(priv);
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

static void inline tft_set_addr_win(struct tft_priv *priv, int xs, int ys, int xe,
                                int ye)
{
    /* set column adddress */
    write_reg(priv, 0x2A, xs >> 8, xs & 0xFF, xe >> 8, xe & 0xFF);
    
    /* set row address */
    write_reg(priv, 0x2B, ys >> 8, ys & 0xFF, ye >> 8, ye & 0xFF);
    
    /* write start */
    write_reg(priv, 0x2C);
}

static int tft_clear(struct tft_priv *priv, u16 clear)
{
    u32 width = priv->display->xres;
    u32 height = priv->display->yres;
    int x, y;

    pr_debug("clearing screen (%d x %d) with color 0x%x\n", width, height, clear);

    priv->tftops->set_addr_win(priv, 0, 0,
                         priv->display->xres - 1,
                         priv->display->yres - 1);
    
    for (x = 0; x < width; x++) {
        for (y = 0; y < height; y++) {
            write_buf_rs(priv, &clear, sizeof(u16), 1);
        }
    }

    return 0;
}

static int tft_blank(struct tft_priv *priv, bool on)
{
    pr_debug("%s\n", __func__);
    return 0;
}

static int tft_sleep(struct tft_priv *priv, bool on)
{
    pr_debug("%s\n", __func__);
    return 0;
}

static void tft_video_sync(struct tft_priv *priv, int xs, int ys, int xe, int ye, void *vmem, size_t len)
{
    //pr_debug("video sync: xs=%d, ys=%d, xe=%d, ye=%d, len=%d\n", xs, ys, xe, ye, len);
    priv->tftops->set_addr_win(priv, xs, ys, xe, ye);
    write_buf_rs(priv, vmem, len * 2, 1);
}

static struct tft_operations default_tft_ops = {
    .init_display    = tft_init_display,
    .reset           = tft_reset,
    .clear           = tft_clear,
    .blank           = tft_blank,
    .sleep           = tft_sleep,
    .set_var         = tft_set_var,
    .set_addr_win    = tft_set_addr_win,
    .video_sync      = tft_video_sync,
};

static int tft_gpio_init(struct tft_priv *priv)
{
    printf("initializing gpios...\n");

#if DISP_OVER_PIO
    gpio_init(priv->gpio.reset);
    // gpio_init(priv->gpio.bl);
    gpio_init(priv->gpio.cs);
    gpio_init(priv->gpio.rs);
    gpio_init(priv->gpio.rd);

    gpio_set_dir(priv->gpio.reset, GPIO_OUT);
    // gpio_set_dir(priv->gpio.bl, GPIO_OUT);
    gpio_set_dir(priv->gpio.cs, GPIO_OUT);
    gpio_set_dir(priv->gpio.rs, GPIO_OUT);
    gpio_set_dir(priv->gpio.rd, GPIO_OUT);
#else
    int *pp = (int *)&priv->gpio;

    int len = sizeof(priv->gpio)/sizeof(priv->gpio.reset);

    while(len--) {
        gpio_init(*pp);
        gpio_set_dir(*pp, GPIO_OUT);
        pp++;
    }
#endif
    return 0;
}

static int tft_hw_init(struct tft_priv *priv)
{
    int ret;

    printf("initializing hardware...\n");

#if DISP_OVER_PIO
    i80_pio_init(priv->gpio.db[0], ARRAY_SIZE(priv->gpio.db), priv->gpio.wr);
#endif
    tft_gpio_init(priv);

    priv->tftops->init_display(priv);

    printf("clearing screen...\n");
    /* clear screen to black */
    priv->tftops->clear(priv, 0);

    return 0;
}

static struct tft_display default_tft_display = {
    .xres   = TFT_X_RES,
    .yres   = TFT_Y_RES,
    .bpp    = 16,
    .rotate = 0,
};

static TaskHandle_t xTaskToNotify = NULL;

const UBaseType_t XArrayIndex = 1;

void tft_video_flush(int xs, int ys, int xe, int ye, void *vmem, uint32_t len)
{
    xTaskToNotify = xTaskGetCurrentTaskHandle();

    g_priv.tftops->video_sync(&g_priv, xs, ys, xe, ye, vmem, len);

    xTaskNotifyGiveIndexed(xTaskToNotify, XArrayIndex);

    xTaskToNotify = NULL;
}

portTASK_FUNCTION(video_flush_task, pvParameters)
{
    const TickType_t xMaxBlockTime = pdMS_TO_TICKS( 100 );
    uint32_t ulNotificationValue;
    struct video_frame vf;

    for (;;) {
        /* if lvgl request to draw */
        if (xQueueReceive(xToFlushQueue, &vf, portMAX_DELAY)) {
            pr_debug("Received video frame to flush\n");
            tft_video_flush(vf.xs, vf.ys, vf.xe, vf.ye, vf.vmem, vf.len);

            /* waiting for notification */
            ulNotificationValue = ulTaskNotifyTakeIndexed(XArrayIndex, pdTRUE, xMaxBlockTime);
            pr_debug("Received notification, val : %d\n", ulNotificationValue);

            if (ulNotificationValue > 0) {
                call_lv_disp_flush_ready();
            } else {
                /* timeout */
            }
        }
    }

    vTaskDelete(NULL);
}

void tft_async_video_flush(struct video_frame *vf)
{
    xQueueSend(xToFlushQueue, (void *)vf, portMAX_DELAY);
}

#define BUF_SIZE 64
static int tft_probe(struct tft_priv *priv)
{
    pr_debug("tft probing ...\n");
    
    priv->buf = (u8 *)malloc(BUF_SIZE);
    
    priv->display = &default_tft_display;
    priv->tftops = &default_tft_ops;

    priv->gpio.bl    = LCD_PIN_BL;
    priv->gpio.reset = LCD_PIN_RST;
    priv->gpio.rd    = 21;
    priv->gpio.rs    = LCD_PIN_RS;
    priv->gpio.wr    = LCD_PIN_WR;
    priv->gpio.cs    = LCD_PIN_CS;

    /* pin0 - pin15 for I8080 data bus */
    for (int i = LCD_PIN_DB_BASE; i < ARRAY_SIZE(priv->gpio.db); i++)
        priv->gpio.db[i] = i;

    tft_apply_patch(priv);

    tft_hw_init(priv);

    return 0;
}

int tft_driver_init(void)
{
    tft_probe(&g_priv);

    return 0;
}