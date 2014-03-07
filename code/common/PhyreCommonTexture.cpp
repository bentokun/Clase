/* SCE CONFIDENTIAL
PhyreEngine(TM) Package 3.7.0.0
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <Phyre.h>
#include <Geometry/PhyreGeometry.h>
#include <Rendering/PhyreRendering.h>

namespace Phyre
{
	namespace PCommon
	{
		// Description:
		// Create a checker texture.
		// Arguments:
		// checkerTexture - The checker texture to configure.
		// width - The width of the texture.
		// height - The height of the texture.
		// checkerWidth - The width of each checker.
		// checkerHeight - The height of each checker.
		// Return Value List:
		// PE_RESULT_NO_ERROR : The checker texture was configured successfully.
		// Other : An error occurred whilst creating the checker texture.
		PResult CreateCheckerTexture(PRendering::PTexture2D &checkerTexture, const PUInt32 width, const PUInt32 height, const PUInt32 checkerWidth, const PUInt32 checkerHeight)
		{
			using namespace PRendering;

			// Configure the texture by setting its dimensions and pixel format
			PHYRE_TRY(checkerTexture.setDimensions(width, height, *PHYRE_GET_TEXTURE_FORMAT(RGBA8)));
			checkerTexture.setIsMappable(true);

			// Populate the texture with pixel data
			PRendering::PTexture2D::PWriteMapResult	writeMapResult;
			PHYRE_TRY(checkerTexture.map(writeMapResult));

			PUInt8 *pixels	= static_cast<PUInt8 *>(writeMapResult.m_mips[0].m_buffer);
			if (pixels)
			{
				PUInt32	stride = writeMapResult.m_mips[0].m_stride;
				for(PUInt32 i = 0; i < height; i++)
				{
					PUInt32 *rowPixels = (PUInt32 *)pixels;
					for(PUInt32 j = 0; j < width; j++)
					{
						if(((j % (checkerWidth * 2)) < checkerWidth) ^ ((i % (checkerHeight * 2)) < checkerHeight))
							*rowPixels++ = PhyreLittleEndianConvert(0xff000000);
						else
							*rowPixels++ = PhyreLittleEndianConvert(0xffffffff);
					}
					pixels += stride;
				}
			}

			// Once we have populated the pixel buffer, unmap it
			PHYRE_TRY(checkerTexture.unmap(writeMapResult));

			return PE_RESULT_NO_ERROR;
		}
	} //! End of PCommon Namespace
} //! End of Phyre Namespace
