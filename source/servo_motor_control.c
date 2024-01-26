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
#include "servo_motor_control.h"
#include "fsl_sctimer.h"
#include "fsl_ctimer.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define SCTIMER_CLK_FREQ        CLOCK_GetFreq(kCLOCK_BusClk)
#define DEMO_FIRST_SCTIMER_OUT  kSCTIMER_Out_4
#define DEMO_SECOND_SCTIMER_OUT kSCTIMER_Out_5
#define CTMRIntCycle 			10		//[ms]
#define PWM0_Low_Limit          1200 	//俯仰舵机占空比最小值，100%对应10000
#define PWM0_High_Limit        	5000 	//俯仰舵机占空比最大值
#define PWM1_Low_Limit          1000 	//水平舵机占空比最小值
#define PWM1_High_Limit        	5000 	//水平舵机占空比最大值

#define PWM0_midpoint			3000	//俯仰舵机运动中心点对应占空比，P1_22
#define PWM1_midpoint			3400 	//水平舵机运动中心点对应占空比，P1_23
#define PI_Kp					50		//PI控制比例系数
#define PI_Ki					25		//PI控制积分系数

/*******************************************************************************
 * Variables
 ******************************************************************************/
typedef struct
{
	int kp;
	int ki;
	int error;
	int integral;
	int integral_high_limit;
	int integral_low_limit;
	uint8_t bLimFlag;
	int16_t out;
}mc_pic_t;

typedef struct
{
	uint16_t PwmDuty;
	mc_pic_t PI_controller;

}mc_rudder_t;

mc_rudder_t g_sM1Drive,g_sM2Drive;
uint16_t count;
static volatile uint16_t cx,cy,cnt;
int mode,error_limit=20;
static uint16_t inc,inc_en,freq=1;
/*******************************************************************************
 * Code
 ******************************************************************************/
void SCT_Init()
{
	sctimer_config_t sctimerInfo;
	sctimer_pwm_signal_param_t pwmParam;
	uint32_t event;
	uint32_t sctimerClock;

	sctimerClock = SCTIMER_CLK_FREQ;

	SCTIMER_GetDefaultConfig(&sctimerInfo);

	sctimerInfo.enableBidirection_l = true;

	/* Initialize SCTimer module */
	SCTIMER_Init(SCT0, &sctimerInfo);

	/* Configure first PWM with frequency 200HZ from first output */
	pwmParam.output           = DEMO_FIRST_SCTIMER_OUT;
	pwmParam.level            = kSCTIMER_HighTrue;
	pwmParam.dutyCyclePercent = 30;
	SCTIMER_SetupPwm(SCT0, &pwmParam, kSCTIMER_CenterAlignedPwm, 10000U, sctimerClock, &event);

	/* Configure second PWM with different duty cycle but same frequency as before */
	pwmParam.output           = DEMO_SECOND_SCTIMER_OUT;
	pwmParam.level            = kSCTIMER_HighTrue;
	pwmParam.dutyCyclePercent = 30;
	SCTIMER_SetupPwm(SCT0, &pwmParam, kSCTIMER_CenterAlignedPwm, 10000U, sctimerClock, &event);

	/* 50 clock div */
	sctimerInfo.prescale_l = 49;
	SCT0->CTRL = SCT_CTRL_BIDIR_L(sctimerInfo.enableBidirection_l) | SCT_CTRL_PRE_L(sctimerInfo.prescale_l);
	/* Clear the counter after changing the PRE value. */
	SCT0->CTRL |= SCT_CTRL_CLRCTR_L_MASK;
	/* Start the 32-bit unify timer */
	SCTIMER_StartTimer(SCT0, kSCTIMER_Counter_U);
}
void InitCTMR(void)
{
	ctimer_match_config_t matchConfig0;
	ctimer_config_t config;

    /* Use 48 MHz clock for the Ctimer0 */
	CLOCK_SetClkDiv(kCLOCK_DivCtimer0Clk, 1u);
    CLOCK_AttachClk(kFRO_HF_to_CTIMER0);
    /* Get defaut configuration */
    CTIMER_GetDefaultConfig(&config);
    /* Init timer */
    CTIMER_Init(CTIMER0, &config);

    /* Configuration */
    matchConfig0.enableCounterReset = true;
    matchConfig0.enableCounterStop = false;
    matchConfig0.matchValue = CLOCK_GetCTimerClkFreq(0U) / 1000;
    matchConfig0.outControl = kCTIMER_Output_NoAction;
    matchConfig0.outPinInitState = false;
    matchConfig0.enableInterrupt = true;

    CTIMER_SetupMatch(CTIMER0, kCTIMER_Match_1, &matchConfig0);
    CTIMER_StartTimer(CTIMER0);

    EnableIRQ(CTIMER0_IRQn);
    NVIC_SetPriority(CTIMER0_IRQn, 2U);
}

