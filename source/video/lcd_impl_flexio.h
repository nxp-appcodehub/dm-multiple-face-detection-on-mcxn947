/*
 * Copyright 2020-2022 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef LCD_IMPL_FLEXIO_H
#define LCD_IMPL_FLEXIO_H

#ifdef LCD_IMPL_FLEXIO

/* SDK drivers */
#include "fsl_clock.h"
#include "fsl_flexio_mculcd.h"
#include "fsl_flexio_mculcd_edma.h"
#include "fsl_gpio.h"

/* Board */
//#include "FreeRTOS.h"
#include "pin_mux.h"
//#include "semphr.h"

//#ifndef LCD_IMPL_FLEXIO_EDMA
//#define LCD_IMPL_FLEXIO_EDMA 1
//#endif

#include "st7796_lcd.h"

typedef struct {
    FLEXIO_MCULCD_Type dev;
#if LCD_IMPL_FLEXIO_EDMA
    SemaphoreHandle_t           semaphore;
    flexio_mculcd_edma_handle_t dma_hdl;
    edma_handle_t               tx_handle;
#endif
} lcd_impl_flexio_t;

void         lcd_impl_init(lcd_impl_flexio_t *impl);
st7796_ret_t lcd_impl_reset(void *impl);
st7796_ret_t lcd_impl_write_cmd(void *impl, uint8_t *cmd, uint8_t len);
st7796_ret_t lcd_impl_write_data(void *impl, uint8_t *data, uint32_t len);

#endif

#endif
