/* SCE CONFIDENTIAL
PhyreEngine(TM) Package 3.7.0.0
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include "PhyreShaderPlatform.h"
#include "PhyreSceneWideParametersD3D.h"

// Context switches
bool PhyreContextSwitches 
< 
string ContextSwitchNames[] = {"ORTHO_CAMERA"}; 
>;

float4 ScreenPosToView;
float2 InvProjXY;

float DepthUnpackScale;

float DeferredInstantIntensity;
float DeferredInstantScatteringIntensity;

float3 DeferredPos;
float4x4 DeferredWorldTransform;
float4x4 DeferredShadowMatrix;

float4x4 DeferredShadowMatrixSplit0;
float4x4 DeferredShadowMatrixSplit1;
float4x4 DeferredShadowMatrixSplit2;
float4x4 DeferredShadowMatrixSplit3;
float4 DeferredSplitDistances;

float3 DeferredDir;
float4 DeferredColor;
float4 DeferredSpotAngles;
float4 DeferredAttenParams;
float4 DeferredAttenParams2;
float3 DeferredAmbientColor;

float4 DeferredShadowMask;
float DeferredShadowAlpha;

float2 ViewportWidthHeightInv;

Texture2D <float> DepthBuffer;
Texture2D <float4> NormalDepthBuffer;
Texture2D <float4> ShadowBuffer;
Texture2D <float4> ColorBuffer;
Texture2D <float4> LightBuffer;
Texture2D <float> ShadowTexture;

Texture2D <float4> InstantLightColorBuffer;
Texture2D <float4> InstantLightNormalBuffer;
Texture2D <float4> InstantLightPositionBuffer;

struct DeferredDirectionalLight
{
	float4 m_color;
	float4 m_shadowMask;
	float3 m_direction;
	uint m_pad;
};
	
cbuffer DeferredLightConstantBuffer
{
	PDeferredLight DeferredLights[64];
}
cbuffer DeferredDirectionalLightConstantBuffer
{
	DeferredDirectionalLight DeferredDirectionalLights[64];
}
uint NumDeferredLights;
uint NumDeferredDirectionalLights;

StructuredBuffer <uint2> LightingOutputBuffer;
RWStructuredBuffer <uint2> RWLightingOutputBuffer;
RWStructuredBuffer <uint2> RWScatteredLightingOutputBuffer;

float4x4 WorldViewInverse;
StructuredBuffer <uint> DeferredLightCount;
StructuredBuffer <PDeferredLight> DeferredInstantLights;

// Tile size: 8 pixels
#define PE_DEFERRED_TILE_SIZE_SHIFT 3
#define PE_DEFERRED_TILE_SIZE (1<<PE_DEFERRED_TILE_SIZE_SHIFT)

///////////////////////////////////////////////////////////////
// structures /////////////////////
///////////////////////////////////////////////////////////////

struct FullscreenVertexIn
{
	float3 vertex		: POSITION;
	float2 uv			: TEXCOORD0;
};

struct FullscreenVertexOut
{
	float4 position		: SV_POSITION;
	float2 uv			: TEXCOORD0;
	float3 screenPos	: TEXCOORD3;
};
struct LightRenderVertexIn
{
	float3 vertex	: POSITION;
};

struct LightRenderVertexOut
{
	float4 Position		: SV_POSITION;
};



sampler PointClampSampler
{
	Filter = Min_Mag_Mip_Point;
	AddressU = Clamp;
	AddressV = Clamp;
};
SamplerComparisonState ShadowMapSampler
{
	Filter = Comparison_Min_Mag_Linear_Mip_Point;
	AddressU = Clamp;
	AddressV = Clamp;
	ComparisonFunc = Less;
};


///////////////////////////////////////////////////////////////
// Vertex programs ////////////////////////////////////////////
///////////////////////////////////////////////////////////////


FullscreenVertexOut FullscreenVP(FullscreenVertexIn input)
{
	FullscreenVertexOut output;

	output.position = float4(input.vertex.x, -input.vertex.y, 1, 1);
	output.uv = input.uv;

	output.screenPos.z = -1.0;
	output.screenPos.xy = output.uv * 2.0 - 1.0;
	output.screenPos.y = -output.screenPos.y;
	output.screenPos.xy *= InvProjXY;
	
	return output;
}


LightRenderVertexOut RenderLightVP(LightRenderVertexIn input)
{
	LightRenderVertexOut output;
	
	float4 worldPosition = mul(float4(input.vertex.xyz,1), DeferredWorldTransform);
	output.Position = mul(float4(worldPosition.xyz,1), ViewProjection);
	
	return output;
}


///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

// Convert a depth value from post projection space to view space. 
float ConvertDepth(float depth)
{	
#ifdef ORTHO_CAMERA
	float viewSpaceZ = -(depth * cameraFarMinusNear + cameraNearFar.x);
#else //! ORTHO_CAMERA
	float viewSpaceZ = -(cameraNearTimesFar / (depth * cameraFarMinusNear - cameraNearFar.y));
#endif //! ORTHO_CAMERA
	return viewSpaceZ;
}

float2 GetScreenPosition(float2 uv)
{
	float2 screenPos = float2(uv.x,1.0f-uv.y) * 2.0f - 1.0f;
	return screenPos;
}

void GetWorldNormalAndPosition(out float3 worldNormal, out float3 worldPosition, float2 uv)
{
	float2 screenPos = GetScreenPosition(uv);
	float4 normalMapValue = NormalDepthBuffer.SampleLevel(PointClampSampler,uv, 0);
	
	float zvalue = DepthBuffer.SampleLevel(PointClampSampler,uv, 0);
	float viewSpaceDepth = ConvertDepth(zvalue);
		
	float2 normalMapNormalXY = normalMapValue.xy * 2.0f - 1.0f;
	float3 viewSpaceNormal = float3(normalMapNormalXY, sqrt( max( 1.0f - dot(normalMapNormalXY.xy, normalMapNormalXY.xy), 0.0f))   );	
	worldNormal = mul(float4(viewSpaceNormal,0), ViewInverse).xyz;
	worldNormal = normalize(worldNormal);
	
#ifdef ORTHO_CAMERA
	float4 viewPos = float4(screenPos * InvProjXY.xy, -viewSpaceDepth, 1);
#else //! ORTHO_CAMERA
	float4 viewPos = float4(screenPos * InvProjXY.xy * viewSpaceDepth, -viewSpaceDepth, 1);
#endif //! ORTHO_CAMERA	
		
	worldPosition = mul(viewPos, ViewInverse).xyz;	
}

float EvaluateSpotFalloff(float dp)
{
	float atten = 1;
	if( dp < DeferredSpotAngles.z)
	{
		atten = 0;
		if( dp > DeferredSpotAngles.w)
		{
			float a = (DeferredSpotAngles.w - dp) / (DeferredSpotAngles.w - DeferredSpotAngles.z);
			a = max(a,0);
			atten = a * a;
		}
	}
	return atten;
}

float calcSpecularLightAmt(float3 normal, float3 lightDir, float3 eyeDirection, float shininess, float specularPower /*, float fresnelPower*/)
{
	// Specular calcs
	float3 floatVec = normalize(eyeDirection + lightDir);
	float nDotH = saturate(dot(normal,floatVec));

	//float fresnel = saturate( 1 - pow(abs(dot(normal, eyeDirection)), fresnelPower) );
	float specularLightAmount = saturate(pow(nDotH, specularPower)) * shininess; // * fresnel

	return specularLightAmount;
}

