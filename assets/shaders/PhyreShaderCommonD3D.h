/* SCE CONFIDENTIAL
PhyreEngine(TM) Package 3.7.0.0
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef PHYRE_SHADER_COMMON_H
#define PHYRE_SHADER_COMMON_H

#include "PhyreShaderPlatform.h"
#include "PhyreShaderDefsD3D.h"

// Common code which is likely to be used by multiple shader files is placed in here. 

#define UNNORMALIZE_SKININDICES(VAR) /* Nothing - skin indices are supplied using the SKININDICES semantic as 4 uints on D3D11. */

float ClampPlusMinusOneToNonNegative(float value)
{
	return saturate(value);
}

SamplerComparisonState ShadowMapSampler
{
	Filter = Comparison_Min_Mag_Linear_Mip_Point;
    AddressU = Clamp;
    AddressV = Clamp;
	ComparisonFunc = Less;
};

// SampleShadowMap
// Sample a shadow map with 1 sample tap. 
float SampleShadowMap(float4 shadowPosition, Texture2D <float> shadowMap)
{
#ifdef DCC_TOOL
	// no shadows in DCC TOOL mode 
	return 1.0f;
#else //! DCC_TOOL
	float4 shadowPositionProjected = shadowPosition / shadowPosition.w;
	float shad = shadowMap.SampleCmpLevelZero(ShadowMapSampler,shadowPositionProjected.xy, shadowPositionProjected.z).x;
	return shad;
#endif //! DCC_TOOL
}


// SampleOrthographicShadowMap
// Sample a shadow map with 1 sample tap and no perspective correction.
float SampleOrthographicShadowMap(float3 shadowPosition, Texture2D <float> shadowMap)
{
#ifdef DCC_TOOL
	// no shadows in DCC TOOL mode 
	return 1.0f;
#else //! DCC_TOOL
//	shadowPosition.y = 1.0f - shadowPosition.y;
	float shad = shadowMap.SampleCmpLevelZero(ShadowMapSampler,shadowPosition.xy, shadowPosition.z).x;
	return shad;
#endif //! DCC_TOOL
}


// SampleShadowMapPCF5
// Sample a shadow map with 5 sample taps. 
float SampleShadowMapPCF5(float4 shadowPosition, Texture2D <float> shadowMap)
{
	float w = shadowPosition.w * 1.0f/1024.0f;
	float h = shadowPosition.w * 1.0f/1024.0f;
	float4 sampleOffsets[5] = 
	{
		float4(0,0,0,0),
		float4(-w,-h,0,0),
		float4(w,-h,0,0),
		float4(-w,h,0,0),
		float4(w,h,0,0)
	};
	const float shadowWeights[5] =
	{
		0.5f,
		0.125f,
		0.125f,
		0.125f,
		0.125f
	};
	float shadowRslt = 0;
	for(int i = 0; i < 5; ++i)
	{
		shadowRslt += SampleShadowMap(shadowPosition + sampleOffsets[i], shadowMap) * shadowWeights[i];
	}
	return shadowRslt;
}

// EvaluateSpotFalloff
// Evaluate the directional attenuation for a spot light.
float EvaluateSpotFalloff(float dp, float cosInnerAngle, float cosOuterAngle)
{
	float a = (cosOuterAngle - dp) / (cosOuterAngle - cosInnerAngle);
	a = saturate(a);
	return a * a;
}

