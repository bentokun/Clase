/* SCE CONFIDENTIAL
PhyreEngine(TM) Package 3.7.0.0
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef PHYRE_SAMPLES_COMMON_ANIMATION_H
#define PHYRE_SAMPLES_COMMON_ANIMATION_H

#include <Animation/PhyreAnimation.h>

namespace Phyre
{
	namespace PCommon
	{
		//!
		//! Animation helper functions
		//!
		PAnimation::PAnimationNetworkInstance *CreateAnimationForClip(PCluster &cluster, PAnimation::PAnimationSet &animSet, PAnimation::PAnimationClip &clip, PArray<PAnimation::PAnimationSlotArrayElement> &animationProcessBuffer);
		PAnimation::PAnimationNetworkInstance *CreateAnimation(PCluster &cluster, PArray<PAnimation::PAnimationSlotArrayElement> &animationProcessBuffer);
		void UpdateAnimations(float elapsedTime, PAnimation::PAnimationNetworkInstance &animationNetworkInstance);

	} //! End of PCommon Namespace
} //! End of Phyre Namespace

#endif //! PHYRE_SAMPLES_COMMON_ANIMATION_H