float4 RenderPointLightFP(LightRenderVertexOut input) : FRAG_OUTPUT_COLOR
{	
	float2 uv = input.Position.xy * ViewportWidthHeightInv;
	
	float3 worldPosition;
	float3 worldNormal;
	GetWorldNormalAndPosition(worldNormal, worldPosition, uv);
	float3 eyeDirection = normalize(worldPosition - EyePosition);
	float4 normalMapValue = NormalDepthBuffer.Load(int3(input.Position.xy, 0));
		
	float3 lightDir = DeferredPos - worldPosition;
	float3 lightDirNrm = normalize((float3)lightDir);
	float dist = length(lightDir);
	float dp = dot(lightDirNrm,worldNormal);
		
	float distanceAttenuation = 1-saturate(smoothstep(DeferredAttenParams.x,DeferredAttenParams.y,dist));
	
	float specularValue = calcSpecularLightAmt(worldNormal, eyeDirection, lightDirNrm, normalMapValue.w, 16.0f);
	
	float3 rslt = DeferredColor.xyz * saturate(dp) * distanceAttenuation;
	
	return float4(rslt,1.0f);
}

float4 RenderSpotLightFP(LightRenderVertexOut input) : FRAG_OUTPUT_COLOR
{	
	float2 uv = input.Position.xy * ViewportWidthHeightInv;
	float3 worldPosition;
	float3 worldNormal;
				
	GetWorldNormalAndPosition(worldNormal, worldPosition, uv);
	
	float3 eyeDirection = normalize(EyePosition - worldPosition);
	float4 normalMapValue = NormalDepthBuffer.Load(int3(input.Position.xy, 0));
	
	float3 lightDir = DeferredPos - worldPosition;
	float3 lightDirNrm = normalize((float3)lightDir);
	float dist = length(lightDir);
	float dp = dot(lightDirNrm,worldNormal);
	
	float specularValue = calcSpecularLightAmt(worldNormal, eyeDirection, lightDirNrm, normalMapValue.w, 7.0f);
	dp = saturate(dp);
	
	float spotDp = dot(lightDirNrm,DeferredDir);
	float spotAttenuation = EvaluateSpotFalloff(max(spotDp,0));
			
	float distanceAttenuation = 1-saturate(smoothstep(DeferredAttenParams.x,DeferredAttenParams.y,dist));

	float3 rslt = DeferredColor.xyz * distanceAttenuation * spotAttenuation * (specularValue + dp);

	float shadowBufferValue = dot(ShadowBuffer.Load(int3(input.Position.xy, 0)), DeferredShadowMask);
	shadowBufferValue += saturate(1.0f - dot(DeferredShadowMask,1.0f));
	
	rslt *= shadowBufferValue;

	return float4(rslt,1.0f);
}


float4 RenderDirectionalLightFP(FullscreenVertexOut input) : FRAG_OUTPUT_COLOR
{
	float3 worldPosition;
	float3 worldNormal;
	
	float4 normalMapValue = NormalDepthBuffer.Sample(PointClampSampler,input.uv.xy);	
	float3 viewSpaceNormal = float3(normalMapValue.xyz * 2.0f - 1.0f);

	worldNormal = mul(float4(viewSpaceNormal,0), ViewInverse).xyz;
	worldNormal = normalize(worldNormal);

	float3 lightValue = DeferredColor.xyz * saturate(dot(DeferredDir, worldNormal));

	return float4(lightValue,1.0f);
}


float4 ShadowSpotLightFP(LightRenderVertexOut input) : FRAG_OUTPUT_COLOR
{	
	float2 uv = input.Position.xy * ViewportWidthHeightInv;
	float2 screenPos = GetScreenPosition(uv);
	
	float zvalue = DepthBuffer.Load(int3(input.Position.xy, 0)).x;  
	float unpackedDepth = abs(ConvertDepth(zvalue));

#ifdef ORTHO_CAMERA
	float4 viewPos = float4(screenPos, unpackedDepth, 1);
#else //! ORTHO_CAMERA
	float4 viewPos = float4(screenPos * unpackedDepth, unpackedDepth, 1);
#endif //! ORTHO_CAMERA
	float4 shadowPosition = mul(viewPos, DeferredShadowMatrix);

	shadowPosition.xyz /= shadowPosition.w;
	
	#define kShadowSize (3.0f/4096.0f)
	
	float4 offsets[2] = 
	{
		float4(-kShadowSize,-kShadowSize,0,0),
		float4( kShadowSize, kShadowSize,0,0),
	};

//	shadowPosition.z = shadowPosition.z * 0.5f + 0.5f;
//	shadowPosition.z -= 0.00001f;

	float shadowValue0 = ShadowTexture.SampleCmpLevelZero(ShadowMapSampler,shadowPosition.xy + offsets[0].xy, shadowPosition.z).x;
	float shadowValue1 = ShadowTexture.SampleCmpLevelZero(ShadowMapSampler,shadowPosition.xy + offsets[1].xy, shadowPosition.z).x;
	float rslt = (shadowValue0+shadowValue1)*0.5f;
	
	return DeferredShadowMask * rslt;
}



