/*
 * Copyright 2020-2022 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* SDK drivers */
#include "fsl_clock.h"
#include "fsl_flexio_mculcd.h"
#include "fsl_flexio_mculcd_edma.h"
#include "fsl_gpio.h"

/* Board */
#include "pin_mux.h"

/* Private */
#include "st7796_lcd.h"

#ifdef LCD_IMPL_FLEXIO

#ifndef LCD_IMPL_FLEXIO_EDMA
#define LCD_IMPL_FLEXIO_EDMA 1
#endif

#define LCD_FLEXIO      FLEXIO0
#define LCD_FLEXIO_FREQ 150000000
#define LCD_FLEXIO_BAUD 150000000

#define BOARD_INITLCDFXIOPINS_LCD_RS_PIN  7
#define BOARD_INITLCDFXIOPINS_LCD_CS_PIN  12
#define BOARD_INITLCDFXIOPINS_LCD_RST_PIN 7

#define SET_GPIO(x, v) GPIO_PinWrite(BOARD_INITLCDFXIOPINS_LCD_##x##_GPIO, BOARD_INITLCDFXIOPINS_LCD_##x##_PIN, v)

static void lcd_impl_set_cs_pin(bool set);
static void lcd_impl_set_rs_pin(bool set);
#if LCD_IMPL_FLEXIO_EDMA
static void lcd_impl_dma_callback(FLEXIO_MCULCD_Type *base, flexio_mculcd_edma_handle_t *handle, status_t status,
                                  void *userData);
#endif

static FLEXIO_MCULCD_Type s_lcd_dev;
static volatile uint8_t   s_dma_flag = 1;

void lcd_impl_init(void) {
    flexio_mculcd_config_t flexio_cfg;

    SET_GPIO(RS, 1U);
    SET_GPIO(RST, 1U);

#if LCD_IMPL_FLEXIO_EDMA
    edma_config_t edma_cfg;

    CLOCK_EnableClock(kCLOCK_Dma0);

    EDMA_GetDefaultConfig(&edma_cfg);
    EDMA_Init(DMA0, &edma_cfg);

    EDMA_EnableChannelRequest(DMA0, 0);
    EDMA_SetChannelMux(DMA0, 0, kDmaRequestMuxFlexIO0ShiftRegister0Request);
#endif

    CLOCK_AttachClk(kPLL0_to_FLEXIO);
    CLOCK_SetClkDiv(kCLOCK_DivFlexioClk, CLOCK_GetPll0OutFreq() / LCD_FLEXIO_FREQ);

    FLEXIO_MCULCD_GetDefaultConfig(&flexio_cfg);

    flexio_cfg.baudRate_Bps     = LCD_FLEXIO_BAUD;
    flexio_cfg.enableFastAccess = false;

    s_lcd_dev.flexioBase = LCD_FLEXIO;
    s_lcd_dev.busType    = kFLEXIO_MCULCD_8080;

    s_lcd_dev.timerIndex = 0U;

    s_lcd_dev.txShifterStartIndex = 0;
    s_lcd_dev.txShifterEndIndex   = 7;
    s_lcd_dev.rxShifterStartIndex = 0;
    s_lcd_dev.rxShifterEndIndex   = 7;

    s_lcd_dev.dataPinStartIndex = 16;
    s_lcd_dev.RDPinIndex        = 0;
    s_lcd_dev.ENWRPinIndex      = 1;

    s_lcd_dev.setCSPin = lcd_impl_set_cs_pin;
    s_lcd_dev.setRSPin = lcd_impl_set_rs_pin;

    FLEXIO_MCULCD_Init(&s_lcd_dev, &flexio_cfg, CLOCK_GetFlexioClkFreq());
}

st7796_ret_t lcd_impl_reset(void *handle) {
    SDK_DelayAtLeastUs(50 * 1000, CLOCK_GetFreq(kCLOCK_CoreSysClk));
    SET_GPIO(RST, 0U);
    SDK_DelayAtLeastUs(50 * 1000, CLOCK_GetFreq(kCLOCK_CoreSysClk));
    SET_GPIO(RST, 1U);
    SDK_DelayAtLeastUs(50 * 1000, CLOCK_GetFreq(kCLOCK_CoreSysClk));

    return ST7796_OK;
}

st7796_ret_t lcd_impl_write_cmd(void *handle, uint8_t *cmd, uint8_t len) {
#if LCD_IMPL_FLEXIO_EDMA
    while (s_dma_flag == 0U) {
        /* Wait for transfer done */
    }
#endif

    FLEXIO_MCULCD_StartTransfer(&s_lcd_dev);

    FLEXIO_MCULCD_WriteCommandBlocking(&s_lcd_dev, (uint32_t)cmd[0]);

    if (len > 1) {
        for (uint8_t i = 1; i < len; i++) {
            uint16_t data = cmd[i]; /* Zero-extend */
            FLEXIO_MCULCD_WriteDataArrayBlocking(&s_lcd_dev, &data, 2);
        }
    }

    FLEXIO_MCULCD_StopTransfer(&s_lcd_dev);
    return ST7796_OK;
}

st7796_ret_t lcd_impl_write_data(void *handle, uint8_t *data, uint32_t len) {
#if LCD_IMPL_FLEXIO_EDMA
    flexio_mculcd_edma_handle_t dma_hdl;
    edma_handle_t               tx_handle;

    while (s_dma_flag == 0U) {
        /* Wait for transfer done */
    }

    EDMA_CreateHandle(&tx_handle, DMA0, 0);
    FLEXIO_MCULCD_TransferCreateHandleEDMA(&s_lcd_dev, &dma_hdl, lcd_impl_dma_callback, NULL, &tx_handle, NULL);
#endif

#if LCD_IMPL_FLEXIO_EDMA
    s_dma_flag = 0;
    FLEXIO_MCULCD_WriteDataEDMA(&s_lcd_dev, &dma_hdl, data, len);

    while (s_dma_flag == 0U) {
        /* Wait for transfer done */
    }

#else
    FLEXIO_MCULCD_StartTransfer(&s_lcd_dev);
    FLEXIO_MCULCD_WriteDataArrayBlocking(&s_lcd_dev, data, len);
    FLEXIO_MCULCD_StopTransfer(&s_lcd_dev);
#endif

    return ST7796_OK;
}

static void lcd_impl_set_cs_pin(bool set) {
    if (set) {
        SET_GPIO(CS, 1);
    } else {
        SET_GPIO(CS, 0);
    }
}

static void lcd_impl_set_rs_pin(bool set) {
    if (set) {
        SET_GPIO(RS, 1);
    } else {
        SET_GPIO(RS, 0);
    }
}

#if LCD_IMPL_FLEXIO_EDMA
static void lcd_impl_dma_callback(FLEXIO_MCULCD_Type *base, flexio_mculcd_edma_handle_t *handle, status_t status,
                                  void *userData) {
    s_dma_flag = 1;
}
#endif

#endif
