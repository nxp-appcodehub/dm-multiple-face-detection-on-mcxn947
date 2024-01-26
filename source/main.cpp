/*
 * Copyright 2020-2022 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "board_init.h"
#include "demo_config.h"
#include "demo_info.h"
#include "fsl_debug_console.h"
#include "image.h"
#include "image_utils.h"
#include "model.h"
#include "output_postproc.h"
#include "timer.h"
#include "video.h"
#include "ov7670.h"
#include "servo_motor_control.h"
int main(void)
{
    BOARD_Init();
    TIMER_Init();


    DEMO_PrintInfo();


    Ov7670_Init();

    display_init();

#if SERVO_ENABLE
    servo_motor_init();
#endif
    ezh_start();

    face_det();

    while(1)
    {

    }
}
