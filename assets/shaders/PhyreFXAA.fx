/* SCE CONFIDENTIAL
PhyreEngine(TM) Package 3.7.0.0
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#define FXAA_PC 1
#define FXAA_HLSL_5 1
#define FXAA_QUALITY__PRESET 29

#ifdef HAVE_FXAA_H
// If you have FXAA3_11.h, uncomment the line below and #define HAVE_FXAA_H at the top of this file
// #include "Fxaa3_11.h"
#endif //! HAVE_FXAA_H

struct FxaaVS_Output 
{  
	float4 Pos : SV_POSITION;              
    float2 Tex : TEXCOORD0; 
};

FxaaVS_Output FxaaVS(uint id : SV_VertexID) 
{
    FxaaVS_Output Output;
    Output.Tex = float2((id << 1) & 2, id & 2);
    Output.Pos = float4(Output.Tex * float2(2.0f, -2.0f) + float2(-1.0f, 1.0f), 0.0f, 1.0f);
    return Output; 
}

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

Texture2D ColorBuffer : COLOURBUFFER;

// Render FXAA
float4 FxaaPS(FxaaVS_Output Input) : SV_TARGET 
{
#ifdef HAVE_FXAA_H
	FxaaTex tex = { LinearClampSampler, ColorBuffer };
	
	float2 pos = Input.Tex.xy;
	
	float2 invViewportSize;
	ColorBuffer.GetDimensions(invViewportSize.x,invViewportSize.y);
	invViewportSize.x = 1.0f / invViewportSize.x;
	invViewportSize.y = 1.0f / invViewportSize.y;
	
	float4 rslt = FxaaPixelShader(pos,0, tex, tex, tex, invViewportSize.xy, 0,0,0, 0.25f,0.1f,0.0833f, 0,0,0,0 );
#else //! HAVE_FXAA_H
	float4 rslt = ColorBuffer.Load(int3(Input.Pos.xy,0));
#endif //! HAVE_FXAA_H
	return float4(rslt.xyz, 1.0f); 
}

// Convert RGBA input to RGBL output
float4 PS_RenderRGBL(FxaaVS_Output Input) : SV_TARGET 
{
	float2 pos = Input.Tex.xy;
	float4 texel = ColorBuffer.SampleLevel(PointClampSampler, pos, 0);
	texel.w = dot(texel.xyz, float3(0.299f, 0.587f, 0.114f));
	
	return texel;
}


BlendState NoBlend {
  AlphaToCoverageEnable = FALSE;
  BlendEnable[0] = FALSE;
  BlendEnable[1] = FALSE;
  BlendEnable[2] = FALSE;
  BlendEnable[3] = FALSE;
};
DepthStencilState NoDepthState {
  DepthEnable = FALSE;
  DepthWriteMask = All;
  DepthFunc = Less;
  StencilEnable = FALSE; 
};
RasterizerState DefaultRasterState 
{
	CullMode = None;
	FillMode = Solid;
	DepthBias = 0;
	ScissorEnable = false;
};
technique11 RenderFXAA
{
	pass p0
	{
		SetVertexShader( CompileShader( vs_4_1, FxaaVS() ) );
		SetPixelShader( CompileShader( ps_5_0, FxaaPS() ) );
		
		SetBlendState( NoBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( NoDepthState, 0);
		SetRasterizerState( DefaultRasterState );
	}
}
technique11 RenderRGBL
{
	pass p0
	{
		SetVertexShader( CompileShader( vs_4_1, FxaaVS() ) );
		SetPixelShader( CompileShader( ps_5_0, PS_RenderRGBL() ) );
		
		SetBlendState( NoBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( NoDepthState, 0);
		SetRasterizerState( DefaultRasterState );
	}
}
