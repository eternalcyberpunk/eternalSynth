/*******************************************************************/
/*                                                                 */
/*                      ADOBE CONFIDENTIAL                         */
/*                   _ _ _ _ _ _ _ _ _ _ _ _ _                     */
/*                                                                 */
/* Copyright 2007-2023 Adobe Inc.                                  */
/* All Rights Reserved.                                            */
/*                                                                 */
/* NOTICE:  All information contained herein is, and remains the   */
/* property of Adobe Inc. and its suppliers, if                    */
/* any.  The intellectual and technical concepts contained         */
/* herein are proprietary to Adobe Inc. and its                    */
/* suppliers and may be covered by U.S. and Foreign Patents,       */
/* patents in process, and are protected by trade secret or        */
/* copyright law.  Dissemination of this information or            */
/* reproduction of this material is strictly forbidden unless      */
/* prior written permission is obtained from Adobe Inc.            */
/* Incorporated.                                                   */
/*                                                                 */
/*******************************************************************/

/*
	eternalSynth.h
*/

#pragma once

#ifndef ETERNALSYNTH_H
#define ETERNALSYNTH_H

typedef unsigned char		u_char;
typedef unsigned short		u_short;
typedef unsigned short		u_int16;
typedef unsigned long		u_long;
typedef short int			int16;
#define PF_TABLE_BITS	12
#define PF_TABLE_SZ_16	4096

#define PF_DEEP_COLOR_AWARE 1	// make sure we get 16bpc pixels; 
								// AE_Effect.h checks for this.

#include "AEConfig.h"

#ifdef AE_OS_WIN
	typedef unsigned short PixelType;
	#include <Windows.h>
#endif



#include "entry.h"
#include "AE_Effect.h"
#include "AE_EffectCB.h"
#include "AE_Macros.h"
#include "Param_Utils.h"
#include "AE_EffectCBSuites.h"
#include "String_Utils.h"
#include "AE_GeneralPlug.h"
#include "AEFX_ChannelDepthTpl.h"
#include "AEGP_SuiteHandler.h"

#include "eternalSynth_Strings.h"

#include <iostream>



/* Versioning information */

#define	MAJOR_VERSION	1
#define	MINOR_VERSION	1
#define	BUG_VERSION		0
#define	STAGE_VERSION	PF_Stage_DEVELOP
#define	BUILD_VERSION	1


/* Parameter defaults */

#define	ETERNALSYNTH_GAIN_MIN		0
#define	ETERNALSYNTH_GAIN_MAX		100
#define	ETERNALSYNTH_GAIN_DFLT		0

enum {
	ETERNALSYNTH_INPUT = 0,
	ETERNALSYNTH_GAIN,
	ETERNALSYNTH_COLOR,
	DIFFUSION_CHECKBOX,
	ETERNALSYNTH_NUM_PARAMS
};

enum {
	GAIN_DISK_ID = 1,
	COLOR_DISK_ID,
	CHECKBOX_DISK_ID,
	ONE_DIM_DITHER_DISK_ID
};

typedef struct GainInfo{
	PF_FpLong	gainF;
	INT			checkboxValue;
	PF_Pixel8	colorNum;
} GainInfo, *GainInfoP, **GainInfoH;


typedef struct {
	PF_EffectWorld* src;  // Pointer to source image
	PF_EffectWorld* dst;  // Pointer to destination image
	A_long width;         // Image width
	A_long height;        // Image height
	PF_FpLong synthValue; // Synth slider value
	PF_Pixel8 color;       // Color picker value
} DitherContext;



extern "C" {

	DllExport
	PF_Err
	EffectMain(
		PF_Cmd			cmd,
		PF_InData		*in_data,
		PF_OutData		*out_data,
		PF_ParamDef		*params[],
		PF_LayerDef		*output,
		void			*extra);

}

#endif // ETERNALSYNTH_H