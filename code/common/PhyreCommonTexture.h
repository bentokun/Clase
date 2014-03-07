/* SCE CONFIDENTIAL
PhyreEngine(TM) Package 3.7.0.0
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef PHYRE_SAMPLES_COMMON_TEXTURE_H
#define PHYRE_SAMPLES_COMMON_TEXTURE_H

namespace Phyre
{
	namespace PCommon
	{
		//!
		//! Texture helper functions
		//!
		PResult CreateCheckerTexture(PRendering::PTexture2D &checkerTexture, const PUInt32 width, const PUInt32 height, const PUInt32 checkerWidth, const PUInt32 checkerHeight);

	} //! End of PCommon Namespace
} //! End of Phyre Namespace

#endif //! PHYRE_SAMPLES_COMMON_TEXTURE_H
