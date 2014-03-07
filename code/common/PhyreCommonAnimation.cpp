/* SCE CONFIDENTIAL
PhyreEngine(TM) Package 3.7.0.0
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <Phyre.h>
#include <Rendering/PhyreRendering.h>
#include <Animation/PhyreAnimation.h>
#include <Scene/PhyreScene.h>

namespace Phyre
{
	namespace PCommon
	{
		// Description:
		// Adds the binders from a PArray to a pointer array for binding.
		// Template Arguments:
		// BINDER_CLASS - The binder class.
		// Arguments:
		// binderArray : The PArray of binders to be added.
		// binders : The pointer array to be updated.
		template <class BINDER_CLASS>
		static void AddAnimationBindersToArray(PArray<BINDER_CLASS> &binderArray, PAnimation::PAnimationChannelTargetBind **&binders)
		{
			PUInt32			count	= binderArray.getCount();
			BINDER_CLASS	*binder	= binderArray.getArray();
			for(PUInt32 i = 0 ; i < count ; i++)
				*binders++ = binder++;
		}

		// Description:
		// Creates and configures an animation network instance for the specified animation clip in the specified animation set in a cluster.
		// This method attempts a general bind for every type of animation target channel in the clip.
		// Typically you should use the correct binding mechanism required by the channels of your animation.
		// Arguments:
		// cluster : The cluster in which to create the network instance.
		// animSet : The animation set for which to create the animation network instance.
		// clip : The animation clip for which to create the animation network instance.
		// animationProcessBuffer : The process buffer to be resized to include the space required for the network instance.
		// Returns:
		// The created animation network instance or a value of NULL if an error occurred.
		PAnimation::PAnimationNetworkInstance *CreateAnimationForClip(PCluster &cluster, PAnimation::PAnimationSet &animSet, PAnimation::PAnimationClip &clip, PArray<PAnimation::PAnimationSlotArrayElement> &animationProcessBuffer)
		{
			using namespace PAnimation;
			using namespace PScene;

			const PAnimationChannelTarget	*targets	= animSet.getTarget(0);
			PUInt32							targetCount	= animSet.getTargetCount();

			// Allocate binders for every type of target
			PArray<PAnimationChannelTargetBindScene>			sceneBinders;
			PArray<PAnimationChannelTargetBindDataMember>		memberBinders;
			PArray<PAnimationChannelTargetBindShaderParameter>	shaderParameterBinders;
			PArray<PAnimationChannelTargetBindFunction>			functionBinders;
			PArray<PAnimationChannelTargetBindMethod>			methodBinders;

			// Lists of class for bound members, methods and functions.
			PArray<const PClassDescriptor *>					boundSpecificMemberClasses;
			PArray<const PClassDescriptor *>					boundSpecificMethodClasses;
			PArray<const PClassDescriptor *>					boundFunctionClasses;

			const PAnimationChannelTarget	*target						= targets;
			bool							addedParameterBufferBinders	= false;
			bool							addedSceneBinders			= false;
			for(PUInt32 t = 0 ; t < targetCount ; t++, target++)
			{
				const PClassDescriptor	&targetObjectType	= target->getTargetObjectType();
				switch(target->getTargetType())
				{
				case PAnimationChannelTarget::PE_TARGETTYPE_NAMED_OBJECT:
					{
						if(&targetObjectType == &PHYRE_CLASS(PNode))
						{
							if(!addedSceneBinders)
							{
								for(PCluster::PObjectIteratorOfType<PNode>	nodeIt(cluster) ; nodeIt ; ++nodeIt)
								{
									PNode &node = *nodeIt;
									if(node.getParent())
										continue;
									PAnimationChannelTargetBindScene bindScene(node);
									sceneBinders.add(PARRAY_ALLOCSITE bindScene);
								}
								addedSceneBinders = true;
							}
						}
						break;
					}
				case PAnimationChannelTarget::PE_TARGETTYPE_NAMED_DATAMEMBER:
					{
						if (boundSpecificMemberClasses.find(&targetObjectType) < 0)
						{
							boundSpecificMemberClasses.add(&targetObjectType);

							// Try to bind to the specific member first, fallback to any.
							PAnimationChannelTargetBindDataMember bindMemberToSpecific(targetObjectType);
							memberBinders.add(PARRAY_ALLOCSITE bindMemberToSpecific);
						}

						// Specialization for skin animations
						if(&targetObjectType == &PHYRE_CLASS(PRendering::PMeshInstance))
						{
							for (PCluster::PObjectIteratorOfType<PRendering::PMeshInstance> meshInstanceIt(cluster); meshInstanceIt; ++meshInstanceIt)
							{
								PRendering::PMeshInstance &meshInstanceCandidate = *meshInstanceIt;
								if(meshInstanceCandidate.getCurrentPoseMatrices())
								{
									PAnimationChannelTargetBindDataMember bindMember(targetObjectType, meshInstanceCandidate);
									memberBinders.add(PARRAY_ALLOCSITE bindMember);
								}
							}
						}
						else
						{
							for(PCluster::PObjectIterator objIt(cluster, targetObjectType); objIt; ++objIt)
							{
								PAnimationChannelTargetBindDataMember bindMember(targetObjectType, *objIt);
								memberBinders.add(PARRAY_ALLOCSITE bindMember);
							}
						}
						break;
					}
				case PAnimationChannelTarget::PE_TARGETTYPE_NAMED_METHOD:
					{
						if (boundSpecificMethodClasses.find(&targetObjectType) < 0)
						{
							boundSpecificMethodClasses.add(&targetObjectType);

							// Try to bind to the specific member first, fallback to any.
							PAnimationChannelTargetBindMethod bindMethodToSpecific(targetObjectType);
							methodBinders.add(PARRAY_ALLOCSITE bindMethodToSpecific);
						}

						for(PCluster::PObjectIterator objIt(cluster, targetObjectType); objIt; ++objIt)
						{
							PAnimationChannelTargetBindMethod bindMethod(targetObjectType, *objIt);
							methodBinders.add(PARRAY_ALLOCSITE bindMethod);
						}
						break;
					}
				case PAnimationChannelTarget::PE_TARGETTYPE_NAMED_FUNCTION:
					{
						if (boundFunctionClasses.find(&targetObjectType) < 0)
						{
							boundFunctionClasses.add(&targetObjectType);

							PAnimationChannelTargetBindFunction bindFunction(targetObjectType);
							functionBinders.add(PARRAY_ALLOCSITE bindFunction);
						}

						break;
					}
				case PAnimationChannelTarget::PE_TARGETTYPE_NAMED_SHADER_PARAMETER:
					{
						if(!addedParameterBufferBinders)
						{
							// Add a binder to attempt to bind to the specific material first.
							PAnimationChannelTargetBindShaderParameter bindShaderParameterToSpecific;
							shaderParameterBinders.add(PARRAY_ALLOCSITE bindShaderParameterToSpecific);

							// Then fall back to adding to every material.
							for(PCluster::PObjectIteratorOfType<PRendering::PParameterBuffer> pbIt(cluster); pbIt; ++pbIt)
							{
								PAnimationChannelTargetBindShaderParameter bindShaderParameter(*pbIt);
								shaderParameterBinders.add(PARRAY_ALLOCSITE bindShaderParameter);
							}
							addedParameterBufferBinders = true;
						}
						break;
					}
				default: // PE_TARGETTYPE_NONE and PE_TARGETTYPE_COUNT
					break;
				}
			}

			// Aggregate the binders into a binder array.
			PUInt32 binderCount =
				sceneBinders.getCount() +
				memberBinders.getCount() +
				shaderParameterBinders.getCount() +
				functionBinders.getCount() +
				methodBinders.getCount();

			PAnimationChannelTargetBind **binders		= (PAnimationChannelTargetBind **)alloca(sizeof(PAnimationChannelTargetBind *) * binderCount);
			PAnimationChannelTargetBind **currentBinder	= binders;
			AddAnimationBindersToArray(sceneBinders,			currentBinder);
			AddAnimationBindersToArray(memberBinders,			currentBinder);
			AddAnimationBindersToArray(shaderParameterBinders,	currentBinder);
			AddAnimationBindersToArray(functionBinders,			currentBinder);
			AddAnimationBindersToArray(methodBinders,			currentBinder);

			// Build the animation network instance simple hierarchy
			PTimeIntervalController				*intervalController		  = cluster.create<PTimeIntervalController>(1);
			PAnimationController				*animationController	  = cluster.create<PAnimationController>(1);
			PAnimationTargetBlenderController	*targetBlenderController  = cluster.create<PAnimationTargetBlenderController>(1);
			PAnimationNetworkInstance			*animationNetworkInstance = cluster.create<PAnimationNetworkInstance>(1);

			// Set up interval controller to loop the anim.
			float start, end;
			clip.getTimeExtents(start, end);
			intervalController->setLocalBase(start);
			intervalController->setLocalRange(end-start);

			// Join 'em all together in a suitable fashion.
			animationController->setAnimationSet(animSet);
			animationController->setAnimationClip(clip);
			animationController->setTimeController(*intervalController);

			targetBlenderController->setAnimationSet(animSet);
			targetBlenderController->setSource(*animationController);

			// Bind using the allocated binders.
			animationNetworkInstance->bind(targetBlenderController, binders, binderCount);

			// Now ensure we have sufficient space to process it.
			PUInt32 slotProcessBufferRequired = animationNetworkInstance->getProcessBufferSize();

			// Resize and reassign the global process buffer if the current global process buffer is too small.
			if(PAnimationNetworkInstance::GetProcessBufferSize() < slotProcessBufferRequired)
			{
				if(animationProcessBuffer.resize(PARRAY_ALLOCSITE slotProcessBufferRequired) == PE_RESULT_NO_ERROR)
					PAnimationNetworkInstance::SetProcessBuffer(animationProcessBuffer.getArray(), slotProcessBufferRequired);
			}

			return animationNetworkInstance;
		}

		// Description:
		// Creates and configures an animation network instance for the first animation clip in the first animation set in a cluster.
		// Uses the CreateAnimationForClip() function to instantiate the animation network instance.
		// Arguments:
		// cluster : The cluster containing animation sets to be included in the network instance and in which to create the network instance.
		// animationProcessBuffer : The process buffer to be resized to include the space required for the network instance.
		// Returns:
		// The created animation network instance or a value of NULL if an error occurred.
		PAnimation::PAnimationNetworkInstance *CreateAnimation(PCluster &cluster, PArray<PAnimation::PAnimationSlotArrayElement> &animationProcessBuffer)
		{
			using namespace PAnimation;

			PCluster::PObjectIteratorOfType<PAnimationSet> animSetIt(cluster);
			if(animSetIt)
			{
				PAnimationSet &animSet = *animSetIt;
				if(animSet.getAnimationClipCount())
				{
					PAnimationClip *animClip = animSet.getAnimationClip(0);
					if(animClip)
						return CreateAnimationForClip(cluster, animSet, *animClip, animationProcessBuffer);
				}
			}
			return NULL;
		}

		// Description:
		// Updates a single animation network instance based on the current elapsed time.
		// Arguments:
		// elapsedTime : The elapsed time.
		// animationNetworkInstance : The animation network instance to update.
		void UpdateAnimations(float elapsedTime, PAnimation::PAnimationNetworkInstance &animationNetworkInstance)
		{
			using namespace PAnimation;

			PTimeController::GlobalTick(elapsedTime);		// Step the timer along.
			PAnimationNetworkInstanceProcessHelper<16>	animProcessHelper;
			animProcessHelper.processAnimationNetworkInstance(animationNetworkInstance);
		}
	} //! End of PCommon Namespace
} //! End of Phyre Namespace