/*
float3 EvaluateNormalMapNormal(float3 inNormal, float2 inUv, float3 inTangent, uniform sampler2D normalMapSampler)
{
	float4 normalMapValue = h4tex2D(normalMapSampler, inUv);
	float3 normalMapNormal = normalize(normalMapValue.xyz * 2.0h - 1.0h);

	// Evaluate tangent basis
	float3 basis0 = normalize(inTangent);
	float3 basis2 = normalize(inNormal);
	float3 basis1 = cross(basis0, basis2);

	float3 normal = (normalMapNormal.x * basis0) + (normalMapNormal.y * basis1) + (normalMapNormal.z * basis2);	
	return normal;
	
}


float3 EvaluateParallaxMapNormal(float3 inNormal, float2 inUv, float3 inTangent, uniform sampler2D normalMapSampler, float3 eyeDir)
{
	// Evaluate tangent basis
	float3 binormal = cross(inTangent, inNormal);
	float3 basis0 = normalize(inTangent);
	float3 basis1 = normalize(binormal);
	float3 basis2 = normalize(inNormal);

	// Get the view vector in tangent space
	float3 viewVecTangentSpace;
	viewVecTangentSpace.x = dot(eyeDir,basis0);
	viewVecTangentSpace.y = dot(eyeDir,basis1);
	viewVecTangentSpace.z = dot(eyeDir,basis2);	

	float3 texCoord = float3(inUv,1);
	
	// Evaluate parallax mapping.
	const float parallax = 0.025f;
	float height = tex2D(normalMapSampler, texCoord.xy).w ;
	float offset = parallax * (2.0f * height - 1.0f) ;
	float2 parallaxTexCoord = texCoord.xy + offset * viewVecTangentSpace.xy;

	// Sample normal map
	float4 normalMapValue = h4tex2D(normalMapSampler, parallaxTexCoord);
	float3 normalMapNormal = normalize(normalMapValue.xyz * 2.0h - 1.0h);

	float3 normal = (normalMapNormal.x * basis0) + (normalMapNormal.y * basis1) + (normalMapNormal.z * basis2);
	return normal;
}
*/

float3 EvaluateStandardNormal(float3 inNormal)
{
	return normalize(inNormal).xyz;
}

float calcDiffuseLightAmt(float3 lightDir, float3 normal)
{
	// Diffuse calcs.
	float diffuseLightAmt = dot(lightDir,normal);

#ifdef WRAP_DIFFUSE_LIGHTING
	diffuseLightAmt = diffuseLightAmt * 0.5h + 0.5h;
	diffuseLightAmt *= diffuseLightAmt;
#else //! WRAP_DIFFUSE_LIGHTING
	diffuseLightAmt = ClampPlusMinusOneToNonNegative(diffuseLightAmt);
#endif //! WRAP_DIFFUSE_LIGHTING

	return diffuseLightAmt;
}

float calcSpecularLightAmt(float3 normal, float3 lightDir, float3 eyeDirection, float shininess, float specularPower /*, float fresnelPower*/)
{
	// Specular calcs
	float3 floatVec = normalize(eyeDirection + lightDir);
	float nDotH = ClampPlusMinusOneToNonNegative(dot(normal,floatVec));

	//float fresnel = saturate( 1 - pow(abs(dot(normal, eyeDirection)), fresnelPower) );
	float specularLightAmount = ClampPlusMinusOneToNonNegative(pow(nDotH, specularPower)) * shininess; // * fresnel

	return specularLightAmount;
}

float calculateAttenuation(float distanceToLight, float4 attenuationProperties)
{
	// attenuationProperties contains:
	// innerRange, outerRange, 1.0f/(outerRange/innerRange), (-innerRange / (outerRange/innerRange)
	float attenValue = ClampPlusMinusOneToNonNegative(distanceToLight * attenuationProperties.z + attenuationProperties.w);
	return 1.0 - (float)attenValue;
}


float calculateAttenuationQuadratic(float distanceToLightSqr, float4 attenuationProperties)
{
	// attenuationProperties contains:
	// innerRange, outerRange, 1.0f/(outerRange/innerRange), (-innerRange / (outerRange/innerRange)
	float rd = (attenuationProperties.y * attenuationProperties.y ) - (attenuationProperties.x*attenuationProperties.x);
	float b = 1.0f / rd;
	float a = attenuationProperties.x*attenuationProperties.x;
	float c = a * b + 1.0f;

#ifdef __psp2__
	float coeff0 = (float)(-b);
	float coeff1 = (float)(c);
	float attenValue = saturate(distanceToLightSqr * coeff0 + coeff1);
#else
	float coeff0 = (float)(-b);
	float coeff1 = (float)(c);
	float attenValuef = saturate(distanceToLightSqr * coeff0 + coeff1);
	float attenValue = (float)attenValuef;
#endif

	return attenValue;
}




#ifdef SPECULAR_ENABLED

