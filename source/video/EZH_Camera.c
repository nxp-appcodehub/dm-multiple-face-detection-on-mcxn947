/*
 * Copyright (c) 2013 - 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "pin_mux.h"
#include "fsl_inputmux.h"
#include "clock_config.h"
#include "board.h"
#include "lcd_impl_flexio.h"
#include "st7796_lcd.h"
#include "fsl_clock.h"
#include "fsl_smartdma.h"
#include "ov7670.h"
#include "string.h"
#include "video.h"

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
/*******************************************************************************
 * Variables
 ******************************************************************************/
/****************************************************************
SmartDMA camera
****************************************************************/
smartdma_camera_param_t smartdmaParam;                  /*!< SMARTDMA function parameters. */

volatile uint32_t g_camera_buffer[320*15*2*2/4]; __attribute__ ((aligned (32)))
__attribute__((section (".ezh_code"))) uint32_t ezh_code[512] __attribute((aligned(4)));
volatile uint32_t __attribute((aligned(4))) p_coord_index=0;
volatile uint8_t  __attribute((aligned(4))) g_samrtdma_stack[64];

volatile uint32_t g_data_ready=0;
//volatile uint8_t g_camera_buffer[480*320*2];
volatile uint32_t g_camera_complete_flag=0;




/*******************************************************************************
 * Code
 ******************************************************************************/
static void ezh_camera_callback(void *param){
	g_camera_complete_flag = 1;
	NVIC_SetPendingIRQ(PLU_IRQn);
 }


/*!
 * @brief Main function
 */
void ezh_start(void)
{
    char ch;
    uint32_t address;

    INPUTMUX_Init(INPUTMUX0);
#ifdef USE_BOARD_BRK
    INPUTMUX_AttachSignal(INPUTMUX0, 0, kINPUTMUX_GpioPort0Pin10ToSmartDma);//P0_10/EZH_CAMERA_VSYNC
	INPUTMUX_AttachSignal(INPUTMUX0, 1, kINPUTMUX_GpioPort0Pin11ToSmartDma);//P0_11/EZH_CAMERA_HSYNC
	INPUTMUX_AttachSignal(INPUTMUX0, 2, kINPUTMUX_GpioPort0Pin14ToSmartDma);//P0_14/EZH_CAMERA_PCLK
#else
	INPUTMUX_AttachSignal(INPUTMUX0, 0, kINPUTMUX_GpioPort0Pin4ToSmartDma);//P0_10/EZH_CAMERA_VSYNC
	INPUTMUX_AttachSignal(INPUTMUX0, 1, kINPUTMUX_GpioPort0Pin11ToSmartDma);//P0_11/EZH_CAMERA_HSYNC
	INPUTMUX_AttachSignal(INPUTMUX0, 2, kINPUTMUX_GpioPort0Pin5ToSmartDma);//P0_14/EZH_CAMERA_PCLK
#endif
	/* Turn off clock to inputmux to save power. Clock is only needed to make changes */
	INPUTMUX_Deinit(INPUTMUX0);



    g_camera_complete_flag=0;

	SMARTDMA_InitWithoutFirmware();
	SMARTDMA_InstallFirmware(ezh_code,s_smartdmaCameraFirmware,
						 SMARTDMA_CAMERA_FIRMWARE_SIZE);
	SMARTDMA_InstallCallback(ezh_camera_callback, NULL);
	NVIC_EnableIRQ(SMARTDMA_IRQn);
	NVIC_SetPriority(SMARTDMA_IRQn, 3);

    smartdmaParam.smartdma_stack = (uint32_t*)g_samrtdma_stack;
	smartdmaParam.p_buffer_ping_pong  		 = (uint32_t*)g_camera_buffer;
	smartdmaParam.p_stripe_index  = (uint32_t*)&p_coord_index;
	SMARTDMA_Boot(kEZH_Camera_320240_Div16_Buf, &smartdmaParam, 0x2);


	NVIC_SetPriority(SysTick_IRQn, 1);
	NVIC_SetPriority(SMARTDMA_IRQn, 3);
	NVIC_SetPriority(PLU_IRQn, 7);
	NVIC_EnableIRQ(PLU_IRQn);

}

void ezh_camera_display_callback(void)
{
	uint32_t camera_buffer_addr = 0;

	if((p_coord_index % 2) == 0){
		camera_buffer_addr = (uint32_t)(g_camera_buffer);

	}else{
		camera_buffer_addr = (uint32_t)(&g_camera_buffer[320*15*2/4]);
	}

	ezh_copy_slice_to_model_input(p_coord_index,(uint16_t*)g_camera_buffer + ((p_coord_index) & 1) * BUFF_HEIGHT * CAMERA_WIDTH,CAMERA_WIDTH,BUFF_HEIGHT, CAMERA_HEIGHT/BUFF_HEIGHT);
	display_show_slice(p_coord_index, (uint32_t)(camera_buffer_addr),CAMERA_HEIGHT/BUFF_HEIGHT);

	g_camera_complete_flag=0;

}

void PLU_DriverIRQHandler(void)
{
	ezh_camera_display_callback();
}
