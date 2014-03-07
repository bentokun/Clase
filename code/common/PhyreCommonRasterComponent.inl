/* SCE CONFIDENTIAL
PhyreEngine(TM) Package 3.7.0.0
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef PHYRE_SAMPLES_COMMON_RASTER_COMPONENT_INL
#define PHYRE_SAMPLES_COMMON_RASTER_COMPONENT_INL

namespace Phyre
{
	namespace PCommon
	{
		// Gets m_width.
		PUInt32 RasterComponent::getWidth() const
		{
			return m_width;
		}

		// Gets m_height.
		PUInt32 RasterComponent::getHeight() const
		{
			return m_height;
		}

		// Gets m_decay.
		float RasterComponent::getDecay() const
		{
			return m_decay;
		}

		// Gets m_scrollStepX.
		PInt32 RasterComponent::getScrollStepX() const
		{
			return m_scrollStepX;
		}

		// Gets m_scrollStepY.
		PInt32 RasterComponent::getScrollStepY() const
		{
			return m_scrollStepY;
		}

		// Description:
		// Sets dot color.
		// Arguments:
		// dotColor : RGB dot color.
		// Return Value List:
		// PE_RESULT_NO_ERROR: The dot color was set successfully.
		// Other : An error from a sub method.
		PResult	RasterComponent::setRasterDotColor(Vectormath::Aos::Vector3 dotColor)
		{
			m_dotColor = dotColor;
			PHYRE_TRY(PCommon::UpdateVec3ParamOnMeshInstance(m_rasterMeshInstance, PD_RASTER_MESH_SEGMENT, "RasterDotColour", dotColor));
			return PE_RESULT_NO_ERROR;
		}

		// Description:
		// Sets m_rasterDotTexture.
		// Arguments:
		// rasterDotTexture : The texture to use for raster dots.
		// Return Value List:
		// PE_RESULT_NULL_POINTER_ARGUMENT
		// PE_RESULT_NO_ERROR
		// Other : An error from a sub method.
		PResult RasterComponent::setRasterDotTexture(PRendering::PTexture2D *rasterDotTexture)
		{
			if (rasterDotTexture == NULL)
				return PE_RESULT_NULL_POINTER_ARGUMENT;

			m_rasterDotTexture = rasterDotTexture;
			PHYRE_TRY(PCommon::UpdateTextureParamOnMeshInstance(m_rasterMeshInstance, PD_RASTER_MESH_SEGMENT, "RasterDotSampler", *m_rasterDotTexture, s_bilinearSampler));
			return PE_RESULT_NO_ERROR;
		}

		// Description:
		// Sets m_decay.
		// Arguments:
		// decay : The value should be greater or equal to zero.
		// Return Value List:
		// PE_RESULT_NO_ERROR
		// PE_RESULT_INVALID_ARGUMENT value less than 0.0 specified.
		// Other : An error from a sub method.
		PResult RasterComponent::setDecay(float decay)
		{
			if (decay < 0.0f)
				return PE_RESULT_INVALID_ARGUMENT;

			m_decay = decay;
			PHYRE_TRY(PCommon::UpdateFloatParamOnMeshInstance(m_rasterMeshInstance, PD_RASTER_MESH_SEGMENT, "RasterDecay", m_decay));

			return PE_RESULT_NO_ERROR;
		}

		// Description:
		// Gets the offscreen target.
		// Arguments:
		// buffer : index of the buffer to get.
		// Returns:
		// The buffer as a 2D texture.
		PRendering::PTexture2D *RasterComponent::getOffscreenTarget(PUInt32 buffer)
		{
			return m_offscreenTarget[buffer]->getTexture2D();
		}

		// Gets m_rasterDotTexture.
		PRendering::PTexture2D *RasterComponent::getRasterDotTexture()
		{
			return m_rasterDotTexture;
		}

		// Gets m_rasterTexture.
		PRendering::PTexture2D *RasterComponent::getRasterTexture()
		{
			return &m_rasterTexture;
		}

		// Description:
		// Sets scroll step.
		// Arguments:
		// x : scroll step in X.
		// y : scroll step in Y.
		void RasterComponent::setScrollStepXY(PInt32 x,PInt32 y)
		{
			m_scrollStepX = x;
			m_scrollStepY = y;
		}

		// Description:
		// Sets raster dot color.
		// Arguments:
		// red - the red component.
		// green - the green component.
		// blue - the pink component, I mean the blue component.
		// Return Value List:
		// PE_RESULT_NO_ERROR
		// Other : An error from a sub method.
		PResult	RasterComponent::setRasterDotColorRGB(float red, float green, float blue)
		{
			return setRasterDotColor(Vectormath::Aos::Vector3(red, green, blue));
		}


		// Description:
		// Sets m_rasterDotTexture from a string name that has previously being registered via RegisterDotTexture.
		// Arguments:
		// name : Name of the texture to use.
		// Return Value List:
		// PE_RESULT_NULL_POINTER_ARGUMENT
		// PE_RESULT_NO_ERROR
		// Other : An error from a sub method.
		PResult RasterComponent::setDotTextureByName(const PChar *name)
		{
			if (!name)
				return PE_RESULT_NULL_POINTER_ARGUMENT;

			return setRasterDotTexture(GetRasterDotTextureByName(name));
		}

		// Description:
		// Get position.
		// Arguments:
		// x : x position.
		// y : y position.
		// Returns:
		// Location to index in the texture with scrolling applied.
		PUInt32 RasterComponent::getPlotAddress(PUInt32 x, PUInt32 y)
		{
			PUInt32 scrolled_x = (x + m_scrollOffsetX) % m_width;
			PUInt32 scrolled_y = (y + m_scrollOffsetY) % m_height;
			return (scrolled_y * m_width) + scrolled_x;
		}

		// Description:
		// Sets a dot on the raster.
		// Arguments:
		// x : x position.
		// y : y position.
		void RasterComponent::set(PUInt32 x, PUInt32 y)
		{
			if (m_raster && ((x <= (m_width - 1)) && (y <= (m_height - 1))))
			{
				if (m_packedRaster)
				{
					PUInt32 index = getPlotAddress(x, y);
					PUInt32 word = (index / PD_RASTER_BITS_PER_WORD);
					PUInt32 bit = index % PD_RASTER_BITS_PER_WORD;
					((PUInt32 *)m_raster)[word] |= (1 << bit);
				}
				else
				{
					((PUInt8 *)m_raster)[getPlotAddress(x,y)] = 0xFF;
				}
			}
			else
			{
				//PHYRE_PRINTF("RASTER: bad set() attempt discarded!! (%d %d)",x,y);
			}
		}

		// Description:
		// Clears a dot on the raster.
		// Arguments:
		// x : x position.
		// y : y position.
		void RasterComponent::reset(PUInt32 x, PUInt32 y)
		{
			if (m_raster && ((x <= (m_width - 1)) && (y <= (m_height - 1))))
			{
				if (m_packedRaster)
				{
					PUInt32 index = getPlotAddress(x, y);
					PUInt32 word = (index / PD_RASTER_BITS_PER_WORD);
					PUInt32 bit = index % PD_RASTER_BITS_PER_WORD;
					((PUInt32 *)m_raster)[word] &= ~(1 << bit);
				}
				else
				{
					((PUInt8 *)m_raster)[getPlotAddress(x,y)] = 0x0;
				}
			}
			else
			{
				//PHYRE_PRINTF("RASTER: bad reset() attempt discarded!! (%d %d)",x,y);
			}
		}

		// Description:
		// Read a dot from the raster. Map the texture if needed
		// Arguments:
		// x : x position.
		// y : y position.
		// Returns:
		// True if raster at coordinates (x,y) is set, otherwise False.
		bool RasterComponent::read(PUInt32 x, PUInt32 y)
		{
			if (m_raster && ((x <= (m_width - 1)) && (y <= (m_height - 1))))
			{
				if (m_packedRaster)
				{
					PUInt32 index = getPlotAddress(x, y);
					PUInt32 word = (index / PD_RASTER_BITS_PER_WORD);
					PUInt32 bit = index % PD_RASTER_BITS_PER_WORD;
					return (((PUInt32 *)m_raster)[word] & (1 << bit)) > 0;
				}
				else
				{
					return ((PUInt8 *)m_raster)[getPlotAddress(x,y)] > 0;
				}
			}
			else
			{
				//PHYRE_PRINTF("RASTER: bad read() attempt discarded!! (%d %d)",x,y);
				return false;
			}
		}


		// Description:
		// Rest the scroll offsets.
		void RasterComponent::resetScroll()
		{
			m_scrollOffsetX = 0;
			m_scrollOffsetY = 0;
		}

		// Description:
		// Helper to to get the ScriptedComponent associated with a raster.
		// Arguments:
		// raster : The raster.
		// Returns:
		// The ScriptedComponent associated with the raster, NULL if no ScriptedComponent found.
		PGameplay::PScriptedComponent *RasterComponent::GetMutableScriptedComponentForRaster(const RasterComponent *raster)
		{
			return (PGameplay::PScriptedComponent *)RasterComponent::GetScriptedComponentForRaster(raster);
		}

		// Description:
		// Helper to to get the ScriptedComponent associated with a raster.
		// Arguments:
		// raster : The raster.
		// Returns:
		// The ScriptedComponent associated with the raster, NULL if no ScriptedComponent found.
		const PGameplay::PScriptedComponent *RasterComponent::GetScriptedComponentForRaster(const RasterComponent *raster)
		{
			// The entity we belong to.
			PEntity *entity = raster->getEntity();

			// Get the class descriptor for the info components we want to find in the cluster.
			const PClassDescriptor *rasterComponentCD = PNamespace::GetGlobalNamespace().getClassDescriptor(PD_RASTER_INFO_COMPONENT_NAME);

			// Get Script
			if (rasterComponentCD && entity)
				return (PGameplay::PScriptedComponent *)entity->getComponentOfTypeOrDerived(*rasterComponentCD);
			else
				return NULL;
		}

	} //! End of PCommon Namespace
} //! End of Phyre Namespace

#endif //! PHYRE_SAMPLES_COMMON_RASTER_COMPONENT_INL
