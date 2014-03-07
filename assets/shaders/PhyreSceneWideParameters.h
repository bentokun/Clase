/* SCE CONFIDENTIAL
PhyreEngine(TM) Package 3.7.0.0
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef PHYRE_SCENE_WIDE_PARAMETERS_H
#define PHYRE_SCENE_WIDE_PARAMETERS_H

///////////////////////////////////////////////////////////////////////////////
// Scene wide parameters
///////////////////////////////////////////////////////////////////////////////

struct SceneWideParameters
{
	float3		EyePosition				: EYEPOSITIONWS;

	float4x4	View					: View;
	float4x4	Projection				: Projection;
	float4x4	ViewProjection			: ViewProjection;	
	float4x4	ViewInverse				: ViewInverse;

	float2		cameraNearFar			: CAMERANEARFAR;
	float		cameraNearTimesFar		: CAMERANEARTIMESFAR;
	float		cameraFarMinusNear		: CAMERAFARMINUSNEAR;

	float2		ViewportWidthHeight		: ViewportWidthHeight;

	float2		screenWidthHeightInv	: SCREENWIDTHHEIGHTINV;
	
	half3		GlobalAmbientColor		: GlobalAmbientColor;

	float		Time					: Time;
};

sampler2D	DitherNoiseTexture : DITHERNOISETEXTURE;

#ifdef __psp2__
SceneWideParameters scene : BUFFER[0];		// Put scene wide parameters in a constant buffer on PlayStation(R)Vita.
#else //! __psp2__
SceneWideParameters scene;
#endif //! __psp2__

#endif //! PHYRE_SCENE_WIDE_PARAMETERS_H
