/* SCE CONFIDENTIAL
PhyreEngine(TM) Package 3.7.0.0
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Switches. 

// Material switch definitions. These are the material switches this shader exposes.
bool PhyreMaterialSwitches 
< 
string MaterialSwitchNames[] = {"LAYERED_TEXTURE_MODE_OVER_NONE_ENABLED", "MULTIPLE_UVS_ENABLED", "VERTEX_COLOR_ENABLED", "LIGHTING_ENABLED", "TEXTURE_ENABLED","ALPHA_ENABLED", "NORMAL_MAPPING_ENABLED", "WRAP_DIFFUSE_LIGHTING", "SPECULAR_ENABLED", "CASTS_SHADOWS", "RECEIVE_SHADOWS", "DOUBLE_SIDED", "MOTION_BLUR_ENABLED", "GENERATE_LIGHTS"}; 
string MaterialSwitchUiNames[] = {"Layered Texture Mode Over None", "Enable Multiple UVs", "Enable Vertex Color", "Enable Lighting","Enable Texture", "Enable Transparency", "Enable Normal Mapping", "Use Wrap Diffuse Lighting", "Enable Specular", "Casts Shadows", "Receive Shadows", "Render Double Sided","Motion Blur Enabled", "Generate Lights"}; 
string MaterialSwitchDefaultValues[] = {"", "", "", "", "", "", "", "", "", "", "1", "", "", ""}; 
>;

#include "PhyreShaderPlatform.h"
#include "PhyreDefaultShaderSharedCodeD3D.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Global shader parameters.
#ifdef ALPHA_ENABLED
float AlphaThreshold : ALPHATHRESHOLD = 0.0;		// The alpha threshold.
#endif //! ALPHA_ENABLED

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Structures
struct ShadowTexturedVSInput
{
#ifdef SKINNING_ENABLED
	float3 SkinnableVertex : POSITION;
#else //! SKINNING_ENABLED
	float3 Position	: POSITION;
#endif //! SKINNING_ENABLED
	float2 Uv	: TEXCOORD0;
#ifdef SKINNING_ENABLED
	uint4	SkinIndices		: BLENDINDICES;
	float4	SkinWeights		: BLENDWEIGHTS;
#endif //! SKINNING_ENABLED
#ifdef MULTIPLE_UVS_ENABLED
	float2 Uv1	: TEXCOORD1;
#endif //! MULTIPLE_UVS_ENABLED
};

struct ShadowTexturedVSOutput
{
#ifdef VERTEX_COLOR_ENABLED
	float4 Color : COLOR0;
#endif //! VERTEX_COLOR_ENABLED
	float4 Position	: SV_POSITION;	
	float2 Uv	: TEXCOORD0;
#ifdef MULTIPLE_UVS_ENABLED
	float2 Uv1	: TEXCOORD1;
#endif //! MULTIPLE_UVS_ENABLED
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Vertex shaders

// Default shadow vertex shader.
ShadowTexturedVSOutput ShadowTexturedVS(ShadowTexturedVSInput IN)
{
	ShadowTexturedVSOutput Out = (ShadowTexturedVSOutput)0;	
#ifdef SKINNING_ENABLED
	float3 position = IN.SkinnableVertex.xyz;
	Out.Position = mul(float4(position.xyz,1), ViewProjection);	
#else //! SKINNING_ENABLED
	float3 position = IN.Position.xyz;
	Out.Position = mul(float4(position.xyz,1), WorldViewProjection);
#endif //! SKINNING_ENABLED
	Out.Uv = IN.Uv;
#ifdef MULTIPLE_UVS_ENABLED
	Out.Uv1 = IN.Uv1;
#endif //! MULTIPLE_UVS_ENABLED
	return Out;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Fragment shaders.

// Forward render fragment shader
float4 ForwardRenderFP(DefaultVSForwardRenderOutput In) : FRAG_OUTPUT_COLOR0
{
#ifdef VERTEX_COLOR_ENABLED
	float4 shadingResult = In.Color * MaterialColour;
#else //! VERTEX_COLOR_ENABLED
	float4 shadingResult = MaterialColour;
#endif //! VERTEX_COLOR_ENABLED

#ifdef TEXTURE_ENABLED
	float4 texValue = TextureSampler.Sample(TextureSamplerSampler, In.Uv);
	shadingResult *= texValue;

#ifdef MULTIPLE_UVS_ENABLED
#ifdef LAYERED_TEXTURE_MODE_OVER_NONE_ENABLED
	half4 tex2 = TextureSampler1.Sample(TextureSampler1Sampler, In.Uv1);
	float3 fc = shadingResult.xyz;
	float  fa = shadingResult.w;
	float3 bc = tex2.xyz;
	float  ba = tex2.w;
	shadingResult.xyz = fc * fa + (bc * (1.0f - fa));
	shadingResult.w = 1.0f - ((1.0f - ba) * (1.0f - fa));
#endif //! LAYERED_TEXTURE_MODE_OVER_NONE_ENABLED
#endif //! MULTIPLE_UVS_ENABLED

#endif //! TEXTURE_ENABLED

#ifdef USE_LIGHTING
	// Read the normal here before any LOD clip, to keep the normal map texture read non-dependent on VITA.
	float3 normal = EvaluateNormal(In);
#endif //! USE_LIGHTING

	// Do alpha test and screendoor LOD Blend early.
#ifdef ALPHA_ENABLED
	clip(shadingResult.w - AlphaThreshold);
#endif //! ALPHA_ENABLED

#ifdef LOD_BLEND
	clip(GetLODDitherValue(GET_LOD_FRAGMENT_UV(In.Position)));
#endif //! LOD_BLEND

	// Lighting
#ifdef USE_LIGHTING
	float glossValue = 1;
#ifdef TEXTURE_ENABLED
	glossValue = texValue.w;
#endif //! TEXTURE_ENABLED
	float3 lightResult = EvaluateLightingDefault(In, In.WorldPositionDepth.xyz, normal, glossValue);
	shadingResult *= float4(((lightResult * MaterialDiffuse) + MaterialEmissiveness), 1);
#endif //! USE_LIGHTING

#ifdef FOG_ENABLED
	shadingResult.xyz = EvaluateFog(shadingResult.xyz, In.WorldPositionDepth.w);
#endif //! FOG_ENABLED
#ifdef TONE_MAP_ENABLED
	shadingResult = ToneMap(shadingResult.xyz);
#endif //! TONE_MAP_ENABLED

	return shadingResult;
}

// Light pre pass second pass shader. Samples the light prepass buffer.
float4 LightPrepassApplyFP(DefaultVSForwardRenderOutput In) : FRAG_OUTPUT_COLOR0
{
#ifdef VERTEX_COLOR_ENABLED
	float4 shadingResult = In.Color * MaterialColour;
#else //! VERTEX_COLOR_ENABLED
	float4 shadingResult = MaterialColour;
#endif //! VERTEX_COLOR_ENABLED
#ifdef TEXTURE_ENABLED
	shadingResult *= TextureSampler.Sample(TextureSamplerSampler, In.Uv);
#ifdef MULTIPLE_UVS_ENABLED
#endif //! MULTIPLE_UVS_ENABLED
#endif //! TEXTURE_ENABLED

	// Lighting
#ifdef USE_LIGHTING
#ifdef LIGHTPREPASS_ENABLED
	float2 screenUv = In.Position.xy * scene.screenWidthHeightInv;
	float4 lightResult = tex2D(LightPrepassSampler, screenUv);
#else //! LIGHTPREPASS_ENABLED
	float4 lightResult = 1;
#endif //! LIGHTPREPASS_ENABLED
#ifdef SPECULAR_ENABLED
	lightResult.xyz += (float)(lightResult.w * Shininess);
#endif //! SPECULAR_ENABLED
	shadingResult.xyz *= (float3)((lightResult.xyz * MaterialDiffuse) + MaterialEmissiveness);
#endif //! SPECULAR_ENABLED

#ifdef FOG_ENABLED
	shadingResult.xyz = EvaluateFog(shadingResult.xyz, In.WorldPositionDepth.w);
#endif //! FOG_ENABLED
#ifdef TONE_MAP_ENABLED
	shadingResult = ToneMap(shadingResult.xyz);
#endif //! TONE_MAP_ENABLED

	return shadingResult;
}


// Textured shadow shader.
float4 ShadowTexturedFP(ShadowTexturedVSOutput IN) : FRAG_OUTPUT_COLOR0
{
#ifdef ALPHA_ENABLED

#ifdef VERTEX_COLOR_ENABLED
	float4 shadingResult = IN.Color * MaterialColour;
#else //! VERTEX_COLOR_ENABLED
	float4 shadingResult = MaterialColour;
#endif //! VERTEX_COLOR_ENABLED

#ifdef TEXTURE_ENABLED
	shadingResult *= TextureSampler.Sample(TextureSamplerSampler, IN.Uv);
#ifdef MULTIPLE_UVS_ENABLED
#endif //! MULTIPLE_UVS_ENABLED
#endif //! TEXTURE_ENABLED

	float alphaValue = shadingResult.w;
	clip(alphaValue - AlphaThreshold);
#endif //! ALPHA_ENABLED
	return 0;
}


#ifdef GENERATE_LIGHTS

// Technique to capture emissive surfaces.

[maxvertexcount(1)] void GS_CaptureEmissiveSurfaces( triangle DefaultVSDeferredRenderOutput input[3], inout PointStream<DefaultVSDeferredRenderOutput> OutputStream, uint TriangleIndex : SV_PRIMITIVEID )
{
	// only output emissive faces
	if(MaterialEmissiveness > 0.95f)
	{
		
		// Cull faces that are not emissive
		
		DefaultVSDeferredRenderOutput Out;
		

		Out.Uv = (input[0].Uv + input[1].Uv + input[2].Uv) / 3.0f;
		Out.Normal = normalize( mul(float4( (input[0].Normal + input[1].Normal + input[2].Normal) / 3.0f, 0.0f), ViewInverse ).xyz );

		float3 areaCross = cross(input[1].WorldPositionDepth.xyz - input[0].WorldPositionDepth.xyz, input[2].WorldPositionDepth.xyz - input[0].WorldPositionDepth.xyz);
		float triArea = length(areaCross) * 0.5f;
		
		Out.WorldPositionDepth = (input[0].WorldPositionDepth + input[1].WorldPositionDepth + input[2].WorldPositionDepth) / 3.0f;
		Out.Color = (input[0].Color + input[1].Color + input[2].Color) / 3.0f;

		// normalize tri area 
		triArea = saturate(triArea * 100.0f);
		Out.Color.w = triArea;
#ifdef USE_TANGENTS
		Out.Tangent = (input[0].Tangent + input[1].Tangent + input[2].Tangent) / 3.0f;
#endif //! USE_TANGENTS	

		uint idx = TriangleIndex; 
		uint2 pixelPosition = uint2(idx, idx >> 7) & 127;
	
		// the pixel position is irrelevant - it's just to avoid pixel shader / thread contention
		float2 pos = (float2(pixelPosition) / 128.0f) * 2.0f - 1.0f;
		pos = pos * 0.8f;
		

		Out.Position = float4(pos,0.0f,1.0f);

		OutputStream.Append( Out );		
	}
}

RWStructuredBuffer <PDeferredLight> RWDeferredLightBuffer;


float4x4 LookAt(float3 pos, float3 dir, float3 lat,float3 up)
{
	float4x4 mat;
	
	mat[0] = float4( lat.x, up.x, dir.x, 0.0f );
	mat[1] = float4( lat.y, up.y, dir.y, 0.0f );
	mat[2] = float4( lat.z, up.z, dir.z, 0.0f );
	mat[3] = float4( -dot(pos,lat), -dot(pos,up), -dot(pos,dir), 1.0f);

	return mat;
}



// Deferred rendering
//InstantLightOutput PS_CaptureEmissiveSurfaces(DefaultVSDeferredRenderOutput In) 
float4 PS_CaptureEmissiveSurfaces(DefaultVSDeferredRenderOutput In) : FRAG_OUTPUT_COLOR0
{ 
	float4 color = In.Color;
#ifdef TEXTURE_ENABLED
	color *= TextureSampler.Sample(TextureSamplerSampler, In.Uv);
#endif //! TEXTURE_ENABLED
	
	float3 centrePosition = In.WorldPositionDepth.xyz;
	float3 centreNormal = In.Normal;
	centreNormal = -normalize(centreNormal);

	float3 worldPosition = centrePosition;
	float3 viewPosition = (float3)(mul(float4(worldPosition,1.0f), View));
	float3 viewNormal = normalize(mul(float4(centreNormal,0.0f), View).xyz);
	float4 projPosition = mul(float4(worldPosition,1.0f),ViewProjection);
	float lightEdgeFade = 1.0f;
	if(projPosition.w > 0)
	{
		projPosition.xy /= projPosition.w;
		lightEdgeFade *= saturate( (0.8f - abs(projPosition.x)) * 4.0f );
	}

	float triArea = In.Color.w;
	float lightAtten = 15.0f * (triArea * 0.7f + 0.3f);
	float spotAngle = 0.3f * (triArea * 0.5f + 0.5f);
	float spotAngleInner = 0.1f * (triArea * 0.5f + 0.5f);
	color.xyz *= lightEdgeFade;

	if(dot(color.xyz, 0.333f) > 0.0001f)
	{
		float3 dir = centreNormal;
		float3 up = abs(dir.y) > 0.9f ? float3(0.0f,0.0f,1.0f) : float3(0.0f,1.0f,0.0f);
		float3 lat = normalize(cross(up,dir));
		up = normalize(cross(lat,dir));
	
		float4x4 worldInverse = LookAt(worldPosition, dir, lat, up);

		uint numLights = RWDeferredLightBuffer.IncrementCounter();

		RWDeferredLightBuffer[numLights].m_viewToLocalTransform = worldInverse;
		RWDeferredLightBuffer[numLights].m_shadowMask = 0;
		RWDeferredLightBuffer[numLights].m_position = viewPosition;
		RWDeferredLightBuffer[numLights].m_direction = viewNormal;
		RWDeferredLightBuffer[numLights].m_color = float4(color.xyz,1.0f);
		RWDeferredLightBuffer[numLights].m_scatterIntensity = 0.06f * triArea;

		RWDeferredLightBuffer[numLights].m_attenuation = float2(0.0f,lightAtten);
		RWDeferredLightBuffer[numLights].m_coneBaseRadius = tan(spotAngle) * lightAtten;

		RWDeferredLightBuffer[numLights].m_spotAngles.x = cos(spotAngleInner - 1e-6f);
		RWDeferredLightBuffer[numLights].m_spotAngles.y = cos(spotAngle - 1e-6f);
		RWDeferredLightBuffer[numLights].m_tanConeAngle = tan(spotAngle);
		RWDeferredLightBuffer[numLights].m_lightType = PD_DEFERRED_LIGHT_TYPE_SPOT;
	}
	return 1;
}

#endif //! GENERATE_LIGHTS

BlendState NoBlend 
{
  BlendEnable[0] = FALSE;
};
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
  DepthWriteMask = All;
  DepthFunc = Less;
};
DepthStencilState NoDepthState {
  DepthEnable = FALSE;
  DepthWriteMask = All;
  DepthFunc = Less;
};
DepthStencilState DepthStateWithNoStencil {
  DepthEnable = TRUE;
  DepthWriteMask = All;
  DepthFunc = Less;
  StencilEnable = FALSE;
};

RasterizerState NoCullRasterState
{
	CullMode = None;
};

#ifdef DOUBLE_SIDED

RasterizerState DefaultRasterState 
{
	CullMode = None;
};

#else //! DOUBLE_SIDED

RasterizerState DefaultRasterState 
{
	CullMode = Front;
};

#endif //! DOUBLE_SIDED

RasterizerState CullRasterState 
{
	CullMode = Front;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Techniques.

#ifndef ALPHA_ENABLED

technique11 ForwardRender
<
	string PhyreRenderPass = "Opaque";
	string VpIgnoreContextSwitches[] = {"NUM_LIGHTS"};
	string FpIgnoreContextSwitches[] = {"INSTANCING_ENABLED"};
>
{
	pass pass0
	{
		SetVertexShader( CompileShader( vs_4_0, DefaultForwardRenderVS() ) );
		SetPixelShader( CompileShader( ps_4_0, ForwardRenderFP() ) );
	
		SetBlendState( NoBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthState, 0);
		SetRasterizerState( DefaultRasterState );		
	}
}

#endif //! ALPHA_ENABLED

#ifdef ALPHA_ENABLED

technique11 ForwardRenderAlpha
<
	string PhyreRenderPass = "Transparent";
	string VpIgnoreContextSwitches[] = {"NUM_LIGHTS"};
	string FpIgnoreContextSwitches[] = {"INSTANCING_ENABLED"};
>
{
	pass pass0
	{
		SetVertexShader( CompileShader( vs_4_0, DefaultForwardRenderVS() ) );
		SetPixelShader( CompileShader( ps_4_0, ForwardRenderFP() ) );
	
		SetBlendState( LinearBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthState, 0);
		SetRasterizerState( DefaultRasterState );
	}
}

#endif //! ALPHA_ENABLED

#ifdef CASTS_SHADOWS

#ifdef ALPHA_ENABLED

technique11 ShadowTransparent
<
	string PhyreRenderPass = "ShadowTransparent";
	string VpIgnoreContextSwitches[] = {"NUM_LIGHTS", "LOD_BLEND"};
	string FpIgnoreContextSwitches[] = {"NUM_LIGHTS", "INSTANCING_ENABLED"};
>
{
	pass p0
	{
		SetVertexShader( CompileShader( vs_4_0, ShadowTexturedVS() ) );
		SetPixelShader( CompileShader( ps_4_0, ShadowTexturedFP() ) );
	
		SetBlendState( NoBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthState, 0);
		SetRasterizerState( DefaultRasterState );		
	}
}

#else //! ALPHA_ENABLED

technique11 Shadow
<
	string PhyreRenderPass = "Shadow";
	string VpIgnoreContextSwitches[] = {"NUM_LIGHTS", "LOD_BLEND"};
	string FpIgnoreContextSwitches[] = {"NUM_LIGHTS", "LOD_BLEND", "INSTANCING_ENABLED"};
>
{
	pass p0
	{
		SetVertexShader( CompileShader( vs_4_0, DefaultShadowVS() ) );
		SetPixelShader( CompileShader( ps_4_0, DefaultShadowFP() ) );
	
		SetBlendState( NoBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthState, 0);
		SetRasterizerState( DefaultRasterState );		
	}
}

#endif //! ALPHA_ENABLED

#endif //! CASTS_SHADOWS

#ifndef ALPHA_ENABLED

technique11 ZPrePass
<
	string PhyreRenderPass = "ZPrePass";
	string VpIgnoreContextSwitches[] = {"NUM_LIGHTS", "LOD_BLEND"};
	string FpIgnoreContextSwitches[] = {"NUM_LIGHTS", "INSTANCING_ENABLED"};
>
{
	pass p0
	{
		SetVertexShader( CompileShader( vs_4_0, DefaultZPrePassVS() ) );
		SetPixelShader( CompileShader( ps_4_0, DefaultUnshadedFP() ) );
	
		SetBlendState( NoBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthState, 0);
		SetRasterizerState( DefaultRasterState );	
	}
}

#endif //! ALPHA_ENABLED



#ifndef ALPHA_ENABLED

// Techniques
technique11 DeferredRender
<
	string PhyreRenderPass = "DeferredRender";
	string VpIgnoreContextSwitches[] = {"NUM_LIGHTS", "LOD_BLEND"};
	string FpIgnoreContextSwitches[] = {"NUM_LIGHTS", "INSTANCING_ENABLED"};
>
{
	pass p0
	{
		SetVertexShader( CompileShader( vs_4_0, DefaultDeferredRenderVS() ) );
		SetPixelShader( CompileShader( ps_4_0, DefaultDeferredRenderFP() ) );
		
		SetBlendState( NoBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthStateWithNoStencil, 0);
		SetRasterizerState( DefaultRasterState );	
	}
}

#endif //! ALPHA_ENABLED




#ifdef GENERATE_LIGHTS

// Techniques
technique11 CaptureEmissiveSurfaces
<
	string PhyreRenderPass = "CaptureEmissiveSurfaces";
	string VpIgnoreContextSwitches[] = {"NUM_LIGHTS", "LOD_BLEND"};
	string FpIgnoreContextSwitches[] = {"NUM_LIGHTS", "LOD_BLEND", "INSTANCING_ENABLED"};
>
{
	pass p0
	{
		SetVertexShader( CompileShader( vs_5_0, DefaultDeferredRenderVS() ) );
		SetGeometryShader( CompileShader( gs_5_0, GS_CaptureEmissiveSurfaces() ) );
		SetPixelShader( CompileShader( ps_5_0, PS_CaptureEmissiveSurfaces() ) );
		
		SetBlendState( NoBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( NoDepthState, 0);
		SetRasterizerState( NoCullRasterState );	
	}
}

#endif //! GENERATE_LIGHTS



#if 0 // Note: These techniques are disabled until future support is added
#ifndef ALPHA_ENABLED

technique11 LightPrePass
<
	string PhyreRenderPass = "LightPrePass";
	string VpIgnoreContextSwitches[] = {"NUM_LIGHTS", "LOD_BLEND"};
	string FpIgnoreContextSwitches[] = {"NUM_LIGHTS", "INSTANCING_ENABLED"};
>
{
	pass p0
	{
		DepthTestEnable=true;
#ifdef ZPREPASS_ENABLED
		DepthMask = false;
#else //! ZPREPASS_ENABLED
		DepthMask = true;	
#endif //! ZPREPASS_ENABLED
		DepthFunc = LEqual;
		ColorMask = bool4(true,true,true,true);
#ifdef DOUBLE_SIDED
		CullFaceEnable = false;
#else //! DOUBLE_SIDED
		CullFaceEnable = true;
#ifndef MAX
		CullFace = back;
#endif //! MAX
#endif //! DOUBLE_SIDED
		VertexProgram = compile vp40 DefaultForwardRenderVS();
		FragmentProgram = compile fp40 DefaultLightPrepassFP();
	}
}

technique11 LightPreMaterialPass
<
	string PhyreRenderPass = "LightPrePassMaterial";
	string VpIgnoreContextSwitches[] = {"NUM_LIGHTS", "LOD_BLEND"};
	string FpIgnoreContextSwitches[] = {"NUM_LIGHTS", "INSTANCING_ENABLED"};
>
{
	pass
	{
		DepthTestEnable=true;
		DepthMask = false;
		DepthFunc = LEqual;
		ColorMask = bool4(true,true,true,true);
#ifdef DOUBLE_SIDED
		CullFaceEnable = false;
#else //! DOUBLE_SIDED
		CullFaceEnable = true;
#ifndef MAX
		CullFace = back;
#endif //! MAX
#endif //! DOUBLE_SIDED
		VertexProgram = compile vp40 DefaultForwardRenderVS();
		FragmentProgram = compile fp40 LightPrepassApplyFP();
	}
}


#endif //! ALPHA_ENABLED

#endif //! Disabled techniques

