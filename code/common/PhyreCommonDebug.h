/* SCE CONFIDENTIAL
PhyreEngine(TM) Package 3.7.0.0
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef PHYRE_SAMPLES_COMMON_DEBUG_H
#define PHYRE_SAMPLES_COMMON_DEBUG_H

namespace Phyre
{
	namespace PScene
	{
		class PNode;
	} //! End of PScene Namespace
	namespace PAnimation
	{
		class PAnimationSet;
	} //! End of PAnimation Namespace

	namespace PCommon
	{
		//!
		//! Debug rendering helper functions
		//!
		void RenderClusterBounds(const PCluster &cluster, const PCamera &camera, PRendering::PRenderer &renderer);
		void RenderMeshInstanceHierarchy(const PRendering::PMeshInstance &meshInstance, const PCamera &camera, PRendering::PRenderer &renderer);
		void RenderMeshInstanceBoundsHierarchy(const PRendering::PMeshInstance &meshInstance, const PCamera &camera, PRendering::PRenderer &renderer);
		void RenderNodeHierarchy(PScene::PNode &node, const PCamera &camera, const PAnimation::PAnimationSet &animationSet, PRendering::PRenderer &renderer);

	} //! End of PCommon Namespace
} //! End of Phyre Namespace

#endif //! PHYRE_SAMPLES_COMMON_DEBUG_H
