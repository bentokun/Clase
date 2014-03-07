/* SCE CONFIDENTIAL
PhyreEngine(TM) Package 3.7.0.0
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

// Default implementation of a shader required by a sprite particle emitter.

#include "PhyreShaderPlatform.h"
#include "PhyreSceneWideParametersD3D.h"

float4x4 WorldViewProjection		: WorldViewProjection;	

///////////////////////////////////////////////////////////////
// structures /////////////////////
///////////////////////////////////////////////////////////////

struct ParticleVertexIn
{
	float3 Position		: POSITION;
	float2 Texcoord 	: TEXCOORD0;
};

struct ParticleVertexOut
{
	float4 position		: SV_POSITION;
	float2 Texcoord 	: TEXCOORD0;	
};
struct ParticleFragIn
{
	float2 Texcoord 	: TEXCOORD0;
};

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

ParticleVertexOut RenderParticlesVP(ParticleVertexIn input)
{
	ParticleVertexOut output;
	float4 localPosition = float4(input.Position.xyz,1.0f);	
	
	output.position = mul(localPosition, WorldViewProjection);
	output.Texcoord = input.Texcoord;

	return output;
}


half4 RenderParticlesFP(ParticleVertexOut input) : FRAG_OUTPUT_COLOR0
{
	half2 p = input.Texcoord * 2.0f - 1.0f;
	half a = length(p * 0.6);
	a = saturate(1.0 - a);
	a = a * a;
	a = a * a;

	return half4(1.0,1.0,1.0,a);
}


BlendState LinearBlend 
{
	BlendEnable[0] = TRUE;
	SrcBlend[0] = SRC_ALPHA;
	DestBlend[0] = INV_SRC_ALPHA;
	BlendOp[0] = ADD;
	SrcBlendAlpha[0] = ONE;
	DestBlendAlpha[0] = ONE;
	BlendOpAlpha[0] = ADD;
};

DepthStencilState DepthState {
  DepthEnable = TRUE;
  DepthWriteMask = Zero;
  DepthFunc = Less;
  StencilEnable = FALSE; 
};

RasterizerState DisableCulling
{
    CullMode = NONE;
};

technique11 Transparent
{
	pass p0
	{
		SetVertexShader( CompileShader( vs_5_0, RenderParticlesVP() ) );
		SetPixelShader( CompileShader( ps_5_0, RenderParticlesFP() ) );

		SetBlendState( LinearBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthState, 0);
		SetRasterizerState( DisableCulling );	
	}
}


