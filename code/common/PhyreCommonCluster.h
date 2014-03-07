/* SCE CONFIDENTIAL
PhyreEngine(TM) Package 3.7.0.0
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef PHYRE_SAMPLES_COMMON_CLUSTER_H
#define PHYRE_SAMPLES_COMMON_CLUSTER_H

namespace Phyre
{
	namespace PCommon
	{

		// Description:
		// The PLoadClusterJob class calls PCluster::LoadAssetFile() to load an asset file into a cluster.
		class PLoadClusterJob : public PThreadPoolJob, public PMemoryBase
		{
			PCluster	*m_cluster;	// The cluster to load into or allocated during loading.
			const PChar	*m_name;	// The name of the cluster to load.
			PAtomicType	*m_count;	// An optional external count to decrement after loading the cluster.
			PResult		m_result;	// The result of the load operation.

		public:
			// Description:
			// The constructor for the PLoadClusterJob class.
			PLoadClusterJob()
				: m_cluster(NULL)
				, m_name(NULL)
				, m_count(NULL)
				, m_result(PE_RESULT_UNKNOWN_ERROR)
			{
			}

			// Description:
			// Sets the members required for loading the cluster.
			// Arguments:
			// cluster : The cluster to load into or allocated during loading.
			// name : The name of the cluster to load.
			// count : An optional external count to decrement after loading the cluster.
			void set(PCluster *cluster, const PChar *name, PAtomicType *count)
			{
				m_cluster	= cluster;
				m_name		= name;
				m_count		= count;
			}

			// Gets m_cluster.
			PCluster *getCluster() const
			{
				return m_cluster;
			}

			// Gets m_result.
			PResult getResult() const
			{
				return m_result;
			}

			// Description:
			// The virtual destructor for the PLoadClusterJob class.
			virtual ~PLoadClusterJob()
			{
			}

			// Description:
			// Calls PCluster::LoadAssetFile() to load the required cluster.
			// Arguments:
			// threadWorkspace :      The thread workspace area. This is unused.
			// threadWorkspaceSize :  The size of the thread workspace. This is unused.
			// threadNum :			  The thread number from the thread pool. This is unused.
			virtual void doWork(PUInt8 *threadWorkspace, PUInt32 threadWorkspaceSize, PUInt32 threadNum)
			{
				(void)threadWorkspace;
				(void)threadWorkspaceSize;
				(void)threadNum;

				if(m_name)
				{
					if(m_cluster)
						m_result = PCluster::LoadAssetFile(*m_cluster, m_name);
					else
						m_result = PCluster::LoadAssetFile(m_cluster, m_name);
					if(m_count)
					{
						PUInt32 oldValue, newValue;
						do
						{
							oldValue = *m_count;
							newValue = oldValue - 1;
						}
						while(!PhyreCompareAndSwap(m_count, oldValue, newValue));
					}
				}
			}
		};

	} //! End of PCommon Namespace
} //! End of Phyre Namespace

#endif //! PHYRE_SAMPLES_COMMON_CLUSTER_H
