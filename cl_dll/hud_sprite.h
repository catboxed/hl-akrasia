// Based on code from Xash3D and BugfixedHL-Rebased

#pragma once
#ifndef HUD_SCALED_H
#define HUD_SCALED_H

#include "cdll_int.h"
#include "com_model.h"

struct OriginalSpriteEngfuncs
{
	void	(*pfnSetCrosshair)( HSPRITE hspr, wrect_t rc, int r, int g, int b );
};

class ScaledRenderer {
public:
	static ScaledRenderer &Instance() {
		static ScaledRenderer instance;

		return instance;
	}

	ScaledRenderer();

	void EnableCustomRendering();
	void DisableCustomRendering();

	float GetHUDScale();

	int ScreenWidthScaled();
	int ScreenHeightScaled();

	int ScaleScreen(int value);
	int UnscaleScreen(int value);

	void SPR_SetInternal(HSPRITE hPic, int r, int g, int b);
	void SPR_DrawInternal(int frame, float x, float y, float width, float height, const wrect_t *dimensions, int mode);

	int HUD_VidInit();
	void HUD_Init();
	void HUD_Frame(double time);

	void SPR_Set(HSPRITE hPic, int r, int g, int b);
	void SPR_DrawAdditive(int frame, int x, int y, const wrect_t *prc);

	void FillRGBA(int x, int y, int width, int height, int r, int g, int b, int a);

	void SetCrosshair(HSPRITE hspr, wrect_t rc, int r, int g, int b);
	void QueryCrosshairInfo(HSPRITE *sprite, model_t **sprite_model, wrect_t *sprite_dimensions, color24 *sprite_color);

private:
	void ResetCrosshair();

	OriginalSpriteEngfuncs origSpriteEngfuncs;

	HSPRITE sprite;
	model_t *sprite_model;
	color24 sprite_color;

	HSPRITE crosshair;
	model_t *crosshair_model;
	wrect_t crosshair_dimensions;
	color24 crosshair_color;

	float hud_renderer_value;
	float hud_auto_scale_value;
};

#endif