float4 ShadowDirectionalLightFP(FullscreenVertexOut input) : FRAG_OUTPUT_COLOR
{	
	float2 uv = input.position.xy * ViewportWidthHeightInv;
	float2 screenPos = GetScreenPosition(uv);
	
	float zvalue = DepthBuffer.Load(int3(input.position.xy, 0)).x;  
	float unpackedDepth = abs(ConvertDepth(zvalue));

#ifdef ORTHO_CAMERA
	float4 viewPos = float4(screenPos, unpackedDepth, 1);
#else //! ORTHO_CAMERA
	float4 viewPos = float4(screenPos * unpackedDepth, unpackedDepth, 1);
#endif //! ORTHO_CAMERA
	
	float4 shadowPosition0 = mul(viewPos, DeferredShadowMatrixSplit0);
	float4 shadowPosition1 = mul(viewPos, DeferredShadowMatrixSplit1);
	float4 shadowPosition2 = mul(viewPos, DeferredShadowMatrixSplit2);
	float4 shadowPosition3 = mul(viewPos, DeferredShadowMatrixSplit3);

	float4 shadowPosition = unpackedDepth < DeferredSplitDistances.y ? 
		(unpackedDepth < DeferredSplitDistances.x ? shadowPosition0 : shadowPosition1)
		:
		(unpackedDepth < DeferredSplitDistances.z ? shadowPosition2 : shadowPosition3);

#define kShadowSize (2.0f/4096.0f)
	
	float4 offsets[5] = 
	{
		float4( 0.0f, 0.0f,0,0),
		float4(-kShadowSize,-kShadowSize,0,0),
		float4( kShadowSize,-kShadowSize,0,0),
		float4(-kShadowSize, kShadowSize,0,0),
		float4( kShadowSize, kShadowSize,0,0),
	};
	
	float rslt = 1.0f;
	if(unpackedDepth < DeferredSplitDistances.w)
	{		
		float shadowValue0 = ShadowTexture.SampleCmpLevelZero(ShadowMapSampler,shadowPosition.xy + offsets[0].xy, shadowPosition.z).x;
		float shadowValue1 = ShadowTexture.SampleCmpLevelZero(ShadowMapSampler,shadowPosition.xy + offsets[1].xy, shadowPosition.z).x;
		float shadowValue2 = ShadowTexture.SampleCmpLevelZero(ShadowMapSampler,shadowPosition.xy + offsets[2].xy, shadowPosition.z).x;
		float shadowValue3 = ShadowTexture.SampleCmpLevelZero(ShadowMapSampler,shadowPosition.xy + offsets[3].xy, shadowPosition.z).x;
		float shadowValue4 = ShadowTexture.SampleCmpLevelZero(ShadowMapSampler,shadowPosition.xy + offsets[4].xy, shadowPosition.z).x;

		rslt = (shadowValue1+shadowValue2+shadowValue3+shadowValue4)*0.125f + shadowValue0 * 0.5f;
	}
		
	return DeferredShadowMask * rslt;
}


float evaluateSpecular(float3 viewPos, float3 normal, float specularPower, float shininess)
{
	float3 eyeDir = normalize(viewPos); 
	
	// Specular calcs
	float3 floatVec = eyeDir; //eyeDirection + lightDir;
	float nDotH = saturate(dot(normal,floatVec));

	float specularLightAmount = saturate(pow(nDotH, specularPower)) * shininess; 

	return specularLightAmount;
}

float4 CompositeToScreenFP(FullscreenVertexOut input) : FRAG_OUTPUT_COLOR
{
	float4 lightValue = LightBuffer.SampleLevel(PointClampSampler,input.uv.xy,0);
	float4 colValue = ColorBuffer.SampleLevel(PointClampSampler,input.uv.xy,0);	
	float4 nrmValue = NormalDepthBuffer.SampleLevel(PointClampSampler,input.uv.xy,0);	
	float emissiveness = colValue.w; 	
		
	float4 colour = float4((lightValue.xyz + emissiveness) * colValue.xyz, colValue.w);
	return colour;
}

float4 PS_CopyCompositedBufferToScreen(FullscreenVertexOut input) : FRAG_OUTPUT_COLOR0
{
	return NormalDepthBuffer.SampleLevel(PointClampSampler,input.uv.xy,0);	
}

float3 ApplyFog(float3 colour, float distance, float fogDensityScale)
{
	float fogAmount = exp( -distance*fogDensityScale );
	float3  fogColor  = float3(0.5,0.6,0.7);
	return lerp( fogColor, colour, saturate(fogAmount) );
}
bool IsValidDepth(float zvalue)
{
	return zvalue < 1.0f;
}


float4 CompositeToScreenTiledFP(FullscreenVertexOut input) : FRAG_OUTPUT_COLOR
{	
	uint screenWidth, screenHeight;
	ColorBuffer.GetDimensions(screenWidth, screenHeight);

	uint pixelIndex = uint(input.position.y) * screenWidth + uint(input.position.x);
	uint2 lightValueHalf = LightingOutputBuffer[pixelIndex];
	float4 lightValue = float4(f16tof32(lightValueHalf), f16tof32(lightValueHalf >> 16));

	float4 colValue = ColorBuffer.Load(int3(input.position.xy,0));
	float4 nrmValue = NormalDepthBuffer.Load(int3(input.position.xy,0));
	float emissiveness = colValue.w; 	

	float zvalue = DepthBuffer.Load(int3(input.position.xy, 0)).x;  
	float4 colour = colValue;
	if(IsValidDepth(zvalue))
	{
		colour = float4((lightValue.xyz + emissiveness) * colValue.xyz, colValue.w);
	}
	
	return colour;
}



float4 NullFP() : FRAG_OUTPUT_COLOR
{
	return 1;
}

float4 CopyBufferFP(FullscreenVertexOut input) : FRAG_OUTPUT_COLOR
{
	return ColorBuffer.Sample(PointClampSampler,input.uv.xy);
}

