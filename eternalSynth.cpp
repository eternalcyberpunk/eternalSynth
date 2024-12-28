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

/*	eternalSynth.cpp

	This is a compiling husk of a project. Fill it in with interesting
	pixel processing code.

	Revision History

	Version		Change													Engineer	Date
	=======		======													========	======
	1.0			(seemed like a good idea at the time)					bbb			6/1/2002

	1.0			Okay, I'm leaving the version at 1.0,					bbb			2/15/2006
				for obvious reasons; you're going to
				copy these files directly! This is the
				first XCode version, though.

	1.0			Let's simplify this barebones sample					zal			11/11/2010

	1.0			Added new entry point									zal			9/18/2017
	1.1			Added 'Support URL' to PiPL and entry point				cjr			3/31/2023

*/

#include "eternalSynth.h"
#include "AE_Effect.h"
#include "AE_EffectUI.h"
#include "Param_Utils.h"
#include "AEFX_SuiteHelper.h"
#include "AEGP_SuiteHandler.h"
#include <thread>
#include <vector>
#include <algorithm>

static PF_Err
About(
	PF_InData* in_data,
	PF_OutData* out_data,
	PF_ParamDef* params[],
	PF_LayerDef* output)
{
	AEGP_SuiteHandler suites(in_data->pica_basicP);

	suites.ANSICallbacksSuite1()->sprintf(out_data->return_msg,
		"%s v%d.%d\r%s",
		STR(StrID_Name),
		MAJOR_VERSION,
		MINOR_VERSION,
		STR(StrID_Description));
	return PF_Err_NONE;
}

static PF_Err
GlobalSetup(
	PF_InData* in_data,
	PF_OutData* out_data,
	PF_ParamDef* params[],
	PF_LayerDef* output)
{
	out_data->my_version = PF_VERSION(MAJOR_VERSION,
		MINOR_VERSION,
		BUG_VERSION,
		STAGE_VERSION,
		BUILD_VERSION);

	out_data->out_flags = PF_OutFlag_DEEP_COLOR_AWARE; // just 16bpc, not 32bpc

	return PF_Err_NONE;
}

