/*
 * Copyright 2019-2022 NXP
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef _FSL_SMARTDMA_H_
#define _FSL_SMARTDMA_H_

#include "fsl_common.h"

/*!
 * @addtogroup smartdma
 * @{
 */

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define SMARTDMA_HANDSHAKE_EVENT  0
#define SMARTDMA_HANDSHAKE_ENABLE 1
#define SMARTDMA_MASK_RESP       2
#define SMARTDMA_ENABLE_AHBBUF   3
#define SMARTDMA_ENABLE_GPISYNCH 4

/*! @name Driver version */
/*@{*/
/*! @brief SMARTDMA driver version */
#define FSL_SMARTDMA_DRIVER_VERSION (MAKE_VERSION(2, 7, 0))
/*@}*/

/*! @brief The firmware used for camera. */
extern const uint8_t s_smartdmaCameraFirmware[];

/*! @brief The s_smartdmaCameraFirmware firmware memory address. */
#define SMARTDMA_CAMERA_MEM_ADDR 0x04000000

/*! @brief Size of s_smartdmacameraFirmware */
#define SMARTDMA_CAMERA_FIRMWARE_SIZE (s_smartdmaCameraFirmwareSize)

/*! @brief Size of s_smartdmacameraFirmware */
extern const uint32_t s_smartdmaCameraFirmwareSize;


/*!
 * @brief The API index when using s_smartdmaCameraFirmware.
 */
enum _smartdma_camera_api
{
	kEZH_Camera_320240_Whole_Buf = 0U,
    kEZH_Camera_320240_Div16_Buf = 1U,  
    kEZH_Camera_480320_Whole_Buf = 2U,
	kEZH_Camera_320240_GrayScale_Div4_Buf = 3U,
};

/*!
 * @brief Parameter for camera
 */
typedef struct _smartdma_camera_param
{
	uint32_t *smartdma_stack;   /* user must allocate min 64bytes */
	uint32_t *p_buffer;         /* user must allocate size for pixel data */
	uint32_t *p_stripe_index;         /* user must allocate size for pixel data */
	uint32_t *p_buffer_ping_pong;         /* user must allocate size for pixel data */
} smartdma_camera_param_t;




/*! @brief Callback function prototype for the smartdma driver. */
typedef void (*smartdma_callback_t)(void *param);

/*******************************************************************************
 * APIs
 ******************************************************************************/

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/*!
 * @brief Initialize the SMARTDMA.
 *
 * @param apiMemAddr The address firmware will be copied to.
 * @param firmware The firmware to use.
 * @param firmwareSizeByte Size of firmware.
 * @deprecated Do not use this function. It has been superceded by @ref GPIO_PinWrite.
 * @ref SMARTDMA_InitWithoutFirmware and @ref SMARTDMA_InstallFirmware.
 */
void SMARTDMA_Init(uint32_t apiMemAddr, const void *firmware, uint32_t firmwareSizeByte);

/*!
 * @brief Initialize the SMARTDMA.
 *
 * This function is similar with @ref SMARTDMA_Init, the difference is this function
 * does not install the firmware, the firmware could be installed using
 * @ref SMARTDMA_InstallFirmware.
 */
void SMARTDMA_InitWithoutFirmware(void);

/*!
 * @brief Install the firmware.
 *
 * @param apiMemAddr The address firmware will be copied to.
 * @param firmware The firmware to use.
 * @param firmwareSizeByte Size of firmware.
 * @note Only call this function when SMARTDMA is not busy.
 */
void SMARTDMA_InstallFirmware(uint32_t apiMemAddr, const void *firmware, uint32_t firmwareSizeByte);

/*!
 * @brief Install the complete callback function.
 *
 * @param callback The callback called when smartdma program finished.
 * @param param Parameter for the callback.
 * @note Only call this function when SMARTDMA is not busy.
 */
void SMARTDMA_InstallCallback(smartdma_callback_t callback, void *param);

/*!
 * @brief Boot the SMARTDMA to run program.
 *
 * @param apiIndex Index of the API to call.
 * @param pParam Pointer to the parameter.
 * @param mask Value set to SMARTDMA_ARM2SMARTDMA[0:1].
 * @note Only call this function when SMARTDMA is not busy.
 */
void SMARTDMA_Boot(uint32_t apiIndex, void *pParam, uint8_t mask);

/*!
 * @brief Deinitialize the SMARTDMA.
 */
void SMARTDMA_Deinit(void);

/*!
 * @brief Reset the SMARTDMA.
 */
void SMARTDMA_Reset(void);

/*!
 * @brief SMARTDMA IRQ.
 */
void SMARTDMA_HandleIRQ(void);

#if defined(__cplusplus)
}
#endif

/* @} */

#endif /* _FSL_SMARTDMA_H_ */