bool ConePlaneTest(float4 planeEq, float3 lightPosition, float3 spotDir, float lightRadius, float coneBaseRadius)
{
	float3 m = cross( cross(-planeEq.xyz, spotDir), spotDir);
	float3 q = lightPosition + spotDir * lightRadius + m * coneBaseRadius;
	float dp0 = dot(float4(lightPosition,1.0f), planeEq);
	float dp1 = dot(float4(q,1.0f), planeEq);

	bool rslt = dp0 >= 0.0f || dp1 >= 0.0f;
	return rslt;
}
bool IsLightVisible(float3 lightPosition, float3 spotDir, float lightRadius, float coneBaseRadius, float4 frustumPlanes[6])
{
	bool inFrustum = true;
	[unroll] for (uint i = 0; i < 6; ++i) 
	{
		bool d = ConePlaneTest(frustumPlanes[i], lightPosition, spotDir, lightRadius, coneBaseRadius);
		inFrustum = inFrustum && d;
	}
	return inFrustum;
}


bool IsLightVisible(float3 lightPosition, float lightRadius, float4 frustumPlanes[6])
{
	bool inFrustum = true;
	[unroll] for (uint i = 0; i < 6; ++i) 
	{
		float d = dot(frustumPlanes[i], float4(lightPosition, 1.0f));
		inFrustum = inFrustum && (d >= -lightRadius);
	}
	return inFrustum;
}

void GenerateFrustumPlanes(out float4 frustumPlanes[6], uint2 tileLocation, float2 tileMinMax)
{
	uint screenWidth, screenHeight;
	DepthBuffer.GetDimensions(screenWidth, screenHeight);
	uint2 numTilesXY = uint2(screenWidth,screenHeight) >> PE_DEFERRED_TILE_SIZE_SHIFT;
	numTilesXY += (uint2(screenWidth,screenHeight) & (PE_DEFERRED_TILE_SIZE-1)) != 0 ? 1 : 0;

	float2 tileScale = float2(numTilesXY) * 0.5f;
	float2 tileBias = tileScale - float2(tileLocation.xy);
	
	float4 cx = float4( -Projection[0].x * tileScale.x, 0.0f, tileBias.x, 0.0f);
	float4 cy = float4(0.0f, Projection[1].y * tileScale.y, tileBias.y, 0.0f);
	float4 cw = float4(0.0f, 0.0f, -1.0f, 0.0f);
		
	frustumPlanes[0] = cw - cx;		// left
	frustumPlanes[1] = cw + cx;		// right
	frustumPlanes[2] = cw - cy;		// bottom
	frustumPlanes[3] = cw + cy;		// top
	frustumPlanes[4] = float4(0.0f, 0.0f, -1.0f, -tileMinMax.x);	// near
	frustumPlanes[5] = float4(0.0f, 0.0f,  1.0f,  tileMinMax.y);	// far

	[unroll] for (uint i = 0; i < 4; ++i) 
	{
		frustumPlanes[i] *= rcp(length(frustumPlanes[i].xyz));
	}
}

#define PE_MAX_ACTIVE_LIGHTS 128

groupshared uint TileMinMaxZ[2];
groupshared uint NumLightsActive;
groupshared uint ActiveLightIndices[PE_MAX_ACTIVE_LIGHTS];


void FrustumCullLights(uint groupIndex, uint numLights, uint2 tilePosition, float2 tileMinMax)
{
	float4 frustumPlanes[6];
	GenerateFrustumPlanes(frustumPlanes, tilePosition, tileMinMax);

	for(uint i = groupIndex; i < numLights; i += (PE_DEFERRED_TILE_SIZE*PE_DEFERRED_TILE_SIZE))
	{
		bool isLightVisible = true;
		if(DeferredLights[i].m_lightType == PD_DEFERRED_LIGHT_TYPE_SPOT)
		{
			isLightVisible = IsLightVisible(DeferredLights[i].m_position, -DeferredLights[i].m_direction, DeferredLights[i].m_attenuation.y,DeferredLights[i].m_coneBaseRadius, frustumPlanes);
		}
		else
		{
			isLightVisible = IsLightVisible(DeferredLights[i].m_position, DeferredLights[i].m_attenuation.y,frustumPlanes);
		}

		[branch] if(isLightVisible)
		{
			uint listIndex;
			InterlockedAdd(NumLightsActive, uint(1), listIndex);
			if(listIndex < PE_MAX_ACTIVE_LIGHTS)
			{
				ActiveLightIndices[listIndex] = i;
			}
		}
	}
	GroupMemoryBarrierWithGroupSync();
}