void Rudder_init()
{
	g_sM1Drive.PwmDuty = PWM0_midpoint;
	g_sM1Drive.PI_controller.kp = PI_Kp;
	g_sM1Drive.PI_controller.ki = PI_Ki;
	g_sM1Drive.PI_controller.integral_high_limit = PWM0_High_Limit-PWM0_midpoint;
	g_sM1Drive.PI_controller.integral_low_limit = PWM0_Low_Limit-PWM0_midpoint;

	g_sM2Drive.PwmDuty = PWM1_midpoint;
	g_sM2Drive.PI_controller.kp = PI_Kp;
	g_sM2Drive.PI_controller.ki = PI_Ki;
	g_sM2Drive.PI_controller.integral_high_limit = PWM1_High_Limit-PWM1_midpoint;
	g_sM2Drive.PI_controller.integral_low_limit = PWM1_Low_Limit-PWM1_midpoint;
}
void SCT_UpdatePwmDuty(SCT_Type *base, uint16_t dutyCyclePercent, uint32_t event)
{
	uint32_t periodMatchReg, pulseMatchReg;
	uint32_t pulsePeriod = 0, period;

	/* Retrieve the match register number for the PWM period */
	periodMatchReg = base->EV[event].CTRL & SCT_EV_CTRL_MATCHSEL_MASK;

	/* Retrieve the match register number for the PWM pulse period */
	pulseMatchReg = base->EV[event + 1U].CTRL & SCT_EV_CTRL_MATCHSEL_MASK;

	period = base->MATCH[periodMatchReg];

	/* For 100% dutycyle, make pulse period greater than period so the event will never occur */
	if (dutyCyclePercent >= 10000U)
	{
		pulsePeriod = period + 2U;
	}
	else
	{
		pulsePeriod = (uint32_t)(((uint64_t)period * dutyCyclePercent) / 10000U);
	}

	/* Stop the counter before updating match register */
	base->CTRL |= (SCT_CTRL_HALT_L_MASK);

	/* Update dutycycle */
	base->MATCH[pulseMatchReg]    = SCT_MATCH_MATCHn_L(pulsePeriod);
	base->MATCHREL[pulseMatchReg] = SCT_MATCHREL_RELOADn_L(pulsePeriod);

	/* Restart the counter */
	base->CTRL &= ~(SCT_CTRL_HALT_L_MASK);
}
void PI_Control(mc_pic_t *ptr)
{
	ptr->integral += ptr->ki * ptr->error / 100;
	ptr->bLimFlag = 0;
	if(ptr->integral > ptr->integral_high_limit)
	{
		ptr->integral = ptr->integral_high_limit;
		ptr->bLimFlag = 1;
	}
	else if(ptr->integral < ptr->integral_low_limit)
	{
		ptr->integral = ptr->integral_low_limit;
		ptr->bLimFlag = 1;
	}
	ptr->out = ptr->kp * ptr->error / 100 + ptr->integral;
}

void CTIMER0_IRQHandler(void)
{
	if(count++ >= CTMRIntCycle)
	{
		count=0;
		//PI控制舵机
		if(cnt == 1)
		{
			cnt = 0;
			g_sM1Drive.PI_controller.error = cy - 120;
			g_sM2Drive.PI_controller.error = cx - 160;

			if((g_sM1Drive.PI_controller.error >= error_limit) || (g_sM1Drive.PI_controller.error <= -error_limit)){
				PI_Control(&g_sM1Drive.PI_controller);
			}
			if((g_sM2Drive.PI_controller.error >= error_limit) || (g_sM2Drive.PI_controller.error <= -error_limit)){
				PI_Control(&g_sM2Drive.PI_controller);
			}

			if(inc_en)
			{
				if(mode == 0)
				{
				g_sM1Drive.PwmDuty+=inc;
				if(g_sM1Drive.PwmDuty >= PWM0_High_Limit || g_sM1Drive.PwmDuty <= PWM0_Low_Limit)
					inc=-inc;
				}else
				{
					g_sM2Drive.PwmDuty+=inc;
					if(g_sM2Drive.PwmDuty >= PWM1_High_Limit || g_sM2Drive.PwmDuty <= PWM1_Low_Limit)
					inc=-inc;

				}
			}
			g_sM1Drive.PwmDuty = g_sM1Drive.PI_controller.out + PWM0_midpoint;
			g_sM2Drive.PwmDuty = g_sM2Drive.PI_controller.out + PWM1_midpoint;
			if(g_sM1Drive.PwmDuty > PWM0_High_Limit)
				g_sM1Drive.PwmDuty = PWM0_High_Limit;
			else if(g_sM1Drive.PwmDuty < PWM0_Low_Limit)
				g_sM1Drive.PwmDuty = PWM0_Low_Limit;
			if(g_sM2Drive.PwmDuty > PWM1_High_Limit)
				g_sM2Drive.PwmDuty = PWM1_High_Limit;
			else if(g_sM2Drive.PwmDuty < PWM1_Low_Limit)
				g_sM2Drive.PwmDuty = PWM1_Low_Limit;
		}
		/* 俯仰舵机占空比更新 */
		SCT_UpdatePwmDuty(SCT0, g_sM1Drive.PwmDuty, 0);
		/* 水平舵机占空比更新 */
		SCT_UpdatePwmDuty(SCT0, g_sM2Drive.PwmDuty, 2);
	}
	CTIMER0->IR = CTIMER_IR_MR1INT(1U);
}

void servo_motor_set_pos(uint16_t x,uint16_t y)
{
	cx = x;
	cy = y;
}

void servo_motor_set_cnt(uint16_t count)
{
	cnt = count;
}

void servo_motor_init()
{
	SCT_Init();
	InitCTMR();
	Rudder_init();
	inc=380*CTMRIntCycle/100/freq;
}
