/* SCE CONFIDENTIAL
PhyreEngine(TM) Package 3.7.0.0
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef PHYRE_SHADER_DEFS_H
#define PHYRE_SHADER_DEFS_H

// A list of context switches that the engine knows about.

//#define NUM_LIGHTS 3
//#define NUM_LIGHTS 1
//#define TONE_MAP_ENABLED
//#define FOG_ENABLED
//#define ZPREPASS_ENABLED
//#define SSAO_ENABLED
//#define MOTION_BLUR_ENABLED
//#define LIGHTPREPASS_ENABLED

// A list of platforms/hosts the engine might define.
//#define PS3
//#define WIN32
//#define WIN32_NVIDIA
//#define WIN32_ATI
//#define MAYA
//#define MAX
//#define LEVEL_EDITOR
//#define DCC_TOOL


// DCC tool mode - disables some context switches and e.g. shadowmapping.
#if defined(MAYA) || defined(MAX) || defined(LEVEL_EDITOR)
#define DCC_TOOL
#endif


// A list of light structures.

struct DirectionalLight
{
	half3 m_direction : LIGHTDIRECTIONWS;
	half3 m_colorIntensity : LIGHTCOLORINTENSITY;	
};

struct PointLight
{
	float3 m_position : LIGHTPOSITIONWS;
	half3 m_colorIntensity : LIGHTCOLORINTENSITY;	
	float4 m_attenuation : LIGHTATTENUATION;
};

struct SpotLight
{
	float3 m_position : LIGHTPOSITIONWS;
	half3 m_direction : LIGHTDIRECTIONWS;
	half3 m_colorIntensity : LIGHTCOLORINTENSITY;
	half4 m_spotAngles : LIGHTSPOTANGLES;
	float4 m_attenuation : LIGHTATTENUATION;
};

struct PCFShadowMap
{
	float4x4 m_shadowTransform : SHADOWTRANSFORM;
	sampler2D m_shadowMap : SHADOWMAP;
};
struct CascadedShadowMap
{
#ifdef __psp2__
	float3x4 m_splitTransformArray[4] : SHADOWTRANSFORMARRAY;
#else //! __psp2__
	float3x4 m_split0Transform : SHADOWTRANSFORMSPLIT0;
	float3x4 m_split1Transform : SHADOWTRANSFORMSPLIT1;
	float3x4 m_split2Transform : SHADOWTRANSFORMSPLIT2;
	float3x4 m_split3Transform : SHADOWTRANSFORMSPLIT3;
#endif //! __psp2__
	sampler2D m_split0ShadowMap : SHADOWMAPSPLIT0;
	sampler2D m_split1ShadowMap : SHADOWMAPSPLIT1;
	sampler2D m_split2ShadowMap : SHADOWMAPSPLIT2;
	sampler2D m_split3ShadowMap : SHADOWMAPSPLIT3;
	float4 m_splitDistances : SHADOWSPLITDISTANCES;
};
struct CombinedCascadedShadowMap
{
#ifdef __psp2__
	float3x4 m_splitTransformArray[4] : SHADOWTRANSFORMARRAY;
#else //! __psp2__
	float3x4 m_split0Transform : SHADOWTRANSFORMSPLIT0;
	float3x4 m_split1Transform : SHADOWTRANSFORMSPLIT1;
	float3x4 m_split2Transform : SHADOWTRANSFORMSPLIT2;
	float3x4 m_split3Transform : SHADOWTRANSFORMSPLIT3;
#endif //! __psp2__
	sampler2D m_shadowMap : SHADOWMAP;
	float4 m_splitDistances : SHADOWSPLITDISTANCES;
};

// Output structure for deferred lighting fragment shader.
struct PSDeferredOutput
{
	float4 Colour : COLOR0;
#ifndef __psp2__
	float4 NormalDepth : COLOR1;
#endif //! __psp2__
};





#endif