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

#define NUM_JITTERSAMPLES 48
//#define TINT_AREAS
#define KERNEL_7_SAMPLES

float FocusPlaneDistance;
float FocusRange;
float FocusBlurRange; 

float JitterSamplesTotalWeight;
float4 GaussianBlurWeights;
float4 GaussianBlurOffsets[7];

float4 JitterSamples[NUM_JITTERSAMPLES/2];
float4 JitterSampleDistances[NUM_JITTERSAMPLES/4];
float4 JitterSampleWeights[NUM_JITTERSAMPLES/4];
float4 FullBlurWeights;

Texture2D <float> DepthBuffer;
Texture2D <float4> ColorBuffer;
Texture2D <float4> FocusBuffer;
Texture2D <float4> DownsampledColorBuffer;
Texture2D <float4> BlurredColorBuffer;


sampler PointClampSampler
{
	Filter = Min_Mag_Mip_Point;
    AddressU = Clamp;
    AddressV = Clamp;
};


sampler LinearClampSampler
{
	Filter = Min_Mag_Linear_Mip_Point;
    AddressU = Clamp;
    AddressV = Clamp;
};


///////////////////////////////////////////////////////////////
// structures /////////////////////
///////////////////////////////////////////////////////////////

struct FullscreenVertexIn
{
	float3 vertex	: POSITION;
	float2 uv			: TEXCOORD0;
};

struct FullscreenVertexOut
{
	float4 position		: SV_POSITION;
	float2 uv			: TEXCOORD0;
};

struct TiledVertexIn
{
	float3 vertex	: POSITION;	
};

struct TiledVertexOut
{
	float4 position		: SV_POSITION;
};

struct GaussianVertexOut
{
	float4 position		: SV_POSITION;
	float2 centreUv		: TEXCOORD0;
	float4 uvs0			: TEXCOORD1;
	float4 uvs1			: TEXCOORD2;
	float4 uvs2			: TEXCOORD3;
};

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

FullscreenVertexOut FullscreenVP(FullscreenVertexIn input)
{
	FullscreenVertexOut output;

	output.position = float4(input.vertex.xy, 1, 1);
	output.position.y = -output.position.y;
	output.uv = input.uv;

	return output;
}

GaussianVertexOut GaussianBlurVP(FullscreenVertexIn input)
{
	GaussianVertexOut output;
	
	output.position = float4(input.vertex.xyz, 1.0f);
	output.position.y = -output.position.y;

	output.centreUv = input.uv + GaussianBlurOffsets[3].xy;
	output.uvs0 = float4(input.uv + GaussianBlurOffsets[0].xy,input.uv + GaussianBlurOffsets[1].xy);
	output.uvs1 = float4(input.uv + GaussianBlurOffsets[2].xy,input.uv + GaussianBlurOffsets[4].xy);
	output.uvs2 = float4(input.uv + GaussianBlurOffsets[5].xy,input.uv + GaussianBlurOffsets[6].xy);
		
	return output;
}

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////





float GetDistanceToFocusPlane(float depth)
{	
	float viewSpaceZ = -(cameraNearTimesFar / (depth * cameraFarMinusNear - cameraNearFar.y));
	
	float d = (abs(viewSpaceZ - FocusPlaneDistance) - FocusRange) * (1.0f / FocusBlurRange);
	return saturate(d);
}
float ConvertDepth(float depth)
{
#ifdef ORTHO_CAMERA
	float viewSpaceZ = -(depth * cameraFarMinusNear + cameraNearFar.x);
#else //! ORTHO_CAMERA
	float viewSpaceZ = -(cameraNearTimesFar / (depth * cameraFarMinusNear - cameraNearFar.y));
#endif //! ORTHO_CAMERA
	
	float d = (viewSpaceZ - FocusPlaneDistance) * (1.0f / (FocusBlurRange + FocusRange));
	return saturate(d);
}

// Mimic the functionality of texDepth2D.
float GetDepth(float4 depthSample)
{
	return depthSample.x;
}

bool FullyInFocus(float4 depthMinMax)
{
	return depthMinMax.x < 0.02f;
}
bool FullyOutOfFocus(float4 depthMinMax)
{
	return depthMinMax.x > 0.98f;
}

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

float4 GaussianBlurFP(GaussianVertexOut input) : FRAG_OUTPUT_COLOR
{	
	float4 sampleCentre = ColorBuffer.Sample(PointClampSampler, input.centreUv);
#ifdef KERNEL_7_SAMPLES
	float4 sample0 = ColorBuffer.Sample(PointClampSampler, input.uvs0.xy);
	float4 sample1 = ColorBuffer.Sample(PointClampSampler, input.uvs0.zw);
	float4 sample2 = ColorBuffer.Sample(PointClampSampler, input.uvs1.xy);
	float4 sample3 = ColorBuffer.Sample(PointClampSampler, input.uvs1.zw);
	float4 sample4 = ColorBuffer.Sample(PointClampSampler, input.uvs2.xy);
	float4 sample5 = ColorBuffer.Sample(PointClampSampler, input.uvs2.zw);

	float4 total = (sampleCentre* GaussianBlurWeights.w) + ((sample0+sample5)*GaussianBlurWeights.x) + ((sample1+sample4)*GaussianBlurWeights.y) + ((sample2+sample3)*GaussianBlurWeights.z);
#else 
	float4 sample0 = ColorBuffer.Sample(PointClampSampler, input.uvs0.xy);
	float4 sample1 = ColorBuffer.Sample(PointClampSampler, input.uvs0.zw);
	float4 sample2 = ColorBuffer.Sample(PointClampSampler, input.uvs1.xy);
	float4 sample3 = ColorBuffer.Sample(PointClampSampler, input.uvs1.zw);
	
	float4 total = (sampleCentre* GaussianBlurWeights.z) + ((sample0+sample3)*GaussianBlurWeights.x) + ((sample1+sample2)*GaussianBlurWeights.y);
#endif
	return total;
}


