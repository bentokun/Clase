/* SCE CONFIDENTIAL
PhyreEngine(TM) Package 3.7.0.0
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include "PhyreShaderPlatform.h"
#include "PhyreSceneWideParametersD3D.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Global shader parameters.

// Un-tweakables
float4x4 World						: World;		
float4x4 WorldView					: WorldView;
float4x4 WorldViewProjection		: WorldViewProjection;
float selectionIDColor;
float4 constantColor = {1,1,1,1};
float4 multipleSelectionIDColor;
float GridFadeStartDistance;
float GridFadeDistanceScale;

#define MatrixMultiply(x,y) mul(y,x)

// Context switches
bool PhyreContextSwitches 
< 
string ContextSwitchNames[] = {"ORTHO_CAMERA", "SKINNING_ENABLED"}; 
>;

sampler LinearClampSampler
{
	Filter = Min_Mag_Linear_Mip_Point;
    AddressU = Clamp;
    AddressV = Clamp;
};

BlendState NoBlend 
{
	BlendEnable[0] = FALSE;
};
BlendState LinearBlend 
{
	BlendEnable[0] = TRUE;
	SrcBlend[0] = Src_Alpha;
	DestBlend[0] = Inv_Src_Alpha;
	BlendOp[0] = ADD;
	SrcBlendAlpha[0] = ONE;
	DestBlendAlpha[0] = ONE;
	BlendOpAlpha[0] = ADD;
	BlendEnable[1] = FALSE;
	RenderTargetWriteMask[0] = 15;
}; 
DepthStencilState DepthState 
{
	DepthEnable = TRUE;
	DepthWriteMask = All;
	DepthFunc = Less;
	StencilEnable = FALSE; 
};
DepthStencilState NoDepthWriteState 
{
	DepthEnable = TRUE;
	DepthWriteMask = Zero;
	DepthFunc = Less;
	StencilEnable = FALSE; 
};
DepthStencilState NoDepthState 
{
	DepthEnable = FALSE;
	DepthWriteMask = All;
	DepthFunc = Less;
	StencilEnable = FALSE; 
};
RasterizerState DefaultRasterState 
{
	CullMode = None;
};
RasterizerState WireRasterState 
{
	CullMode = None;
	FillMode = wireframe;
};


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

void EvaluateSkinPosition4Bones( inout float3 position, float4 weights, uint4 boneIndices )
{
	uint indexArray[4] = {boneIndices.x,boneIndices.y,boneIndices.z,boneIndices.w};
	float4 inPosition = float4(position,1);
	
 	position = 
		mul(inPosition, BoneTransforms[indexArray[0]]).xyz * weights.x
	+	mul(inPosition, BoneTransforms[indexArray[1]]).xyz * weights.y
	+	mul(inPosition, BoneTransforms[indexArray[2]]).xyz * weights.z
	+	mul(inPosition, BoneTransforms[indexArray[3]]).xyz * weights.w;
}

#endif // SKINNING_ENABLED

struct ObjectSelectionVPInput
{
	float4 Position			: POSITION;
};

#ifdef SKINNING_ENABLED
struct ObjectSelectionVPInputWithSkinning
{
	float3 SkinnableVertex	: POSITION;
	float3 SkinnableNormal	: NORMAL;
	uint4 SkinIndices		: BLENDINDICES;
	float4 SkinWeights		: BLENDWEIGHTS;
};
#endif // SKINNING_ENABLED

// Single Pixel Selection

struct SingleSelectionVPOutput
{
	float4 Position		 : SV_POSITION;
	float3 WorldPosition : TEXCOORD0;
	float ViewSpaceZ	 : TEXCOORD1;
};

struct SingleSelectionFPOutput
{
	float4 IdColorAndDepth	: FRAG_OUTPUT_COLOR0;
	float4 FaceNormal		: FRAG_OUTPUT_COLOR1;
};

#ifdef SKINNING_ENABLED
// Single selection render vertex shader
SingleSelectionVPOutput SingleSelectionVP(ObjectSelectionVPInputWithSkinning IN)
{
	SingleSelectionVPOutput OUT;

	float3 position = IN.SkinnableVertex.xyz;
	EvaluateSkinPosition4Bones(position.xyz, IN.SkinWeights, IN.SkinIndices);
	OUT.Position = mul(float4(position, 1.0f), ViewProjection);
	OUT.WorldPosition = position;
	OUT.ViewSpaceZ = mul(float4(position, 1.0f), View).z;
	
	return OUT;
}

#else // SKINNING_ENABLED

// Single selection render vertex shader
SingleSelectionVPOutput SingleSelectionVP(ObjectSelectionVPInput IN)
{
	SingleSelectionVPOutput OUT;

	float3 position = IN.Position.xyz;
	OUT.Position = mul(float4(position, 1.0f), WorldViewProjection);
	OUT.WorldPosition = mul(float4(position, 1.0f), World).xyz;
	OUT.ViewSpaceZ = mul(float4(position, 1.0f), WorldView).z;
	
	return OUT;
}
#endif // SKINNING_ENABLED

// Single selection render fragment shader
SingleSelectionFPOutput SingleSelectionFP(SingleSelectionVPOutput IN)
{
	SingleSelectionFPOutput OUT;
	
	// Face Normal calculation
	
	float3 dx = ddx(IN.WorldPosition);
	float3 dy = ddy(IN.WorldPosition);
	float epsilon = 0.00001f;
	
	if(length(dx) > epsilon)
		dx = normalize(dx);
	if(length(dy) > epsilon)
		dy = normalize(dy);
	
	float3 faceNormal = -cross(dx,dy);
	if(length(faceNormal) > epsilon)
		faceNormal = normalize(faceNormal);

	OUT.FaceNormal = float4(faceNormal * 0.5f + 0.5f, 1.0f); 
	OUT.IdColorAndDepth = float4(selectionIDColor, abs(IN.ViewSpaceZ), 0.0f, 0.0f);
	return OUT;
}



technique11 SingleSelection
{
	pass main
	{
		SetVertexShader( CompileShader( vs_4_0, SingleSelectionVP() ) );
		SetPixelShader( CompileShader( ps_4_0, SingleSelectionFP() ) );
		
		SetBlendState( NoBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthState, 0);
		SetRasterizerState( DefaultRasterState );	
	}
}

technique11 SingleSelectionSolid
{
	pass main
	{
		SetVertexShader( CompileShader( vs_4_0, SingleSelectionVP() ) );
		SetPixelShader( CompileShader( ps_4_0, SingleSelectionFP() ) );
		
		SetBlendState( NoBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthState, 0);
		SetRasterizerState( DefaultRasterState );	
	}
}

// Multiple Pixel Selection

struct MultipleSelectionVPOutput
{
	float4 Position		 : SV_POSITION;
};

struct MultipleSelectionFPOutput
{
	float4 IdColor	     : FRAG_OUTPUT_COLOR0;
};

#ifdef SKINNING_ENABLED

// Multiple selection render vertex shader
MultipleSelectionVPOutput MultipleSelectionVP(ObjectSelectionVPInputWithSkinning IN)
{
	MultipleSelectionVPOutput OUT;

	float3 position = IN.SkinnableVertex.xyz;
	EvaluateSkinPosition4Bones(position.xyz, IN.SkinWeights, IN.SkinIndices);
	OUT.Position = mul(float4(position, 1.0f), ViewProjection);
	
	return OUT;
}
#else // SKINNING_ENABLED

// Multiple selection render vertex shader
MultipleSelectionVPOutput MultipleSelectionVP(ObjectSelectionVPInput IN)
{
	MultipleSelectionVPOutput OUT;

	OUT.Position = mul(float4(IN.Position.xyz, 1.0f), WorldViewProjection);
	return OUT;
}
#endif // SKINNING_ENABLED

// Multiple selection render fragment shader
MultipleSelectionFPOutput MultipleSelectionFP()
{
	MultipleSelectionFPOutput OUT;
	
	OUT.IdColor = multipleSelectionIDColor;
	return OUT;
}




technique11 MultipleSelection
{
	pass main
	{
		SetVertexShader( CompileShader( vs_4_0, MultipleSelectionVP() ) );
		SetPixelShader( CompileShader( ps_4_0, MultipleSelectionFP() ) );
		
		SetBlendState( NoBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthState, 0);
		SetRasterizerState( DefaultRasterState );	
	}
}

technique11 MultipleSelectionSolid
{
	pass main
	{
		SetVertexShader( CompileShader( vs_4_0, MultipleSelectionVP() ) );
		SetPixelShader( CompileShader( ps_4_0, MultipleSelectionFP() ) );
		
		SetBlendState( NoBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthState, 0);
		SetRasterizerState( DefaultRasterState );	
	}
}

/////////////////////////////////////////////////////////////

#ifdef SKINNING_ENABLED
struct FlatColorVPInputWithSkinning
{
	float3 SkinnableVertex	: POSITION;
	float3 SkinnableNormal	: NORMAL;
	uint4 SkinIndices		: BLENDINDICES;
	float4 SkinWeights		: BLENDWEIGHTS;
};
#endif // SKINNING_ENABLED

struct FlatColorVPInput
{
	float3 Position			: POSITION;
};


struct FlatColorVPOutput
{
	float4 Position		: SV_POSITION;
};


struct FlatColorFPOutput
{
	float4 Color	     : FRAG_OUTPUT_COLOR0;
};

#ifdef SKINNING_ENABLED
// Simple vertex shader
FlatColorVPOutput FlatColorVP(FlatColorVPInputWithSkinning IN)
{
	FlatColorVPOutput OUT;

	float3 position = IN.SkinnableVertex.xyz;
	EvaluateSkinPosition4Bones(position.xyz, IN.SkinWeights, IN.SkinIndices);
	OUT.Position = mul(float4(position, 1.0f), ViewProjection);
	
	return OUT;
}

#else // SKINNING_ENABLED

// Simple vertex shader
FlatColorVPOutput FlatColorVP(FlatColorVPInput IN)
{
	FlatColorVPOutput OUT;

	OUT.Position = mul(float4(IN.Position.xyz, 1.0f), WorldViewProjection);
	
	return OUT;
}
#endif // SKINNING_ENABLED

// Simple fragment shader
FlatColorFPOutput FlatColorFP()
{
	FlatColorFPOutput OUT;
	
	OUT.Color = constantColor;

	return OUT;
}

technique11 FlatColor
{
	pass main
	{
		SetVertexShader( CompileShader( vs_4_0, FlatColorVP() ) );
		SetPixelShader( CompileShader( ps_4_0, FlatColorFP() ) );
		
		SetBlendState( NoBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthState, 0);
		SetRasterizerState( DefaultRasterState );	
	}
}

technique11 FlatColorTransparent
{
	pass main
	{
		SetVertexShader( CompileShader( vs_4_0, FlatColorVP() ) );
		SetPixelShader( CompileShader( ps_4_0, FlatColorFP() ) );
		
		SetBlendState( LinearBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( NoDepthWriteState, 0);
		SetRasterizerState( DefaultRasterState );	
	}
}

technique11 Outline
{
	pass main
	{
		SetVertexShader( CompileShader( vs_4_0, FlatColorVP() ) );
		SetPixelShader( CompileShader( ps_4_0, FlatColorFP() ) );
		
		SetBlendState( LinearBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthState, 0);
		SetRasterizerState( WireRasterState );	
	}
}




//////////////////////////////////////////////////////////////////////////////

struct ManipulatorVPInput
{
	float3 Position		: POSITION;
};


struct ManipulatorVPOutput
{
	float4 Position		: SV_POSITION;
};


struct ManipulatorFPOutput
{
	float4 Color	     : FRAG_OUTPUT_COLOR0;
};

// Simple vertex shader.
ManipulatorVPOutput ManipulatorVP(ManipulatorVPInput IN)
{
	ManipulatorVPOutput OUT;

	OUT.Position = mul(float4(IN.Position.xyz, 1), WorldViewProjection);
	OUT.Position.z = 0;
	
	return OUT;
}

// Simple fragment shader.
ManipulatorFPOutput ManipulatorFP()
{
	ManipulatorFPOutput OUT;
	
	OUT.Color = constantColor;

	return OUT;
}


technique11 ManipulatorAxis
{
	pass main
	{
		SetVertexShader( CompileShader( vs_4_0, ManipulatorVP() ) );
		SetPixelShader( CompileShader( ps_4_0, ManipulatorFP() ) );
		
		SetBlendState( LinearBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( NoDepthState, 0);
		SetRasterizerState( DefaultRasterState );	
	}
}


//////////////////////////////////////////////////////////////////////////////

// WorldAxes vertex shader
FlatColorVPOutput WorldAxesVP(FlatColorVPInput IN)
{
	FlatColorVPOutput OUT;

	// Set the rotation matrix
	OUT.Position = mul(float4(mul(float4(IN.Position.xyz,0), WorldView).xyz, 1.0f), Projection);

	// Offset to bottom left of screen.
	float aspect = Projection._m11/Projection._m00;
	float maintainSizeScale = 720.0f/ViewportWidthHeight.y;
	float inset = 0.2f * maintainSizeScale;
	float4 screenOffset = float4(-1.0f + inset/aspect, -1.0f + inset, 0.0f, 0.0f);
	OUT.Position = OUT.Position + screenOffset;
	OUT.Position.z = 0.0f;
	
	return OUT;
}


technique11 WorldAxes
{
	pass main
	{
		SetVertexShader( CompileShader( vs_4_0, WorldAxesVP() ) );
		SetPixelShader( CompileShader( ps_4_0, FlatColorFP() ) );
		
		SetBlendState( LinearBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( NoDepthState, 0);
		SetRasterizerState( DefaultRasterState );	
	}
}

//////////////////////////////////////////////////////////////////////////////

struct CameraManipulatorVPInput
{
	float3 Position		: SV_POSITION;
	float3 Normal		: NORMAL;
};

struct CameraManipulatorVPOutput
{
	float4 Position		: SV_POSITION;
	float4 ViewSpacePos	: TEXCOORD0;	
};

// Camera manipulator vertex shader.
CameraManipulatorVPOutput CameraManipulatorVP(CameraManipulatorVPInput IN)
{
	CameraManipulatorVPOutput OUT;

	// Set the rotation matrix
	
	OUT.ViewSpacePos = float4(mul(float4(IN.Position.xyz,0.0),WorldView).xyz, 1.0f);
	OUT.Position = mul(OUT.ViewSpacePos, Projection);

	// Offset to top right of screen.
	float aspect = Projection._m11/Projection._m00;
	float maintainSizeScale = 720.0f/ViewportWidthHeight.y;
	float inset = 0.2f * maintainSizeScale;
	float4 screenOffset = float4(1.0f - inset/aspect, 1.0f - inset, 0.0f, 0.0f);
	OUT.Position = OUT.Position + screenOffset;

	// Set the depth value.
	OUT.Position.z = OUT.ViewSpacePos.z / 10000.0f;

	return OUT;
}

// Camera manipulator lit fragment shader
ManipulatorFPOutput CameraManipulatorLitFP(CameraManipulatorVPOutput IN)
{
	ManipulatorFPOutput OUT;
	
	// Do lighting in camera space.
	float3 P = IN.ViewSpacePos.xyz;
	float3 N = -cross(normalize(ddx(P)), normalize(ddy(P)));
	float3 L = float3(0,0,1);

	// Taking the absolute value of the dot product because ddy() is flipped on GXM meaning N would be negated.
	OUT.Color = constantColor * abs(dot(N,L));

	return OUT;
}

// Flat fragment shader
ManipulatorFPOutput CameraManipulatorFlatFP(CameraManipulatorVPOutput IN)
{
	ManipulatorFPOutput OUT;
	
	OUT.Color = constantColor;

	return OUT;
}

// Camera Indicator Selection vertex shader
SingleSelectionVPOutput CameraIndicatorSelectionVP(ObjectSelectionVPInput IN)
{
	SingleSelectionVPOutput OUT;

	OUT.WorldPosition = mul(float4(IN.Position.xyz,0.0f), WorldView).xyz;
	OUT.Position = mul(float4(OUT.WorldPosition, 1.0f), Projection);
	OUT.Position.z = 0.0f;
	OUT.ViewSpaceZ = 0.0f;

	return OUT;
}

// Camera Indicator Selection fragment shader
SingleSelectionFPOutput CameraIndicatorSelectionFP(SingleSelectionVPOutput IN)
{
	SingleSelectionFPOutput OUT;

	OUT.FaceNormal = float4(0.0f, 0.0f, 1.0f, 0.0f);
	OUT.IdColorAndDepth = float4(selectionIDColor, -1.0f, 0.0f, 0.0f);

	return OUT;
}

technique11 SingleSelectionCameraIndicator
{
	pass main
	{
		SetVertexShader( CompileShader( vs_4_0, CameraIndicatorSelectionVP() ) );
		SetPixelShader( CompileShader( ps_4_0, CameraIndicatorSelectionFP() ) );
		
		SetBlendState( NoBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthState, 0);
		SetRasterizerState( DefaultRasterState );	
	}
}

//////////////////////////////////////////////////////////////////////////////

struct ManipulatorHiddenLineVPOutput
{
	float4 Position		: SV_POSITION;
	float3 ViewSpaceCentreToPos : TEXCOORD0;
	float3 ViewSpaceCentreToCam : TEXCOORD1;
};

// Simple vertex shader which doesnt project.
ManipulatorHiddenLineVPOutput ManipulatorHiddenLineVP(ManipulatorVPInput IN)
{
	ManipulatorHiddenLineVPOutput OUT;
	OUT.Position = mul(float4(IN.Position.xyz, 1), WorldViewProjection);
	OUT.Position.z = 0.0f;
	
	OUT.ViewSpaceCentreToPos = normalize( mul(float4(normalize( mul(float4(IN.Position.xyz, 1), World).xyz - mul(float4(0,0,0,1), World).xyz ),0),View  )).xyz;
#ifdef ORTHO_CAMERA
	OUT.ViewSpaceCentreToCam = float3(0,0,1);
#else // ORTHO_CAMERA
	OUT.ViewSpaceCentreToCam = -normalize( mul(mul(float4(0,0,0,1),World),View).xyz) ;
#endif // ORTHO_CAMERA
	return OUT;
}

// Simple fragment shader
ManipulatorFPOutput ManipulatorHiddenLineFP(ManipulatorHiddenLineVPOutput IN)
{
	ManipulatorFPOutput OUT;
	
	half drawMeOrNot = half(dot(IN.ViewSpaceCentreToPos, IN.ViewSpaceCentreToCam));

	half alpha = half(saturate(drawMeOrNot * 5.0f));
	clip(alpha);
	OUT.Color = constantColor;
	OUT.Color.w *= alpha;
	
	return OUT;
}

technique11 ManipulatorAxisHiddenLine
{
	pass main
	{
		SetVertexShader( CompileShader( vs_4_0, ManipulatorHiddenLineVP() ) );
		SetPixelShader( CompileShader( ps_4_0, ManipulatorHiddenLineFP() ) );
		
		SetBlendState( LinearBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( NoDepthState, 0);
		SetRasterizerState( DefaultRasterState );	
	}
}

///////////////////////////////////////////////////////////////////////////////////

struct MultipleSelectionHiddenLineVPOutput
{
	float4 Position		 : SV_POSITION;
	float3 ViewSpaceCentreToPos : TEXCOORD1;
	float3 ViewSpaceCentreToCam : TEXCOORD2;
};

struct MultipleSelectionHiddenLineFPOutput
{
	float4 IdColor	     : FRAG_OUTPUT_COLOR0;
};


// Multiple selection render vertex shader

// Multiple selection render vertex shader
MultipleSelectionHiddenLineVPOutput MultipleSelectionHiddenLineVP(ObjectSelectionVPInput IN)
{
	MultipleSelectionHiddenLineVPOutput OUT;
	OUT.Position = mul(float4(IN.Position.xyz, 1), WorldViewProjection);
	OUT.Position.z = 0.0f;
	
	// Get the vectors for needed for hidden line clipping
	OUT.ViewSpaceCentreToPos = normalize(mul(float4(normalize(mul(float4(IN.Position.xyz, 1), World).xyz - mul(float4(0,0,0,1), World).xyz ), 0), View)).xyz;
#ifdef ORTHO_CAMERA
	OUT.ViewSpaceCentreToCam = float3(0,0,1);
#else // ORTHO_CAMERA
	OUT.ViewSpaceCentreToCam = -normalize(mul(mul(float4(0,0,0,1),World),View).xyz) ;
#endif // ORTHO_CAMERA

	return OUT;
}

// Multiple selection render fragment shader
MultipleSelectionHiddenLineFPOutput MultipleSelectionHiddenLineFP(MultipleSelectionHiddenLineVPOutput IN)
{
	MultipleSelectionHiddenLineFPOutput OUT;

	// Clip the hidden lines
	half drawMeOrNot = half(dot(IN.ViewSpaceCentreToPos, IN.ViewSpaceCentreToCam));
	half alpha = drawMeOrNot;
	clip(alpha + 0.1f);
	
	OUT.IdColor = multipleSelectionIDColor;

	return OUT;
}

/*

technique MultipleSelectionHiddenLine
{
	pass main
	{
		DepthFunc = lessequal;
		DepthMask = true;

		VertexProgram = compile vp40 MultipleSelectionHiddenLineVP();
		FragmentProgram = compile fp40 MultipleSelectionHiddenLineFP();
	}		
}
*/