[numthreads(PE_DEFERRED_TILE_SIZE, PE_DEFERRED_TILE_SIZE, 1)]
void CS_GenerateLightingTiled(	uint3 GroupId : SV_GroupID, 
						uint3 DispatchThreadId : SV_DispatchThreadID, 
						uint3 GroupThreadId : SV_GroupThreadID,
						uint GroupIndex : SV_GroupIndex)
{
	// Load shared data 
	uint groupIndex = GroupThreadId.y * PE_DEFERRED_TILE_SIZE + GroupThreadId.x;
	uint2 pixelPosition = DispatchThreadId.xy;
	uint2 tilePosition = GroupId.xy;

	// Get the size of the depth buffer
	uint screenWidth, screenHeight;
	DepthBuffer.GetDimensions(screenWidth, screenHeight);
	
	if(groupIndex == 0)
	{
		TileMinMaxZ[0] = asuint(1.0f);
		TileMinMaxZ[1] = asuint(0.0f);
		NumLightsActive = 0;
	}
	GroupMemoryBarrierWithGroupSync();
	  
	float zvalue = DepthBuffer.Load(int3(pixelPosition, 0)).x;  
	if(IsValidDepth(zvalue))
	{
		InterlockedMin(TileMinMaxZ[0], asuint(zvalue));
		InterlockedMax(TileMinMaxZ[1], asuint(zvalue));
	}
				
	GroupMemoryBarrierWithGroupSync();

	float2 tileMinMax = float2( asfloat(TileMinMaxZ[0]), asfloat(TileMinMaxZ[1]) );
	tileMinMax.x = ConvertDepth(tileMinMax.x);
	tileMinMax.y = ConvertDepth(tileMinMax.y);

	FrustumCullLights(groupIndex, NumDeferredLights, tilePosition, tileMinMax);
		
	if(pixelPosition.x < screenWidth && pixelPosition.y < screenHeight)
	{
		float3 lightRslt = DeferredAmbientColor;	
		
		float2 uv = float2(pixelPosition) * ViewportWidthHeightInv;
		float2 screenPos = GetScreenPosition(uv);
		float4 shadowResults = ShadowBuffer.Load(int3(pixelPosition, 0));

		if(IsValidDepth(zvalue))
		{
			float4 normalMapValue = NormalDepthBuffer.Load(int3(pixelPosition, 0));
			float viewSpaceDepth = ConvertDepth(zvalue);
			float3 viewSpaceNormal = normalize(float3(normalMapValue.xyz * 2.0f - 1.0f));

			for(uint i = 0; i < NumDeferredDirectionalLights; ++i)
			{
				DeferredDirectionalLight light = DeferredDirectionalLights[i];
				float dp = saturate(dot(light.m_direction,viewSpaceNormal));

				float shadowBufferValue = dot(shadowResults, light.m_shadowMask);
				shadowBufferValue += saturate(1.0f - dot(light.m_shadowMask,1.0f));
				
				lightRslt += (float3)(light.m_color * dp) * shadowBufferValue;
			}

	
			float4 viewPosition = float4(screenPos * InvProjXY.xy * viewSpaceDepth, -viewSpaceDepth, 1);
	
			float3 eyeDirection = normalize(-viewPosition.xyz);

			for(uint i = 0; i < min(NumLightsActive,uint(PE_MAX_ACTIVE_LIGHTS)); ++i)
			{
				uint lightIndex = ActiveLightIndices[i];
				PDeferredLight light = DeferredLights[lightIndex];

				float3 rslt = light.m_color.xyz;

				float3 lightDir = light.m_position - viewPosition.xyz;
				float3 lightDirNrm = normalize(lightDir);
				float dist = length(lightDir);
				float dp = dot(lightDirNrm,viewSpaceNormal);

				float distanceAttenuation = 1-saturate(smoothstep(light.m_attenuation.x,light.m_attenuation.y,dist));

				float specularValue = calcSpecularLightAmt(viewSpaceNormal, eyeDirection, lightDirNrm, normalMapValue.w, 7.0f);
				dp = saturate(dp);
				rslt *= (specularValue + dp) * distanceAttenuation;

				if(light.m_lightType == PD_DEFERRED_LIGHT_TYPE_SPOT)
				{
					float spotDp = dot(lightDirNrm,light.m_direction);
					float spotAttenuation = saturate((light.m_spotAngles.y - max(spotDp,0)) / (light.m_spotAngles.y - light.m_spotAngles.x));
					spotAttenuation = spotAttenuation * spotAttenuation;
					rslt *= spotAttenuation;
				}
				
				float shadowBufferValue = dot(shadowResults, light.m_shadowMask);
				shadowBufferValue += saturate(1.0f - dot(light.m_shadowMask,1.0f));
				rslt *= shadowBufferValue;
			
				lightRslt += rslt;
			}
		}
		else
		{
			lightRslt = 1.0f;
		}

		uint pixelIndex = DispatchThreadId.y * screenWidth + DispatchThreadId.x;
				
		// pack to half
		uint2 lightRsltHalf = f32tof16(lightRslt.xy) | (f32tof16(float2(lightRslt.z,1.0f)) << 16);
		RWLightingOutputBuffer[pixelIndex] = lightRsltHalf;
	}

}


// Inscattering function - derivations from Miles Macklin
// This one is faster but fails for wide cone angles > pi/4
float evaluateInScatteringThinCones(float3 start, float3 dir, float3 lightPos, float d)
{
	float3 q = start - lightPos;

	float b = dot(dir, q);
	float c = dot(q, q);
	float s = 1.0f / sqrt(c - b*b);
	// Trig identity version from Miles Macklin's faster fog update - appears to fail if outer cone angle > pi/4
	float x = d*s;
	float y = b*s;
	float l = s * atan( (x) / (1.0+(x+y)*y));
	return l;	
}


// Inscattering function - derivations from Miles Macklin
// Slower but works for wide angles
float evaluateInScattering(float3 start, float3 dir, float3 lightPos, float d)
{
	float3 q = start - lightPos;

	float b = dot(dir, q);
	float c = dot(q, q);
	float s = 1.0f / sqrt(c - b*b);
	float l = s * (atan( (d + b) * s) - atan( b*s ));
	return l;	
}

void solveQuadratic(float a, float b, float c, out float r0, out float r1)
{
	float discrim = b*b - 4.0f*a*c;
	
	float q = sqrt(discrim) * (b < 0.0f ? -1.0f : 1.0f);
	q = (q + b) * -0.5f;

	float rslt0 = q/a;
	float rslt1 = c/q;

	r0 = min(rslt0, rslt1);
	r1 = max(rslt0, rslt1);
}

void intersectCone(float3 localOrigin, float3 localDir, float tanConeAngle, float height, out float minT, out float maxT)
{
	localDir = localDir.xzy;
	localOrigin = localOrigin.xzy;

	// Stop massively wide-angled cones from exploding by clamping the tangent. 
	tanConeAngle = min(tanConeAngle, 6.0f);

	float tanTheta = tanConeAngle * tanConeAngle;
	
	float a = localDir.x*localDir.x + localDir.z*localDir.z - localDir.y*localDir.y*tanTheta;
	float b = 2.0*(localOrigin.x*localDir.x + localOrigin.z*localDir.z - localOrigin.y*localDir.y*tanTheta);
	float c = localOrigin.x*localOrigin.x + localOrigin.z*localOrigin.z - localOrigin.y*localOrigin.y*tanTheta;

	solveQuadratic(a, b, c, minT, maxT);

	
	float y1 = localOrigin.y + localDir.y*minT;
	float y2 = localOrigin.y + localDir.y*maxT;
		
	// should be possible to simplify these branches if the compiler isn't already doing it
	
	if (y1 > 0.0f && y2 > 0.0f)
	{
		// both intersections are in the reflected cone so return degenerate value
		minT = 0.0;
		maxT = -1.0;
	}
	else if (y1 > 0.0f && y2 < 0.0f)
	{
		// closest t on the wrong side, furthest on the right side => ray enters volume but doesn't leave it (so set maxT arbitrarily large)
		minT = maxT;
		maxT = 10000.0;
	}
	else if (y1 < 0.0f && y2 > 0.0f)
	{
		// closest t on the right side, largest on the wrong side => ray starts in volume and exits once
		maxT = minT;
		minT = 0.0;		
	}	
	
}

