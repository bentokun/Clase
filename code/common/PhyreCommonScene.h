/* SCE CONFIDENTIAL
PhyreEngine(TM) Package 3.7.0.0
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef PHYRE_SAMPLES_COMMON_SCENE_H
#define PHYRE_SAMPLES_COMMON_SCENE_H

namespace Phyre
{
	namespace PCommon
	{
		//!
		//! Scene helper functions
		//!
		void UpdateWorldMatricesForNodes(PCluster &cluster);
		void UpdateWorldMatricesForNodes(PWorld &world);

		//!
		//! Scene context helper functions
		//!
		PUInt32 PopulateSceneContextWithLights(PRendering::PSceneContext &sceneContext, const PCluster &cluster, PUInt32 minLightCount = 0);

		//!
		//! Scene mesh creation helper functions
		//!
		PRendering::PMeshInstance *CreateFullscreenMeshInstance(PCluster &cluster);
		PRendering::PMeshInstance *CreateFullscreenMeshInstance(PCluster &cluster, PRendering::PMaterial &material);
		void TessellatePlane(PUInt32 orientation, PInt32 positive, PUInt32 tessellation, float *&vertices, float *&normals, float *&sts, const Vectormath::Aos::Vector3 &size);

		//!
		//! Shader / effect helper functions
		//!
		void ReplaceEffect(PSharray<PCluster *>& clusters, PCluster &newEffectCluster, const PRendering::PEffect &newEffect, const PRendering::PEffect &oldEffect);
		void ReplaceEffect(PRendering::PMaterial &material, PCluster &newEffectCluster, const PRendering::PEffect &newEffect, const PRendering::PEffect &oldEffect);

		//!
		//! Parameter setting helpers.
		//!
		PResult UpdateFloatParamOnMeshInstance(PRendering::PMeshInstance *meshInstance,PUInt32 segment, const char *param, float val);
		PResult UpdateTextureParamOnMeshInstance(PRendering::PMeshInstance *meshInstance,PUInt32 segment, const char *param, PRendering::PTexture2D& texture,PRendering::PSamplerState& sampler, PUInt32 bufferIndex = 0 );
		PResult UpdateVec3ParamOnMeshInstance(PRendering::PMeshInstance *meshInstance,PUInt32 segment, const char *param, const Vectormath::Aos::Vector3 val);


	} //! End of PCommon Namespace
} //! End of Phyre Namespace

#endif //! PHYRE_SAMPLES_COMMON_SCENE_H