technique11 MultipleSelectionHiddenLine
{
	pass main
	{
		SetVertexShader( CompileShader( vs_4_0, MultipleSelectionHiddenLineVP() ) );
		SetPixelShader( CompileShader( ps_4_0, MultipleSelectionHiddenLineFP() ) );
		
		SetBlendState( NoBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthState, 0);
		SetRasterizerState( DefaultRasterState );			
	}
}
///////////////////////////////////////////////////////////////////////////////////

struct SingleSelectionHiddenLineVPOutput
{
	float4 Position		 : SV_POSITION;
	float3 WorldPosition : TEXCOORD0;
	float ViewSpaceZ	 : TEXCOORD1;

	float3 ViewSpaceCentreToPos : TEXCOORD2;
	float3 ViewSpaceCentreToCam : TEXCOORD3;

};

struct SingleSelectionHiddenLineFPOutput
{
	float IdColor	     : FRAG_OUTPUT_COLOR0;
	float3 FaceNormal	 : FRAG_OUTPUT_COLOR1;
	float Depth			 : FRAG_OUTPUT_COLOR2;
};

// Single selection render vertex shader
SingleSelectionHiddenLineVPOutput SingleSelectionHiddenLineVP(ObjectSelectionVPInput IN)
{
	SingleSelectionHiddenLineVPOutput OUT;
	
	OUT.Position = mul(float4(IN.Position.xyz, 1), WorldViewProjection);	
	OUT.Position.z = 0.0f;
	OUT.ViewSpaceCentreToPos = normalize(mul(float4(normalize( mul(float4(IN.Position.xyz, 1), World).xyz - mul(float4(0,0,0,1), World).xyz ),0),View)).xyz;
#ifdef ORTHO_CAMERA
	OUT.ViewSpaceCentreToCam = float3(0,0,1);
#else // ORTHO_CAMERA
	OUT.ViewSpaceCentreToCam = -normalize( mul(mul(float4(0,0,0,1),World),View).xyz);
#endif // ORTHO_CAMERA
	
	OUT.WorldPosition = mul(float4(IN.Position.xyz, 1.0f), World).xyz;
	OUT.ViewSpaceZ = mul(float4(IN.Position.xyz, 1), WorldView).z;
	
	return OUT;
}

