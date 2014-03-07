/* SCE CONFIDENTIAL
PhyreEngine(TM) Package 3.7.0.0
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef PHYRE_DEFAULT_SHADER_SHARED_CODE_D3D11_H
#define PHYRE_DEFAULT_SHADER_SHARED_CODE_D3D11_H

#include "PhyreShaderCommonD3D.h"
#include "PhyreSceneWideParametersD3D.h"

// Shared code and shader implementations.

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Context Switch definitions

// The maximum number of lights this shader supports.
#define MAX_NUM_LIGHTS 3

// Context switches
bool PhyreContextSwitches 
< 
string ContextSwitchNames[] = {"NUM_LIGHTS", "LOD_BLEND", "INSTANCING_ENABLED"}; 
int MaxNumLights = MAX_NUM_LIGHTS; 
string SupportedLightTypes[] = {"DirectionalLight","PointLight","SpotLight"};
string SupportedShadowTypes[] = {"PCFShadowMap", "CascadedShadowMap", "CombinedCascadedShadowMap"};
int NumSupportedShaderLODLevels = 1;
>;

#ifdef SHADER_LOD_LEVEL

#if SHADER_LOD_LEVEL > 0
	#ifdef NORMAL_MAPPING_ENABLED
		#undef NORMAL_MAPPING_ENABLED
	#endif //! NORMAL_MAPPING_ENABLED
#endif //! SHADER_LOD_LEVEL > 0
#endif //! SHADER_LOD_LEVEL

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Preprocessing of switches. 
// Setup macros for common combinations of switches
#ifdef NUM_LIGHTS
#if NUM_LIGHTS > MAX_NUM_LIGHTS
#error Maximum number of supported lights exceeded.
#endif //! NUM_LIGHTS > MAX_NUM_LIGHTS
#endif //! NUM_LIGHTS

#ifdef NUM_LIGHTS
#if defined(LIGHTING_ENABLED) && NUM_LIGHTS > 0
#define USE_LIGHTING
#endif //! defined(LIGHTING_ENABLED) && NUM_LIGHTS > 0
#endif //! NUM_LIGHTS

#ifdef NORMAL_MAPPING_ENABLED
#define USE_TANGENTS
#endif //! NORMAL_MAPPING_ENABLED

#if defined(ALPHA_ENABLED) && defined(TEXTURE_ENABLED)
#define SHADOW_TEXTURE_SAMPLING_ENABLED
#define ZPREPASS_TEXTURE_SAMPLING_ENABLED
#endif //! defined(ALPHA_ENABLED) && defined(TEXTURE_ENABLED)

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Global shader parameters.

// Un-tweakables
float4x4 World		: World;		
float4x4 WorldView	: WorldView;		
float4x4 WorldInverse	: WorldInverse;
float4x4 WorldViewProjection		: WorldViewProjection;	
float4x4 WorldViewInverse	: WorldViewInverse;

#ifdef MOTION_BLUR_ENABLED
float4x4 PreviousWorld		: PreviousWorld;	
float4x4 PreviousWorldViewProjection : PreviousWorldViewProjection;
#endif //! MOTION_BLUR_ENABLED

#ifdef SKINNING_ENABLED

	#if 0
		// This is the standard non constant buffer implementation.
		#define NUM_SKIN_TRANSFORMS 80 // Note: This number is mirrored in Core as PD_MATERIAL_SKINNING_MAX_GPU_BONE_COUNT
		float4x4 BoneTransforms[NUM_SKIN_TRANSFORMS] : BONETRANSFORMS;
	#else
		// This is the constant buffer implementation that uses the constant buffer from PhyreCoreShaderShared.h
		cbuffer BoneTransformConstantBuffer
		{
			PSkinTransforms SkinTransforms; 
		}
		#define BoneTransforms SkinTransforms.Mtxs
	#endif

#endif //! SKINNING_ENABLED

// Distance fog parameters.
#ifdef FOG_ENABLED
float4 FogColor : FOGCOLOR;
// FogRangeParameters : x = Near, y = Far, z = 1/(Far-Near)
float4 FogRangeParameters : FOGRANGEPARAMETERS;
#endif //! FOG_ENABLED

// Tone mapping parameters.
#ifdef TONE_MAP_ENABLED
float SceneBrightnessScale;
float InvAverageVisibleLuminanceValueScaled;
#endif //! TONE_MAP_ENABLED

// Material Parameters
float4 MaterialColour : MaterialColour = float4(1.0f,1.0f,1.0f,1.0f);
// Material-supplied lighting Parameters.
float MaterialDiffuse : MaterialDiffuse = 1.0f;
float MaterialEmissiveness : MaterialEmissiveness = 0.0f;
#ifdef SPECULAR_ENABLED
float Shininess : Shininess 
<
float UIMin = 0; float UIMax = 20.0;
>  = 0.0f;
float SpecularPower : SpecularPower
<
float UIMin = 0; float UIMax = 64.0;
> = 16.0f;
float FresnelPower : FresnelPower
<
float UIMin = 0; float UIMax = 64.0;
>  = 5.0f;
#endif //! SPECULAR_ENABLED

// Engine-supplied lighting parameters
#ifdef USE_LIGHTING

// Combined parameters for SIMD lighting calculations..
float4 CombinedLightAttenuationCoeff0 : COMBINEDLIGHTATTENUATIONCOEFF0;		// The first distance attenuation coefficient for all the lights
float4 CombinedLightAttenuationCoeff1 : COMBINEDLIGHTATTENUATIONCOEFF1;		// The second distance attenuation coefficient for all the lights
float4 CombinedLightSpotCoeff0 : COMBINEDLIGHTSPOTCOEFF0;					// The first spot attenuation coefficient for all the lights
float4 CombinedLightSpotCoeff1 : COMBINEDLIGHTSPOTCOEFF1;					// The second spot attenuation coefficient for all the lights

// Separate lighting structures

#if NUM_LIGHTS > 0
LIGHTTYPE_0 Light0 : LIGHT0;
#ifndef SHADOWTYPE_0
#define LightShadow0 0.0f
#define LightShadowMap0 0.0f
#else //! SHADOWTYPE_0
SHADOWTYPE_0 LightShadow0 : LIGHTSHADOW0;
Texture2D <float> LightShadowMap0 : LIGHTSHADOWMAP0;
#endif //! SHADOWTYPE_0
#endif //! NUM_LIGHTS > 0

#if NUM_LIGHTS > 1
LIGHTTYPE_1 Light1 : LIGHT1;
#ifndef SHADOWTYPE_1
#define LightShadow1 0.0f
#define LightShadowMap1 0.0f
#else //! SHADOWTYPE_1
SHADOWTYPE_1 LightShadow1 : LIGHTSHADOW1;
Texture2D <float> LightShadowMap1 : LIGHTSHADOWMAP1;
#endif //! SHADOWTYPE_1
#endif //! NUM_LIGHTS > 1

#if NUM_LIGHTS > 2
LIGHTTYPE_2 Light2 : LIGHT2;
#ifndef SHADOWTYPE_2
#define LightShadow2 0.0f
#define LightShadowMap2 0.0f
#else //! SHADOWTYPE_2
SHADOWTYPE_2 LightShadow2 : LIGHTSHADOW2;
Texture2D <float> LightShadowMap2 : LIGHTSHADOWMAP2;
#endif //! SHADOWTYPE_2
#endif //! NUM_LIGHTS > 2

#if NUM_LIGHTS > 3
LIGHTTYPE_3 Light3 : LIGHT3;
#ifndef SHADOWTYPE_3
#define LightShadow3 0.0f
#define LightShadowMap3 0.0f
#else //! SHADOWTYPE_3
SHADOWTYPE_3 LightShadow3 : LIGHTSHADOW3;
Texture2D <float> LightShadowMap3 : LIGHTSHADOWMAP3;
#endif //! SHADOWTYPE_3
#endif //! NUM_LIGHTS > 3

#endif //! USE_LIGHTING

// Textures
#ifdef TEXTURE_ENABLED
Texture2D <float4> TextureSampler;
sampler TextureSamplerSampler
{
	Filter = Min_Mag_Mip_Linear;
    AddressU = Wrap;
    AddressV = Wrap;
};
#ifdef MULTIPLE_UVS_ENABLED
Texture2D <float4> TextureSampler1;
sampler TextureSampler1Sampler
{
	Filter = Min_Mag_Mip_Linear;
    AddressU = Wrap;
    AddressV = Wrap;
};
#endif //! MULTIPLE_UVS_ENABLED
#endif //! TEXTURE_ENABLED

#ifdef NORMAL_MAPPING_ENABLED
Texture2D <float4> NormalMapSampler;

sampler NormalMapSamplerSampler
{
	Filter = Min_Mag_Mip_Linear;
    AddressU = Wrap;
    AddressV = Wrap;
};
#endif //! NORMAL_MAPPING_ENABLED

#ifdef SSAO_ENABLED
Texture2D <float4> SSAOSampler;
#endif //! SSAO_ENABLED

#ifdef LIGHTPREPASS_ENABLED
Texture2D <float4> LightPrepassSampler;
#endif //! LIGHTPREPASS_ENABLED

#ifdef LOD_BLEND
float LodBlendValue : LodBlendValue;

#define GET_LOD_FRAGMENT_UV(uv) (float2(uv.xy))

#endif //! LOD_BLEND



sampler LinearWrapSampler
{
	Filter = Min_Mag_Mip_Linear;
    AddressU = Wrap;
    AddressV = Wrap;
};

sampler DitherSampler
{
	Filter = Min_Mag_Point_Mip_Linear;
    AddressU = Wrap;
    AddressV = Wrap;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Structures

#ifdef INSTANCING_ENABLED
struct InstancingInput
{
	float4	InstanceTransform0	: TEXCOORD9;
	float4	InstanceTransform1	: TEXCOORD10;
	float4	InstanceTransform2	: TEXCOORD11;
};
#endif //! INSTANCING_ENABLED

struct ZVSInput
{
#ifdef SKINNING_ENABLED
	float3	SkinnableVertex		: POSITION;
	uint4	SkinIndices			: BLENDINDICES;
	float4	SkinWeights			: BLENDWEIGHTS;
#else //! SKINNING_ENABLED
	float3	Position			: POSITION;
#endif //! SKINNING_ENABLED

#ifdef INSTANCING_ENABLED
	InstancingInput instancingInput;
#endif //! INSTANCING_ENABLED
};

struct DefaultVSInput
{
#ifdef VERTEX_COLOR_ENABLED
	float4  Color : COLOR0;
#endif //! VERTEX_COLOR_ENABLED
#ifdef SKINNING_ENABLED
	float3	SkinnableVertex		: POSITION;
	float3	SkinnableNormal		: NORMAL; 	
	uint4	SkinIndices			: BLENDINDICES;
	float4	SkinWeights			: BLENDWEIGHTS;
#else //! SKINNING_ENABLED
	float3	Position			: POSITION;
	float3	Normal				: NORMAL; 	
#endif //! SKINNING_ENABLED

	float2	Uv					: TEXCOORD0;

#ifdef USE_TANGENTS
#ifdef SKINNING_ENABLED
	float3	SkinnableTangent	: TANGENT;
#else //! SKINNING_ENABLED
	float3	Tangent				: TANGENT;
#endif //! SKINNING_ENABLED
#endif //! USE_TANGENTS
#ifdef MULTIPLE_UVS_ENABLED
	float2 Uv1					: TEXCOORD2;
#endif //! MULTIPLE_UVS_ENABLED

#ifdef INSTANCING_ENABLED
	InstancingInput instancingInput;
#endif //! INSTANCING_ENABLED
};

#ifdef USE_TANGENTS
#define FWD_SCREENPOSITION_TEXCOORD TEXCOORD4
#else //! USE_TANGENTS
#define FWD_SCREENPOSITION_TEXCOORD TEXCOORD3
#endif //! USE_TANGENTS

struct DefaultVSForwardRenderOutput
{
#ifdef VERTEX_COLOR_ENABLED
	float4  Color : COLOR0;
#endif //! VERTEX_COLOR_ENABLED
	float4	Position			: SV_POSITION;	
	float2	Uv					: TEXCOORD0;
	float4	WorldPositionDepth	: TEXCOORD1;
	float3	Normal				: TEXCOORD2;
#ifdef USE_TANGENTS
	float3	Tangent				: TEXCOORD3;
#endif //! USE_TANGENTS
#ifdef MULTIPLE_UVS_ENABLED
	float2 Uv1					: TEXCOORD4;
#endif //! MULTIPLE_UVS_ENABLED
};

struct DefaultVSDeferredRenderOutput
{
	float4	Position			: SV_POSITION;	
	float2	Uv					: TEXCOORD0;
	float3	Normal				: TEXCOORD1; 	
	float4	WorldPositionDepth	: TEXCOORD2;
	float4	Color				: COLOR0;
#ifdef USE_TANGENTS
	float3	Tangent				: TEXCOORD3;
#endif //! USE_TANGENTS
#ifdef MULTIPLE_UVS_ENABLED
	float2 Uv1					: TEXCOORD4;
#endif //! MULTIPLE_UVS_ENABLED
};

#ifdef SKINNING_ENABLED

// Evaluate skin for position, normal and tangent, for 4 bone weights.
void EvaluateSkinPositionNormalTangent4Bones(inout float3 position, inout float3 normal, inout float3 tangent, float4 weights, uint4 boneIndices)
{
	

	uint indexArray[4] = {boneIndices.x,boneIndices.y,boneIndices.z,boneIndices.w};

	float4 inPosition = float4(position,1);
	float4 inNormal = float4(normal,0);
	float4 inTangent = float4(tangent,0);
	
 	position = 
		mul(inPosition, BoneTransforms[indexArray[0]]).xyz * weights.x
	+	mul(inPosition, BoneTransforms[indexArray[1]]).xyz * weights.y
	+	mul(inPosition, BoneTransforms[indexArray[2]]).xyz * weights.z
	+	mul(inPosition, BoneTransforms[indexArray[3]]).xyz * weights.w;
	
	normal = 
		mul(inNormal, BoneTransforms[indexArray[0]]).xyz * weights.x
	+	mul(inNormal, BoneTransforms[indexArray[1]]).xyz * weights.y
	+	mul(inNormal, BoneTransforms[indexArray[2]]).xyz * weights.z
	+	mul(inNormal, BoneTransforms[indexArray[3]]).xyz * weights.w;

	tangent = 
		mul(inTangent, BoneTransforms[indexArray[0]]).xyz * weights.x
	+	mul(inTangent, BoneTransforms[indexArray[1]]).xyz * weights.y
	+	mul(inTangent, BoneTransforms[indexArray[2]]).xyz * weights.z
	+	mul(inTangent, BoneTransforms[indexArray[3]]).xyz * weights.w;
		
	
}

void EvaluateSkinPositionNormal4Bones(inout float3 position, inout float3 normal, float4 weights, uint4 boneIndices)
{
	uint indexArray[4] = {boneIndices.x,boneIndices.y,boneIndices.z,boneIndices.w};

	float4 inPosition = float4(position,1);
	float4 inNormal = float4(normal,0);
	
 	position = 
		mul(inPosition, BoneTransforms[indexArray[0]]).xyz * weights.x
	+	mul(inPosition, BoneTransforms[indexArray[1]]).xyz * weights.y
	+	mul(inPosition, BoneTransforms[indexArray[2]]).xyz * weights.z
	+	mul(inPosition, BoneTransforms[indexArray[3]]).xyz * weights.w;
	
	normal = 
		mul(inNormal, BoneTransforms[indexArray[0]]).xyz * weights.x
	+	mul(inNormal, BoneTransforms[indexArray[1]]).xyz * weights.y
	+	mul(inNormal, BoneTransforms[indexArray[2]]).xyz * weights.z
	+	mul(inNormal, BoneTransforms[indexArray[3]]).xyz * weights.w;
	
}

void EvaluateSkinPosition4Bones(inout float3 position, float4 weights, uint4 boneIndices)
{
	uint indexArray[4] = {boneIndices.x,boneIndices.y,boneIndices.z,boneIndices.w};
	float4 inPosition = float4(position,1);
	
 	position = 
		mul(inPosition, BoneTransforms[indexArray[0]]).xyz * weights.x
	+	mul(inPosition, BoneTransforms[indexArray[1]]).xyz * weights.y
	+	mul(inPosition, BoneTransforms[indexArray[2]]).xyz * weights.z
	+	mul(inPosition, BoneTransforms[indexArray[3]]).xyz * weights.w;
}

#endif //! SKINNING_ENABLED

#ifdef INSTANCING_ENABLED
void ApplyInstanceTransformVertex(InstancingInput IN, inout float3 toTransform)
{
	float3 instanceTransformedPosition;
	instanceTransformedPosition.x = dot(IN.InstanceTransform0, float4(toTransform,1));
	instanceTransformedPosition.y = dot(IN.InstanceTransform1, float4(toTransform,1));
	instanceTransformedPosition.z = dot(IN.InstanceTransform2, float4(toTransform,1));
	toTransform = instanceTransformedPosition;
}

void ApplyInstanceTransformNormal(InstancingInput IN, inout float3 toTransform)
{
	float3 instanceTransformedNormal;
	instanceTransformedNormal.x = dot(IN.InstanceTransform0.xyz, toTransform);
	instanceTransformedNormal.y = dot(IN.InstanceTransform1.xyz, toTransform);
	instanceTransformedNormal.z = dot(IN.InstanceTransform2.xyz, toTransform);
	toTransform = instanceTransformedNormal;
}
void ApplyInstanceTransform(inout DefaultVSInput IN)
{
#ifdef SKINNING_ENABLED
	ApplyInstanceTransformVertex(IN.instancingInput, IN.SkinnableVertex.xyz);
	ApplyInstanceTransformNormal(IN.instancingInput, IN.SkinnableNormal.xyz);
	#ifdef USE_TANGENTS
		ApplyInstanceTransformNormal(IN.instancingInput, IN.SkinnableTangent.xyz);
	#endif //! USE_TANGENTS
#else //! SKINNING_ENABLED
	ApplyInstanceTransformVertex(IN.instancingInput, IN.Position.xyz);
	ApplyInstanceTransformNormal(IN.instancingInput, IN.Normal.xyz);
	#ifdef USE_TANGENTS
		ApplyInstanceTransformNormal(IN.instancingInput, IN.Tangent.xyz);
	#endif //! USE_TANGENTS
#endif //! SKINNING_ENABLED
}
#endif //! INSTANCING_ENABLED

float4 GenerateScreenProjectedUv(float4 projPosition)
{
	float2 clipSpaceDivided = projPosition.xy / projPosition.w;
	clipSpaceDivided.y = -clipSpaceDivided.y;
	float2 tc = clipSpaceDivided.xy * 0.5 + 0.5;

	tc *= float2(960.0f/256.0f,544.0f/256.0f);

	return float4(tc * projPosition.w,0,projPosition.w);	
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Vertex shaders

// Default shadow vertex shader.
float4 DefaultShadowVS(ZVSInput IN) : SV_POSITION
{
#ifdef SKINNING_ENABLED
	float3 position = IN.SkinnableVertex.xyz;
	EvaluateSkinPosition4Bones(position.xyz, IN.SkinWeights, IN.SkinIndices);	
#else  //! SKINNING_ENABLED
	float3 position = IN.Position.xyz;
#endif //! SKINNING_ENABLED
#ifdef INSTANCING_ENABLED
	ApplyInstanceTransformVertex(IN.instancingInput, position);
#endif //! INSTANCING_ENABLED

#ifdef SKINNING_ENABLED
	return mul(float4(position.xyz,1), ViewProjection);
#else //! SKINNING_ENABLED
	return mul(float4(position.xyz,1), WorldViewProjection);
#endif //! SKINNING_ENABLED
}

// Default Z prepass vertex shader.
float4 DefaultZPrePassVS(ZVSInput IN) : SV_POSITION
{
#ifdef SKINNING_ENABLED
	float3 position = IN.SkinnableVertex.xyz;
	EvaluateSkinPosition4Bones(position.xyz, IN.SkinWeights, IN.SkinIndices);
#else //! SKINNING_ENABLED
	float3 position = IN.Position.xyz;
#endif //! SKINNING_ENABLED

#ifdef INSTANCING_ENABLED
	ApplyInstanceTransformVertex(IN.instancingInput, position);
#endif //! INSTANCING_ENABLED

#ifdef SKINNING_ENABLED
	return mul(float4(position.xyz,1), ViewProjection);
#else //! SKINNING_ENABLED
	return mul(float4(position.xyz,1), WorldViewProjection);
#endif //! SKINNING_ENABLED
}

// Default forward render vertex shader
DefaultVSForwardRenderOutput DefaultForwardRenderVS(DefaultVSInput IN)
{
	DefaultVSForwardRenderOutput OUT;

#ifdef SKINNING_ENABLED
	#ifdef USE_TANGENTS
		EvaluateSkinPositionNormalTangent4Bones(IN.SkinnableVertex.xyz, IN.SkinnableNormal.xyz, IN.SkinnableTangent.xyz, IN.SkinWeights, IN.SkinIndices);
	#else //! USE_TANGENTS
		EvaluateSkinPositionNormal4Bones(IN.SkinnableVertex.xyz, IN.SkinnableNormal.xyz, IN.SkinWeights, IN.SkinIndices);
	#endif //! USE_TANGENTS
	float3 position = IN.SkinnableVertex.xyz;
	OUT.Position = mul(float4(position,1.0f), ViewProjection);
	OUT.WorldPositionDepth = float4(position.xyz, -mul(float4(position,1.0f), View).z);
	OUT.Normal = normalize(IN.SkinnableNormal.xyz);
	#ifdef USE_TANGENTS
		OUT.Tangent = normalize(IN.SkinnableTangent.xyz);
	#endif //! USE_TANGENTS
#else //! SKINNING_ENABLED
	#ifdef INSTANCING_ENABLED
		ApplyInstanceTransform(IN);
	#endif //! INSTANCING_ENABLED
		float3 position = IN.Position.xyz;
		OUT.Position = mul(float4(position,1.0f), WorldViewProjection);
		OUT.WorldPositionDepth = float4(mul(float4(position,1.0f),World).xyz, -mul(float4(position,1.0f),WorldView).z);
		OUT.Normal = normalize(mul(float4(IN.Normal,0), World).xyz);
	#ifdef USE_TANGENTS
		OUT.Tangent = normalize(mul(float4(IN.Tangent,0), World).xyz);
	#endif //! USE_TANGENTS
#endif //! SKINNING_ENABLED	

	OUT.Uv.xy = IN.Uv;

#ifdef MULTIPLE_UVS_ENABLED
	OUT.Uv1.xy = IN.Uv1;
#endif //! MULTIPLE_UVS_ENABLED

#ifdef VERTEX_COLOR_ENABLED
	OUT.Color = IN.Color * MaterialColour;
#endif //! VERTEX_COLOR_ENABLED
   
	return OUT;
}

// Default forward render vertex shader
DefaultVSDeferredRenderOutput DefaultDeferredRenderVS(DefaultVSInput IN)
{
	DefaultVSDeferredRenderOutput OUT;

#ifdef SKINNING_ENABLED
	#ifdef USE_TANGENTS
		EvaluateSkinPositionNormalTangent4Bones(IN.SkinnableVertex.xyz, IN.SkinnableNormal.xyz, IN.SkinnableTangent.xyz, IN.SkinWeights, IN.SkinIndices);
	#else //! USE_TANGENTS
		EvaluateSkinPositionNormal4Bones(IN.SkinnableVertex.xyz, IN.SkinnableNormal.xyz, IN.SkinWeights, IN.SkinIndices);
	#endif //! USE_TANGENTS
	float3 position = IN.SkinnableVertex.xyz;
	OUT.Position = mul(float4(position,1.0f), ViewProjection);
	OUT.WorldPositionDepth = float4(position.xyz, -mul(float4(position,1.0f), View).z);
	OUT.Normal = normalize(mul(float4(IN.SkinnableNormal.xyz,0), View).xyz);
	#ifdef USE_TANGENTS
		OUT.Tangent = normalize(mul(float4(IN.SkinnableTangent.xyz,0), View).xyz);
	#endif //! USE_TANGENTS
#else //! SKINNING_ENABLED
	#ifdef INSTANCING_ENABLED
		ApplyInstanceTransform(IN);
	#endif //! INSTANCING_ENABLED
	float3 position = IN.Position.xyz;
	OUT.Position = mul(float4(position,1.0), WorldViewProjection);
	OUT.WorldPositionDepth = float4(mul(float4(position,1.0), World).xyz, -mul(float4(position,1.0), WorldView).z);
	OUT.Normal = normalize(mul(float4(IN.Normal,0), WorldView).xyz);
	#ifdef USE_TANGENTS
		OUT.Tangent = normalize(mul(float4(IN.Tangent,0), WorldView).xyz);
	#endif //! USE_TANGENTS
#endif //! SKINNING_ENABLED

	OUT.Uv.xy = IN.Uv;
#ifdef MULTIPLE_UVS_ENABLED
	OUT.Uv1.xy = IN.Uv1;
#endif //! MULTIPLE_UVS_ENABLED
	OUT.Color = MaterialColour * float4(MaterialDiffuse,MaterialDiffuse,MaterialDiffuse,1.0);

#ifdef VERTEX_COLOR_ENABLED
	OUT.Color = OUT.Color * IN.Color;
#endif //! VERTEX_COLOR_ENABLED
    
	return OUT;
}

#ifdef NORMAL_MAPPING_ENABLED
float3 EvaluateNormalMapNormal(float3 inNormal, float2 inUv, float3 inTangent)
{
	float4 normalMapValue = NormalMapSampler.Sample(NormalMapSamplerSampler,inUv);
	float3 normalMapNormal = normalize(normalMapValue.xyz * 2.0h - 1.0h);

	// Evaluate tangent basis
	float3 basis0 = normalize(inTangent);
	float3 basis2 = normalize(inNormal);
	float3 basis1 = cross(basis0, basis2);

	float3 normal = (normalMapNormal.x * basis0) + (normalMapNormal.y * basis1) + (normalMapNormal.z * basis2);	
	return normal;
}
#endif //! NORMAL_MAPPING_ENABLED

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fragment utility macros. Just make the fragment shader code a bit neater by hiding some of the combination handling / argument passing here.

#ifdef PARALLAX_OFFSET_MAPPING_ENABLED
#define EvaluateNormal(In) EvaluateParallaxMapNormal(In.Normal.xyz,In.Uv,In.Tangent, NormalMapSampler, normalize(mul(float4(IN.WorldPositionDepth.xyz,1), View).xyz))
#elif defined(NORMAL_MAPPING_ENABLED)
#define EvaluateNormal(In) EvaluateNormalMapNormal(In.Normal.xyz,In.Uv,In.Tangent)
#else //! PARALLAX_OFFSET_MAPPING_ENABLED
#define EvaluateNormal(In) EvaluateStandardNormal(In.Normal.xyz)
#endif //! PARALLAX_OFFSET_MAPPING_ENABLED

#ifdef RECEIVE_SHADOWS
#define EvaluateShadowValue(LightId, LightShadowId, ShadowMapId, worldPos, viewDepth) EvaluateShadow(LightId, LightShadowId, ShadowMapId, worldPos, viewDepth)
#else //! RECEIVE_SHADOWS
#define EvaluateShadowValue(LightId, LightShadowId, ShadowMapId, worldPos, viewDepth) 1.0f
#endif //! RECEIVE_SHADOWS

#ifdef SPECULAR_ENABLED
#define EvaluateLightFunction(LightIndex) \
	{ \
	float shad = EvaluateShadowValue(Light##LightIndex, LightShadow##LightIndex, LightShadowMap##LightIndex, worldPosition, In.WorldPositionDepth.w); \
		lightResult += EvaluateLight(Light##LightIndex, worldPosition, normal, -eyeDirection, shad, shininess, SpecularPower); \
	}
#else //! SPECULAR_ENABLED
#define EvaluateLightFunction(LightIndex) \
	{ \
		float shad = EvaluateShadowValue(Light##LightIndex, LightShadow##LightIndex, LightShadowMap##LightIndex, worldPosition, In.WorldPositionDepth.w); \
		lightResult += EvaluateLight(Light##LightIndex, worldPosition, normal, shad); \
	}
#endif //! SPECULAR_ENABLED

#ifdef LOD_BLEND

float LODDitherRandom(int n)
{
	n = (n << 13) ^ n;
	int rval = (n * (n*n*15731+789221) + 1376312589) & 0x7fffffff;

	return float(rval) * (1.0f/2147483647.0f);
}

float GetLODDitherValue(float2 screenUv)
{	
	
	float4 ditherValue = DitherNoiseTexture.Load(int3(screenUv,0) & 255);
	float threshold = (1.0f - abs(LodBlendValue));

	float lodBlendValueSign = sign(LodBlendValue);
	float rslt = ((ditherValue.x >= threshold) ? 1.0f : -1.0f) * lodBlendValueSign;
	return rslt;
}
#endif //! LOD_BLEND

// Tone mapping.
#ifdef TONE_MAP_ENABLED

float4 ToneMap(float3 colourValue)
{
	float lum = dot(colourValue,float3(0.299h,0.587h,0.144h)) ;
	float lumToneMap = lum * InvAverageVisibleLuminanceValueScaled;
    float lumToneMap1 = lumToneMap + 1;
    float lD = (lumToneMap*(1 + (lumToneMap * SceneBrightnessScale)))/lumToneMap1;
    
	// divide by luminance 
    float3 colourResult = colour / lum;
   	colourResult *= lD;
   
	return float4(colourResult, (lum * SceneBrightnessScale) + (1.0f/255.0f));
}

#endif //! TONE_MAP_ENABLED

// Fog.
#ifdef FOG_ENABLED

float3 EvaluateFog(float3 colourValue, float3 viewPosition)
{
	float fogAmt = saturate((abs(viewPosition.z)-FogRangeParameters.x) * FogRangeParameters.z);
	return lerp(colourValue,FogColor.xyz,fogAmt);
}

#endif //! FOG_ENABLED

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fragment shaders

// Default fragment shader. 
float4 DefaultUnshadedFP(float4 ScreenPosition : SV_POSITION) : FRAG_OUTPUT_COLOR0
{
#ifdef LOD_BLEND
	clip(GetLODDitherValue(GET_LOD_FRAGMENT_UV(ScreenPosition)));
#endif //! LOD_BLEND
	return 1;
}

// Default fragment shader. 
float4 DefaultShadowFP() : FRAG_OUTPUT_COLOR0
{
	return 0.0f;
}

float4 PackNormalAndViewSpaceDepth(float3 normal, float viewSpaceZ)
{
	float normalizedViewZ = viewSpaceZ / cameraNearFar.y;
	float2 depthPacked = float2(floor(normalizedViewZ * 256.0f) / 255.0f,  frac(normalizedViewZ * 256.0f));
	float4 rslt = float4(normal.xy, depthPacked);
	return rslt;
}
float4 PackNormalAndDepth(float3 normal, float depth)
{
	float viewSpaceZ = -(cameraNearTimesFar / (depth * cameraFarMinusNear - cameraNearFar.y));
	return PackNormalAndViewSpaceDepth(normal,viewSpaceZ);
}

// Default light pre pass first pass shader. Outputs normal and defaulted specular power only.
float4 DefaultLightPrepassFP(DefaultVSForwardRenderOutput In) : FRAG_OUTPUT_COLOR0
{
	float3 normal = EvaluateNormal(In);
#ifdef SPECULAR_ENABLED
	float specPower = SpecularPower;
#else //! SPECULAR_ENABLED
	float specPower = 0.0f;
#endif //! SPECULAR_ENABLED

	return float4(normal.xyz * 0.5f+0.5f,specPower);
}


// Deferred rendering
PSDeferredOutput DefaultDeferredRenderFP(DefaultVSDeferredRenderOutput In) 
{
	float3 normal = EvaluateNormal(In);

#ifdef SPECULAR_ENABLED
	// could vary by pixel with a texture lookup
	float specPower = SpecularPower;
	float gloss = Shininess;
#else //! SPECULAR_ENABLED
	float specPower = 0;
	float gloss = 0;
#endif //! SPECULAR_ENABLED

	float4 colour = In.Color;
#ifdef TEXTURE_ENABLED
	colour *= TextureSampler.Sample(TextureSamplerSampler, In.Uv);
#endif //! TEXTURE_ENABLED
	colour.w *= gloss;

	float3 viewSpaceNormal = normal;
				
#ifdef LOD_BLEND
	clip(GetLODDitherValue(GET_LOD_FRAGMENT_UV(In.Position)));
#endif //! LOD_BLEND
	
	PSDeferredOutput Out;
	// Albedo Colour.xyz, Emissiveness
	Out.Colour = float4(colour.xyz, MaterialEmissiveness);
	// Normal.xyz, Gloss
	Out.NormalDepth = float4(viewSpaceNormal.xyz * 0.5f + 0.5f, colour.w);
	return Out;
}



float3 EvaluateLightingDefault(DefaultVSForwardRenderOutput In, float3 worldPosition, float3 normal, float glossValue)
{
	// Lighting
	float3 lightResult = 1;	

#ifdef USE_LIGHTING
	lightResult = GlobalAmbientColor;

#ifdef SPECULAR_ENABLED
	float3 eyeDirection = normalize(worldPosition - EyePosition);
	float shininess = Shininess * glossValue;
#endif //! SPECULAR_ENABLED

	
#if NUM_LIGHTS > 0
	EvaluateLightFunction(0);
#endif //! NUM_LIGHTS > 0
#if NUM_LIGHTS > 1
	EvaluateLightFunction(1);
#endif //! NUM_LIGHTS > 1
#if NUM_LIGHTS > 2
	EvaluateLightFunction(2);
#endif //! NUM_LIGHTS > 2
#if NUM_LIGHTS > 3
	EvaluateLightFunction(3);
#endif //! NUM_LIGHTS > 2

#endif //! USE_LIGHTING

	return lightResult;
}

#endif