float4 PS_EvaluateScatteringLights(FullscreenVertexOut input) : FRAG_OUTPUT_COLOR0
{
	// Load shared data 
	uint2 pixelPosition = (uint2)input.position.xy;
		
	float2 uv = float2(pixelPosition) * ViewportWidthHeightInv;
	float2 screenPos = GetScreenPosition(uv);
		
	float zvalue = DepthBuffer.Load(int3(pixelPosition, 0)).x; 
	
	float3 scatterResult = 0.0f;
	if(IsValidDepth(zvalue))
	{
	 
	float viewSpaceDepth = ConvertDepth(zvalue);		
	float4 viewRayDirection = float4( normalize(float3(screenPos.xy * InvProjXY.xy, -1.0f)), 0);
			
	
	for(uint i = 0; i < NumDeferredLights; ++i)
	{
		PDeferredLight light = DeferredLights[i];
			
		float3 rayPos = mul(float4(0,0,0,1.0f), light.m_viewToLocalTransform).xyz;
		float3 rayDir = normalize( mul(viewRayDirection, light.m_viewToLocalTransform).xyz );
		
		float t0 = 0.0f;
		float t1 = viewSpaceDepth;
		
		[branch] if(light.m_lightType == PD_DEFERRED_LIGHT_TYPE_SPOT)
		{
			intersectCone(rayPos, rayDir, light.m_tanConeAngle, light.m_attenuation.y, t0, t1);
			t1 = clamp(t1, 0.0f, viewSpaceDepth);
			t0 = max(0.0f, t0);
		}	
			
		[branch] if(t1 > t0)
		{
			scatterResult += (float3)(light.m_color * evaluateInScattering(rayPos + rayDir*t0, rayDir, 0, t1-t0) * light.m_scatterIntensity);
		}
	}

	scatterResult = pow(scatterResult, 1.0f/2.2f);
	}	
	return float4(scatterResult,0);
	
}



void GenerateFrustumPlanesInstantLights(out float4 frustumPlanes[6], uint2 tileLocation, float2 tileMinMax)
{
	uint screenWidth, screenHeight;
	screenWidth = uint(ViewportWidthHeight.x);
	screenHeight = uint(ViewportWidthHeight.y);

	uint2 numTilesXY = uint2(screenWidth,screenHeight) >> PE_DEFERRED_TILE_SIZE_SHIFT;
	numTilesXY += (uint2(screenWidth,screenHeight) & (PE_DEFERRED_TILE_SIZE-1)) != 0 ? 1 : 0;
	
	float2 tileScale = float2(numTilesXY) * 0.5f;
	float2 tileBias = tileScale - float2(tileLocation.xy);
	
	float4 cx = float4( -Projection[0].x * tileScale.x, 0.0f, tileBias.x, 0.0f);
	float4 cy = float4(0.0f, Projection[1].y * tileScale.y, tileBias.y, 0.0f);
	float4 cw = float4(0.0f, 0.0f, -1.0f, 0.0f);
		
	frustumPlanes[0] = cw - cx;		// left
	frustumPlanes[1] = cw + cx;		// right
	frustumPlanes[2] = cw - cy;		// bottom
	frustumPlanes[3] = cw + cy;		// top
	frustumPlanes[4] = float4(0.0f, 0.0f, -1.0f, -tileMinMax.x);	// near
	frustumPlanes[5] = float4(0.0f, 0.0f,  1.0f,  tileMinMax.y);	// far

	for (uint i = 0; i < 4; ++i) 
	{
		frustumPlanes[i] /= length(frustumPlanes[i].xyz);
	}
}


bool IsInstantLightVisible(float3 lightPosition, float3 spotDir, float lightRadius, float coneBaseRadius, float4 frustumPlanes[6])
{
	bool inFrustum = true;
	for (uint i = 0; i < 4; ++i) 
	{
		bool d = ConePlaneTest(frustumPlanes[i], lightPosition, spotDir, lightRadius, coneBaseRadius);
		inFrustum = inFrustum && d;
	}
	return inFrustum;
}


bool IsInstantLightVisible(float3 lightPosition, float lightRadius, float4 frustumPlanes[6])
{
	bool inFrustum = true;
	for (uint i = 0; i < 4; ++i) 
	{
		float d = dot(frustumPlanes[i], float4(lightPosition, 1.0f));
		inFrustum = inFrustum && (d >= -lightRadius);
	}
	return inFrustum;
}


void FrustumCullInstantLights(uint groupIndex, uint numLights, uint2 tilePosition, float2 tileMinMax)
{
	float4 frustumPlanes[6];
	GenerateFrustumPlanesInstantLights(frustumPlanes, tilePosition, tileMinMax);

	for(uint i = groupIndex; i < numLights; i += (PE_DEFERRED_TILE_SIZE*PE_DEFERRED_TILE_SIZE))
	{
		bool isLightVisible = true;
		if(DeferredInstantLights[i].m_lightType == PD_DEFERRED_LIGHT_TYPE_SPOT)
		{
			isLightVisible = IsInstantLightVisible(DeferredInstantLights[i].m_position, -DeferredInstantLights[i].m_direction, DeferredInstantLights[i].m_attenuation.y * 3.0f,DeferredInstantLights[i].m_coneBaseRadius * 3.0f, frustumPlanes);
		}
		else
		{
			isLightVisible = IsInstantLightVisible(DeferredInstantLights[i].m_position, DeferredInstantLights[i].m_attenuation.y * 3.0f,frustumPlanes);
		}
		
		if(isLightVisible && DeferredInstantLights[i].m_attenuation.y > 0.0001f)
		{
			uint listIndex;
			InterlockedAdd(NumLightsActive, uint(1), listIndex);
			if(listIndex < PE_MAX_ACTIVE_LIGHTS)
			{
				ActiveLightIndices[listIndex] = i;
			}
		}
	}
	GroupMemoryBarrierWithGroupSync();
}