// Single selection render fragment shader
SingleSelectionHiddenLineFPOutput SingleSelectionHiddenLineFP(SingleSelectionHiddenLineVPOutput IN)
{
	SingleSelectionHiddenLineFPOutput OUT;
	
	half drawMeOrNot = half(dot(IN.ViewSpaceCentreToPos, IN.ViewSpaceCentreToCam));

	half alpha = half(drawMeOrNot);
	clip(alpha);
	OUT.IdColor = selectionIDColor;

	// Face Normal calculation
	float3 faceNormal = -normalize(cross(normalize(ddx(IN.WorldPosition)), normalize(ddy(IN.WorldPosition))));
	OUT.FaceNormal = float3(faceNormal * 0.5f + 0.5f); 
	OUT.Depth = abs(IN.ViewSpaceZ);

	return OUT;
}

/*
technique SingleSelectionHiddenLine
{
	pass main
	{
		DepthFunc = lessequal;
		DepthMask = true;

		VertexProgram = compile vp40 SingleSelectionHiddenLineVP();
		FragmentProgram = compile fp40 SingleSelectionHiddenLineFP();	
	}		
}

*/

technique11 SingleSelectionHiddenLine
{
	pass main
	{
		SetVertexShader( CompileShader( vs_4_0, MultipleSelectionHiddenLineVP() ) );
		SetPixelShader( CompileShader( ps_4_0, MultipleSelectionHiddenLineFP() ) );
		
		SetBlendState( NoBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthState, 0);
		SetRasterizerState( DefaultRasterState );
	}
}