float4 DownsampleColorFP(FullscreenVertexOut input) : FRAG_OUTPUT_COLOR
{
    return ColorBuffer.Sample(LinearClampSampler, input.uv);
}

float4 CalculateFocusDistanceFP(FullscreenVertexOut input) : FRAG_OUTPUT_COLOR
{
	float depth = GetDepth(DepthBuffer.Sample(PointClampSampler, input.uv)).x;
	float distanceToFocusPlane = GetDistanceToFocusPlane(depth);
	float focusDepth = ConvertDepth(depth);

	return float4(distanceToFocusPlane,distanceToFocusPlane,distanceToFocusPlane,focusDepth);
}


float4 FullDofGPUFP(FullscreenVertexOut input) : FRAG_OUTPUT_COLOR
{
	float4 focusVal = FocusBuffer.Sample(LinearClampSampler, input.uv);
	float4 distanceToFocusPlane = focusVal.x;
	float4 depth = focusVal.w;
	float4 col = ColorBuffer.Sample(LinearClampSampler, input.uv);
			
	distanceToFocusPlane = saturate(distanceToFocusPlane + 0.2f);
	
	float4 uvIn = float4(input.uv,input.uv);
	float4 totalWeights = (1.0f - distanceToFocusPlane) ;
	totalWeights *= totalWeights;
	float4 total = col * totalWeights.x;
	totalWeights *= 0.25h;

	for(int i = 1; i<(NUM_JITTERSAMPLES/4)-1;i++)
	{	
		float4 uv0 = uvIn + JitterSamples[i*2 + 0];
		float4 uv1 = uvIn + JitterSamples[i*2 + 1];
		// sample some from downsampeld and some from colourmap
		float4 colSample0 = ColorBuffer.Sample(LinearClampSampler, uv0.xy);
		float4 colSample1 = DownsampledColorBuffer.Sample(LinearClampSampler, uv0.zw);
		float4 colSample2 = ColorBuffer.Sample(LinearClampSampler, uv1.xy);
		float4 colSample3 = DownsampledColorBuffer.Sample(LinearClampSampler, uv1.zw);
		
		float4 focusSample0 = FocusBuffer.Sample(LinearClampSampler, uv0.xy);
		float4 focusSample1 = FocusBuffer.Sample(LinearClampSampler, uv0.zw);
		float4 focusSample2 = FocusBuffer.Sample(LinearClampSampler, uv1.xy);
		float4 focusSample3 = FocusBuffer.Sample(LinearClampSampler, uv1.zw);

		float4 sampleDistancesToFocusPlane = float4(focusSample0.x,focusSample1.x,focusSample2.x,focusSample3.x);
		sampleDistancesToFocusPlane = saturate(sampleDistancesToFocusPlane + 0.3f);
		float4 depthSamples = float4(focusSample0.w,focusSample1.w,focusSample2.w,focusSample3.w);
		
		float4 sampleCirclesOfConfusion = depthSamples > depth ? distanceToFocusPlane : sampleDistancesToFocusPlane;
		float4 weights = saturate( (sampleCirclesOfConfusion - JitterSampleDistances[i]) * 15.0h );
		weights *= JitterSampleWeights[i];

		totalWeights += weights;
		total += colSample0 * weights.x;
		total += colSample1 * weights.y;
		total += colSample2 * weights.z;
		total += colSample3 * weights.w;
	}
	
	float totalWeightSummed = dot(totalWeights,1.0f);

#ifdef TINT_AREAS
	return (float4((total / totalWeightSummed).xyz,1) + 0.3f) * float4(1,0.2f,0.2f,1);
#else
	return float4((total / totalWeightSummed).xyz,1);
#endif
	
}



///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

BlendState NoBlend 
{
  BlendEnable[0] = FALSE;
};

DepthStencilState DepthState {
  DepthEnable = FALSE;
  DepthWriteMask = All;
  DepthFunc = Less;
  StencilEnable = FALSE; 
};

RasterizerState DefaultRasterState 
{
	CullMode = None;
	FillMode = solid;
};


technique11 GaussianBlur
{
	pass pass0
	{
		SetVertexShader( CompileShader( vs_4_0, GaussianBlurVP() ) );
		SetPixelShader( CompileShader( ps_4_0, GaussianBlurFP() ) );
	
		SetBlendState( NoBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthState, 0);
		SetRasterizerState( DefaultRasterState );		
	}
}

technique11 CalculateFocusDistance
{
	pass pass0
	{
		SetVertexShader( CompileShader( vs_4_0, FullscreenVP() ) );
		SetPixelShader( CompileShader( ps_4_0, CalculateFocusDistanceFP() ) );
	
		SetBlendState( NoBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthState, 0);
		SetRasterizerState( DefaultRasterState );		
	}
}


technique11 DownsampleColorBuffer
{
	pass pass0
	{
		SetVertexShader( CompileShader( vs_4_0, FullscreenVP() ) );
		SetPixelShader( CompileShader( ps_4_0, DownsampleColorFP() ) );
	
		SetBlendState( NoBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthState, 0);
		SetRasterizerState( DefaultRasterState );		
	}
}


technique11 FullDofGPU
{
	pass pass0
	{
		SetVertexShader( CompileShader( vs_4_0, FullscreenVP() ) );
		SetPixelShader( CompileShader( ps_4_0, FullDofGPUFP() ) );
	
		SetBlendState( NoBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthState, 0);
		SetRasterizerState( DefaultRasterState );		
	}
}