[numthreads(PE_DEFERRED_TILE_SIZE, PE_DEFERRED_TILE_SIZE, 1)]
void CS_GenerateLightingInstantLightsTiled(	uint3 GroupId : SV_GroupID, 
						uint3 DispatchThreadId : SV_DispatchThreadID, 
						uint3 GroupThreadId : SV_GroupThreadID,
						uint GroupIndex : SV_GroupIndex)
{
   // Load shared data 
	uint groupIndex = GroupThreadId.y * PE_DEFERRED_TILE_SIZE + GroupThreadId.x;
	uint2 pixelPosition = DispatchThreadId.xy;
	uint2 tilePosition = GroupId.xy;

	uint numLights = DeferredLightCount[0];

	// Get the size of the depth buffer
	uint screenWidth, screenHeight;
	DepthBuffer.GetDimensions(screenWidth, screenHeight);
	
	uint pixelIndex = DispatchThreadId.y * screenWidth + DispatchThreadId.x;

	if(groupIndex == 0)
	{
		NumLightsActive = 0;
	}
	GroupMemoryBarrierWithGroupSync();

	float zvalue = DepthBuffer.Load(int3(pixelPosition, 0)).x;  

	float2 tileMinMax = cameraNearFar;
	FrustumCullInstantLights(groupIndex, numLights, tilePosition, tileMinMax);

	if(pixelPosition.x < screenWidth && pixelPosition.y < screenHeight)
	{
		float3 lightRslt = 0;	
		
		float2 uv = float2(pixelPosition) * ViewportWidthHeightInv;
		float2 screenPos = GetScreenPosition(uv);
		float3 scatterResult = 0;
		
		if(IsValidDepth(zvalue))
		{
			float4 normalMapValue = NormalDepthBuffer.Load(int3(pixelPosition, 0));
			float viewSpaceDepth = ConvertDepth(zvalue);
			float3 viewSpaceNormal = normalize(float3(normalMapValue.xyz * 2.0f - 1.0f));
					
			float3 viewRayDirection = float3(screenPos * InvProjXY.xy, -1.0f);
			float3 viewPosition = viewRayDirection * viewSpaceDepth;

			float3 eyeDirection = normalize(-viewPosition);

			//float3 viewRayDirection = normalize(viewPosition); //normalize(float3(screenPos * InvProjXY, -1.0f));
			float3 worldRayPos = ViewInverse[3].xyz;//mul(float4(0,0,0,1.0f), ViewInverse).xyz;
			float3 worldRayDir = mul(float4(viewRayDirection,0.0f), ViewInverse).xyz;

			for(int i = 0; i < min(int(NumLightsActive),PE_MAX_ACTIVE_LIGHTS); ++i)
			{
				uint lightIndex = ActiveLightIndices[i];
				PDeferredLight light = DeferredInstantLights[lightIndex];

				float3 rslt = light.m_color.xyz;

				float3 lightDir = light.m_position - viewPosition;
				float dist = length(lightDir);
				float3 lightDirNrm = (lightDir) * (1.0f/dist);
				float dp = dot(lightDirNrm,viewSpaceNormal);

				float distanceAttenuation = 1-saturate(smoothstep(light.m_attenuation.x,light.m_attenuation.y,dist));

				float specularValue = calcSpecularLightAmt(viewSpaceNormal, eyeDirection, lightDirNrm, normalMapValue.w, 7.0f);
				dp = saturate(dp);
				rslt *= (specularValue + dp) * distanceAttenuation;

				float3 rayPos = mul(float4(worldRayPos,1.0f), light.m_viewToLocalTransform).xyz;
				float3 rayDir = normalize( mul(float4(worldRayDir,0.0f), light.m_viewToLocalTransform).xyz );
			
				float t0 = 0.0f;
				float t1 = viewSpaceDepth;

				if(light.m_lightType == PD_DEFERRED_LIGHT_TYPE_SPOT)
				{
					float spotDp = dot(lightDirNrm,light.m_direction);
					float spotAttenuation = saturate((light.m_spotAngles.y - max(spotDp,0)) / (light.m_spotAngles.y - light.m_spotAngles.x));
					spotAttenuation = spotAttenuation * spotAttenuation;
					rslt *= spotAttenuation;
					
					intersectCone(rayPos, rayDir, light.m_tanConeAngle, light.m_attenuation.y, t0, t1);
					t1 = clamp(t1, 0.0f, viewSpaceDepth);
					t0 = max(0.0f, t0);
				}
								
				lightRslt += rslt;
							
				if(t1 > t0)
				{
					scatterResult += (float3)(light.m_color * evaluateInScatteringThinCones(rayPos + rayDir*t0, rayDir, 0, t1-t0) * light.m_scatterIntensity);
				}
			}

			lightRslt *= DeferredInstantIntensity;

			// pack to half
			uint2 previousRslt = RWLightingOutputBuffer[pixelIndex];
			float4 previousLightResult = float4(f16tof32(previousRslt), f16tof32(previousRslt >> 16));
			lightRslt.xyz += previousLightResult.xyz;
			uint2 lightRsltHalf = f32tof16(lightRslt.xy) | (f32tof16(float2(lightRslt.z,1.0f)) << 16);
			RWLightingOutputBuffer[pixelIndex] = lightRsltHalf;

			scatterResult *= DeferredInstantScatteringIntensity;
			scatterResult = pow(scatterResult, 1.0f/2.2f);		
		}

		float4 scatterLightRslt = float4(scatterResult,0.0f);
		uint2 lightRsltHalf = f32tof16(scatterLightRslt.xy) | (f32tof16(float2(scatterLightRslt.z,1.0f)) << 16);
		RWScatteredLightingOutputBuffer[pixelIndex] = lightRsltHalf;
	}
}


float4 PS_EvaluateScatteringInstantLights(FullscreenVertexOut input) : FRAG_OUTPUT_COLOR0
{
	uint pixelIndex = uint(input.position.y) * uint(ViewportWidthHeight.x) + uint(input.position.x);
	uint2 lightValueHalf = LightingOutputBuffer[pixelIndex];
	float4 lightValue = float4(f16tof32(lightValueHalf), f16tof32(lightValueHalf >> 16));

	return lightValue;
}

BlendState NoBlend 
{
  BlendEnable[0] = FALSE;
  RenderTargetWriteMask[0] = 15;
};
BlendState AdditiveBlend 
{
	BlendEnable[0] = TRUE;
	SrcBlend[0] = ONE;
	DestBlend[0] = ONE;
	BlendOp[0] = ADD;
	SrcBlendAlpha[0] = ONE;
	DestBlendAlpha[0] = ONE;
	BlendOpAlpha[0] = ADD;
	BlendEnable[1] = FALSE;
	RenderTargetWriteMask[0] = 15;
};

