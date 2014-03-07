/* SCE CONFIDENTIAL
PhyreEngine(TM) Package 3.7.0.0
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <Phyre.h>
#include <Rendering/PhyreRendering.h>
#include <Scene/PhyreScene.h>
#include <Animation/PhyreAnimation.h>

using namespace Vectormath::Aos;

namespace Phyre
{
	namespace PCommon
	{
		//!
		//! Render bounds
		//!

		// Description:
		// Renders the bounds for mesh instances in a cluster.
		// Arguments:
		// cluster : The cluster containing bounds for mesh instances to be rendered.
		// camera : The camera to be used for rendering.
		// renderer : The renderer to be used for rendering.
		void RenderClusterBounds(const PCluster &cluster, const PCamera &camera, PRendering::PRenderer &renderer)
		{
			for(PCluster::PConstObjectIteratorOfType<PRendering::PMeshInstanceBounds> boundsIt(cluster); boundsIt; ++boundsIt)
				boundsIt->debugDrawBounds(renderer, camera);
		}

		//!
		//! Render animation hierarchy using the mesh instance
		//!

		// Description:
		// The PHierarchyRenderData class stores information for rendering the hierarchy in a PMeshInstance object.
		class PMeshInstanceHierarchyRenderData
		{
			PHYRE_PRIVATE_ASSIGNMENT_OPERATOR(PMeshInstanceHierarchyRenderData);
		public:
			Matrix4							m_viewProjection;			// The matrix to be used to render the hierarchy.
			const PRendering::PMeshInstance	&m_meshInstance;			// The mesh instance hierarchy to be rendered.

			// Description:
			// The constructor for the PMeshInstanceHierarchyRenderData class.
			// Gets the view projection matrix from the camera and sets the mesh instance to be rendered.
			// Arguments:
			// meshInstance : The mesh instance hierarchy to be rendered.
			// camera : The camera to be used to render the mesh instance hierarchy.
			PMeshInstanceHierarchyRenderData(const PRendering::PMeshInstance &meshInstance, const PCamera &camera)
				: m_viewProjection(camera.getViewProjectionMatrix())
				, m_meshInstance(meshInstance)
			{
			}
		};

		// Description:
		// A callback method to render a mesh instance hierarchy using the debug draw API.
		// Arguments:
		// data :  The render callback data.
		void RenderMeshInstanceHierarchyCallback(const PRendering::PRenderCallbackData &data)
		{
			PRendering::PRenderInterface		&renderInterface		= data.m_renderInterface;
			PMeshInstanceHierarchyRenderData	*hierarchyRenderData	= static_cast<PMeshInstanceHierarchyRenderData *>(data.m_userData);
			const Matrix4						&viewProjection			= hierarchyRenderData->m_viewProjection;
			const PRendering::PMeshInstance		&meshInstance			= hierarchyRenderData->m_meshInstance;

			// Collapse the matrix hierarchy for the segments in the mesh instance.
			const PGeometry::PMesh	&mesh					= meshInstance.getMesh();
			PUInt32					matrixCount				= mesh.getHierarchyMatrixCount();
			if(!matrixCount)
				return;

			const PInt32		*parentIndices	= mesh.getMatrixParentIndices();
			PArray<PMatrix4>	globalMatricesArray(matrixCount +1);
			PMatrix4			*globalMatrices	= globalMatricesArray.getArray();
			if(!globalMatrices)
				return;
			globalMatrices++;
			meshInstance.buildGlobalMeshInstanceMatrixHierarchy(globalMatrices);

			Vector4 *points = (Vector4 *)alloca(matrixCount * 2 * sizeof(Vector4));
			Vector4 *point = points;
			for (PUInt32 matrixIndex = 0; matrixIndex < matrixCount; matrixIndex++, parentIndices++)
			{
				PInt32 parent = *parentIndices;
				if(parent >= 0)
				{
					*point++ = Vector4(globalMatrices[parent].getTranslation(), 1.0f);
					*point++ = Vector4(globalMatrices[matrixIndex].getTranslation(), 1.0f);
				}
			}

			Vector4 color(1.0f, 1.0f, 1.0f, 1.0f);
			renderInterface.debugDraw(PGeometry::PE_PRIMITIVE_LINES, points, color, NULL, (PUInt32) (point - points), &viewProjection);
		}

		// Description:
		// Adds a callback to render the hierarchy of the specified mesh instance.
		// Arguments:
		// meshInstance : The mesh instance to render.
		// camera :    The camera.
		// renderer :  The renderer to add this render call to.
		void RenderMeshInstanceHierarchy(const PRendering::PMeshInstance &meshInstance, const PCamera &camera, PRendering::PRenderer &renderer)
		{
			void *userData = NULL;
			renderer.addRenderCallbackWithAllocatedBuffer(RenderMeshInstanceHierarchyCallback, sizeof(PMeshInstanceHierarchyRenderData), userData);
			if(userData)
				new(userData) PMeshInstanceHierarchyRenderData(meshInstance, camera);
		}

		// Description:
		// A callback method to render a mesh instance bounds hierarchy using the debug draw API.
		// Arguments:
		// data :  The render callback data.
		void RenderMeshInstanceBoundsHierarchyCallback(const PRendering::PRenderCallbackData &data)
		{
			using namespace PGeometry;

			PRendering::PRenderInterface		&renderInterface		= data.m_renderInterface;
			PMeshInstanceHierarchyRenderData	*hierarchyRenderData	= static_cast<PMeshInstanceHierarchyRenderData *>(data.m_userData);
			const Matrix4						&viewProjection			= hierarchyRenderData->m_viewProjection;
			const PRendering::PMeshInstance		&meshInstance			= hierarchyRenderData->m_meshInstance;

			// Collapse the matrix hierarchy for the segments in the mesh instance.
			const PMesh					&mesh		= meshInstance.getMesh();
			const PSkeletonJointBounds *bounds		= mesh.getSkeletonBounds();
			PUInt32						matrixCount	= mesh.getHierarchyMatrixCount();
			PUInt32						jointCount	= mesh.getSkeletonMatrixCount();
			if(!matrixCount || !bounds)
				return;

			PArray<PMatrix4>	globalMatricesArray(matrixCount +1);
			PMatrix4			*globalMatrices = globalMatricesArray.getArray();
			if(!globalMatrices)
				return;
			globalMatrices++;
			meshInstance.buildGlobalMeshInstanceMatrixHierarchy(globalMatrices);

			PUInt32			indexCount	= jointCount * 24;
			PArray<Vector4> pointsArray(jointCount * 8);
			PArray<PUInt32> indicesArray(indexCount);
			Vector4 		*points		= pointsArray.getArray();
			PUInt32 		*indices	= indicesArray.getArray();
			if(!points || !indices)
				return;

			// Build the indices for each box
			static PUInt32 s_boxIndices[24] = {
				0, 4,	0, 2,	0, 1,	7, 3,	7, 5,	7, 6,
				4, 6,	4, 5,	2, 6,	2, 3,	1, 5,	1, 3 };
			PUInt32 jointIndexBase = 0;
			for (PUInt32 jointIndex = 0; jointIndex < jointCount; jointIndex++, jointIndexBase += 8)
				for(PUInt32 i = 0 ; i < 24 ; i++)
					*indices++ = jointIndexBase + s_boxIndices[i];

			// Create the points
			// Use the segments since they map skeleton to hierarchy
			for (PUInt32 jointIndex = 0; jointIndex < jointCount; jointIndex++, bounds++)
			{
				const PMatrix4	&globalMatrixForJoint	= globalMatrices[bounds->getHierarchyMatrixIndex()];
				Point3			boundsPoints[8];
				PRendering::PMeshInstanceBounds::GetBoundsPoints(bounds->getMin(), bounds->getMin() + bounds->getSize(), boundsPoints);
				for(PUInt32 j = 0 ; j < 8 ; j++)
					*points++ = Vector4(globalMatrixForJoint * boundsPoints[j]);
			}

			Vector4 color(0.0f, 1.0f, 0.0f, 1.0f);
			renderInterface.debugDraw(PGeometry::PE_PRIMITIVE_LINES, pointsArray.getArray(), color, indicesArray.getArray(), indexCount, &viewProjection);
		}

		// Description:
		// Adds a callback to render the bounds of the hierarchy of the specified mesh instance.
		// Arguments:
		// meshInstance : The mesh instance to render.
		// camera :    The camera.
		// renderer :  The renderer to add the bounds to.
		void RenderMeshInstanceBoundsHierarchy(const PRendering::PMeshInstance &meshInstance, const PCamera &camera, PRendering::PRenderer &renderer)
		{
			void *userData = NULL;
			renderer.addRenderCallbackWithAllocatedBuffer(RenderMeshInstanceBoundsHierarchyCallback, sizeof(PMeshInstanceHierarchyRenderData), userData);
			if(userData)
				new(userData) PMeshInstanceHierarchyRenderData(meshInstance, camera);
		}

		//!
		//! Render animation hierarchy using the node
		//!

		// Description:
		// The PHierarchyRenderData class stores information for rendering the hierarchy in a PNode object.
		class PNodeHierarchyRenderData
		{
			PHYRE_PRIVATE_ASSIGNMENT_OPERATOR(PNodeHierarchyRenderData);
		public:
			Matrix4		m_viewProjection;	// The matrix to be used to render the hierarchy.
			PScene::PNode					&m_node;			// The node to render from.
			const PAnimation::PAnimationSet &m_animationSet;	// The animation set.

			// Description:
			// The constructor for the PNodeHierarchyRenderData class.
			// Gets the view projection matrix from the camera and sets the mesh instance to be rendered.
			// Arguments:
			// node : The node hierarchy to be rendered.
			// camera : The camera to be used to render the mesh instance hierarchy.
			// animationSet : The animation set.
			PNodeHierarchyRenderData(PScene::PNode &node, const PCamera &camera, const PAnimation::PAnimationSet &animationSet)
				: m_viewProjection(camera.getViewProjectionMatrix())
				, m_node(node)
				, m_animationSet(animationSet)
			{
			}
		};

		// Description:
		// A callback method to render a node hierarchy using the debug draw API.
		// Arguments:
		// data :  The render callback data.
		void RenderNodeHierarchyCallback(const PRendering::PRenderCallbackData &data)
		{
			PRendering::PRenderInterface	&renderInterface		= data.m_renderInterface;
			PNodeHierarchyRenderData		*hierarchyRenderData	= static_cast<PNodeHierarchyRenderData *>(data.m_userData);
			const Matrix4	&viewProjection			= hierarchyRenderData->m_viewProjection;
			PScene::PNode					&node					= hierarchyRenderData->m_node;
			const PAnimation::PAnimationSet	&animationSet			= hierarchyRenderData->m_animationSet;

			PUInt32 targetCount = animationSet.getTargetCount();
			if (targetCount)
			{
				const PAnimation::PAnimationChannelTarget *targets = animationSet.getTarget(0);
				if (!targets)
					return;

				// Find all the nicknamed nodes in one traversal
				const PChar **nicknames = (const PChar **)alloca(targetCount * sizeof(PChar *));

				// Capture names for PNodes.
				PUInt32 i;
				for (i=0; i<targetCount; i++)
				{
					const PAnimation::PAnimationChannelTarget &target = targets[i];
					const PClassDescriptor &cd = target.getTargetObjectType();
					const PChar *nickname = NULL;
					if ((target.getTargetType() == PAnimation::PAnimationChannelTarget::PE_TARGETTYPE_NAMED_OBJECT) && (cd.isTypeOf(PHYRE_CLASS(PScene::PNode))))
						nickname = targets[i].getTargetName();
					nicknames[i] = nickname;
				}

				PScene::PNode **nodes = (PScene::PNode **)alloca(targetCount * sizeof(PScene::PNode *));
				memset(nodes, 0, sizeof(nodes[0]) * targetCount);

				PAnimation::PVisitorFindNicknamedNodes findNicknamedNodes(node);
				findNicknamedNodes.findNodes(nicknames, nodes, targetCount);

				Vector4 color(1.0f);
				Vector4 points[2];
				for (i=0; i<targetCount; i++)
				{
					if (nodes[i])
					{
						PMatrix4 wm	= nodes[i]->calculateWorldMatrix();
						points[0] = Vector4(wm.getTranslation(), 1.0f);
						for(PScene::PNode *child = nodes[i]->getFirstChild() ; child ; child = child->getNextSibling())
						{
							PMatrix4 wmc = child->calculateWorldMatrix();
							points[1] = Vector4(wmc.getTranslation(), 1.0f);
							renderInterface.debugDraw(PGeometry::PE_PRIMITIVE_LINES, points, color, NULL, 2, &viewProjection);
						}
					}
				}
			}
		}

		// Description:
		// Adds a callback to render the hierarchy of the specified node.
		// Arguments:
		// node : The node to render.
		// camera :    The camera.
		// animationSet : The animation set.
		// renderer :  The renderer to add this bound box to.
		void RenderNodeHierarchy(PScene::PNode &node, const PCamera &camera, const PAnimation::PAnimationSet &animationSet, PRendering::PRenderer &renderer)
		{
			void *userData = NULL;
			renderer.addRenderCallbackWithAllocatedBuffer(RenderNodeHierarchyCallback, sizeof(PNodeHierarchyRenderData), userData);
			if(userData)
				new(userData) PNodeHierarchyRenderData(node, camera, animationSet);
		}

	} //! End of PCommon Namespace
} //! End of Phyre Namespace
