/*
 * Copyright 2017-2018 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
  
#include <string.h>
#include "ov7670.h"
#include "fsl_debug_console.h"
#include "fsl_lpi2c.h"
#include "fsl_lpflexcomm.h"
#include "video.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* I2C for the touch. */
#ifdef USE_BOARD_BRK
#define CAMERA_I2C                       LPI2C0
#define CAMERA_FLEXCOMM                  0
#else
#define CAMERA_I2C                       LPI2C7
#define CAMERA_FLEXCOMM                  7
#endif
#define CAMERA_I2C_CLOCK_FREQ            CLOCK_GetBusClkFreq()
#define CAMERA_I2C_BAUDRATE              100000U

#define OV7670_QVGA_RGB_1     /*camera resolution define*/
//#define OV7670_VGA_RGB     /*camera resolution define*/
/*******************************************************************************
 * Variables
 ******************************************************************************/
uint8_t g_master_rxBuff[2];

static  uint8_t ov7670_init_regtbl[][2]=
{

#ifdef  OV7670_QQVGA_RGB  
//Output format
    {0x12, 0x04},	//VGA(640*480) RGB
    {0x1e, 0x00}, //MVFP: Mirror/VFlip,Normal image

    //configure the ouput timming
    //PCLK does not toggle during horizontal blank, one PCLK, one pixel
    {0x15, 0x20}, 	//COM10
    {0x3c, 0x00},   //COM12,No HREF when VSYNC is low


    //Brightness Control, with signal -128 to +128, 0x00 is middle value
    {0x55, 0x0},

    //Internal clock pre-scalar,F(internal clock) = F(input clock)/(Bit[5:0]+1)
    {0x11, 0x81},   // Clock Div, Input/(n+1), bit6 set to 1 to diable divider

    //SCALING_PCLK_DIV,
    {0x73, 0xF2},//0: Enable clock divider,010: Divided by 4
    //Common Control 14,Bit[4]: DCW and scaling PCLK enable,Bit[3]: Manual scaling
    {0x3e, 0x1A},
    //Common Control 3,Bit[2]: DCW enable,   Bit[3]: Scale enable
    //  {0x0c, 0x00},
    {0x0c, 0x04},    

    //DBLV,Bit[7:6]: PLL control
    {0x6b, 0x40},//0:Bypass PLL.,40: Input clock x4  , 80: Input clock x6  ,C0: Input clock x8

    //test pattern, useful in some case
    {0x70, 0x3A},
    {0x71, 0x35},

    //DCW Control,
    {0x72, 0x22},

    //Output Drive Capability
    {0x09, 0x00}, //Common Control 2, Output Drive Capability: 1x

    {0xa2, 0x02},

    {0xa5, 0x05},
    {0xab, 0x07},

    {0xaa, 0x94},

    {0x0d, 0x00},
    {0x0f, 0x4b},
    {0x3b, 0x9F},	//Night mode
    {0x3F, 0x04},//Edge Enhancement Adjustment
    {0x4C, 0x00},//De-noise Strength

    {0x92, 0x00},
    {0x93, 0x00},

    //reserved
    {0x16, 0x02},
    {0x21, 0x02},
    {0x22, 0x91},
    {0x29, 0x07},
    {0x35, 0x0b},
    {0x33, 0x0b},
    {0x37, 0x1d},
    {0x38, 0x71},
    {0x39, 0x2a},
    {0x0e, 0x61},
    {0x56, 0x40},
    {0x57, 0x80},
    {0x69, 0x00},
    {0x74, 0x19},

    //display , need retain
    {0x40, 0xD0},//Common Control 15
    {0x3a, 0x0C},//Line Buffer Test Option
    {0x3d, 0x80},//Common Control 13
    {0x67, 0x11},//Manual U Value
    {0x68, 0xFF},//Manual V Value
    //config the output window data, this can be configed later
    {0x17, 0x16},   // HSTART
    {0x18, 0x04},   // HSTOP
    {0x19, 0x02},   // VSTRT
    {0x1a, 0x7a},   // VSTOP
    {0x32, 0x80},   // HREF
    {0x03, 0x0a},   // VREF
    //AGC/AEC - Automatic Gain Control/Automatic exposure Control
    {0x00, 0x00},   // AGC
    {0x07, 0x3F},		//Exposure Value
    {0x10, 0xFF},
    {0x13, 0x66},
    {0x14, 0x21},   // limit the max gain
    {0x24, 0x75},
    {0x25, 0x63},
    {0x26, 0xA5},

    //Automatic white balance control
    {0x43, 0x14},
    {0x44, 0xf0},
    {0x45, 0x34},
    {0x46, 0x58},
    {0x47, 0x28},
    {0x48, 0x3a},

    //Matrix Coefficient
    {0x4f, 0x80},
    {0x50, 0x80},
    {0x51, 0x00},
    {0x52, 0x22},
    {0x53, 0x5e},
    {0x54, 0x80},

    //AWB Control
    {0x59, 0x88},
    {0x5a, 0x88},
    {0x5b, 0x44},
    {0x5c, 0x67},
    {0x5d, 0x49},
    {0x5e, 0x0e},
    {0x6c, 0x0a},
    {0x6d, 0x55},
    {0x6e, 0x11},
    {0x6f, 0x9f},


    //Lens Correction Option
    {0x62, 0x00},
    {0x63, 0x00},
    {0x64, 0x04},
    {0x65, 0x20},
    {0x66, 0x05},
    {0x94, 0x04},//effective only when LCC5[2] is high
    {0x95, 0x08},//effective only when LCC5[2] is high

    //Gamma Curve, needn't config
    {0x7a, 0x20},
    {0x7b, 0x1c},
    {0x7c, 0x28},
    {0x7d, 0x3c},
    {0x7e, 0x55},
    {0x7f, 0x68},
    {0x80, 0x76},
    {0x81, 0x80},
    {0x82, 0x88},
    {0x83, 0x8f},
    {0x84, 0x96},
    {0x85, 0xa3},
    {0x86, 0xaf},
    {0x87, 0xc4},
    {0x88, 0xd7},
    {0x89, 0xe8},

    //Histogram-based AEC/AGC Control
    {0x9f, 0x78},
    {0xa0, 0x68},
    {0x31, 0xff},
    {0xa1, 0x03},
    {0xa6, 0xdf},
    {0xa7, 0xdf},
    {0xa8, 0xf0},
    {0xa9, 0x90},

    //Automatic black Level Compensation
    {0xb0, 0x84},
    {0xb1, 0x0c},
    {0xb2, 0x0e},
    {0xb3, 0x82},
    {0xb8, 0x0a},
#endif
    
#ifdef  OV7670_QVGA_RGB_0  
    //Output format
//    {0x12, 0x04},	//VGA(640*480) RGB
    {0x12, 0x14},   //QVGA(320*240) RGB
    {0x1e, 0x00}, //MVFP: Mirror/VFlip,Normal image

    //configure the ouput timming
    //PCLK does not toggle during horizontal blank, one PCLK, one pixel
    {0x15, 0x20}, 	//COM10
    {0x3c, 0x00},   //COM12,No HREF when VSYNC is low


    //Brightness Control, with signal -128 to +128, 0x00 is middle value
    {0x55, 0x2f},

    //Internal clock pre-scalar,F(internal clock) = F(input clock)/(Bit[5:0]+1)
    {0x11, 0x81},   // Clock Div, Input/(n+1), bit6 set to 1 to diable divider
    
    //SCALING_PCLK_DIV,
    {0x73, 0xF1},//0: Enable clock divider,010: Divided by 4
    //Common Control 14,Bit[4]: DCW and scaling PCLK enable,Bit[3]: Manual scaling
    {0x3e, 0x19},
    //Common Control 3,Bit[2]: DCW enable,   Bit[3]: Scale enable
    {0x0c, 0x04},    

    //DBLV,Bit[7:6]: PLL control
    {0x6b, 0x40},//0:Bypass PLL.,40: Input clock x4  , 80: Input clock x6  ,C0: Input clock x8

    //test pattern, useful in some case
    {0x70, 0x3A},
    {0x71, 0x35},

    //DCW Control,
    {0x72, 0x11},

    //Output Drive Capability
    {0x09, 0x00}, //Common Control 2, Output Drive Capability: 1x
    {0xa2, 0x02},
    {0xa5, 0x05},
    {0xab, 0x07},
    {0xaa, 0x94},

    {0x0d, 0x00},
    {0x0f, 0x4b},
    {0x3b, 0x9F},	//Night mode
    {0x3F, 0x04},//Edge Enhancement Adjustment
    {0x4C, 0x00},//De-noise Strength

    {0x92, 0x00},
    {0x93, 0x00},

    //reserved
    {0x16, 0x02},
    {0x21, 0x02},
    {0x22, 0x91},
    {0x29, 0x07},
    {0x35, 0x0b},
    {0x33, 0x0b},
    {0x37, 0x1d},
    {0x38, 0x71},
    {0x39, 0x2a},
    {0x0e, 0x61},
    {0x56, 0x40},
    {0x57, 0x80},
    {0x69, 0x00},
    {0x74, 0x19},

    //display , need retain
    {0x40, 0xD0},//Common Control 15
    {0x3a, 0x0C},//Line Buffer Test Option
    {0x3d, 0x80},//Common Control 13
    {0x67, 0x11},//Manual U Value
    {0x68, 0xFF},//Manual V Value

    //config the output window data, this can be configed later
    {0x17, 0x15},   // HSTART
    {0x18, 0x03},   // HSTOP
    {0x19, 0x02},   // VSTRT
    {0x1a, 0x7a},   // VSTOP
    {0x32, (unsigned char)((1<<8)|(3<<3)|(3<<0))},   // HREF
    {0x03, (unsigned char)((2<<2)|(2<<0))},   // VREF

    //AGC/AEC - Automatic Gain Control/Automatic exposure Control
    {0x00, 0x00},   // AGC
    {0x07, 0x3F},		//Exposure Value
    {0x10, 0xFF},
    {0x13, 0x66},
    {0x14, 0x21},   // limit the max gain
    {0x24, 0x75},
    {0x25, 0x63},
    {0x26, 0xA5},

    //Automatic white balance control
    {0x43, 0x14},
    {0x44, 0xf0},
    {0x45, 0x34},
    {0x46, 0x58},
    {0x47, 0x28},
    {0x48, 0x3a},

    //Matrix Coefficient
    {0x4f, 0x80},
    {0x50, 0x80},
    {0x51, 0x00},
    {0x52, 0x22},
    {0x53, 0x5e},
    {0x54, 0x80},

    //AWB Control
    {0x59, 0x88},
    {0x5a, 0x88},
    {0x5b, 0x44},
    {0x5c, 0x67},
    {0x5d, 0x49},
    {0x5e, 0x0e},
    {0x6c, 0x0a},
    {0x6d, 0x55},
    {0x6e, 0x11},
    {0x6f, 0x9f},

    //Lens Correction Option
    {0x62, 0x00},
    {0x63, 0x00},
    {0x64, 0x04},
    {0x65, 0x20},
    {0x66, 0x05},
    {0x94, 0x04},//effective only when LCC5[2] is high
    {0x95, 0x08},//effective only when LCC5[2] is high

    //Gamma Curve, needn't config
    {0x7a, 0x20},
    {0x7b, 0x1c},
    {0x7c, 0x28},
    {0x7d, 0x3c},
    {0x7e, 0x55},
    {0x7f, 0x68},
    {0x80, 0x76},
    {0x81, 0x80},
    {0x82, 0x88},
    {0x83, 0x8f},
    {0x84, 0x96},
    {0x85, 0xa3},
    {0x86, 0xaf},
    {0x87, 0xc4},
    {0x88, 0xd7},
    {0x89, 0xe8},

    //Histogram-based AEC/AGC Control
    {0x9f, 0x78},
    {0xa0, 0x68},
    {0x31, 0xff},
    {0xa1, 0x03},
    {0xa6, 0xdf},
    {0xa7, 0xdf},
    {0xa8, 0xf0},
    {0xa9, 0x90},

    //Automatic black Level Compensation
    {0xb0, 0x84},
    {0xb1, 0x0c},
    {0xb2, 0x0e},
    {0xb3, 0x82},
    {0xb8, 0x0a},
#endif        
#ifdef  OV7670_QVGA_RGB_1  //better than OV7670_VGA_RGB_0 configuration
    //Output format
    {0x12, 0x14},	//QVGA(320*240) RGB
//    {0x12, 0x04},	//VGA(640*480) RGB
#ifdef USE_BOARD_BRK
	{0x1e, 0x20},
#else
    {0x1e, 0x20}, //MVFP: Mirror/VFlip,Normal image
#endif
    //configure the ouput timming
    //PCLK does not toggle during horizontal blank, one PCLK, one pixel
    {0x15, 0x20}, 	//COM10
    {0x3c, 0x00},   //COM12,No HREF when VSYNC is low
    //Brightness Control, with signal -128 to +128, 0x00 is middle value
    {0x55, 0x2f},

    //Internal clock pre-scalar,F(internal clock) = F(input clock)/(Bit[5:0]+1)
    {0x11, 0x81},   // Clock Div, Input/(n+1), bit6 set to 1 to diable divider

    //SCALING_PCLK_DIV,
    {0x73, 0x00},//0: Enable clock divider,010: Divided by 4
    //Common Control 14,Bit[4]: DCW and scaling PCLK enable,Bit[3]: Manual scaling
    {0x3e, 0x00},

    //DBLV,Bit[7:6]: PLL control
    {0x6b, 0x40},//0:Bypass PLL.,40: Input clock x4  , 80: Input clock x6  ,C0: Input clock x8

    //test pattern, useful in some case
    {0x70, 0},
    {0x71, 0},
    
    //Output Drive Capability
    {0x09, 0x00}, //Common Control 2, Output Drive Capability: 1x
    {0xa2, 0x02},
    {0xa5, 0x05},
    {0xab, 0x07},
    {0xaa, 0x94},

    {0x0c, 0x00},
    {0x0d, 0x00},
    {0x0f, 0x4b},
    {0x3b, 0x9F},	//Night mode
    {0x3F, 0x04},//Edge Enhancement Adjustment
    {0x4C, 0x00},//De-noise Strength

    {0x92, 0x00},
    {0x93, 0x00},

    //reserved
    {0x16, 0x02},
    {0x21, 0x02},
    {0x22, 0x91},
    {0x29, 0x07},
    {0x35, 0x0b},
    {0x33, 0x0b},
    {0x37, 0x1d},
    {0x38, 0x71},
    {0x39, 0x2a},
    {0x0e, 0x61},
    {0x56, 0x40},
    {0x57, 0x80},
    {0x69, 0x00},
    {0x74, 0x19},

    //display , need retain
    {0x40, 0xD0},//Common Control 15
    {0x3a, 0x0C},//Line Buffer Test Option
    {0x3d, 0x80},//Common Control 13
    {0x67, 0x11},//Manual U Value
    {0x68, 0xFF},//Manual V Value
    //config the output window data, this can be configed later
	{0x17, 0x15},   // HSTART
	{0x18, 0x03},   // HSTOP
	{0x19, 0x02},   // VSTRT
	{0x1a, 0x7a},   // VSTOP
	{0x32, (unsigned char)((1<<8)|(3<<3)|(3<<0))},   // HREF
	{0x03, (unsigned char)((2<<2)|(2<<0))},   // VREF

    //DCW Control,
    {0x72, 0x11}, // SCALING: V by 2, H by 2

    //AGC/AEC - Automatic Gain Control/Automatic exposure Control
    {0x00, 0x00},   // AGC
    {0x07, 0x3F},		//Exposure Value
    {0x10, 0xFF},
    {0x13, 0x66},
    {0x14, 0x21},   // limit the max gain
    {0x24, 0x75},
    {0x25, 0x63},
    {0x26, 0xA5},
    //Automatic white balance control
    {0x43, 0x14},
    {0x44, 0xf0},
    {0x45, 0x34},
    {0x46, 0x58},
    {0x47, 0x28},
    {0x48, 0x3a},
    //Matrix Coefficient
    {0x4f, 0x80},
    {0x50, 0x80},
    {0x51, 0x00},
    {0x52, 0x22},
    {0x53, 0x5e},
    {0x54, 0x80},
    //AWB Control
    {0x59, 0x88},
    {0x5a, 0x88},
    {0x5b, 0x44},
    {0x5c, 0x67},
    {0x5d, 0x49},
    {0x5e, 0x0e},
    {0x6c, 0x0a},
    {0x6d, 0x55},
    {0x6e, 0x11},
    {0x6f, 0x9f},
    //Lens Correction Option
    {0x62, 0x00},
    {0x63, 0x00},
    {0x64, 0x04},
    {0x65, 0x20},
    {0x66, 0x05},
    {0x94, 0x04},//effective only when LCC5[2] is high
    {0x95, 0x08},//effective only when LCC5[2] is high
    //Gamma Curve, needn't config
    {0x7a, 0x20},
    {0x7b, 0x1c},
    {0x7c, 0x28},
    {0x7d, 0x3c},
    {0x7e, 0x55},
    {0x7f, 0x68},
    {0x80, 0x76},
    {0x81, 0x80},
    {0x82, 0x88},
    {0x83, 0x8f},
    {0x84, 0x96},
    {0x85, 0xa3},
    {0x86, 0xaf},
    {0x87, 0xc4},
    {0x88, 0xd7},
    {0x89, 0xe8},
    //Histogram-based AEC/AGC Control
    {0x9f, 0x78},
    {0xa0, 0x68},
    {0x31, 0xff},
    {0xa1, 0x03},
    {0xa6, 0xdf},
    {0xa7, 0xdf},
    {0xa8, 0xf0},
    {0xa9, 0x90},
    //Automatic black Level Compensation
    {0xb0, 0x84},
    {0xb1, 0x0c},
    {0xb2, 0x0e},
    {0xb3, 0x82},
    {0xb8, 0x0a},
#endif 
#ifdef  OV7670_VGA_RGB  //better than OV7670_VGA_RGB_0 configuration
    //Output format
    {0x12, 0x04},	//VGA(640*480) RGB
    //{0x1e, 0x30}, //MVFP: Mirror/VFlip,Normal image
    {0x1e, 0x00}, //MVFP: Mirror/VFlip,Normal image

    //configure the ouput timming
    //PCLK does not toggle during horizontal blank, one PCLK, one pixel
    {0x15, 0x20}, 	//COM10
    {0x3c, 0x00},   //COM12,No HREF when VSYNC is low
    //Brightness Control, with signal -128 to +128, 0x00 is middle value
    {0x55, 0x2f},

    //Internal clock pre-scalar,F(internal clock) = F(input clock)/(Bit[5:0]+1)
    {0x11, 0x81},   // Clock Div, Input/(n+1), bit6 set to 1 to diable divider

    //SCALING_PCLK_DIV,
    {0x73, 0x00},//0: Enable clock divider,010: Divided by 4
    //Common Control 14,Bit[4]: DCW and scaling PCLK enable,Bit[3]: Manual scaling
    {0x3e, 0x00},

    //DBLV,Bit[7:6]: PLL control
    {0x6b, 0x40},//0:Bypass PLL.,40: Input clock x4  , 80: Input clock x6  ,C0: Input clock x8

    //test pattern, useful in some case
    {0x70, 0X3A},
    {0x71, 0X35},
    
    //Output Drive Capability
    {0x09, 0x00}, //Common Control 2, Output Drive Capability: 1x
    {0xa2, 0x02},
    {0xa5, 0x05},
    {0xab, 0x07},
    {0xaa, 0x94},

    {0x0c, 0x00},
    {0x0d, 0x00},
    {0x0f, 0x4b},
    {0x3b, 0x9F},	//Night mode
    {0x3F, 0x04},//Edge Enhancement Adjustment
    {0x4C, 0x00},//De-noise Strength

    {0x92, 0x00},
    {0x93, 0x00},

    //reserved
    {0x16, 0x02},
    {0x21, 0x02},
    {0x22, 0x91},
    {0x29, 0x07},
    {0x35, 0x0b},
    {0x33, 0x0b},
    {0x37, 0x1d},
    {0x38, 0x71},
    {0x39, 0x2a},
    {0x0e, 0x61},
    {0x56, 0x40},
    {0x57, 0x80},
    {0x69, 0x00},
    {0x74, 0x19},

    //display , need retain
    {0x40, 0xD0},//Common Control 15
    {0x3a, 0x0C},//Line Buffer Test Option
    {0x3d, 0x80},//Common Control 13
    {0x67, 0x11},//Manual U Value
    {0x68, 0xFF},//Manual V Value
    //config the output window data, this can be configed later
//    {0x17, 0x16},   // HSTART,176
//    {0x18, 0x04},   // HSTOP,32
//    {0x19, 0x02},   // VSTRT
//    {0x1a, 0x7a},   // VSTOP
//    {0x32, 0x80},   // HREF
//    {0x03, 0x0a},   // VREF
	{0x17, 0x15},   // HSTART
	{0x18, 0x03},   // HSTOP
	{0x19, 0x02},   // VSTRT
	{0x1a, 0x7a},   // VSTOP
	{0x32, (unsigned char)((1<<8)|(3<<3)|(3<<0))},   // HREF
	{0x03, (unsigned char)((2<<2)|(2<<0))},   // VREF
    //DCW Control,
    {0x72, 0x11},

    //AGC/AEC - Automatic Gain Control/Automatic exposure Control
    {0x00, 0x00},   // AGC
    {0x07, 0x3F},		//Exposure Value
    {0x10, 0xFF},
    {0x13, 0x66},
    {0x14, 0x21},   // limit the max gain
    {0x24, 0x75},
    {0x25, 0x63},
    {0x26, 0xA5},
    //Automatic white balance control
    {0x43, 0x14},
    {0x44, 0xf0},
    {0x45, 0x34},
    {0x46, 0x58},
    {0x47, 0x28},
    {0x48, 0x3a},
    {0x49, 0x9e},
    //Matrix Coefficient
    {0x4f, 0xc0},
    {0x50, 0xc0},
    {0x51, 0x00},
    {0x52, 0x33},
    {0x53, 0x8d},
    {0x54, 0xc0},
    //AWB Control
    {0x59, 0x88},
    {0x5a, 0x88},
    {0x5b, 0x44},
    {0x5c, 0x67},
    {0x5d, 0x49},
    {0x5e, 0x0e},
    {0x6c, 0x0a},
    {0x6d, 0x55},
    {0x6e, 0x11},
    {0x6f, 0x9f},
    //Lens Correction Option
    {0x62, 0x00},
    {0x63, 0x00},
    {0x64, 0x04},
    {0x65, 0x20},
    {0x66, 0x05},
    {0x94, 0x04},//effective only when LCC5[2] is high
    {0x95, 0x08},//effective only when LCC5[2] is high
    //Gamma Curve, needn't config
    {0x7a, 0x20},
    {0x7b, 0x1c},
    {0x7c, 0x28},
    {0x7d, 0x3c},
    {0x7e, 0x55},
    {0x7f, 0x68},
    {0x80, 0x76},
    {0x81, 0x80},
    {0x82, 0x88},
    {0x83, 0x8f},
    {0x84, 0x96},
    {0x85, 0xa3},
    {0x86, 0xaf},
    {0x87, 0xc4},
    {0x88, 0xd7},
    {0x89, 0xe8},
    //Histogram-based AEC/AGC Control
    {0x9f, 0x78},
    {0xa0, 0x68},
    {0x31, 0xff},
    {0xa1, 0x03},
    {0xa6, 0xdf},
    {0xa7, 0xdf},
    {0xa8, 0xf0},
    {0xa9, 0x90},
    //Automatic black Level Compensation
    {0xb0, 0x84},
    {0xb1, 0x0c},
    {0xb2, 0x0e},
    {0xb3, 0x82},
    {0xb8, 0x0a},
#endif   
};
/*******************************************************************************
 * Code
 ******************************************************************************/
