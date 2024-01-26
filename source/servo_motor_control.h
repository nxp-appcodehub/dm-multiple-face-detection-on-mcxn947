/*
 * Copyright 2020-2022 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SERVO_MOTOR_CONTROL_H_
#define SERVO_MOTOR_CONTROL_H_

#ifndef SERVO_ENABLE
#define SERVO_ENABLE 0
#endif
#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */
void servo_motor_init();
void servo_motor_set_pos(uint16_t x,uint16_t y);
void servo_motor_set_cnt(uint16_t count);

#if defined(__cplusplus)
}
#endif /* __cplusplus */
#endif /* SERVO_MOTOR_CONTROL_H_ */
