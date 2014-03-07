/* SCE CONFIDENTIAL
PhyreEngine(TM) Package 3.7.0.0
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <cell/fios.h>
#include <cell/fios/compression/edge/edgezlib.h>
#include <cell/fios/compression/edge/edgezlib_decompressor.h>
#include <sysutil/sysutil_syscache.h>

#ifdef _DEBUG
PHYRE_PRAGMA_COMMENT_LIB(edgezlib_dbg)
#else //! _DEBUG
PHYRE_PRAGMA_COMMENT_LIB(edgezlib)
#endif //! _DEBUG

using namespace Phyre;

namespace Phyre
{
	namespace PSamplesCommon
	{
		// Description:
		// Namespace for classes internal to the samples common namespace.
		namespace PInternal
		{
			// Description:
			// Implements the interface defined by the cell::fios::allocator class.
			class PFIOSAllocator : public cell::fios::allocator
			{
			public:
				// Description:
				// The constructor for the PFIOSAllocator class.
				PFIOSAllocator() {}

				// Description:
				// The destructor for the PFIOSAllocator class.
				virtual ~PFIOSAllocator() {}

				// Description:
				// Allocates memory.
				// Arguments:
				// size : The size of the memory.
				// flags : The attributes of the memory.
				// pFile : A file name for debug.
				// line : A line number for debug.
				// Returns:
				// A pointer to the allocated memory.
				virtual void *Allocate(uint32_t size, uint32_t flags, const char* pFile = 0, int line = 0)
				{
					(void) pFile;
					(void) line;
					return PHYRE_ALLOCATE_ALIGNED(PUInt8, size, FIOS_ALIGNMENT_FROM_MEMFLAGS(flags));
				}

				// Description:
				// Deallocates memory.
				// Arguments:
				// pMemory : A pointer to the memory.
				// flags : The attributes of the memory.
				// pFile : A file name for debug.
				// line : A line number for debug.
				virtual void Deallocate(void* pMemory, uint32_t flags, const char* pFile = 0, int line = 0)
				{
					(void) flags;
					(void) pFile;
					(void) line;
					PHYRE_FREE(pMemory);
				}

				// Description:
				// Reallocates memory.
				// Arguments:
				// pMemory : A pointer to the memory.
				// newSize : The new size of the memory
				// flags : The attributes of the memory.
				// pFile : A file name for debug.
				// line : A line number for debug.
				// Returns:
				// NULL since according to the sample "FIOS does not use Reallocate"
				virtual void* Reallocate(void* pMemory, uint32_t newSize, uint32_t flags, const char* pFile = 0, int line = 0)
				{
					(void) pMemory;
					(void) newSize;
					(void) flags;
					(void) pFile;
					(void) line;
					return NULL;
				}
			};

			// Description:
			// The default allocator to redirect memory requests to Phyre.
			static PFIOSAllocator s_allocator;

			// Description:
			// The default title ID to use if PFIOSInterfaceParams::m_titleID is not set.
			static const char* s_defaultTitleID = "PHYRETST";

			// Description:
			// The name of the directory to use as a cache for FIOS.
			static const char* s_hddCacheDirectory = "FIOSCACHE";

			static cell::fios::media			*s_appHomeMedia;		// The FIOS media access layer for host access via SYS_APP_HOME.
			static cell::fios::overlay			*s_overlay;				// The FIOS overlay
			static cell::fios::ramcache			*s_ramCache;			// The FIOS RAM cache used by the HDD cache.
			static cell::fios::media			*s_gameDataMedia;		// The FIOS media access layer for host access via game data.
			static cell::fios::dearchiver		*s_dearchiver;			// The FIOS dearchiver for accessing the contents of archives.
			static cell::fios::scheduler		*s_mainScheduler;		// The FIOS scheduler for accessing files.
			static cell::fios::scheduler		*s_gameDataScheduler;	// The FIOS scheduler for accessing files from game data.
			static cell::fios::media			*s_hddSysCacheMedia;	// The FIOS media access layer for syscache access.
			static cell::fios::scheduler		*s_hddScheduler;		// The FIOS scheduler for accessing files from the HDD.
			static cell::fios::schedulercache	*s_hddSchedulerCache;	// The FIOS scheduler cache for caching files in the HDD cache.

			static cell::fios::Compression::EdgeZlibDecompressor *s_edgeZlibDecompressor;	// The decompressor used to decompress archives loaded with FIOS.

		} //! End of PInternal namespace

		//
		// PFIOSArchivePS3 implementation
		//

		// Description:
		// The constructor for the PFIOSArchivePS3 class.
		PFIOSArchivePS3::PFIOSArchivePS3()
			: m_fileHandle(NULL)
		{
		}

		//
		// PFIOSInterfacePS3 implementation
		//

		// Description:
		// Initializes FIOS based on the specified parameters.
		// Arguments:
		// fiosInterfaceParams : The parameters to be used for initializing FIOS.
		// Return Value List:
		// PE_RESULT_INSUFFICIENT_INFORMATION : Host file access has been disabled and no game data path has been provided so no loading can occur.
		// PE_RESULT_UNKNOWN_ERROR : A FIOS error occurred.
		// PE_RESULT_OUT_OF_MEMORY : Unable to allocate memory for FIOS.
		// PE_RESULT_NO_ERROR : FIOS initialized successfully.
		PResult PFIOSInterfacePS3::Initialize(const PFIOSInterfaceParams &fiosInterfaceParams)
		{
			if(!fiosInterfaceParams.m_enableHostFileAccess
				&& !fiosInterfaceParams.m_gameDataPath)
				return PHYRE_SET_LAST_ERROR(PE_RESULT_INSUFFICIENT_INFORMATION, "Host file access has been disabled and no game data path has been provided so no loading can occur.");

			using namespace PInternal;

			cell::fios::fios_parameters parameters = FIOS_PARAMETERS_INITIALIZER;
			parameters.pAllocator = &s_allocator;
			parameters.pLargeMemcpy = 0; // Use memcpy
			parameters.pVprintf = 0;     // Use vprintf
			cell::fios::FIOSInit(&parameters);

			// Mount System Cache on /dev_hdd1.
			// System Cache is used for HDCache afterwards.
			const PChar *titleID = fiosInterfaceParams.m_titleID;
			if(!titleID)
				titleID = s_defaultTitleID;
			CellSysCacheParam sysCacheParam;
			PHYRE_STRNCPY(sysCacheParam.cacheId, titleID, strlen(titleID) + 1);
			sysCacheParam.reserved = 0;

			int result = cellSysCacheMount(&sysCacheParam);
			if(result != CELL_SYSCACHE_RET_OK_CLEARED &&
				result != CELL_SYSCACHE_RET_OK_RELAYED)
				return PHYRE_SET_LAST_ERROR(PE_RESULT_UNKNOWN_ERROR, "Can't mount syscache (error = 0x%08x)", result);

			// Create media on /app_home.
			if(fiosInterfaceParams.m_enableHostFileAccess)
			{
				s_appHomeMedia = new cell::fios::ps3media(SYS_APP_HOME);
				if(s_appHomeMedia == 0)
					return PHYRE_SET_LAST_ERROR(PE_RESULT_OUT_OF_MEMORY, "Can't allocate app home media access");
			}

			// Create Game Data on /dev_hdd0.
			// Create media on /dev_hdd0(Game Data)
			if(fiosInterfaceParams.m_gameDataPath)
			{
				s_gameDataMedia = new cell::fios::ps3media(fiosInterfaceParams.m_gameDataPath);
				if(s_gameDataMedia == 0)
					return PHYRE_SET_LAST_ERROR(PE_RESULT_OUT_OF_MEMORY, "Can't allocate game data media layer");

				cell::fios::scheduler_attr sched_attr = FIOS_SCHEDULER_ATTR_INITIALIZER;
				s_gameDataScheduler = cell::fios::scheduler::createSchedulerForMedia(s_gameDataMedia, &sched_attr);
				if(s_gameDataScheduler == 0) 
					return PHYRE_SET_LAST_ERROR(PE_RESULT_UNKNOWN_ERROR, "Can't allocate gamedata scheduler");
			}

			s_overlay = new cell::fios::overlay(s_appHomeMedia ? s_appHomeMedia : s_gameDataMedia);

			// Create HDCache on System Cache
			// If your game is multi-disc type, assign unique discID to the
			// schedulercache on each disc. The discID is used to distinguish
			// these discs.
			const PUInt64	discID            = 0x6566676869707173ULL;
			const bool		useSingleFile     = true;
			const bool		checkModification = true;
			// Create 1GB HDD cache here.
			const PInt32	numBlocks         = 1024;
			const size_t	blockSize         = 1024 * 1024;

			s_hddSysCacheMedia = new cell::fios::ps3media(sysCacheParam.getCachePath);
			if(s_hddSysCacheMedia == 0)
				return PHYRE_SET_LAST_ERROR(PE_RESULT_OUT_OF_MEMORY, "Unable to allocate media layer for HDD syscache");

			s_hddScheduler = cell::fios::scheduler::createSchedulerForMedia(s_hddSysCacheMedia);
			if(s_hddScheduler == 0)
				return PHYRE_SET_LAST_ERROR(PE_RESULT_UNKNOWN_ERROR, "Unable to create scheduler for HDD syscache");

			s_hddSchedulerCache = new cell::fios::schedulercache(s_overlay, s_hddScheduler,
				s_hddCacheDirectory,
				discID,
				useSingleFile,
				checkModification, numBlocks,
				blockSize);
			if(s_hddSchedulerCache == 0)
				return PHYRE_SET_LAST_ERROR(PE_RESULT_OUT_OF_MEMORY, "Unable to allocate schedulercache for HDD syscache");

			// Create 512KB ram cache.
			s_ramCache = new cell::fios::ramcache(s_hddSchedulerCache, 16, 32 * 1024);
			if(s_ramCache == 0)
				return PHYRE_SET_LAST_ERROR(PE_RESULT_OUT_OF_MEMORY, "Can't allocate RAM cache");

			// Create EdgeZlibDecompressor for fios::dearchiver.
			s_edgeZlibDecompressor = new cell::fios::Compression::EdgeZlibDecompressor(PhyreGetSpurs());
			if(s_edgeZlibDecompressor == 0)
				return PHYRE_SET_LAST_ERROR(PE_RESULT_OUT_OF_MEMORY, "Can't allocate Edge Zlib Decompressor");

			// Create dearchiver.
			s_dearchiver = new cell::fios::dearchiver(s_ramCache, s_edgeZlibDecompressor);
			if(s_dearchiver == 0)
				return PHYRE_SET_LAST_ERROR(PE_RESULT_OUT_OF_MEMORY, "Can't allocate dearchiver");
			
			// Create main scheduler.
			s_mainScheduler = cell::fios::scheduler::createSchedulerForMedia(s_dearchiver);
			if(s_mainScheduler == 0)
				return PHYRE_SET_LAST_ERROR(PE_RESULT_UNKNOWN_ERROR, "Can't allocate main scheduler");

			// Make mMainScheduler_ the default scheduler.
			s_mainScheduler->setDefault();
			PSerialization::PStreamFilePS3::SetFIOSScheduler(s_mainScheduler);

			return PE_RESULT_NO_ERROR;
		}

		// Description:
		// Mounts an archive with the specified name.
		// Arguments:
		// name : The name of the archive file to mount.
		// archive : The archive object to store FIOS information.
		// Return Value List:
		// PE_RESULT_OBJECT_OF_SAME_NAME_EXISTS : The archive is already mounted.
		// PE_RESULT_UNKNOWN_ERROR : A FIOS error occurred.
		// PE_RESULT_NO_ERROR : Archive mounted successfully.
		PResult PFIOSInterfacePS3::MountArchive(const PChar *name, PFIOSArchive &archive)
		{
			if(archive.m_fileHandle)
				return PHYRE_SET_LAST_ERROR(PE_RESULT_OBJECT_OF_SAME_NAME_EXISTS, "The archive has already been mounted");
			cell::fios::err_t err = PInternal::s_mainScheduler->openFileSync(NULL, name, cell::fios::kO_RDONLY, &archive.m_fileHandle);
			if(err != cell::fios::CELL_FIOS_NOERROR)
				return PHYRE_SET_LAST_ERROR(PE_RESULT_UNKNOWN_ERROR, "Error from openFileSync() 0x%08x", err);
			return PE_RESULT_NO_ERROR;
		}

		// Description:
		// Unmounts the specified archive.
		// Arguments:
		// archive : The archive object that stores FIOS information.
		// Return Value List:
		// PE_RESULT_UNINITIALIZED_DATA : The archive is not mounted.
		// PE_RESULT_INSUFFICIENT_INFORMATION : The FIOS scheduler has been terminated.
		// PE_RESULT_UNKNOWN_ERROR : A FIOS error occurred.
		// PE_RESULT_NO_ERROR : Archive unmounted successfully.
		PResult PFIOSInterfacePS3::UnmountArchive(PFIOSArchive &archive)
		{
			cell::fios::filehandle *fileHandle = archive.m_fileHandle;
			if(!fileHandle)
				return PHYRE_SET_LAST_ERROR(PE_RESULT_UNINITIALIZED_DATA, "The archive has not been mounted");
			if(!PInternal::s_mainScheduler)
				return PHYRE_SET_LAST_ERROR(PE_RESULT_INSUFFICIENT_INFORMATION, "Scheduler has been terminated before closing all archives.");
			cell::fios::err_t err = PInternal::s_mainScheduler->closeFileSync(NULL, fileHandle);
			archive.m_fileHandle = NULL;
			if(err != cell::fios::CELL_FIOS_NOERROR)
				return PHYRE_SET_LAST_ERROR(PE_RESULT_UNKNOWN_ERROR, "Error from closeFileSync() 0x%08x", err);
			return PE_RESULT_NO_ERROR;
		}

		// Description:
		// Terminates the use of FIOS.
		void PFIOSInterfacePS3::Terminate()
		{
			using namespace PInternal;

			PSerialization::PStreamFilePS3::SetFIOSScheduler(NULL);
			cell::fios::scheduler::destroyScheduler(s_mainScheduler);
			s_mainScheduler = NULL;

			delete s_dearchiver;
			delete s_edgeZlibDecompressor;
			delete s_ramCache;

			delete s_hddSchedulerCache;
			cell::fios::scheduler::destroyScheduler(s_hddScheduler);
			delete s_hddSysCacheMedia;

			delete s_overlay;

			cell::fios::scheduler::destroyScheduler(s_gameDataScheduler);
			delete s_gameDataMedia;

			delete s_appHomeMedia;

			cell::fios::FIOSTerminate();
		}

		// Description:
		// Gets a FIOS overlay that can be used on PlayStation(R)3.
		// Returns:
		// The FIOS overlay.
		cell::fios::overlay *PFIOSInterfacePS3::GetPS3Overlay()
		{
			return PInternal::s_overlay;
		}

	} //! End of PSamplesCommon Namespace
} //! End of Phyre Namespace