BlendState NoColourBlend 
{
	BlendEnable[0] = FALSE;
	RenderTargetWriteMask[0] = 0;
};

DepthStencilState DepthState {
  DepthEnable = FALSE;
  DepthWriteMask = All;
  DepthFunc = Less;
  StencilEnable = FALSE; 
};

DepthStencilState TwoSidedStencilDepthState {
  DepthEnable = TRUE;
  DepthWriteMask = Zero;
  DepthFunc = Less;
  StencilEnable = TRUE; 

  FrontFaceStencilFail = Keep;
  FrontFaceStencilDepthFail = Incr;
  FrontFaceStencilPass = Keep;
  FrontFaceStencilFunc = Always;
  BackFaceStencilFail = Keep;
  BackFaceStencilDepthFail = Decr;
  BackFaceStencilPass = Keep;
  BackFaceStencilFunc = Always;
  StencilReadMask = 255;
  StencilWriteMask = 255;
};

DepthStencilState StencilTestDepthState 
{
  DepthEnable = FALSE;
  DepthWriteMask = Zero;
  DepthFunc = Less;
  StencilEnable = TRUE; 

  FrontFaceStencilFail = Keep;
  FrontFaceStencilDepthFail = Keep;
  FrontFaceStencilPass = Keep;
  FrontFaceStencilFunc = Less;
  BackFaceStencilFail = Keep;
  BackFaceStencilDepthFail = Keep;
  BackFaceStencilPass = Keep;
  BackFaceStencilFunc = Less;
  StencilReadMask = 255;
  StencilWriteMask = 0;
};

RasterizerState DefaultRasterState 
{
	CullMode = None;
};

RasterizerState LightRasterState 
{
	CullMode = Back;
};

technique11 RenderPointLight
{
	pass pass1
	{
		SetVertexShader( CompileShader( vs_4_0, RenderLightVP() ) );
		SetPixelShader( CompileShader( ps_4_0, RenderPointLightFP() ) );
	
		SetBlendState( AdditiveBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthState, 0);
		SetRasterizerState( LightRasterState );		
	}
}


technique11 RenderSpotLight
{
	pass pass1
	{
		SetVertexShader( CompileShader( vs_4_0, RenderLightVP() ) );
		SetPixelShader( CompileShader( ps_4_0, RenderSpotLightFP() ) );
	
		SetBlendState( AdditiveBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthState, 0);
		SetRasterizerState( LightRasterState );		
	}
}


technique11 RenderDirectionalLight
{
	pass pass0
	{
		SetVertexShader( CompileShader( vs_4_0, FullscreenVP() ) );
		SetPixelShader( CompileShader( ps_4_0, RenderDirectionalLightFP() ) );
	
		SetBlendState( AdditiveBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthState, 0);
		SetRasterizerState( DefaultRasterState );		
	}
}


technique11 ShadowSpotLight
{
	pass pass1
	{
		SetVertexShader( CompileShader( vs_4_0, RenderLightVP() ) );
		SetPixelShader( CompileShader( ps_4_0, ShadowSpotLightFP() ) );
	
		SetBlendState( AdditiveBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthState, 0);
		SetRasterizerState( LightRasterState );		
	}
}

technique11 ShadowDirectionalLight
{
	pass pass0
	{
		SetVertexShader( CompileShader( vs_4_0, FullscreenVP() ) );
		SetPixelShader( CompileShader( ps_4_0, ShadowDirectionalLightFP() ) );
	
		SetBlendState( AdditiveBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthState, 0);
		SetRasterizerState( DefaultRasterState );		
	}
}



technique11 CompositeToScreen
{
	pass pass0
	{
		SetVertexShader( CompileShader( vs_4_0, FullscreenVP() ) );
		SetPixelShader( CompileShader( ps_4_0, CompositeToScreenFP() ) );
	
		SetBlendState( NoBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthState, 0);
		SetRasterizerState( DefaultRasterState );		
	}
}


technique11 CompositeToScreenTiled
{
	pass pass0
	{
		SetVertexShader( CompileShader( vs_4_0, FullscreenVP() ) );
		SetPixelShader( CompileShader( ps_4_0, CompositeToScreenTiledFP() ) );
	
		SetBlendState( NoBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthState, 0);
		SetRasterizerState( DefaultRasterState );		
	}
}
technique11 CopyBuffer
{
	pass pass0
	{
		SetVertexShader( CompileShader( vs_4_0, FullscreenVP() ) );
		SetPixelShader( CompileShader( ps_4_0, CopyBufferFP() ) );
	
		SetBlendState( NoBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthState, 0);
		SetRasterizerState( DefaultRasterState );		
	}
}


technique11 GenerateLightScattering
{
	pass pass0
	{
		SetVertexShader( CompileShader( vs_4_0, FullscreenVP() ) );
		SetPixelShader( CompileShader( ps_4_0, PS_EvaluateScatteringLights() ) );
	
		SetBlendState( AdditiveBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthState, 0);
		SetRasterizerState( DefaultRasterState );		
	}
}


technique11 GenerateLightScatteringInstantLights
{
	pass pass0
	{
		SetVertexShader( CompileShader( vs_4_0, FullscreenVP() ) );
		SetPixelShader( CompileShader( ps_4_0, PS_EvaluateScatteringInstantLights() ) );
	
		SetBlendState( AdditiveBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthState, 0);
		SetRasterizerState( DefaultRasterState );		
	}
}



technique11 GenerateLightingTiled
{
	pass p0
	{
		SetComputeShader( CompileShader( cs_5_0, CS_GenerateLightingTiled() ) );
	}
}

technique11 GenerateLightingInstantLightsTiled
{
	pass p0
	{
		SetComputeShader( CompileShader( cs_5_0, CS_GenerateLightingInstantLightsTiled() ) );
	}
}

technique11 CopyCompositedOutputToScreen
{
	pass pass0
	{
		SetVertexShader( CompileShader( vs_4_0, FullscreenVP() ) );
		SetPixelShader( CompileShader( ps_4_0, PS_CopyCompositedBufferToScreen() ) );
	
		SetBlendState( NoBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthState, 0);
		SetRasterizerState( DefaultRasterState );		
	}
}
