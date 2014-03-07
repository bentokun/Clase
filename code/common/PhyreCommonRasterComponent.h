/* SCE CONFIDENTIAL
PhyreEngine(TM) Package 3.7.0.0
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef PHYRE_SAMPLES_COMMON_RASTER_COMPONENT_H
#define PHYRE_SAMPLES_COMMON_RASTER_COMPONENT_H

namespace Phyre
{
	namespace PCommon
	{
		// Description:
		// The name of the component holding the parameters needed to initialize the
		// raster. This doesn't need to be the same as the scripted component.
		#define PD_RASTER_INFO_COMPONENT_NAME						"ScriptedRaster"

		// Description:
		// Only meshes with a single segment are currently handled.
		#define PD_RASTER_MESH_SEGMENT								0

		// Description:
		// The segment for the offscreen mesh.
		#define PD_RASTER_OFFSCREEN_TEXTURE_MESH_SEGMENT			0

		// Description:
		// The sampler to replace on the raster mesh.
		#define PD_RASTER_MESH_SAMPLER								"TextureSampler"

		// Description:
		// The number of bits per word when a raster is packed.
		#define PD_RASTER_BITS_PER_WORD								32U

		// Description:
		// The number of bytes per word when raster is packed.
		#define PD_RASTER_BYTES_PER_WORD							(PD_RASTER_BITS_PER_WORD/4U)

		// Description:
		// Represents a texture that can be updated via the
		// scripting interface.
		class RasterComponent : public PComponent
		{
			PHYRE_BIND_DECLARE_CLASS(RasterComponent, PComponent);

			PUInt32										m_width;				// The width of the raster.
			PUInt32										m_height;				// The height of the raster.
			float										m_decay;				// The raster 'decay'. This is exp(-age * decay) where age is in frames. Please see raster_decay.cgfx for more details.
			PInt32										m_scrollStepX;			// The scroll step in the x direction.
			PInt32										m_scrollStepY;			// The scroll step in the y direction.
			PInt32										m_scrollOffsetX;		// The scroll offset in the x direction.
			PInt32										m_scrollOffsetY;		// The scroll offset in the y direction.
			bool										m_packedRaster;			// A flag that specifies whether the raster should be packed.
			Vectormath::Aos::Vector3					m_dotColor;				// The color applied to the raster dot.
			PRendering::PMeshInstance					*m_updateMeshInstance;	// The offscreen mesh instance for rendering raster 'decay'.
			PRendering::PTexture2D						m_rasterTexture;		// The dynamically created texture used to hold the raster image.
			PRendering::PTexture2D						*m_rasterScreenTexture;	// The texture of the raster screen.
			PRendering::PTexture2D						*m_rasterDotTexture;	// The texture to be used for the raster dot.
			PRendering::PMeshInstance					*m_rasterMeshInstance;	// The mesh instance that represents the raster.
			PUInt32										m_updateBuffer;			// The current buffer for updates.
			PCluster									*m_offscreenCluster;	// The cluster holding the offscreen surfaces.
			PGeometry::PMaterialSet						m_materialSet;			// The material set of the raster.
			PGeometry::PMaterialSet						*m_oldMaterialSet;		// The material set originally on the raster.
			PRendering::PRenderTarget					*m_offscreenTarget[2];	// These buffers hold the 'age' of a raster element in frames. When a raster
																				// element is updated, the value is set to zero. On subsequent frames the value
																				// is incremented (see raster_decay.cgfx).  Because the texture cannot be read
																				// and written in the same shader, two buffers are used in a ping-pong fashion.
																				// Please note that this is not a very efficient use of video memory.
			void										*m_raster;				// The buffer holding the raster.
			static PRendering::PSamplerState			s_bilinearSampler;		// The bilinear texture sampler.
			static PRendering::PSamplerState			s_pointSampler;			// The point texture sampler.
			static PMapDynamic<PString, PRendering::PTexture2D *> s_dotTextures;	// A map of different raster dot textures.

			PResult										initOffscreenSurfaces( PCluster *offscreenCluster);
			PResult										initOffscreenMesh(PCluster *offscreenCluster);
			PResult										applyRasterMaterialToMesh(PRendering::PMeshInstance *meshInstance, PRendering::PMaterial *newMaterial, PRendering::PTexture2D *dotTexture, PUInt32 segment=0);
			PResult										makeRasterMaterial(PRendering::PMaterial *newMaterial, PRendering::PTexture2D *dotTexture);
			inline PUInt32								getPlotAddress(PUInt32 x, PUInt32 y);

			static PResult								FindAndRegisterDotTexture(const PChar *assetPath, PCluster *cluster);
			static PResult								RegisterDotTexture(const PChar *name, PRendering::PTexture2D *texture);
			static PRendering::PTexture2D*				GetRasterDotTextureByName(const PChar *name);

		public:
														RasterComponent();

			static PResult								Terminate();
			static PResult								Initialize();

			static PResult								ReloadScriptForRaster(RasterComponent *raster, const PChar *mediaPath);
			static PUInt32								createRastersInWorld(PWorld& world, PArray<RasterComponent *>& rasters);

			static inline PGameplay::PScriptedComponent	*GetMutableScriptedComponentForRaster(const RasterComponent *raster);
			static inline const PGameplay::PScriptedComponent *GetScriptedComponentForRaster(const RasterComponent *raster);

			PResult										initialize(PRendering::PMeshInstance *rasterMeshInstance,PUInt32 width, PUInt32 height,PRendering::PTexture2D *dotTexture=NULL);
			PResult										initialize(PRendering::PMeshInstance *rasterMeshInstance, PComponent *component);

			PResult										initOffscreenBuffers(PRendering::PRenderer &renderer);
			PResult										updateSurfaces(PRendering::PRenderer &renderer);
			PResult										uninitialize();

			void										startRasterUpdate();
			PResult										endRasterUpdate();

			inline PUInt32								getWidth() const;
			inline PUInt32								getHeight() const;
			inline float								getDecay() const;
			inline PInt32								getScrollStepX() const;
			inline PInt32								getScrollStepY() const;

			inline PResult								setDecay(float decay);
			inline PResult								setRasterDotColor(Vectormath::Aos::Vector3 dotColor);
			inline PResult								setRasterDotColorRGB(float red, float green, float blue);
			inline PResult								setRasterDotTexture(PRendering::PTexture2D *rasterDotTexture);
			inline void									setScrollStepXY(PInt32 x=0, PInt32 y=0);
			inline void									resetScroll();
			inline PResult								setDotTextureByName(const PChar *name);

			inline bool									read(PUInt32 x, PUInt32 y);
			inline void									set(PUInt32 x, PUInt32 y);
			inline void									reset(PUInt32 x, PUInt32 y);
			void										clear();

			inline PRendering::PTexture2D				*getOffscreenTarget(PUInt32 buffer);
			inline PRendering::PTexture2D				*getRasterDotTexture();
			inline PRendering::PTexture2D				*getRasterTexture();
		};

	} //! End of PCommon Namespace
} //! End of Phyre Namespace

#endif //! PHYRE_SAMPLES_COMMON_RASTER_COMPONENT_H