static PF_Err
ParamsSetup(
	PF_InData* in_data,
	PF_OutData* out_data,
	PF_ParamDef* params[],
	PF_LayerDef* output)
{
	PF_Err		err = PF_Err_NONE;
	PF_ParamDef	def;

	AEFX_CLR_STRUCT(def);
	PF_ADD_FLOAT_SLIDERX("synth",
		ETERNALSYNTH_GAIN_MIN,
		ETERNALSYNTH_GAIN_MAX,
		ETERNALSYNTH_GAIN_MIN,
		ETERNALSYNTH_GAIN_MAX,
		ETERNALSYNTH_GAIN_DFLT,
		PF_Precision_TENTHS,
		0,
		0,
		GAIN_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_COLOR(STR(StrID_Color_Param_Name),
		PF_HALF_CHAN8,
		PF_MAX_CHAN8,
		PF_MAX_CHAN8,
		COLOR_DISK_ID);

	AEFX_CLR_STRUCT(def);
	PF_ADD_CHECKBOX(STR(StrID_CheckBox_Param_Name),
		"glitches shi",
		NULL,
		0,
		CHECKBOX_DISK_ID);

	out_data->num_params = ETERNALSYNTH_NUM_PARAMS;

	return err;
}

static PF_Err
MySimpleGainFunc16(
	void* refcon,
	A_long		xL,
	A_long		yL,
	PF_Pixel16* inP,
	PF_Pixel16* outP)
{
	PF_Err		err = PF_Err_NONE;

	GainInfo* giP = reinterpret_cast<GainInfo*>(refcon);
	PF_FpLong	tempF = 0;

	if (giP) {
		tempF = giP->gainF * PF_MAX_CHAN16 / 100.0;
		if (tempF > PF_MAX_CHAN16) {
			tempF = PF_MAX_CHAN16;
		};

		outP->alpha = inP->alpha;
		outP->red = MIN((inP->red + (A_u_char)tempF), PF_MAX_CHAN16);
		outP->green = MIN((inP->green + (A_u_char)tempF), PF_MAX_CHAN16);
		outP->blue = MIN((inP->blue + (A_u_char)tempF), PF_MAX_CHAN16);
	}

	return err;
}

static PF_Err
MySimpleGainFunc8(
	void* refcon,
	A_long		xL,
	A_long		yL,
	PF_Pixel8* inP,
	PF_Pixel8* outP)
{
	PF_Err		err = PF_Err_NONE;

	GainInfo* giP = reinterpret_cast<GainInfo*>(refcon);
	PF_FpLong	tempF = 0;

	if (giP) {
		tempF = giP->gainF * PF_MAX_CHAN8 / 100.0;
		if (tempF > PF_MAX_CHAN8) {
			tempF = PF_MAX_CHAN8;
		};

		outP->alpha = inP->alpha;
		outP->red = MIN((inP->red + (A_u_char)tempF), PF_MAX_CHAN8);
		outP->green = MIN((inP->green + (A_u_char)tempF), PF_MAX_CHAN8);
		outP->blue = MIN((inP->blue + (A_u_char)tempF), PF_MAX_CHAN8);
	}

	if (giP->checkboxValue == 1) {
		outP->alpha = inP->alpha;
		outP->red = giP->colorNum.red;
		outP->green = giP->colorNum.green;
		outP->blue = giP->colorNum.blue;
	};

	return err;
}

static PF_Err
ErrorDiffusionDither8(
	void* refcon,
	A_long xL,
	A_long yL,
	PF_Pixel8* inP,
	PF_Pixel8* outP)
{
	PF_Err err = PF_Err_NONE;

	DitherContext* context = reinterpret_cast<DitherContext*>(refcon);

	// Convert input to grayscale
	uint8_t grayscale = (uint8_t)(0.299 * inP->red + 0.587 * inP->green + 0.114 * inP->blue);

	// Adjust threshold based on synth slider
	uint8_t threshold = static_cast<uint8_t>(127 + context->synthValue * 1.27);

	// Apply dithering threshold
	uint8_t new_value = grayscale > threshold ? 255 : 0;

	// Apply color if above threshold, else set to black
	if (new_value == 255) {
		outP->red = context->color.red;
		outP->green = context->color.green;
		outP->blue = context->color.blue;
	}
	else {
		outP->red = outP->green = outP->blue = 0;
	}

	outP->alpha = inP->alpha;

	// Calculate error
	int error = grayscale - new_value;

	// Spread error to the next pixel in the same row
	if (xL + 1 < context->width) {
		PF_Pixel8* neighbor = (PF_Pixel8*)((char*)context->src->data + yL * context->src->rowbytes + (xL + 1) * sizeof(PF_Pixel8));
		neighbor->red = (uint8_t)std::min<int>(255, std::max<int>(0, neighbor->red + error));
	}

	return err;
}

static PF_Err Render(
	PF_InData* in_data,
	PF_OutData* out_data,
	PF_ParamDef* params[],
	PF_LayerDef* output)
{
	PF_Err err = PF_Err_NONE;

	// Check if the effect is turned off
	if (params[DIFFUSION_CHECKBOX]->u.bd.value == 0) {
		// Copy input directly to output
		if (params[ETERNALSYNTH_INPUT] && params[ETERNALSYNTH_INPUT]->u.ld.data) {
			PF_LayerDef* original_layer = &params[ETERNALSYNTH_INPUT]->u.ld;
			PF_Pixel8* orig_pixels = (PF_Pixel8*)original_layer->data;
			PF_Pixel8* out_pixels = (PF_Pixel8*)output->data;

			for (int y = 0; y < output->height; ++y) {
				for (int x = 0; x < output->width; ++x) {
					PF_Pixel8* orig_pixel = orig_pixels + y * original_layer->rowbytes / sizeof(PF_Pixel8) + x;
					PF_Pixel8* out_pixel = out_pixels + y * output->rowbytes / sizeof(PF_Pixel8) + x;

					// Copy original pixel values directly to output
					*out_pixel = *orig_pixel;
				}
			}
		}
		return err; // Skip further processing
	}

	// Existing dithering logic or additional processing

	PF_Pixel black = { 0, 0, 0, 0 };
	ERR(PF_FILL(&black, NULL, output));

	// Setup context for rendering
	DitherContext context;
	context.src = &params[ETERNALSYNTH_INPUT]->u.ld;
	context.dst = output;
	context.width = output->width;
	context.height = output->height;

	// Cap the synth slider value
	PF_FpLong cappedSynthValue = std::clamp(params[ETERNALSYNTH_GAIN]->u.fs_d.value, 0.0, 100.0);
	context.synthValue = cappedSynthValue;
	context.color = params[ETERNALSYNTH_COLOR]->u.cd.value;

	// Multi-threaded rendering
	const int NUM_THREADS = std::thread::hardware_concurrency();
	int rows_per_thread = context.height / NUM_THREADS;

	std::vector<std::thread> workers;

	for (int t = 0; t < NUM_THREADS; ++t) {
		int start_row = t * rows_per_thread;
		int end_row = (t == NUM_THREADS - 1) ? context.height : start_row + rows_per_thread;

		workers.emplace_back([&context, start_row, end_row]() {
			for (int y = start_row; y < end_row; ++y) {
				for (int x = 0; x < context.width; ++x) {
					PF_Pixel8* in_pixel = reinterpret_cast<PF_Pixel8*>(
						(char*)context.src->data + y * context.src->rowbytes + x * sizeof(PF_Pixel8));
					PF_Pixel8* out_pixel = reinterpret_cast<PF_Pixel8*>(
						(char*)context.dst->data + y * context.dst->rowbytes + x * sizeof(PF_Pixel8));

					ErrorDiffusionDither8(&context, x, y, in_pixel, out_pixel);
				}
			}
			});
	}

	for (auto& worker : workers) {
		worker.join();
	}

	return err;
}



extern "C" DllExport
PF_Err PluginDataEntryFunction2(
	PF_PluginDataPtr inPtr,
	PF_PluginDataCB2 inPluginDataCallBackPtr,
	SPBasicSuite* inSPBasicSuitePtr,
	const char* inHostName,
	const char* inHostVersion)
{
	PF_Err result = PF_Err_INVALID_CALLBACK;

	result = PF_REGISTER_EFFECT_EXT2(
		inPtr,
		inPluginDataCallBackPtr,
		"eternalSynth", // Name
		"ADBE eternalSynth", // Match Name
		"Eternal Plugz", // Category
		AE_RESERVED_INFO, // Reserved Info
		"EffectMain",	// Entry point
		"https://www.adobe.com");	// support URL

	return result;
}


PF_Err
EffectMain(
	PF_Cmd			cmd,
	PF_InData* in_data,
	PF_OutData* out_data,
	PF_ParamDef* params[],
	PF_LayerDef* output,
	void* extra)
{
	PF_Err		err = PF_Err_NONE;

	try {
		switch (cmd) {
		case PF_Cmd_ABOUT:

			err = About(in_data,
				out_data,
				params,
				output);
			break;

		case PF_Cmd_GLOBAL_SETUP:

			err = GlobalSetup(in_data,
				out_data,
				params,
				output);
			break;

		case PF_Cmd_PARAMS_SETUP:

			err = ParamsSetup(in_data,
				out_data,
				params,
				output);
			break;

		case PF_Cmd_RENDER:

			err = Render(in_data,
				out_data,
				params,
				output);
			break;
		}
	}
	catch (PF_Err& thrown_err) {
		err = thrown_err;
	}
	return err;
}
