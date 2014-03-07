/* SCE CONFIDENTIAL
PhyreEngine(TM) Package 3.7.0.0
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef PHYRE_SAMPLES_COMMON_LEVEL_H
#define PHYRE_SAMPLES_COMMON_LEVEL_H

#include <Level/PhyreLevel.h>

namespace Phyre
{
	namespace PCommon
	{
		// Description:
		// Calls PLevel::LoadLevel() to load a level file into a cluster, and load its dependencies into a PSharray of PClusters.
		class PLoadLevelJob : public PThreadPoolJob, public PMemoryBase
		{
			PHYRE_PRIVATE_ASSIGNMENT_OPERATOR(PLoadLevelJob);

		protected:
			PSharray<PCluster *> &m_loadedClusters;		// An array of the clusters loaded by this job.
			const PChar			*m_name;				// The name of the level to load
			const PChar			*const*m_alreadyLoaded;	// The names of the already loaded files.
			PUInt32				m_alreadyLoadedCount;	// The number of names in the m_alreadyLoaded array.
			volatile PUInt32	m_percentDone;			// The percentage of the level that has been loaded.
			volatile PResult	m_result;				// The result of the PLevel::LoadLevel() function.
			volatile bool		m_isDone;				// A flag that indicates whether loading is complete.

		public:
			// Description:
			// The constructor for the PLoadLevelJob class.
			// Arguments:
			// loadedClusters : An array to store the loaded clusters in.
			// name : The name of the level to load.
			// alreadyLoaded : The names of already loaded files.
			// alreadyLoadedCount : The number of names in the alreadyLoaded array.
			PLoadLevelJob(PSharray<PCluster *> &loadedClusters, const PChar *name, const PChar *const*alreadyLoaded, PUInt32 alreadyLoadedCount)
				: m_loadedClusters(loadedClusters)
				, m_name(name)
				, m_alreadyLoaded(alreadyLoaded)
				, m_alreadyLoadedCount(alreadyLoadedCount)
				, m_percentDone(0)
				, m_result(PE_RESULT_UNKNOWN_ERROR)
				, m_isDone(false)
			{
			}

			// Gets m_percentDone.
			PUInt32 getPercentDone() const
			{
				return m_percentDone;
			}

			// Gets m_result.
			PResult getResult() const
			{
				return m_result;
			}

			// Gets m_isDone.
			bool isDone() const
			{
				return m_isDone;
			}

			// Description:
			// The virtual destructor for the PLoadLevelJob class.
			virtual ~PLoadLevelJob()
			{
			}

			// Description:
			// Calls PLevel::LoadLevel() to load the required level.
			// Arguments:
			// threadWorkspace :      The thread workspace area. This is unused.
			// threadWorkspaceSize :  The size of the thread workspace. This is unused.
			// threadNum :			  The thread number from the thread pool. This is unused.
			virtual void doWork(PUInt8 *threadWorkspace, PUInt32 threadWorkspaceSize, PUInt32 threadNum)
			{
				(void)threadWorkspace;
				(void)threadWorkspaceSize;
				(void)threadNum;

				m_result = PLevel::LoadLevel(m_loadedClusters, m_name, m_alreadyLoaded, m_alreadyLoadedCount, m_percentDone);
				m_isDone = true;
			}
		};

	} //! End of PCommon Namespace
} //! End of Phyre Namespace

#endif //! PHYRE_SAMPLES_COMMON_LEVEL_H
