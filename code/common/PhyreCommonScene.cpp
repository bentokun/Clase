/* SCE CONFIDENTIAL
PhyreEngine(TM) Package 3.7.0.0
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <Phyre.h>
#include <Geometry/PhyreGeometry.h>
#include <Rendering/PhyreRendering.h>
#include <Scene/PhyreScene.h>


namespace Phyre
{
	namespace PCommon
	{
		// Description:
		// A templated helper function to create jobs for processing blocks of objects.
		// Template Arguments:
		// JOB_CLASS : The type of job to instantiate in order to process the objects.
		// ITERATOR_CLASS : The type of iterator.
		// Arguments:
		// objectIt : The iterator of the objects to process.
		// purpose : The purpose of the iteration.
		// maxObjectCountPerBlock : Optional. The maximum number of objects in each block. Defaults to 100.
		// Note:
		// JOB_CLASS is assumed to be derived from PProcessObjectBlockThreadPoolJob.
		template <class JOB_CLASS, class ITERATOR_CLASS>
		static void ProcessObjectBlocksFromIteratorWithJob(ITERATOR_CLASS &objectIt, const PChar *purpose, PUInt32 maxObjectCountPerBlock = 100)
		{
			PThreadPool	&tp	   = PThreadPool::GetSingleton();
			JOB_CLASS *lastJob = NULL;
			typedef typename JOB_CLASS::PObjectType PObjectType;
			while(objectIt)
			{
				PUInt32		blockSize = objectIt.getCountOfObjectsInBlock();
				PObjectType	*objects  = &*objectIt;
				PUInt32		stride	  = objectIt.getClassDescriptorForCurrentObject().getSize();
				objectIt += blockSize;

				while(blockSize)
				{
					// Populate the job
					JOB_CLASS	*job  = PRingBufferedThreadPoolJob::AllocateJobForThreadPool<JOB_CLASS>();
					PUInt32		count = PhyreMinOfPair(blockSize, maxObjectCountPerBlock);
					job->m_purpose = purpose;
					job->m_objects = objects;
					job->m_count   = count;
					job->m_stride  = stride;
					blockSize	  -= count;
					objects		   = PhyreOffsetPointerByBytes(objects, count * stride);

					tp.enqueueJob(*job);
					lastJob = job;
				}
			}

			// This mechanism takes advantage of the batching of jobs
			if(lastJob)
				tp.waitCompleted(*lastJob);
		}

		// Description:
		// Implements support for processing blocks of a specific type of object.
		// Template Arguments:
		// OBJECT_CLASS - The object.
		template <class OBJECT_CLASS>
		class PProcessObjectBlockThreadPoolJob : public PRingBufferedThreadPoolJob
		{
		protected:
			// Description:
			// The virtual destructor for the PProcessObjectBlockThreadPoolJob class.
			virtual ~PProcessObjectBlockThreadPoolJob()
			{
			}
		public:
			OBJECT_CLASS *m_objects;			// The block of objects to process.
			PUInt32		  m_count;				// The number of objects in the block.
			PUInt32		  m_stride;				// The stride, measured in bytes, between the objects in the block.

			typedef OBJECT_CLASS PObjectType;	// The type definition for the object type which is processed by this job class.
		};

		// Description:
		// Calls PScene::PNode::updateWorldMatrix() to update the matrices of a block of scene nodes.
		class PUpdateLocalMatrixJob : public PProcessObjectBlockThreadPoolJob<PScene::PNode>
		{
		public:
			// Description:
			// The virtual destructor for the PUpdateLocalMatrixJob class.
			virtual ~PUpdateLocalMatrixJob()
			{
			}

			// Description:
			// Calls PScene::PNode::updateWorldMatrix() to update the matrices of a block of scene nodes.
			// Arguments:
			// threadWorkspace :      The thread workspace area. This is unused.
			// threadWorkspaceSize :  The size of the thread workspace. This is unused.
			// threadNum :			  The thread number from the thread pool. This is unused.
			virtual void doWork(PUInt8 *threadWorkspace, PUInt32 threadWorkspaceSize, PUInt32 threadNum)
			{
				(void)threadWorkspace;
				(void)threadWorkspaceSize;
				(void)threadNum;

				PScene::PNode	*nodes = m_objects;
				PUInt32			count  = m_count;
				for(PUInt32 i = 0; i < count; i++, nodes++)
					nodes->updateWorldMatrix();
				setBlockNonBusy();
			}
		};

		// Description:
		// Updates the matrices for PScene::PNodes in a cluster using the PUpdateLocalMatrixJob class.
		// Arguments:
		// cluster : The cluster containing the nodes to be updated.
		void UpdateWorldMatricesForNodes(PCluster &cluster)
		{
			PCluster::PObjectIteratorOfType<PScene::PNode> iterator(cluster);
			ProcessObjectBlocksFromIteratorWithJob<PUpdateLocalMatrixJob>(iterator, "PUpdateLocalMatrixJob_Cluster");
		}

		// Description:
		// Updates the matrices for PScene::PNodes in a world using the PUpdateLocalMatrixJob class.
		// Arguments:
		// world : The world containing the nodes to be updated.
		void UpdateWorldMatricesForNodes(PWorld &world)
		{
			PWorld::PObjectIteratorOfType<PScene::PNode> iterator(world);
			ProcessObjectBlocksFromIteratorWithJob<PUpdateLocalMatrixJob>(iterator, "PUpdateLocalMatrixJob_World");
		}

		// Description:
		// Populates the scene context with lights from the specified cluster.
		// Arguments:
		// sceneContext - The scene context to populate.
		// cluster - The cluster containing the lights.
		// minLightCount - The minimum number of lights to support. Defaults to 0.
		// Returns:
		// The number of lights found.
		PUInt32 PopulateSceneContextWithLights(PRendering::PSceneContext &sceneContext, const PCluster &cluster, PUInt32 minLightCount /* = 0 */)
		{
			// Count the number of lights in the cluster
			PUInt32 lightCount = 0;
			for(PCluster::PConstObjectIteratorOfType<PRendering::PLight> itLight(cluster); itLight; ++itLight)
				lightCount++;

			if(lightCount)
			{
				// The scene context encapsulates information about the current scene, so add the lights from the cluster in order to light the scene
				PUInt32 maxLights = minLightCount ? PhyreMaxOfPair(lightCount, minLightCount) : lightCount;
				sceneContext.m_lights.resize(PARRAY_ALLOCSITE maxLights);
				const PRendering::PLight **light = sceneContext.m_lights.getArray();
				for(PCluster::PConstObjectIteratorOfType<PRendering::PLight> it(cluster); it; ++it)
					*light++ = &*it;
			}
			return lightCount;
		}

		// Description:
		// Creates a full screen mesh instance.
		// Arguments:
		// cluster : The cluster in which to allocate the material, mesh and mesh instance.
		// material : The material.
		// Returns:
		// A pointer to the mesh instance. NULL is returned if the operation fails.
		PRendering::PMeshInstance *CreateFullscreenMeshInstance(PCluster &cluster, PRendering::PMaterial &material)
		{
			using namespace PGeometry;
			PMesh *mesh = cluster.create<PMesh>(1);
			mesh->setMeshSegmentCount(1);

			PMeshSegment &segment = mesh->getMeshSegment(0);
			segment.setPrimitiveType(PE_PRIMITIVE_TRIANGLES);
			segment.setVertexDataCount(2);

			PDataBlock &positionDataBlock = segment.getVertexData(0);
			positionDataBlock.initialize(sizeof(float) * 3, 3, 1);

			PVertexStream &positionVertexStream = positionDataBlock.getVertexStream(0);
			positionVertexStream.initialize(PE_TYPE_FLOAT3, PHYRE_GET_RENDERTYPE(Vertex));

			PDataBlock &stDataBlock = segment.getVertexData(1);
			stDataBlock.initialize(sizeof(float)* 2, 3, 1);

			PVertexStream &stVertexStream = stDataBlock.getVertexStream(0);
			stVertexStream.initialize(PE_TYPE_FLOAT2, PHYRE_GET_RENDERTYPE(ST));

			PDataBlock::PWriteMapResult positionMap, stMap;
			positionDataBlock.map(positionMap);
			stDataBlock.map(stMap);

			float *vertices = static_cast<float *>(positionMap.m_buffer);
			float *sts = static_cast<float *>(stMap.m_buffer);

			const float y = -1.0f;
			*vertices++ = -1.0f; *vertices++ = y;		  *vertices++ = 1.0f;
			*vertices++ =  3.0f; *vertices++ = y;		  *vertices++ = 1.0f;
			*vertices++ = -1.0f; *vertices++ = y * -3.0f; *vertices++ = 1.0f;
			*sts++ = 0.0f; *sts++ = 0.0f;
			*sts++ = 2.0f; *sts++ = 0.0f;
			*sts++ = 0.0f; *sts++ = 2.0f;

			positionDataBlock.unmap(positionMap);
			stDataBlock.unmap(stMap);

			PMaterialSet *materialSet = cluster.create<PMaterialSet>(1);
			materialSet->setMaterialCount(1);

			materialSet->setMaterial(0, &material);

			PRendering::PMeshInstance *meshInstance = cluster.allocateInstanceAndConstruct<PRendering::PMeshInstance>(*mesh);
			meshInstance->setMaterialSet(materialSet);

			return meshInstance;
		}

		// Description:
		// Creates a full screen mesh instance.
		// Arguments:
		// cluster : The cluster in which to allocate the material.
		// Returns:
		// A pointer to the mesh instance. NULL is returned if the operation fails.
		PRendering::PMeshInstance *CreateFullscreenMeshInstance(PCluster &cluster)
		{
			PRendering::PMaterial *material = NULL;
			for(PCluster::PObjectIteratorOfType<PRendering::PMaterial> materialIt(cluster) ; materialIt && material == NULL; ++materialIt)
				material = &*materialIt;

			if(material)
				return CreateFullscreenMeshInstance(cluster, *material);
			else
				return NULL;
		}

		// Description:
		// Tessellates a plane.
		// Arguments:
		// orientation -	The axis that the face to be created will be oriented towards (0 - x, 1 - y, 2 - z).
		// positive -		The direction of the plane's face. Specify -1 if it is facing towards the positive axis. Specify 1 if it is facing towards the negative axis.
		// tessellation -	The tessellation level.
		// vertices -		The buffer to fill in with the generated vertex data.
		// normals -		The buffer to fill in with the generated normal data.
		// sts -			The buffer to fill in with the generated ST data.
		// size -			The dimensions of the mesh in XYZ.
		void TessellatePlane(PUInt32 orientation, PInt32 positive, PUInt32 tessellation, float *&vertices, float *&normals, float *&sts, const Vectormath::Aos::Vector3 &size)
		{
			float normal = (positive > 0) ? 1.0f : -1.0f;
			float nx = 0.0f, ny = 0.0f, nz = 0.0f;

			const float sizeX = size.getX();
			const float sizeY = size.getY();
			const float sizeZ = size.getZ();

			float planeSize = sizeX;

			switch(orientation)
			{
			default:
			case 0:	// X axis
				nx = normal;
				break;
			case 1: // Y axis
				ny = normal;
				planeSize = sizeY;
				break;
			case 2: // Z axis
				nz = normal;
				planeSize = sizeZ;
				break;
			}

			float xDivs = 2 * sizeX / tessellation;
			float yDivs = 2 * sizeY / tessellation;
			float zDivs = 2 * sizeZ / tessellation;
			float planePosition = positive * planeSize;

			for(PUInt32 i = 0; i < tessellation + 1; i++)
				for(PUInt32 j = 0; j < tessellation + 1; j++)
				{
					// Vertices
					switch(orientation)
					{
					case 0:	// X axis
						*vertices++ = planePosition;
						if(positive < 0) { *vertices++ = -sizeY + i * yDivs; *vertices++ = -sizeZ + j * zDivs; }
						else			 { *vertices++ = -sizeY + j * yDivs; *vertices++ = -sizeZ + i * zDivs; }
						break;
					case 1:	// Y axis
						if(positive < 0) { *vertices++ = -sizeX + j * xDivs; *vertices++ = planePosition; *vertices++ = -sizeZ + i * zDivs;	}
						else			 { *vertices++ = -sizeX + i * xDivs; *vertices++ = planePosition; *vertices++ = -sizeZ + j * zDivs;	}
						break;
					case 2:	// Z axis
						if(positive < 0) { *vertices++ = -sizeX + i * xDivs; *vertices++ = -sizeY + j * yDivs; }
						else			 { *vertices++ = -sizeX + j * xDivs; *vertices++ = -sizeY + i * yDivs; }
						*vertices++ = planePosition;
						break;
					}
					// Normals
					*normals++ = nx; *normals++ = ny; *normals++ = nz;
					// STs
					*sts++ = (float)i / (float)tessellation; *sts++ = (float)j / (float)tessellation;
				}
		}


		// Description:
		// Replaces an effect used by a material with a new effect. The effect variant to use during the replacement is determined by using the material
		// switches from the old effect variant. A new parameter buffer is built to match the new effect and parameter data is copied
		// where matches can be found.
		// Arguments:
		// material - The material to replace the effect on.
		// newEffectCluster - The cluster containing the new effect.
		// newEffect - The new effect.
		// oldEffect - The old effect to replace.
		void ReplaceEffect(PRendering::PMaterial &material, PCluster &newEffectCluster, const PRendering::PEffect &newEffect, const PRendering::PEffect &oldEffect)
		{
			if(&material.getEffectVariant().getEffect() == &oldEffect)
			{
				// Locate the variant on the new effect and the old effect.
				const PRendering::PEffectVariant *newVariant = newEffect.getVariant(material.getEffectVariant().m_switches);
				if(newVariant)
				{
					// Build a new parameter buffer for the new effect variant.
					PRendering::PParameterBuffer *newParameterBuffer = newVariant->createParameterBuffer(newEffectCluster);
					if(newParameterBuffer)
					{
						// Loop through all the parameters in the new parameter buffer and attempt to find them in the old parameter buffer -
						// then copy the value across if found.
						for(PUInt32 i = 0; i < newParameterBuffer->getShaderParameterDefinitions().getCount(); ++i)
						{
							const PRendering::PShaderParameterDefinition &newParamDef = newParameterBuffer->getShaderParameterDefinitions()[i];
							const PChar *parameterName = newParamDef.getName();
							bool foundParameter = false;

							const PRendering::PParameterBuffer &oldParameterBuffer = material.getParameterBuffer();
							for(PUInt32 j = 0; j < oldParameterBuffer.getShaderParameterDefinitions().getCount() && !foundParameter; ++j)
							{
								// Match parameters by name
								if(!strcmp(oldParameterBuffer.getShaderParameterDefinitions()[j].getName(), parameterName))
								{
									// Copy the old value to the new value.
									const void *oldValue = oldParameterBuffer.getParameter(oldParameterBuffer.getShaderParameterDefinitions()[j]);
									newParameterBuffer->setParameterTypeless(newParamDef, oldValue);
									foundParameter = true;
								}
							}
							if(!foundParameter)
								PHYRE_WARN("Unable to find parameter %s when replacing effect\n", parameterName);
						}

						// Replace the effect variant on the material with the new effect variant and parameter buffer.
						material.setEffectVariant(*newVariant, *newParameterBuffer);
					}
					else
					{
						PHYRE_WARN("Unable to create parameter buffer when replacing effect");
					}
				}
				else
				{
					PHYRE_WARN("Unable to find variant when replacing effect");
				}
			}
		}

		// Description:
		// Replaces an old effect with a new effect where usage of the old effect, on a material, is detected in an array of clusters.
		//
		// Arguments:
		// clusters - The clusters to replace the effect on.
		// newEffectCluster - The cluster containing the new effect.
		// newEffect - The new effect.
		// oldEffect - The old effect to replace.
		void ReplaceEffect(PSharray<Phyre::PCluster *>& clusters, PCluster &newEffectCluster, const PRendering::PEffect &newEffect, const PRendering::PEffect &oldEffect)
		{
			// Loop through all the clusters
			for(PUInt32 ci = 0; ci < clusters.getCount(); ++ci)
			{
				PCluster *cluster = clusters[ci];
				// Loop through all the materials in each cluster
				for(PCluster::PObjectIteratorOfType<PRendering::PMaterial> materialIt(*cluster) ; materialIt; ++materialIt)
				{
					// Replace any instances of the effect on the material
					PRendering::PMaterial &material = *materialIt;
					ReplaceEffect(material, newEffectCluster, newEffect, oldEffect);
				}
			}
		}

		// Description:
		// Updates a floating point parameter on the specified material of a mesh instance.
		// Arguments:
		// meshInstance : The mesh instance to update.
		// segment : The material segment to update.
		// param : The name of the floating point parameter to update.
		// val : The new floating point value to update the floating point parameter with.
		// Return Value List:
		// PE_RESULT_NO_ERROR: The floating point parameter was updated successfully.
		// PE_RESULT_OBJECT_NOT_FOUND: The operation failed because the floating point parameter could not be found.
		// PE_RESULT_NULL_POINTER_ARGUMENT: The operation failed because the meshInstance argument was NULL.
		PResult UpdateFloatParamOnMeshInstance(PRendering::PMeshInstance *meshInstance,PUInt32 segment, const char *param, float val)
		{
			if (meshInstance)
			{
				PHYRE_ASSERT(segment <= meshInstance->getMaterialSet().getMaterialCount());
				const PRendering::PEffectVariant &effectVariant = meshInstance->getMaterialSet().getMaterials()[segment]->getEffectVariant();
				const PRendering::PShaderParameterDefinition *parameterDefinition = effectVariant.findShaderParameterByName(param);
				if (parameterDefinition)
				{
					PRendering::PParameterBuffer &paramBuffer = meshInstance->getMaterialSet().getMaterials()[segment]->getParameterBuffer();
					PHYRE_TRY(paramBuffer.setParameter(*parameterDefinition, &val));
					return PE_RESULT_NO_ERROR;
				}
				else
				{
					return PE_RESULT_OBJECT_NOT_FOUND;
				}
			}
			return PE_RESULT_NULL_POINTER_ARGUMENT;
		}

		// Description:
		// Updates a texture sampler parameter on the specified material of a mesh instance.
		// Arguments:
		// meshInstance : The mesh instance to update.
		// segment : The material segment to update.
		// param : The name of the texture sampler parameter to update.
		// texture : The new texture to update the parameter with.
		// sampler : The new sampler to update the parameter with.
		// bufferIndex : The buffer instance of the texture.
		// Return Value List:
		// PE_RESULT_NO_ERROR: The texture sampler parameter was updated successfully.
		// PE_RESULT_OBJECT_NOT_FOUND: The operation failed because the texture sampler parameter could not be found.
		// PE_RESULT_NULL_POINTER_ARGUMENT: The operation failed because the meshInstance argument was NULL.
		PResult UpdateTextureParamOnMeshInstance(PRendering::PMeshInstance *meshInstance, PUInt32 segment, const char *param, PRendering::PTexture2D &texture,PRendering::PSamplerState &sampler, PUInt32 bufferIndex)
		{
			if (meshInstance)
			{
				PHYRE_ASSERT(segment <= meshInstance->getMaterialSet().getMaterialCount());

				const PRendering::PEffectVariant &effectVariant = meshInstance->getMaterialSet().getMaterials()[segment]->getEffectVariant();
				const PRendering::PShaderParameterDefinition *textureParam = effectVariant.findShaderParameterByName(param);
				if(textureParam)
				{
					PRendering::PParameterBuffer &paramBuffer = meshInstance->getMaterialSet().getMaterials()[segment]->getParameterBuffer();
					PRendering::PShaderParameterCaptureBufferTexture2D textureCapture;
					textureCapture.m_texture = &texture;
					textureCapture.m_samplerState = &sampler;
					textureCapture.m_textureBufferIndex = bufferIndex;
					PHYRE_TRY(paramBuffer.setParameter(*textureParam, &textureCapture));
					return PE_RESULT_NO_ERROR;
				}
				else
				{
					return PE_RESULT_OBJECT_NOT_FOUND;
				}
			}
			return PE_RESULT_NULL_POINTER_ARGUMENT;
		}

		// Description:
		// Updates a Vec3 shader parameter on the specified material of a mesh instance.
		// Arguments:
		// meshInstance : The mesh instance to update.
		// segment : The material segment to update.
		// param : The name of the shader parameter to update.
		// val : The new Vec3 value to update the shader parameter to.
		// Return Value List:
		// PE_RESULT_NO_ERROR: The shader parameter was updated successfully.
		// PE_RESULT_OBJECT_NOT_FOUND: The operation failed because the shader parameter could not be found.
		// PE_RESULT_NULL_POINTER_ARGUMENT: The operation failed because the meshInstance argument was NULL.
		PResult UpdateVec3ParamOnMeshInstance(PRendering::PMeshInstance *meshInstance, PUInt32 segment, const char *param, const Vectormath::Aos::Vector3 val)
		{
			if (meshInstance)
			{
				PHYRE_ASSERT(segment <= meshInstance->getMaterialSet().getMaterialCount());

				const PRendering::PEffectVariant &effectVariant = meshInstance->getMaterialSet().getMaterials()[segment]->getEffectVariant();
				const PRendering::PShaderParameterDefinition *parameterDefinition = effectVariant.findShaderParameterByName(param);
				if (parameterDefinition)
				{
					PRendering::PParameterBuffer &paramBuffer = meshInstance->getMaterialSet().getMaterials()[segment]->getParameterBuffer();
					PHYRE_TRY(paramBuffer.setParameter(*parameterDefinition, &val));
					return PE_RESULT_NO_ERROR;
				}
				else
				{
					return PE_RESULT_OBJECT_NOT_FOUND;
				}
			}
			return PE_RESULT_NULL_POINTER_ARGUMENT;
		}

	} //! End of PCommon Namespace
} //! End of Phyre Namespace