//////////////////////////////////////////////////////////////////////////////

// Simple vertex shader.
struct GridVPOutput
{
	float4 Position		: SV_POSITION;
	float3 ViewPosition : TEXCOORD0;
	float4 DepthPos		: TEXCOORD1;
};

GridVPOutput RenderGridVP(float3 Position : POSITION)
{
	GridVPOutput Out;
	Out.Position = mul(float4(Position.xyz, 1), WorldViewProjection);
	float4 viewPos =  mul(float4(Position.xyz, 1), WorldView);
	Out.ViewPosition = viewPos.xyz;

	viewPos.z -= 0.1f;
	Out.DepthPos = mul(viewPos, Projection);
	return Out;
}

// Simple fragment shader
float4 RenderGridFP(GridVPOutput In, out float Depth : FRAG_OUTPUT_DEPTH) : FRAG_OUTPUT_COLOR0
{
	Depth = In.DepthPos.z / In.DepthPos.w;

	float dist = length(In.ViewPosition);
	float fadeValue = 1.0f - saturate((dist - GridFadeStartDistance) * GridFadeDistanceScale);
	return constantColor * float4(1.0f,1.0f,1.0f,fadeValue);
}

technique11 RenderGrid
{
	pass main
	{
		SetVertexShader( CompileShader( vs_4_0, RenderGridVP() ) );
		SetPixelShader( CompileShader( ps_4_0, RenderGridFP() ) );
		
		SetBlendState( LinearBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthState, 0);
		SetRasterizerState( DefaultRasterState );	
	}
}

