/* SCE CONFIDENTIAL
PhyreEngine(TM) Package 3.7.0.0
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef PHYRE_SHADER_PLATFORM_H
#define PHYRE_SHADER_PLATFORM_H

#ifdef __psp2__
	// Disable the D6204: profile 'sce_v/fp_psp2' does not support uniform default values warning
	#pragma warning (disable:6204)
#endif //! __psp2__

#ifdef PHYRE_D3DFX
	#define FRAG_OUTPUT_COLOR SV_TARGET
	#define FRAG_OUTPUT_COLOR0 SV_TARGET0
	#define FRAG_OUTPUT_COLOR1 SV_TARGET1
	#define FRAG_OUTPUT_COLOR2 SV_TARGET2
	#define FRAG_OUTPUT_COLOR3 SV_TARGET3
	#define FRAG_OUTPUT_DEPTH SV_DEPTH

	#define SYSTEM_PRIMITIVE_INDEX SV_PRIMITIVEID
#endif //! _PHYRE_D3DFX

//! Define fragment shader outputs if not defined yet.
#ifndef FRAG_OUTPUT_COLOR
	#define FRAG_OUTPUT_COLOR COLOR
#endif //! FRAG_OUTPUT_COLOR

#ifndef FRAG_OUTPUT_COLOR0
	#define FRAG_OUTPUT_COLOR0 COLOR0
#endif //! FRAG_OUTPUT_COLOR0

#ifndef FRAG_OUTPUT_COLOR1
	#define FRAG_OUTPUT_COLOR1 COLOR1
#endif //! FRAG_OUTPUT_COLOR1

#ifndef FRAG_OUTPUT_COLOR2
	#define FRAG_OUTPUT_COLOR2 COLOR2
#endif //! FRAG_OUTPUT_COLOR2

#ifndef FRAG_OUTPUT_COLOR3
	#define FRAG_OUTPUT_COLOR3 COLOR3
#endif //! FRAG_OUTPUT_COLOR3

#ifndef FRAG_OUTPUT_DEPTH
	#define FRAG_OUTPUT_DEPTH DEPTH
#endif //! FRAG_OUTPUT_DEPTH

#ifndef SYSTEM_PRIMITIVE_INDEX
	#define SYSTEM_PRIMITIVE_INDEX SV_PRIMITIVEID
#endif //! SYSTEM_PRIMITIVE_INDEX

#endif //! PHYRE_SHADER_PLATFORM_H