void Ov7670_Init(void)
{
    lpi2c_master_config_t masterConfig;
    status_t status;

    CLOCK_SetClkDiv(kCLOCK_DivFlexcom4Clk, 1u);
#ifdef USE_BOARD_BRK
    CLOCK_AttachClk(kFRO12M_to_FLEXCOMM0);
    CLOCK_EnableClock(kCLOCK_LPFlexComm0);
    CLOCK_EnableClock(kCLOCK_LPI2c0);
    CLOCK_SetClkDiv(kCLOCK_DivFlexcom0Clk, 1u);
#else
/* init I2C0*/
    CLOCK_AttachClk(kFRO12M_to_FLEXCOMM7);
    CLOCK_EnableClock(kCLOCK_LPFlexComm7);
    CLOCK_EnableClock(kCLOCK_LPI2c7);
    CLOCK_SetClkDiv(kCLOCK_DivFlexcom7Clk, 1u);
#endif
    /*
     * masterConfig.enableMaster = true;
     * masterConfig.baudRate_Bps = 100000U;
     * masterConfig.enableTimeout = false;
     */
    LPI2C_MasterGetDefaultConfig(&masterConfig);

    /* Change the default baudrate configuration */
    masterConfig.baudRate_Hz = CAMERA_I2C_BAUDRATE;

    LP_FLEXCOMM_Init(CAMERA_FLEXCOMM, LP_FLEXCOMM_PERIPH_LPI2C);
    /* Initialize the I2C master peripheral */
    LPI2C_MasterInit(CAMERA_I2C, &masterConfig, CLOCK_GetFreq(kCLOCK_Fro12M));  

    lpi2c_master_transfer_t masterXfer;			
    memset(&masterXfer, 0, sizeof(masterXfer));
    
    uint8_t deviceAddress     = 0x0A; //Product ID Number - MSB (read only = 0x76)
    masterXfer.slaveAddress   =	0x21U;
    masterXfer.direction      = kLPI2C_Write;
    masterXfer.subaddress     = 0;
    masterXfer.subaddressSize = 0;
    masterXfer.data           = &deviceAddress;
    masterXfer.dataSize       = 1;
    masterXfer.flags          = kLPI2C_TransferDefaultFlag;		
    status = LPI2C_MasterTransferBlocking(CAMERA_I2C, &masterXfer);		

    if (status != kStatus_Success)
    {
        PRINTF("\r\nN10 I2C read id fail.\r\n");
    }  
    
    masterXfer.slaveAddress   = 0x21U;
    masterXfer.direction      = kLPI2C_Read;
    masterXfer.subaddress     = 0;
    masterXfer.subaddressSize = 0;
    masterXfer.data           = g_master_rxBuff;
    masterXfer.dataSize       = 1;
    masterXfer.flags          = kLPI2C_TransferDefaultFlag;
    status = LPI2C_MasterTransferBlocking(CAMERA_I2C, &masterXfer);

    if (status != kStatus_Success)
    {
        PRINTF("\r\nN10 I2C read id fail.\r\n");
    }  
   
    PRINTF("Camera Sensor ID = 0x%x(it should be 0x76).\r\n", g_master_rxBuff[0]);
    
    for(int j=0; j<ARRAY_SIZE(ov7670_init_regtbl); j++){
        masterXfer.slaveAddress   = 0x21U;
        masterXfer.direction      = kLPI2C_Write;
        masterXfer.subaddress     = 0;
        masterXfer.subaddressSize = 0;
        masterXfer.data           = &ov7670_init_regtbl[j][0];
        masterXfer.dataSize       = 2;
        masterXfer.flags          = kLPI2C_TransferDefaultFlag;
        status = LPI2C_MasterTransferBlocking(CAMERA_I2C, &masterXfer);
      
        if (status != kStatus_Success)
        {
          PRINTF("\r\nN10 I3C read id fail.\r\n");
        }
      }  
    PRINTF("\r\nCamera init successfully\r\n");
}


