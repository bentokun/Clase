/* SCE CONFIDENTIAL
PhyreEngine(TM) Package 3.7.0.0
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <Phyre.h>
#include <Framework/PhyreFramework.h>
#include <Rendering/PhyreRendering.h>
#include <Geometry/PhyreGeometry.h>
#include <Gameplay/PhyreGameplay.h>
#include <Scene/PhyreScene.h>
#include <Scripting/PhyreScripting.h>

#include "PhyreCommon.h"
#include "PhyreCommonDebug.h"
#include "PhyreCommonScene.h"
#include "PhyreCommonTexture.h"
#include "PhyreCommonScripting.h"
#include "PhyreCommonRasterComponent.h"
#include "PhyreCommonRasterComponent.inl"
#include "PhyreCommonRasterComponentAssets.h"

namespace Phyre
{
	namespace PCommon
	{

		using namespace PFramework;
		using namespace PRendering;
		using namespace PGeometry;
		using namespace PSerialization;
		using namespace Vectormath::Aos;
		using namespace PScripting;
		using namespace PGameplay;

		//! Statics for common assets
		static bool										s_init = false; // Have the static assets being loaded?
		static PCluster									*s_rasterLoadedClusters[g_rasterFileNamesCount]; // The static assets.

		//! Statics for class
		PSamplerState									RasterComponent::s_bilinearSampler;
		PSamplerState									RasterComponent::s_pointSampler;
		PMapDynamic<PString, PRendering::PTexture2D *>	RasterComponent::s_dotTextures;

		// Description:
		// The naming convention for the raster dots. This enables the name to be extracted from
		// the asset path ...raster_dot_NAME. NAME is then available for
		// use in the script via setDotTexture().
		#define PD_RASTER_DOT_TEXTURE_NAME_PREFIX		"raster_dot_"

		// Description:
		// The declaration for the scene render pass for raster decay.
		PHYRE_DEFINE_SCENE_RENDER_PASS_TYPE(RasterDecay)

		// Description:
		// Clears the texture for the raster.
		// Arguments:
		// texture : The texture to clear.
		// val : The value to clear to. This defaults to zero.
		// mipLevel : The mip level to clear. This defaults to zero.
		// Return Value List:
		// PE_RESULT_NO_ERROR : The raster texture was cleared successfully.
		// Other : An error from a sub method.
		static PResult ClearRasterTextureL8(PRendering::PTexture2D &texture, const PUInt8 val=0x0, const PUInt32 mipLevel=0x0)
		{
			// Populate the texture with pixel data
			PRendering::PTexture2D::PWriteMapResult	writeMapResult;
			PHYRE_TRY(texture.map(writeMapResult));

			PUInt8 *pixels = static_cast<PUInt8 *>(writeMapResult.m_mips[mipLevel].m_buffer);
			if(pixels)
			{
				PUInt32 width = texture.getWidth();
				PUInt32 height = texture.getHeight();

				// If the stride matches the width copy in one go.
				if (writeMapResult.m_mips[mipLevel].m_stride == width)
				{
					memset(pixels, val, sizeof(PUInt8) * width * height);
				}
				else
				{
					// Otherwise clear a line at a time.
					PUInt32 stride = writeMapResult.m_mips[mipLevel].m_stride;
					for (PUInt32 y=0; y < height; y++)
						memset(&pixels[y*stride],val,width);
				}
			}

			// Once we have populated the pixel buffer, unmap it
			PHYRE_TRY(texture.unmap(writeMapResult));

			return PE_RESULT_NO_ERROR;
		}

		// Description:
		// Initializes a texture for the raster.
		// Arguments:
		// texture : The texture to initialize.
		// width : The width of the texture.
		// height : The height of the texture.
		// buffers : The buffer count for the texture.
		// val : The value to initialize the texture to.
		// mipLevel : The mip level to initialize.
		// Return Value List:
		// PE_RESULT_NO_ERROR : The texture was successfully initialized.
		// Other : An error from a sub method.
		static PResult InitializeRasterTextureL8(PRendering::PTexture2D &texture, const PUInt32 width, const PUInt32 height, const PUInt32 buffers, const PUInt8 val=0x0, const PUInt32 mipLevel=0x0)
		{
			// Configure the texture by setting its dimensions and pixel format
			PHYRE_TRY(texture.setDimensions(width, height, *PHYRE_GET_TEXTURE_FORMAT(L8)));
			texture.setIsMappable(true);
			texture.setBufferCount(buffers);
			texture.setAutoMipMap(false);

			// Clear texture
			PHYRE_TRY(ClearRasterTextureL8(texture, val, mipLevel));

			return PE_RESULT_NO_ERROR;
		}

		// Description:
		// A helper function which uses a sampler name to find a texture in a mesh.
		// Arguments:
		// meshInstance : The mesh instance to search for the texture in.
		// segment : The mesh segment to check for the texture.
		// samplerName : The sampler name to identify the texture with.
		// Returns:
		// The texture if one was found; otherwise NULL is returned.
		static Phyre::PRendering::PTexture2D *GetTextureFromMeshBySamplerName(PMeshInstance *meshInstance, PUInt32 segment, const PChar *samplerName)
		{
			PHYRE_ASSERT(segment <= meshInstance->getMaterialSet().getMaterialCount());

			const PRendering::PMaterial *material = meshInstance->getMaterialSet().getMaterials()[segment];
			if(material)
			{
				PUInt32 numParameters = material->getParameterBuffer().getShaderParameterDefinitions().getCount();
				for(PUInt32 i = 0; i < numParameters; ++i)
				{
					const PRendering::PShaderParameterDefinition &param = material->getParameterBuffer().getShaderParameterDefinitions()[i];
					if(param.getParameterType() == PRendering::PE_SHADER_PARAMETER_TEXTURE2D)
					{
						if(!strcmp(param.getName(), samplerName))
						{
							const void *parameterValue = material->getParameterBuffer().getParameter(param);
							const PRendering::PShaderParameterCaptureBufferTextureBase *texParam = (const PRendering::PShaderParameterCaptureBufferTextureBase *)parameterValue;

							if(texParam && (texParam->m_parameterType == PRendering::PE_SHADER_PARAMETER_TEXTURE2D))
							{
								const PRendering::PShaderParameterCaptureBufferTexture2D *texParam2D = (const PRendering::PShaderParameterCaptureBufferTexture2D *)texParam;
								return (PTexture2D *)texParam2D->m_texture;
							}
						}
					}
				}
			}
			return NULL;
		}

		PHYRE_BIND_START(RasterComponent)
			PHYRE_BIND_METHOD(getWidth)
			PHYRE_BIND_METHOD(getHeight)
			PHYRE_BIND_METHOD(setDecay)
			PHYRE_BIND_METHOD(getDecay)
			PHYRE_BIND_METHOD(set)
			PHYRE_BIND_METHOD(reset)
			PHYRE_BIND_METHOD(read)
			PHYRE_BIND_METHOD(startRasterUpdate)
			PHYRE_BIND_METHOD(endRasterUpdate)
			PHYRE_BIND_METHOD(clear)
			PHYRE_BIND_METHOD(setRasterDotColorRGB)
			PHYRE_BIND_METHOD(setDotTextureByName)
			PHYRE_BIND_METHOD(setScrollStepXY)
			PHYRE_BIND_METHOD(getScrollStepX)
			PHYRE_BIND_METHOD(getScrollStepY)
			PHYRE_BIND_METHOD(resetScroll)
		PHYRE_BIND_END

		// Description:
		// The constructor for the RasterComponent class.
		RasterComponent::RasterComponent()
			: PComponent(PHYRE_CLASS(RasterComponent))
			, m_width(0)
			, m_height(0)
			, m_decay(20.0f)
			, m_scrollStepX(0)
			, m_scrollStepY(0)
			, m_scrollOffsetX(0)
			, m_scrollOffsetY(0)
			, m_packedRaster(false)
			, m_dotColor(Vectormath::Aos::Vector3(0.0f, 1.0f, 0.0f))
			, m_updateMeshInstance(NULL)
			, m_rasterScreenTexture(NULL)
			, m_rasterDotTexture(NULL)
			, m_rasterMeshInstance(NULL)
			, m_updateBuffer(0)
			, m_offscreenCluster(NULL)
			, m_oldMaterialSet(NULL)
			, m_raster(NULL)
		{
			for (PUInt32 i=0; i< PHYRE_STATIC_ARRAY_SIZE(m_offscreenTarget); ++i)
				m_offscreenTarget[i] = NULL;
		}

		// Description:
		// Initializes a raster component.
		// Arguments:
		// rasterMeshInstance : The mesh to apply the raster effect to.
		// component : The component which holds the properties describing the raster.
		// Return Value List:
		// PE_RESULT_NO_ERROR : The raster component was successfully initialized.
		// PE_RESULT_INSUFFICIENT_DATA : The operation failed because the component specified didn't contain the information needed
		// to instantiate the raster.
		// Other : An error from a sub method.
		PResult RasterComponent::initialize(PRendering::PMeshInstance *rasterMeshInstance, PComponent *component)
		{
			if (!(DynamicPropertyIsPresent(component, "m_width") && DynamicPropertyIsPresent(component, "m_height")))
				return PE_RESULT_INSUFFICIENT_DATA;

			// Read the custom properties needed to initialize the raster
			PUInt32 *width = GetDynamicPropertyValuePtr<PUInt32>(component, "m_width");
			PUInt32 *height = GetDynamicPropertyValuePtr<PUInt32>(component, "m_height");

			// Check we got something valid.
			if (!width || !height || *width == 0 || *height == 0)
				return PE_RESULT_INSUFFICIENT_DATA;

			// If a dot texture is present use that.
			PTexture2D *dotTexture = GetDynamicPropertyPtr<PTexture2D>(component, "m_dotTexture");

			// If a name is supplied register with the map so can be referenced from Lua.
			if (dotTexture && DynamicPropertyIsPresent(component, "m_dotTextureName"))
			{
				PString *name = GetDynamicPropertyValuePtr<PString>(component, "m_dotTextureName");
				if (name)
					RegisterDotTexture(name->c_str(), dotTexture);
			}

			// Now initialize.
			return initialize(rasterMeshInstance, *width, *height, dotTexture);
		}

		// Description:
		// Registers a dot texture that can be used by the raster and associates a name with it.
		// Arguments:
		// name: The name to associate with the dot texture.
		// texture: The dot texture to register.
		// Return Value List:
		// PE_RESULT_NO_ERROR : The dot texture was successfully registered.
		// PE_RESULT_NULL_POINTER_ARGUMENT : The operation failed because the name or texture argument was NULL.
		PResult RasterComponent::RegisterDotTexture(const PChar *name, PTexture2D *texture)
		{
			if (!texture || !name)
				return PE_RESULT_NULL_POINTER_ARGUMENT;

			PHYRE_PRINTF("RasterComponent: Registered dot texture: %s\n", name);
			s_dotTextures.insertAsPair(PString(name), texture);

			return PE_RESULT_NO_ERROR;
		}

		// Description:
		// Gets a dot texture that was previously registered with RegisterDotTexture().
		// Arguments:
		// name : The name of the dot texture to get.
		// Returns:
		// A pointer to the requested dot texture if it was found; otherwise NULL is returned.
		PTexture2D* RasterComponent::GetRasterDotTextureByName(const PChar *name)
		{
			PTexture2D** texture = s_dotTextures.find(PString(name));
			if (texture)
				return *texture;
			else
				return NULL;
		}

		// Description:
		// Registers a texture from a cluster.
		// Arguments:
		// assetPath: The asset path for the cluster.
		// cluster: The cluster containing the texture.
		// Return Value List:
		// PE_RESULT_OBJECT_NOT_FOUND - The operation failed because the texture could not be found.
		// PE_RESULT_NO_ERROR - The operation was successful and the located texture was registered.
		// Other : An error from a sub method.
		PResult RasterComponent::FindAndRegisterDotTexture(const PChar *assetPath, PCluster *cluster)
		{
			const PChar *dot = strstr(assetPath, PD_RASTER_DOT_TEXTURE_NAME_PREFIX);

			// See if a dot texture using our naming convention (RasterComponent.h).
			if (!dot)
				return PE_RESULT_OBJECT_NOT_FOUND;

			// Extract the name from the file.
			const PUInt32 bufferSize = 16;
			const PChar *name= dot + strlen(PD_RASTER_DOT_TEXTURE_NAME_PREFIX);
			PChar tempBuffer[bufferSize];
			PUInt32 l = 0;
			while ((name[l] != '.') && (l < (bufferSize - 1)))
			{
				tempBuffer[l] = name[l];
				l++;
			}
			tempBuffer[l] = '\0';

			// Register so we can access via script.
			PHYRE_TRY(RegisterDotTexture(tempBuffer, FindFirstInstanceInCluster<PTexture2D>(*cluster)));

			return PE_RESULT_NO_ERROR;
		}

		// Description:
		// Initializes the static members of the RasterComponent class.
		// Return Value List:
		// PE_RESULT_NO_ERROR : The raster component was initialized successfully.
		// Other: An error occurred in a sub-method.
		PResult	RasterComponent::Initialize()
		{
			// Load the common assets if not done already
			if (!s_init)
			{
				PHYRE_TRY(PhyreOS::SetCurrentDirToPhyre());
				for(PUInt32 i = 0; i < g_rasterFileNamesCount; i++)
				{
					// Load the asset
					PHYRE_TRY(PCluster::LoadAssetFile(s_rasterLoadedClusters[i], g_rasterFileNames[i]));

					// Register the texture
					FindAndRegisterDotTexture(g_rasterFileNames[i], s_rasterLoadedClusters[i]);
				}
				PHYRE_TRY(PApplication::FixupClusters(s_rasterLoadedClusters, g_rasterFileNamesCount));

				// Init and bind samplers.
				s_bilinearSampler.setMinFilter(PRendering::PSamplerState::PE_SAMPLER_STATE_FILTER_LINEAR_MIPMAP_LINEAR);
				s_bilinearSampler.setMagFilter(PRendering::PSamplerState::PE_SAMPLER_STATE_FILTER_LINEAR);
				s_bilinearSampler.setWrapS(PRendering::PSamplerState::PE_SAMPLER_STATE_WRAP_REPEAT);
				s_bilinearSampler.setWrapT(PRendering::PSamplerState::PE_SAMPLER_STATE_WRAP_REPEAT);
				s_bilinearSampler.setWrapR(PRendering::PSamplerState::PE_SAMPLER_STATE_WRAP_REPEAT);

				s_pointSampler.setMinFilter(PRendering::PSamplerState::PE_SAMPLER_STATE_FILTER_NEAREST);
				s_pointSampler.setMagFilter(PRendering::PSamplerState::PE_SAMPLER_STATE_FILTER_NEAREST);
				s_pointSampler.setWrapS(PRendering::PSamplerState::PE_SAMPLER_STATE_WRAP_CLAMP_TO_EDGE);
				s_pointSampler.setWrapT(PRendering::PSamplerState::PE_SAMPLER_STATE_WRAP_CLAMP_TO_EDGE);
				s_pointSampler.setWrapR(PRendering::PSamplerState::PE_SAMPLER_STATE_WRAP_CLAMP_TO_EDGE);

				PHYRE_TRY(s_bilinearSampler.bind());
				PHYRE_TRY(s_pointSampler.bind());

				s_init = true;
			}

			return PE_RESULT_NO_ERROR;
		}

		// Description:
		// Terminates the raster and frees all allocated resources.
		// Return Value List:
		// PE_RESULT_NO_ERROR : The operation was successful.
		// Other : An error from a sub method.
		PResult RasterComponent::Terminate()
		{
			PHYRE_TRY(s_bilinearSampler.unbind());
			PHYRE_TRY(s_pointSampler.unbind());
			if (s_rasterLoadedClusters[0])
			{
				for(PUInt32 i = 0 ; i < g_rasterFileNamesCount; i++)
					delete s_rasterLoadedClusters[i];
				s_rasterLoadedClusters[0]  = NULL;
			}

			// Free the dot map
			s_dotTextures.clear();

			return PE_RESULT_NO_ERROR;
		}

		// Description:
		// Initializes a raster component.
		// Arguments:
		// rasterMeshInstance : The mesh to apply the raster effect to.
		// width : The width of the raster.
		// height : The height of the raster.
		// dotTexture : The texture for the raster dot.
		// Return Value List:
		// PE_RESULT_NO_ERROR : The operation was successful.
		// PE_RESULT_OBJECT_NOT_FOUND : The operation failed because a required effect variant could not be found in a cluster.
		// Other : An error from a sub method.
		PResult	RasterComponent::initialize(PMeshInstance *rasterMeshInstance, PUInt32 width, PUInt32 height, PRendering::PTexture2D *dotTexture /*=NULL*/)
		{
			// Call RasterComponent::Initialize first please.
			PHYRE_ASSERTMSG(s_init, "RasterComponent:Initialize() before initialize().");

			// These are properties of the raster and cannot change after init
			m_width = width;
			m_height = height;
			m_rasterMeshInstance = rasterMeshInstance;

			// Set the dot shape texture - use the specified one if present, if not use a built-in shape.
			if (dotTexture)
				m_rasterDotTexture = dotTexture;
			else
				m_rasterDotTexture = FindFirstInstanceInCluster<PTexture2D>(*s_rasterLoadedClusters[g_roundRasterDotTextureIndex]);

			// Get the cluster we will use for offscreen surfaces
			m_offscreenCluster = s_rasterLoadedClusters[g_rasterDecayShaderIndex];

			// Initialize the offscreen buffers (setting the value 'age' to max value)
			PHYRE_TRY(initOffscreenSurfaces(m_offscreenCluster));

			// Create the mesh to be used for offscreen rendering.
			PHYRE_TRY(initOffscreenMesh(m_offscreenCluster));

			// Initialize raster textures.
			// Use 2 buffers to avoid blocking.
			PHYRE_TRY(InitializeRasterTextureL8(m_rasterTexture, m_width, m_height, 2));

			// Find the base texture of the screen material - we want to use this with the raster shader.
			m_rasterScreenTexture = GetTextureFromMeshBySamplerName(rasterMeshInstance, PD_RASTER_MESH_SEGMENT, PD_RASTER_MESH_SAMPLER);

			// Get the raster shader and apply to the raster mesh, setting appropriate parameters.
			PCluster *rasterShaderCluster = s_rasterLoadedClusters[g_rasterShaderIndex];
			PEffectVariant *effectVariant = FindFirstInstanceInCluster<PEffectVariant>(*rasterShaderCluster);
			if(effectVariant == NULL)
				return PE_RESULT_OBJECT_NOT_FOUND;
			PMaterial *newMaterial = effectVariant->createMaterial(*rasterShaderCluster);

			// Get the old material set
			m_oldMaterialSet = &m_rasterMeshInstance->getMaterialSet();

			// Copy the original material set
			m_materialSet.copyFrom(*m_oldMaterialSet);

			// Set on the mesh
			m_rasterMeshInstance->setMaterialSet(&m_materialSet);

			// Apply our raster material to the mesh.
			PHYRE_TRY(applyRasterMaterialToMesh(m_rasterMeshInstance, newMaterial, m_rasterDotTexture,PD_RASTER_MESH_SEGMENT));

			// Finally make our raster.
			if (m_packedRaster)
				m_raster = PHYRE_ALLOCATE(PUInt32, ((m_width * m_height) / PD_RASTER_BITS_PER_WORD) + 1);
			else
				m_raster = PHYRE_ALLOCATE(PUInt8, m_width * m_height);

			return PE_RESULT_NO_ERROR;
		}

		// Description:
		// Clear the raster.
		void RasterComponent::clear()
		{
			// Clear the raster
			if (m_packedRaster)
				memset(m_raster, 0x0, (m_width * m_height) / PD_RASTER_BYTES_PER_WORD + 1);
			else
				memset(m_raster, 0x0, m_width * m_height);
		}

		// Description:
		// Prepares to write to the raster texture.
		void RasterComponent::startRasterUpdate()
		{
			// Update scrolling if enabled.
			if (m_scrollStepX != 0)
			{
				m_scrollOffsetX += m_scrollStepX;
				m_scrollOffsetX = m_scrollOffsetX % m_width;
			}
			if (m_scrollStepY != 0)
			{
				m_scrollOffsetY += m_scrollStepY;
				m_scrollOffsetY = m_scrollOffsetY % m_height;
			}
		}

		// Description:
		// Ends a texture update and unmaps the texture.
		// Return Value List:
		// PE_RESULT_NO_ERROR : The operation was successful.
		// Other : An error from a sub method.
		PResult RasterComponent::endRasterUpdate()
		{
			PTexture2D::PWriteMapResult	writeMapResult;

			// Map the texture and copy the buffer to it
			PHYRE_TRY(m_rasterTexture.map(writeMapResult));

			// Get the pixels for mip level 0
			PUInt8 *writePixels = static_cast<PUInt8 *>(writeMapResult.m_mips[0].m_buffer);

			// Copy the data
			if (m_packedRaster)
			{
				const PUInt32 stride = writeMapResult.m_mips[0].m_stride;
				for (PUInt32 x=0; x < m_width; x++)
				{
					for (PUInt32 y=0; y < m_height; y++)
					{
						PUInt32 index = y * m_width + x;
						PUInt32 word = (index / PD_RASTER_BITS_PER_WORD);
						PUInt32 bit = index % PD_RASTER_BITS_PER_WORD;
						writePixels[y*stride+x] = ((PUInt32 *)m_raster)[word] & (1 << bit)? 0xFF : 0x0;
					}
				}
			}
			else
			{
				// If the stride matches the width copy in one go, otherwise use helper to copy line by line.
				if (writeMapResult.m_mips[0].m_stride == m_width)
					memcpy(writePixels, m_raster, m_width * m_height);
				else
					PTextureCommonBase::StridedMemcpy(writePixels, writeMapResult.m_mips[0].m_stride, m_raster, m_width, m_width, m_height);
			}

			// Finish
			PHYRE_TRY(m_rasterTexture.unmap(writeMapResult));

			return PE_RESULT_NO_ERROR;
		}

		// Description:
		// Creates and initializes the offscreen surfaces.
		// Arguments:
		// offscreenCluster : The cluster the create the offscreen surfaces in.
		// Return Value List:
		// PE_RESULT_OUT_OF_MEMORY :  The operation failed because the name or texture argument was NULL.
		// Other : An error from a sub method.
		PResult RasterComponent::initOffscreenSurfaces(PCluster *offscreenCluster)
		{
			for (PUInt32 i=0; i < PHYRE_STATIC_ARRAY_SIZE(m_offscreenTarget) ; i++)
			{
				// Create the surface
				PTexture2D *offscreencolorTexture = offscreenCluster->create<PTexture2D>(1);
				if(!offscreencolorTexture)
					return PHYRE_SET_LAST_ERROR(PE_RESULT_OUT_OF_MEMORY, "Unable to create color texture");

				// Set the size to raster dimensions
				PHYRE_TRY(offscreencolorTexture->setDimensions(m_width, m_height, *PHYRE_GET_TEXTURE_FORMAT(RGBA8)));
				offscreencolorTexture->setAutoMipMap(false);

				// Now construct
				m_offscreenTarget[i] = offscreenCluster->allocateInstanceAndConstruct<PRenderTarget>(*offscreencolorTexture);
				if(!m_offscreenTarget[i])
					return PHYRE_SET_LAST_ERROR(PE_RESULT_OUT_OF_MEMORY, "Unable to create render target");
			}

			return PE_RESULT_NO_ERROR;
		}

		// Description:
		// Creates the mesh instance and material used to render to the offscreen 'decay' surfaces.
		// Arguments:
		// offscreenCluster : The cluster to create the mesh in.
		// Return Value List:
		// PE_RESULT_NO_ERROR : The operation was successful.
		// PE_RESULT_OBJECT_NOT_FOUND  The operation failed because the required effect variant could not be found in the offscreen cluster.
		// PE_RESULT_UNABLE_TO_ALLOCATE The operation failed because the fullscreen mesh instance could not be allocated.
		// Other : An error from a sub method.
		PResult RasterComponent::initOffscreenMesh(PCluster *offscreenCluster)
		{
			PEffectVariant *effectVariant = FindFirstInstanceInCluster<PEffectVariant>(*offscreenCluster);
			if(effectVariant == NULL)
				return PE_RESULT_OBJECT_NOT_FOUND;

			PMaterial *material = effectVariant->createMaterial(*offscreenCluster);
			PUInt32 offscreenTextureTarget = (m_updateBuffer + 1) & 0x1;
			const PShaderParameterDefinition *parameterRasterSampler = effectVariant->findShaderParameterByName("RasterSampler");
			if(parameterRasterSampler)
			{
				PShaderParameterCaptureBufferTexture2D textureCapture;
				textureCapture.m_texture = &m_rasterTexture;
				textureCapture.m_textureBufferIndex = 0;
				textureCapture.m_samplerState = &s_pointSampler;
				PHYRE_TRY(material->getParameterBuffer().setParameter(*parameterRasterSampler, &textureCapture));
			}

			const PShaderParameterDefinition *parameterRasterPersistenceSampler = effectVariant->findShaderParameterByName("RasterPersistenceSampler");
			if(parameterRasterPersistenceSampler)
			{
				PShaderParameterCaptureBufferTexture2D textureCapture;
				textureCapture.m_texture = m_offscreenTarget[offscreenTextureTarget]->getTexture2D();
				textureCapture.m_textureBufferIndex = 0;
				textureCapture.m_samplerState = &s_pointSampler;
				PHYRE_TRY(material->getParameterBuffer().setParameter(*parameterRasterPersistenceSampler, &textureCapture));
			}

			const PShaderParameterDefinition *linearWrapSamplerParameterDefinition = effectVariant->findShaderParameterByName("LinearWrapSampler");
			if(linearWrapSamplerParameterDefinition)
				PHYRE_TRY(material->getParameterBuffer().setParameter(*linearWrapSamplerParameterDefinition, &s_bilinearSampler));

			m_updateMeshInstance = CreateFullscreenMeshInstance(*offscreenCluster, *material);

			if(m_updateMeshInstance == NULL)
				return PHYRE_SET_LAST_ERROR(PE_RESULT_UNABLE_TO_ALLOCATE, "Unable to create the full screen mesh.");

			return PE_RESULT_NO_ERROR;
		}

		// Description:
		// Makes a raster material.
		// Arguments
		// newMaterial : The new material for the raster.
		// dotTexture : The texture for the raster dot.
		// Return Value List:
		// PE_RESULT_NO_ERROR : The operation was successful.
		// Other : An error from a sub method.
		PResult RasterComponent::makeRasterMaterial(PMaterial *newMaterial, PTexture2D *dotTexture)
		{
			const PEffectVariant &effectVariant = newMaterial->getEffectVariant();
			const PShaderParameterDefinition *rasterDotSamplerParameterDefinition = effectVariant.findShaderParameterByName("RasterDotSampler");

			if(rasterDotSamplerParameterDefinition)
			{
				PShaderParameterCaptureBufferTexture2D textureCapture;
				textureCapture.m_texture = dotTexture;
				textureCapture.m_textureBufferIndex = 0;
				textureCapture.m_samplerState = &s_bilinearSampler;
				PHYRE_TRY(newMaterial->getParameterBuffer().setParameter(*rasterDotSamplerParameterDefinition, &textureCapture));
			}

			const PShaderParameterDefinition *rasterPersistenceSamplerParameterDefinition = effectVariant.findShaderParameterByName("RasterPersistenceSampler");
			if(rasterPersistenceSamplerParameterDefinition)
			{
				PShaderParameterCaptureBufferTexture2D textureCapture;
				textureCapture.m_texture = m_offscreenTarget[m_updateBuffer]->getTexture2D();
				textureCapture.m_textureBufferIndex = 0;
				textureCapture.m_samplerState = &s_pointSampler;
				PHYRE_TRY(newMaterial->getParameterBuffer().setParameter(*rasterPersistenceSamplerParameterDefinition, &textureCapture));
			}

			const PShaderParameterDefinition *screenSamplerParameterDefinition = effectVariant.findShaderParameterByName("TextureSampler");
			if(screenSamplerParameterDefinition)
			{
				PShaderParameterCaptureBufferTexture2D textureCapture;
				textureCapture.m_texture = m_rasterScreenTexture;
				textureCapture.m_textureBufferIndex = 0;
				textureCapture.m_samplerState = &s_bilinearSampler;
				PHYRE_TRY(newMaterial->getParameterBuffer().setParameter(*screenSamplerParameterDefinition, &textureCapture));
			}

			const PShaderParameterDefinition *pointClampSamplerParameterDefinition = effectVariant.findShaderParameterByName("PointClampSampler");
			if(pointClampSamplerParameterDefinition)
				PHYRE_TRY(newMaterial->getParameterBuffer().setParameter(*pointClampSamplerParameterDefinition, &s_pointSampler));

			const PShaderParameterDefinition *linearWrapSamplerParameterDefinition = effectVariant.findShaderParameterByName("LinearWrapSampler");
			if(linearWrapSamplerParameterDefinition)
				PHYRE_TRY(newMaterial->getParameterBuffer().setParameter(*linearWrapSamplerParameterDefinition, &s_bilinearSampler));

			return PE_RESULT_NO_ERROR;
		}

		// Description:
		// Creates and sets a raster material on a given mesh instance.
		// Arguments:
		// meshInstance : The mesh to apply the raster material to.
		// newMaterial : The raster material to apply.
		// dotTexture : The texture to use for the raster dot.
		// segment : The mesh segment to apply the material to. This defaults to 0.
		// Return Value List:
		// PE_RESULT_NO_ERROR : The operation was successful.
		// Other : An error from a sub method.
		PResult RasterComponent::applyRasterMaterialToMesh(PMeshInstance *meshInstance, PMaterial *newMaterial, PTexture2D *dotTexture, PUInt32 segment)
		{
			// Make the new material.
			PHYRE_TRY(makeRasterMaterial(newMaterial, dotTexture));

			// Apply to the mesh.
			PHYRE_TRY(meshInstance->getMaterialSet().setMaterial(segment, newMaterial));

			// Set the shader params.
			PHYRE_TRY(UpdateFloatParamOnMeshInstance(m_rasterMeshInstance, segment, "RasterDecay",    (float)m_decay));
			PHYRE_TRY(UpdateFloatParamOnMeshInstance(m_rasterMeshInstance, segment, "RasterWidth",    (float)m_width));
			PHYRE_TRY(UpdateFloatParamOnMeshInstance(m_rasterMeshInstance, segment, "RasterHeight",   (float)m_height));
			PHYRE_TRY(UpdateVec3ParamOnMeshInstance(m_rasterMeshInstance,  segment, "RasterDotColour",       m_dotColor));

			return PE_RESULT_NO_ERROR;
		}

		// Description:
		// Removes the raster material from the mesh and restores the original one.
		// Return Value List:
		// PE_RESULT_NO_ERROR : The operation was successful.
		// Other : An error from a sub method.
		PResult RasterComponent::uninitialize()
		{
			// Restore the old material to the mesh.
			m_rasterMeshInstance->setMaterialSet(m_oldMaterialSet);

			// Remove the raster texture
			PHYRE_TRY(m_rasterTexture.unbind());

			// Free the raster itself
			PHYRE_FREE(m_raster);
			m_raster = NULL;

			return PE_RESULT_NO_ERROR;
		}

		// Description:
		// Clears offscreen buffers to avoid using initialized data.
		// Arguments:
		// renderer : The renderer to use.
		// Return Value List:
		// PE_RESULT_NO_ERROR : The operation was successful.
		// Other : An error from a sub method.
		PResult RasterComponent::initOffscreenBuffers(PRendering::PRenderer& renderer)
		{
			for (PUInt32 i=0; i < PHYRE_STATIC_ARRAY_SIZE(m_offscreenTarget); i++)
			{
				PRenderSurface colorSurface(m_offscreenTarget[i]);
				PHYRE_TRY(renderer.setColorTarget(&colorSurface));
				PHYRE_TRY(renderer.setClearColor(1.0f, 1.0f, 1.0f, 1.0f));
				PHYRE_TRY(renderer.beginScene(PRenderInterfaceBase::PE_CLEAR_COLOR_BUFFER_BIT));
				// Just clear
				PHYRE_TRY(renderer.endScene());
			}

			// Restore the color and depth targets to use the back buffer.
			PHYRE_TRY(renderer.setColorTarget(NULL));
			PHYRE_TRY(renderer.setDepthStencilTarget(NULL));

			return PE_RESULT_NO_ERROR;
		}

		// Description:
		// Uses the supplied renderer to update the 'decay' offscreen buffer.
		// Arguments:
		// renderer : The renderer to use.
		// Return Value List:
		// PE_RESULT_NO_ERROR : The operation was successful.
		// Other : An error from a sub method.
		PResult RasterComponent::updateSurfaces(PRendering::PRenderer &renderer)
		{
			// Update
			renderer.setSceneRenderPassType(PHYRE_GET_SCENE_RENDER_PASS_TYPE(RasterDecay));
			PHYRE_TRY(renderer.setDepthStencilTarget(NULL));
			PRenderSurface colorSurface(m_offscreenTarget[m_updateBuffer]);
			PHYRE_TRY(renderer.setColorTarget(&colorSurface));

			PHYRE_TRY(renderer.beginScene(0)); // no buffer clear
			PHYRE_TRY(renderer.renderMeshInstance(*m_updateMeshInstance));
			PHYRE_TRY(renderer.endScene());

			// Swap around the inputs to the offscreen buffers
			PHYRE_TRY(UpdateTextureParamOnMeshInstance(m_updateMeshInstance, PD_RASTER_OFFSCREEN_TEXTURE_MESH_SEGMENT, "RasterPersistenceSampler", *m_offscreenTarget[m_updateBuffer]->getTexture2D(), s_pointSampler));

			// Toggle buffers
			m_updateBuffer = (m_updateBuffer + 1) & 0x1;

			// Change the persistence sampler we use for the main render next time
			PHYRE_TRY(UpdateTextureParamOnMeshInstance(m_rasterMeshInstance, PD_RASTER_MESH_SEGMENT, "RasterPersistenceSampler", *m_offscreenTarget[m_updateBuffer]->getTexture2D(), s_pointSampler));

			// Restore the color and depth targets to use the back buffer.
			PHYRE_TRY(renderer.setColorTarget(NULL));
			PHYRE_TRY(renderer.setDepthStencilTarget(NULL));

			return PE_RESULT_NO_ERROR;
		}

		// Description:
		// Attempts to reload a raster script from the host PC.
		// NOTE: This can result in the program exiting if the edit made the program malformed.
		// Arguments:
		// raster : The raster whose script is to be reloaded.
		// mediaPath : The path to the script.
		// Return Value List:
		// PE_RESULT_NO_ERROR : The operation was successful.
		// PE_RESULT_NULL_POINTER_ARGUMENT : The operation failed because the raster or mediaPath argument was NULL.
		// PE_RESULT_OBJECT_NOT_FOUND : The operation failed because a required component could not be found.
		// Other : An error from a sub method.
		PResult RasterComponent::ReloadScriptForRaster(RasterComponent *raster, const PChar *mediaPath)
		{
			if (!raster)
				return PE_RESULT_NULL_POINTER_ARGUMENT;

			// The entity we belong to.
			PEntity *entity = raster->getEntity();

			if (!entity)
			{
				PHYRE_PRINTF("Failed to reload script.\n");
				return PE_RESULT_NULL_POINTER_ARGUMENT;
			}

			// Get the class descriptor for the info components we want to find in the cluster.
			const PClassDescriptor *rasterComponentCD = PNamespace::GetGlobalNamespace().getClassDescriptor(PD_RASTER_INFO_COMPONENT_NAME);

			// Return if not found.
			if (rasterComponentCD == NULL)
				return PE_RESULT_OBJECT_NOT_FOUND;

			// Get the scripted component
			PScriptedComponent *sc = (PScriptedComponent *)entity->getComponentOfTypeOrDerived(*rasterComponentCD);

			// Check it's present.
			if (!sc)
				return PE_RESULT_OBJECT_NOT_FOUND;

			// Try to reload.
			PHYRE_TRY(TryReloadScriptedComponentScript(sc,mediaPath));

			return PE_RESULT_NO_ERROR;
		}

		// Description:
		// A helper function to find and create rasters in a world.
		// Arguments:
		// world : The world to populate the rasters in.
		// rasters : An array of pointers to which the created rasters are added.
		// Returns:
		// The number of rasters found and created.
		PUInt32 RasterComponent::createRastersInWorld(Phyre::PWorld& world, Phyre::PArray<RasterComponent *>& rasters)
		{
			// Get the class descriptor for the info components we want to find in the cluster.
			rasters.resize(0);

			const PClassDescriptor *rasterComponentCD = PNamespace::GetGlobalNamespace().getClassDescriptor(PD_RASTER_INFO_COMPONENT_NAME);
			if (rasterComponentCD)
			{
				// Find and setup the raster objects
				for (PWorld::PObjectIteratorOfType<PComponent> it(world) ; it ; ++it)
				{
					PComponent *rasterInfo = &*it;
					const PClassDescriptor *cd = &rasterInfo->getComponentType();

					// Find RasterComponentInfos.
					if (cd == rasterComponentCD)
					{
						// The entity we belong to.
						PEntity *entity = rasterInfo->getEntity();
						if (entity)
						{
							// Add a raster component to the object.
							RasterComponent *rc = PHYRE_NEW RasterComponent;
							if (rc)
							{
								// Get the mesh to use.
								PInstancesComponent *ic = entity->getComponentOfType<PInstancesComponent>();
								PMeshInstance *rasterMesh = ic ? ic->getTypedInstance<PMeshInstance>() : NULL;

								// Need a mesh to continue.
								if (rasterMesh)
								{
									// Create the raster.
									PResult result = rc->initialize(rasterMesh,rasterInfo);
									if (result == PE_RESULT_NO_ERROR)
									{
										// Add to the component.
										entity->addComponent(*rc);

										// Add to our list of rasters.
										rasters.add(rc);
									}
								}
								else
								{
									delete rc;
								}
							}
						}
					}
				}
			}

			// Return the count
			return rasters.getCount();
		}

	} //! End of PCommon Namespace
} //! End of Phyre Namespace
