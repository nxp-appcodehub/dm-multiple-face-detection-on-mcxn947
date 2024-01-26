 /*
 * Copyright 2020-2022 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

 // #include "board_init.h"

#include <stdio.h>
#include "fsl_debug_console.h"
#include "image.h"
#include "image_utils.h"
#include "model.h"
#include "output_postproc.h"
#include "timer.h"
#include "video.h"
#include "yolo_post_processing.h"
#include "servo_motor_control.h"
extern "C" {

#define MODEL_IN_W	160
#define MODEL_IN_H  128
#define MODEL_IN_C	3
#define MODEL_IN_COLOR_BGR 0

#define BOX_SCORE_THRESHOLD 0.90
#define MAX_OD_BOX_CNT  10




typedef struct tagODResult_t
{
    union {
        int16_t xyxy[4];
        struct {
            int16_t x1;
            int16_t y1;
            int16_t x2;
            int16_t y2;
        };
    };
    float score;
    int label;
}ODResult_t;


ODResult_t s_odRets[MAX_OD_BOX_CNT];
__attribute__((section (".model_input_buffer"))) static uint8_t model_input_buf[MODEL_IN_W*MODEL_IN_H*MODEL_IN_C] = {0};
int s_odRetCnt = 0;
uint32_t s_infUs = 0;
volatile uint8_t g_isImgBufReady = 0;

#define WND_X0 0
#define WND_Y0 0

void draw_rect_on_slice_buffer(uint16_t* pCam, int srcW,
	int curY, int stride, ODResult_t *pODRet, int retCnt, int slice_height)
{
	int i = 0;
	int bri;
	for (i=0; i<retCnt; i++, pODRet++) {
		bri = (int)((pODRet->score - 0.62f) * 100.0f);
		if (bri < 0)
			bri = 0;
		if (bri > 31)
			bri = 31;
		uint32_t color16 = bri | (bri*2<<5) | bri<<11;
		uint32_t color = color16 | color16<<16;
		color = 0xFFFFFFFF;
		uint16_t *pHorLine = 0;
		int stripeY1 = pODRet->y1 - curY;
		if (stripeY1 >= 0 && stripeY1 < slice_height) {
			for (int j = 0; j<4 && stripeY1 + j < slice_height; j++) {
				pHorLine = pCam + srcW * (j + stripeY1) + pODRet->x1;
				memset(pHorLine, color, (pODRet->x2 - pODRet->x1) * 2);
			}
		}

		int stripeY2 = pODRet->y2 - curY;
		if (stripeY2 >=0 && stripeY2 < slice_height) {
		  for (int j = 0; j<4 && stripeY2 + j < slice_height; j++) {
				pHorLine = pCam + srcW * (j + stripeY2) + pODRet->x1;
				memset(pHorLine, color, (pODRet->x2 - pODRet->x1) * 2);
		  }
		}

		uint16_t *pVtcLineL = pCam + pODRet->x1;
		uint16_t *pVtcLineR = pCam + pODRet->x2;

		for (int y=curY; y < curY + slice_height; y++) {
			if (y > pODRet->y1 && y < pODRet->y2) {
				memset(pVtcLineL, color, 8);
				memset(pVtcLineR, color, 8);
				// pVtcLineL[0] = 0x3F<<5;
				// pVtcLineR[0] = 0x3F<<5;
			}
			pVtcLineL += srcW;
			pVtcLineR += srcW;
		}

	}
}

void Rgb565StridedToBgr888(const uint16_t* pIn, int srcW, int wndW, int wndH, int wndX0, int wndY0,
	uint8_t* p888, int stride, uint8_t isSub128) {
	const uint16_t* pSrc = pIn;
	uint32_t datIn, datOut, datOuts[3];
	uint8_t* p888out = p888;
	for (int y = wndY0,y1=(wndH-wndY0)/stride-wndY0; y < wndH; y += stride,y1--) {
		pSrc = pIn + srcW * y + wndX0;
		//p888out = p888 + y1*wndW*3/stride;
		for (int x = 0; x < wndW; x += stride * 4) {
			datIn = pSrc[0];
			pSrc += stride;
			datOuts[0] = (datIn & 31) << 3 | (datIn & 63 << 5) << 5 | (datIn & 31 << 11) << 8;
			// datOuts[0] = (datIn & 31) << 19| (datIn & 63 << 5) << 5 | ((datIn>>8) & 0xf8);

			datIn = pSrc[0];
			pSrc += stride;
			datOut = (datIn & 31) << 3 | (datIn & 63 << 5) << 5 | (datIn & 31 << 11) << 8;
			// datOut = (datIn & 31) << 19| (datIn & 63 << 5) << 5 | ((datIn>>8) & 0xf8);
			datOuts[0] |= datOut << 24;
			datOuts[1] = datOut >> 8;

			datIn = pSrc[0];
			pSrc += stride;
			datOut = (datIn & 31) << 3 | (datIn & 63 << 5) << 5 | (datIn & 31 << 11) << 8;
			// datOut = (datIn & 31) << 19| (datIn & 63 << 5) << 5 | ((datIn>>8) & 0xf8);
			datOuts[1] |= (datOut << 16) & 0xFFFF0000;
			datOuts[2] = datOut >> 16;

			datIn = pSrc[0];
			pSrc += stride;
			datOut = (datIn & 31) << 3 | (datIn & 63 << 5) << 5 | (datIn & 31 << 11) << 8;
			// datOut = (datIn & 31) << 19| (datIn & 63 << 5) << 5 | ((datIn>>8) & 0xf8);

			datOuts[2] |= datOut << 8;

			if (isSub128) {
				// subtract 128 bytewisely, equal to XOR with 0x80
				datOuts[0] ^= 0x80808080;
				datOuts[1] ^= 0x80808080;
				datOuts[2] ^= 0x80808080;
			}
			memcpy(p888out, datOuts, 3 * 4);
			p888out += 3 * 4;
		}
	}
}

void Rgb565StridedToRgb888(const uint16_t* pIn, int srcW, int wndW, int wndH, int wndX0, int wndY0,
	uint8_t* p888, int stride, uint8_t isSub128) {
	const uint16_t* pSrc = pIn;
	uint32_t datIn, datOut, datOuts[3];
	uint8_t* p888out = p888;

	for (int y = wndY0,y1=(wndH-wndY0)/stride-wndY0; y < wndH; y += stride,y1--) {
		pSrc = pIn + srcW * y + wndX0;

		//p888out = p888 + y1*wndW*3/stride;
		for (int x = 0; x < wndW; x += stride * 4) {
			datIn = pSrc[0];
			pSrc += stride;
			// datOuts[0] = (datIn & 31) << 3 | (datIn & 63 << 5) << 5 | (datIn & 31 << 11) << 8;
			datOuts[0] = (datIn & 31) << 19| (datIn & 63 << 5) << 5 | ((datIn>>8) & 0xf8);

			datIn = pSrc[0];
			pSrc += stride;
			// datOut = (datIn & 31) << 3 | (datIn & 63 << 5) << 5 | (datIn & 31 << 11) << 8;
			datOut = (datIn & 31) << 19| (datIn & 63 << 5) << 5 | ((datIn>>8) & 0xf8);
			datOuts[0] |= datOut << 24;
			datOuts[1] = datOut >> 8;

			datIn = pSrc[0];
			pSrc += stride;
			// datOut = (datIn & 31) << 3 | (datIn & 63 << 5) << 5 | (datIn & 31 << 11) << 8;
			datOut = (datIn & 31) << 19| (datIn & 63 << 5) << 5 | ((datIn>>8) & 0xf8);
			datOuts[1] |= (datOut << 16) & 0xFFFF0000;
			datOuts[2] = datOut >> 16;

			datIn = pSrc[0];
			pSrc += stride;
			// datOut = (datIn & 31) << 3 | (datIn & 63 << 5) << 5 | (datIn & 31 << 11) << 8;
			datOut = (datIn & 31) << 19| (datIn & 63 << 5) << 5 | ((datIn>>8) & 0xf8);

			datOuts[2] |= datOut << 8;

			if (isSub128) {
				// subtract 128 bytewisely, equal to XOR with 0x80
				datOuts[0] ^= 0x80808080;
				datOuts[1] ^= 0x80808080;
				datOuts[2] ^= 0x80808080;
			}
			memcpy(p888out, datOuts, 3 * 4);
			p888out += 3 * 4;
		}
	}
}

void ezh_copy_slice_to_model_input(uint32_t idx, uint32_t cam_slice_buffer, uint32_t cam_slice_width, uint32_t cam_slice_height, uint32_t max_idx)
{
	static uint8_t* pCurDat;
	uint32_t curY;
	uint32_t s_imgStride = cam_slice_width / MODEL_IN_W;
	#define WND_X0 0
	#define WND_Y0 0

	if (idx > max_idx)
		return;
	//uint32_t ndx = max_idx -1 - idx;
	uint32_t ndx = idx;
	curY = ndx * cam_slice_height;
	int wndY = (s_imgStride - (curY - WND_Y0) % s_imgStride) % s_imgStride;


	if (idx +1 >= max_idx)
		g_isImgBufReady = 1;

	pCurDat = model_input_buf + 3 * ((cam_slice_height * ndx + wndY) * cam_slice_width / s_imgStride / s_imgStride);

	if (curY + cam_slice_height >= WND_Y0){

		if (MODEL_IN_COLOR_BGR == 1) {
			Rgb565StridedToBgr888((uint16_t*)cam_slice_buffer, cam_slice_width, cam_slice_width, cam_slice_height, WND_X0, wndY, pCurDat, s_imgStride, 1);
		}else {
			Rgb565StridedToRgb888((uint16_t*)cam_slice_buffer, cam_slice_width, cam_slice_width, cam_slice_height, WND_X0, wndY, pCurDat, s_imgStride, 1);
		}

		if (s_odRetCnt)
				draw_rect_on_slice_buffer((uint16_t  *)cam_slice_buffer, cam_slice_width,  idx*cam_slice_height, 1, s_odRets, s_odRetCnt,cam_slice_height);
	}
}

const char * GetBriefString(void) {
	uint8_t model_switch = 0;
	static char sz[21] = "xPU:";
	sz[0] = model_switch ? 'C' : 'N';
	sz[4] = 0;
	sprintf(sz+5, "%dobj,%04dms", s_odRetCnt, s_infUs/1000);
	sz[17] = 0;
	return sz;
}

void MODEL_ODPrintResult(const ODResult_t *p, int retCnt) {
	PRINTF("Found boxes count %d\r\n",retCnt);
    for (int i=0; i<retCnt; i++,p++) {
        PRINTF("%0.2f%%, x1: %d, y1: %d, x2: %d, y2: %d\r\n",
            p->score*100.0f, p->x1, p->y1, p->x2, p->y2);
    }
}


void face_det()
{
	tensor_dims_t inputDims;
	tensor_type_t inputType;
	uint8_t* inputData;

	tensor_dims_t outputDims;
	tensor_type_t outputType;
	uint8_t* outputData;
	size_t arenaSize;

	if (MODEL_Init() != kStatus_Success)
	{
		PRINTF("Failed initializing model");
		for (;;) {}
	}

	size_t usedSize = MODEL_GetArenaUsedBytes(&arenaSize);
	PRINTF("\r\n%d/%d kB (%0.2f%%) tensor arena used\r\n", usedSize / 1024, arenaSize / 1024, 100.0*usedSize/arenaSize);

	inputData = MODEL_GetInputTensorData(&inputDims, &inputType);
	outputData = MODEL_GetOutputTensorData(&outputDims, &outputType);
	uint32_t out_size = MODEL_GetOutputSize();

	yolo::object_detection::PostProcessParams postProcessParams =  yolo::object_detection::PostProcessParams{
	    	.inputImgRows = inputDims.data[1],
	    	.inputImgCols =	inputDims.data[2],
			.output_size = out_size,
			.originalImageWidth = CAMERA_WIDTH,
			.originalImageHeight = CAMERA_HEIGHT,
			.threshold = 0.70,
			.nms = 0.45,
			.numClasses = 1,
#if SERVO_ENABLE
			.topN = 1
#else
			.topN = 0
#endif
	    };
	TfLiteTensor* outputTensor[3];
	float *anchors = MODEL_GetAnchors();
	for (int i=0; i < out_size; i ++)
	{
		outputTensor[i] = MODEL_GetOutputTensor(i);
		postProcessParams.anchors[i][0] = *(anchors + 6*i);
		postProcessParams.anchors[i][1] = *(anchors + 6*i + 1);
		postProcessParams.anchors[i][2] = *(anchors + 6*i + 2);
		postProcessParams.anchors[i][3] = *(anchors + 6*i + 3);
		postProcessParams.anchors[i][4] = *(anchors + 6*i + 4);
		postProcessParams.anchors[i][5] = *(anchors + 6*i + 5);
	}

	std::vector<yolo::object_detection::DetectionResult> results;
	yolo::DetectorPostProcess postProcess = yolo::DetectorPostProcess((const TfLiteTensor**)outputTensor,
	                results, postProcessParams);

	while(1)
	{
		if (g_isImgBufReady == 0)
			continue;

		uint8_t *buf = 0;

		memset(inputData,0,inputDims.data[1]*inputDims.data[2]*inputDims.data[3]);
		buf = inputData + (inputData,inputDims.data[1] - MODEL_IN_H) /2 * MODEL_IN_W * MODEL_IN_C;
		memcpy(buf, model_input_buf, MODEL_IN_W*MODEL_IN_H*MODEL_IN_C);

		results.clear();
		auto startTime = TIMER_GetTimeInUS();
		MODEL_RunInference();
		auto endTime = TIMER_GetTimeInUS();

		auto dt = endTime - startTime;
		s_infUs = (uint32_t)dt;
		s_odRetCnt = 0;
		if (!postProcess.DoPostProcess()) {
			PRINTF("Post-processing failed.");
			s_odRetCnt = 0;
		}
		uint32_t area_max = 0;
		for (const auto& result: results) {
			if(result.m_normalisedVal > BOX_SCORE_THRESHOLD) //score of box
			{
				s_odRets[s_odRetCnt].x1 = result.m_x0;
				s_odRets[s_odRetCnt].x2 = result.m_x0 + result.m_w;
				s_odRets[s_odRetCnt].y1 = result.m_y0;
				s_odRets[s_odRetCnt].y2 = result.m_y0 + result.m_h;
				s_odRets[s_odRetCnt].score = result.m_normalisedVal;
#if SERVO_ENABLE
				if((result.m_w * result.m_h) > area_max){
					servo_motor_set_pos((uint16_t)(result.m_x0 + (int(result.m_w) >> 1)),(uint16_t)(result.m_y0 + (int(result.m_h) >> 1)));
					area_max = result.m_w * result.m_h;

				}
#endif
				s_odRetCnt ++;
			}
		}
		if (s_odRetCnt > 0) {
#if SERVO_ENABLE
			servo_motor_set_cnt(1);
#endif
			MODEL_ODPrintResult(s_odRets, s_odRetCnt);
		}

	}
}


}