float3 EvaluateLighting(float3 normal, float3 lightDir, float3 eyeDirection, float shadowAmount, float3 lightColour, float attenuationResult, float shininess, float specularPower/*, float fresnelPower*/)
{
	// Diffuse and specular calcs.
	float diffuseLightAmt = calcDiffuseLightAmt(lightDir, normal);
	float specularLightAmount = calcSpecularLightAmt(normal, lightDir, eyeDirection, shininess, specularPower/*, fresnelPower*/);

	float3 lightResult = lightColour * (diffuseLightAmt + specularLightAmount);

	return lightResult * shadowAmount * attenuationResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Lighting code - could/should be defined in an external .h file. 

float3 EvaluateLight(DirectionalLight light,float3 worldPosition, float3 normal, float3 eyeDirection, float shadowAmount, float shininess, float specularPower/*, float fresnelPower*/)
{
	return EvaluateLighting(normal,light.m_direction,eyeDirection,shadowAmount, light.m_colorIntensity, 1, shininess, specularPower/*, fresnelPower*/);
}
float3 EvaluateLight(PointLight light,float3 worldPosition, float3 normal, float3 eyeDirection, float shadowAmount, float shininess, float specularPower/*, float fresnelPower*/)
{
	float3 offset = light.m_position - worldPosition;
	float vecLengthSqr = dot(offset, offset);
  	float3 lightDir = offset / sqrt(vecLengthSqr);
	float atten = calculateAttenuationQuadratic(vecLengthSqr, light.m_attenuation);

	//return 0.8f;//float4(normalize(offset)*0.5f+0.5f,1.0f);

	return EvaluateLighting(normal,lightDir,eyeDirection,shadowAmount, light.m_colorIntensity, atten, shininess, specularPower/*, fresnelPower*/);	
}
float3 EvaluateLight(SpotLight light,float3 worldPosition, float3 normal, float3 eyeDirection, float shadowAmount, float shininess, float specularPower/*, float fresnelPower*/)
{
	float3 offset = light.m_position - worldPosition;
	float vecLengthSqr = dot(offset, offset);
  	float3 lightDir = offset / sqrt(vecLengthSqr);
	float angle = dot(lightDir, light.m_direction);
	if(angle > light.m_spotAngles.w)
	{
		float atten = EvaluateSpotFalloff( angle, light.m_spotAngles.z, light.m_spotAngles.w );
		atten *= calculateAttenuationQuadratic(vecLengthSqr, light.m_attenuation);
		return EvaluateLighting(normal,lightDir,eyeDirection,shadowAmount, light.m_colorIntensity, atten, shininess, specularPower/*, fresnelPower*/);	
	}
	return 0;
}

#else //! SPECULAR_ENABLED

float3 EvaluateLighting(float3 normal, float3 lightDir, float shadowAmount, float3 lightColour, float attenuationResult)
{
	// Diffuse calcs.
	float diffuseLightAmt = calcDiffuseLightAmt(lightDir, normal);

	float3 lightResult = lightColour * diffuseLightAmt;

	return lightResult * shadowAmount * attenuationResult;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Lighting code - could/should be defined in an external .h file. 

float3 EvaluateLight(DirectionalLight light,float3 worldPosition, float3 normal, float shadowAmount)
{	
	return EvaluateLighting(normal, light.m_direction, shadowAmount, light.m_colorIntensity, 1);
}
float3 EvaluateLight(PointLight light,float3 worldPosition, float3 normal, float shadowAmount)
{	
	float3 offset = light.m_position - worldPosition;
	float vecLengthSqr = dot(offset, offset);
  	float3 lightDir = offset / sqrt(vecLengthSqr);
  	float atten = calculateAttenuationQuadratic(vecLengthSqr, light.m_attenuation);

	//return 0.3f;//float4(normalize(offset)*0.5f+0.5f,1.0f);

	return EvaluateLighting(normal, lightDir, shadowAmount, light.m_colorIntensity, atten);	
}
float3 EvaluateLight(SpotLight light,float3 worldPosition, float3 normal, float shadowAmount)
{
	float3 offset = light.m_position - worldPosition;
	float vecLengthSqr = dot(offset, offset);
  	float3 lightDir = offset / sqrt(vecLengthSqr);
	float angle = dot(lightDir, light.m_direction);

	if(angle > light.m_spotAngles.w)
	{
		float atten = EvaluateSpotFalloff( angle, light.m_spotAngles.z, light.m_spotAngles.w );
		atten *= calculateAttenuationQuadratic(vecLengthSqr, light.m_attenuation);
		return EvaluateLighting(normal, lightDir, shadowAmount, light.m_colorIntensity, atten);	
	}
	return 0;
}

#endif //! SPECULAR_ENABLED


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Shadow code.

// No shadow.
float EvaluateShadow(DirectionalLight light, float dummy, float dummy2, float3 worldPosition, float viewDepth)
{
	return 1;
}
float EvaluateShadow(PointLight light, float dummy, float dummy2, float3 worldPosition, float viewDepth)
{
	return 1;
}
float EvaluateShadow(SpotLight light, float dummy, float dummy2, float3 worldPosition, float viewDepth)
{
	return 1;
}

// PCF shadow.
float EvaluateShadow(SpotLight light, PCFShadowMap shadow, Texture2D <float> shadowMap, float3 worldPosition, float viewDepth)
{
	float4 shadowPosition = mul(float4(worldPosition,1), shadow.m_shadowTransform);
	return SampleShadowMap(shadowPosition, shadowMap);
}

float EvaluateShadow(DirectionalLight light, CascadedShadowMap shadow, Texture2D <float> shadowMap, float3 worldPosition, float viewDepth)
{
	return 1.0f;
}

/*
float EvaluateShadow(DirectionalLight light, CascadedShadowMap shadow, Texture2D <float> shadowMap, float3 worldPosition, float viewDepth)
{
	// Brute force cascaded split map implementation - sample all the splits, then determine which result to use.

	float3 shadowPosition0 = mul(shadow.m_split0Transform,float4(worldPosition,1)).xyz;
	float3 shadowPosition1 = mul(shadow.m_split1Transform,float4(worldPosition,1)).xyz;
	float3 shadowPosition2 = mul(shadow.m_split2Transform,float4(worldPosition,1)).xyz;
	float3 shadowPosition3 = mul(shadow.m_split3Transform,float4(worldPosition,1)).xyz;

	float split0 = SampleOrthographicShadowMap(shadowPosition0,shadowMap);
	float split1 = SampleOrthographicShadowMap(shadowPosition1,shadowMap);
	float split2 = SampleOrthographicShadowMap(shadowPosition2,shadowMap);
	float split3 = SampleOrthographicShadowMap(shadowPosition3,shadowMap);

	if(viewDepth < shadow.m_splitDistances.x)
		return split0;
	else if(viewDepth < shadow.m_splitDistances.y)
		return split1;
	else if(viewDepth < shadow.m_splitDistances.z)
		return split2;
	else if(viewDepth < shadow.m_splitDistances.w)
		return split3;
	else 
		return 1;
	
}
*/

float EvaluateShadow(DirectionalLight light, CombinedCascadedShadowMap shadow, Texture2D <float> shadowMap, float3 worldPosition, float viewDepth)
{
	// Brute force cascaded split map implementation - sample all the splits, then determine which result to use.

	float3 shadowPosition0 = mul(float4(worldPosition,1), shadow.m_split0Transform).xyz;
	float3 shadowPosition1 = mul(float4(worldPosition,1), shadow.m_split1Transform).xyz;
	float3 shadowPosition2 = mul(float4(worldPosition,1), shadow.m_split2Transform).xyz;
	float3 shadowPosition3 = mul(float4(worldPosition,1), shadow.m_split3Transform).xyz;
	
	float3 shadowPosition = viewDepth < shadow.m_splitDistances.y ? 
							(viewDepth < shadow.m_splitDistances.x ? shadowPosition0 : shadowPosition1)
							:
							(viewDepth < shadow.m_splitDistances.z ? shadowPosition2 : shadowPosition3);

	float result = viewDepth < shadow.m_splitDistances.w ? SampleOrthographicShadowMap(shadowPosition,shadowMap) : 1;
	
	return result;
}


/*
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Skinning 

// Evaluate skin for position, normal and tangent, for 4 bone weights.
void EvaluateSkinPositionNormalTangent4Bones( inout float3 position, inout float3 normal, inout float3 tangent, float4 weights, int4 boneIndices, uniform float3x4 skinTransforms[] )
{
	int indexArray[4] = {boneIndices.x,boneIndices.y,boneIndices.z,boneIndices.w};

	float4 inPosition = float4(position,1);
	float4 inNormal = float4(normal,0);
	float4 inTangent = float4(tangent,0);
	
 	position = 
		mul(skinTransforms[indexArray[0]], inPosition).xyz * weights.x
	+	mul(skinTransforms[indexArray[1]], inPosition).xyz * weights.y
	+	mul(skinTransforms[indexArray[2]], inPosition).xyz * weights.z
	+	mul(skinTransforms[indexArray[3]], inPosition).xyz * weights.w;
	
	normal = 
		mul(skinTransforms[indexArray[0]], inNormal).xyz * weights.x
	+	mul(skinTransforms[indexArray[1]], inNormal).xyz * weights.y
	+	mul(skinTransforms[indexArray[2]], inNormal).xyz * weights.z
	+	mul(skinTransforms[indexArray[3]], inNormal).xyz * weights.w;

	tangent = 
		mul(skinTransforms[indexArray[0]], inTangent).xyz * weights.x
	+	mul(skinTransforms[indexArray[1]], inTangent).xyz * weights.y
	+	mul(skinTransforms[indexArray[2]], inTangent).xyz * weights.z
	+	mul(skinTransforms[indexArray[3]], inTangent).xyz * weights.w;
		
}

void EvaluateSkinPositionNormal4Bones( inout float3 position, inout float3 normal, float4 weights, int4 boneIndices, uniform float3x4 skinTransforms[] )
{
	int indexArray[4] = {boneIndices.x,boneIndices.y,boneIndices.z,boneIndices.w};

	float4 inPosition = float4(position,1);
	float4 inNormal = float4(normal,0);
	
 	position = 
		mul(skinTransforms[indexArray[0]], inPosition).xyz * weights.x
	+	mul(skinTransforms[indexArray[1]], inPosition).xyz * weights.y
	+	mul(skinTransforms[indexArray[2]], inPosition).xyz * weights.z
	+	mul(skinTransforms[indexArray[3]], inPosition).xyz * weights.w;
	
	normal = 
		mul(skinTransforms[indexArray[0]], inNormal).xyz * weights.x
	+	mul(skinTransforms[indexArray[1]], inNormal).xyz * weights.y
	+	mul(skinTransforms[indexArray[2]], inNormal).xyz * weights.z
	+	mul(skinTransforms[indexArray[3]], inNormal).xyz * weights.w;
}

void EvaluateSkinPosition1Bone( inout float3 position, float4 weights, int4 boneIndices, uniform float3x4 skinTransforms[] )
{
	int index = boneIndices.x;

	float4 inPosition = float4(position,1.0);
	position = mul(skinTransforms[index], inPosition).xyz;		
}

void EvaluateSkinPosition2Bones( inout float3 position, float4 weights, int4 boneIndices, uniform float3x4 skinTransforms[] )
{
	int indexArray[2] = {boneIndices.x,boneIndices.y};

	float4 inPosition = float4(position,1);
	float scale = 1.0f / (weights.x + weights.y);
 	position = 
		mul(skinTransforms[indexArray[0]], inPosition).xyz * (weights.x * scale)
	+	mul(skinTransforms[indexArray[1]], inPosition).xyz * (weights.y * scale);
}

void EvaluateSkinPosition3Bones( inout float3 position, float4 weights, int4 boneIndices, uniform float3x4 skinTransforms[] )
{
	int indexArray[3] = {boneIndices.x,boneIndices.y,boneIndices.z};

	float4 inPosition = float4(position,1);
	float scale = 1.0f / (weights.x + weights.y + weights.z);
 	position = 
		mul(skinTransforms[indexArray[0]], inPosition).xyz * (weights.x * scale)
	+	mul(skinTransforms[indexArray[1]], inPosition).xyz * (weights.y * scale)
	+	mul(skinTransforms[indexArray[2]], inPosition).xyz * (weights.z * scale);
}

void EvaluateSkinPosition4Bones( inout float3 position, float4 weights, int4 boneIndices, uniform float3x4 skinTransforms[] )
{
	int indexArray[4] = {boneIndices.x,boneIndices.y,boneIndices.z,boneIndices.w};

	float4 inPosition = float4(position,1);
	
 	position = 
		mul(skinTransforms[indexArray[0]], inPosition).xyz * weights.x
	+	mul(skinTransforms[indexArray[1]], inPosition).xyz * weights.y
	+	mul(skinTransforms[indexArray[2]], inPosition).xyz * weights.z
	+	mul(skinTransforms[indexArray[3]], inPosition).xyz * weights.w;
}
*/

#endif