//////////////////////////////////////////////////////////////////////////////

technique11 RenderBoundBox
{
	pass main
	{
		SetVertexShader( CompileShader( vs_4_0, RenderGridVP() ) );
		SetPixelShader( CompileShader( ps_4_0, RenderGridFP() ) );
		
		SetBlendState( LinearBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthState, 0);
		SetRasterizerState( DefaultRasterState );	
	}
}

// Simple vertex shader.
struct RenderTransparentVPOutput
{
	float4 Position		: SV_POSITION;
};

RenderTransparentVPOutput RenderTransparentVP(float4 Position	: POSITION)
{
	RenderTransparentVPOutput Out;
	Out.Position = mul(float4(Position.xyz, 1), WorldViewProjection);
	return Out;
}
// Simple fragment shader
float4 RenderTransparentFP() : FRAG_OUTPUT_COLOR0
{
	return constantColor;
}


technique11 RenderTransparentPass < string PhyreRenderPass = "Transparent"; >
{
	pass main
	{
		SetVertexShader( CompileShader( vs_4_0, RenderTransparentVP() ) );
		SetPixelShader( CompileShader( ps_4_0, RenderTransparentFP() ) );
		
		SetBlendState( NoBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( DepthState, 0);
		SetRasterizerState( DefaultRasterState );	
	}
}

//////////////////////////////////////////////////////////////////////////////

Texture2D <float4> BitmapFontTexture;
float3 textColor = { 1.0f, 1.0f, 1.0f };
float CameraAspectRatio;

struct VPInput
{
	float3 position		: POSITION;
	float2 uv			: TEXCOORD0;
};

struct VPOutput
{
	float4 position		: SV_POSITION;
	float2 uv			: TEXCOORD0;
};

VPOutput TextVP(VPInput IN)
{
	VPOutput OUT;
	
	OUT.position = mul(float4(IN.position.xy, 1.0f, 1.0f), World);
	OUT.position.x *= CameraAspectRatio;
	OUT.uv = IN.uv;
	
	return OUT;
}

float4 TextFP(VPOutput IN) : FRAG_OUTPUT_COLOR0
{
	return float4(textColor, BitmapFontTexture.Sample(LinearClampSampler, IN.uv).x);
}



technique11 RenderText_AlphaBlend
{
	pass main
	{
		SetVertexShader( CompileShader( vs_4_0, TextVP() ) );
		SetPixelShader( CompileShader( ps_4_0, TextFP() ) );
		
		SetBlendState( LinearBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( NoDepthState, 0);
		SetRasterizerState( DefaultRasterState );	
	}
}

//////////////////////////////////////////////////////////

float4 StatisticsHUDTextColor = {1,1,1,1};
float4 StatisticsHUDBackgroundColor = {0,0,0,1};

// Simple vertex shader
FlatColorVPOutput GameEditStatisticsHUDVP(FlatColorVPInput IN)
{
	FlatColorVPOutput OUT;
		
	OUT.Position = mul(float4(IN.Position.xy, 0.0f, 1.0f), World);
	OUT.Position.x *= CameraAspectRatio;
		
	return OUT;
}

// Simple fragment shader
FlatColorFPOutput GameEditStatisticsHUDFP()
{
	FlatColorFPOutput OUT;
	
	OUT.Color = StatisticsHUDBackgroundColor;

	return OUT;
}


technique11 GameEditStatisticsHUD
{
	pass main
	{
		SetVertexShader( CompileShader( vs_4_0, GameEditStatisticsHUDVP() ) );
		SetPixelShader( CompileShader( ps_4_0, GameEditStatisticsHUDFP() ) );
		
		SetBlendState( LinearBlend, float4( 0.0f, 0.0f, 0.0f, 0.0f ), 0xFFFFFFFF );
		SetDepthStencilState( NoDepthState, 0);
		SetRasterizerState( DefaultRasterState );	
	}
}

////////////////////////////////////
