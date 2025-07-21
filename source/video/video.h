/*
 * Copyright 2020-2022 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */


#ifndef VIDEO_VIDEO_H_
#define VIDEO_VIDEO_H_
/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define CAMERA_WIDTH 320
#define CAMERA_HEIGHT 240
#define STRIPE_HEIGHT 15
#define BUFF_HEIGHT 15
#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */
/*******************************************************************************
 * Functions
 ******************************************************************************/
void ezh_start();
void display_init();
void display_show_bg_image();
void display_show_slice(uint32_t g_stripe_index, uint32_t buffer,uint32_t maxidx);
void ezh_copy_slice_to_model_input(uint32_t idx, uint32_t cam_slice_buffer, uint32_t cam_slice_width, uint32_t cam_slice_height, uint32_t max_idx);
#if defined(__cplusplus)
}
#endif /* __cplusplus */
#endif /* VIDEO_VIDEO_H_ */
