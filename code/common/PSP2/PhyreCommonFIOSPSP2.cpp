/* SCE CONFIDENTIAL
PhyreEngine(TM) Package 3.7.0.0
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <fios2.h>

using namespace Phyre;

namespace Phyre
{
	namespace PSamplesCommon
	{
		namespace PInternal
		{
			// Description:
			// The FIOS2 default maximum path. This is set to 1024, but games can normally use a much smaller value.
			#define PD_MAX_PATH_LENGTH_FIOS2 256

			// Buffers for FIOS2 initialization.
			// These are the typical values that a game might use, but they should be adjusted as needed.
			// They are of type int64_t to avoid alignment issues.
			//
			// Description:
			// The storage for FIOS2 operations.
			static int64_t s_fios2OpStorage[SCE_FIOS_OP_STORAGE_SIZE(64, PD_MAX_PATH_LENGTH_FIOS2) / sizeof(int64_t) + 1]; 

			// Description:
			// The storage for FIOS2 chunks: 2048 chunks, 64KiB.
			static int64_t s_fios2ChunkStorage[SCE_FIOS_CHUNK_STORAGE_SIZE(2048) / sizeof(int64_t) + 1];

			// Description:
			// The storage for FIOS2 file handles.
			static int64_t s_fios2FileHandleStorage[SCE_FIOS_FH_STORAGE_SIZE(16, PD_MAX_PATH_LENGTH_FIOS2) / sizeof(int64_t) + 1]; 

			// Description:
			// The storage for FIOS2 directory handles.
			static int64_t s_fios2DirectoryHandleStorage[SCE_FIOS_DH_STORAGE_SIZE(4, PD_MAX_PATH_LENGTH_FIOS2) / sizeof(int64_t) + 1];

			// Description:
			// The FIOS2 dearchiver context.
			static SceFiosPsarcDearchiverContext s_fios2DearchiverContext = SCE_FIOS_PSARC_DEARCHIVER_CONTEXT_INITIALIZER;

			// Description:
			// The FIOS2 dearchiver work buffer.
			static char s_fios2DearchiverWorkBuffer[SCE_FIOS_PSARC_DEARCHIVER_WORK_BUFFER_SIZE] __attribute__((aligned(SCE_FIOS_PSARC_DEARCHIVER_WORK_BUFFER_ALIGNMENT)));

			// Description:
			// The index value used for the FIOS2 archive media layer.
			static int s_fios2ArchiveIndex = 0;

		} //! End of PInternal namespace

		//
		// PFIOSArchivePSP2 implementation
		//

		// Description:
		// The constructor for the PFIOSArchivePSP2 class.
		PFIOSArchivePSP2::PFIOSArchivePSP2()
			: m_fileHandle(0)
		{
		}

		//
		// PFIOSInterfacePSP2 implementation
		//

		// Description:
		// Initializes FIOS based on the specified parameters.
		// Arguments:
		// fiosInterfaceParams : The parameters to be used for initializing FIOS.
		// Return Value List:
		// PE_RESULT_UNKNOWN_ERROR : A FIOS error occurred.
		// PE_RESULT_NO_ERROR : FIOS initialized successfully.
		PResult PFIOSInterfacePSP2::Initialize(const PFIOSInterfaceParams &fiosInterfaceParams)
		{
			using namespace PInternal;

			(void)&fiosInterfaceParams;
			SceFiosParams params = SCE_FIOS_PARAMS_INITIALIZER;
			params.opStorage.pPtr			= s_fios2OpStorage;
			params.opStorage.length			= sizeof(s_fios2OpStorage);
			params.chunkStorage.pPtr		= s_fios2ChunkStorage;
			params.chunkStorage.length		= sizeof(s_fios2ChunkStorage);
			params.fhStorage.pPtr			= s_fios2FileHandleStorage;
			params.fhStorage.length			= sizeof(s_fios2FileHandleStorage);
			params.dhStorage.pPtr			= s_fios2DirectoryHandleStorage;
			params.dhStorage.length			= sizeof(s_fios2DirectoryHandleStorage);
			params.pathMax					= PD_MAX_PATH_LENGTH_FIOS2;

			SceInt32 err = sceFiosInitialize(&params);
			if(err != SCE_FIOS_OK) 
				return PHYRE_SET_LAST_ERROR(PE_RESULT_UNKNOWN_ERROR, "Error from sceFiosInitialize() 0x%08x", err);

			// Add a dearchiver I/O filter. Both the context struct and the work buffer must last for the lifetime of the dearchiver.
			s_fios2DearchiverContext.workBufferSize	= sizeof(s_fios2DearchiverWorkBuffer);
			s_fios2DearchiverContext.pWorkBuffer		= s_fios2DearchiverWorkBuffer;
			err = sceFiosIOFilterAdd(s_fios2ArchiveIndex, sceFiosIOFilterPsarcDearchiver, &s_fios2DearchiverContext);
			if(err != SCE_FIOS_OK) 
				return PHYRE_SET_LAST_ERROR(PE_RESULT_UNKNOWN_ERROR, "Error from sceFiosIOFilterAdd() 0x%08x", err);

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
		PResult PFIOSInterfacePSP2::MountArchive(const PChar *name, PFIOSArchive &archive)
		{
			if(archive.m_fileHandle)
				return PHYRE_SET_LAST_ERROR(PE_RESULT_OBJECT_OF_SAME_NAME_EXISTS, "The archive has already been mounted");

			const PChar *streamPrefix = PSerialization::PStreamFile::GetStreamPrefix();
			SceFiosSize result = -1;
			const PChar *filenameUsed = name;
			if(streamPrefix)
			{
				size_t fullLength = strlen(name) + strlen(streamPrefix) + 1;
				PChar *fullname = (PChar *)alloca(fullLength);
				PHYRE_SNPRINTF(fullname, fullLength, "%s%s", streamPrefix, name);
				result = sceFiosArchiveGetMountBufferSizeSync(NULL, fullname, NULL);
				if(result >= 0)
					filenameUsed = fullname;
			}

			// Find out how much memory is needed to mount the archive.
			if(result < 0)
			{
				result = sceFiosArchiveGetMountBufferSizeSync(NULL, name, NULL);
				if (result < 0)
					return PHYRE_SET_LAST_ERROR(PE_RESULT_UNKNOWN_ERROR, "Error from sceFiosArchiveGetMountBufferSizeSync() 0x%08x", (PInt32)result);
			}

			// Allocate a mount buffer.
			SceFiosBuffer g_MountBuffer = SCE_FIOS_BUFFER_INITIALIZER;
			g_MountBuffer.length = (size_t)result;
			g_MountBuffer.pPtr = PHYRE_ALLOCATE(PUInt8, g_MountBuffer.length);
			if(!g_MountBuffer.pPtr)
				return PHYRE_SET_LAST_ERROR(PE_RESULT_OUT_OF_MEMORY, "Unable to allocate %d bytes for mount buffer", g_MountBuffer.length);

			// Mount the archive.
			SceInt32 err = sceFiosArchiveMountSync(NULL, &archive.m_fileHandle, filenameUsed, PSerialization::PStreamFilePSP2::GetFIOS2ArchiveMountPoint(), g_MountBuffer, NULL);
			if(err != SCE_FIOS_OK)
				return PHYRE_SET_LAST_ERROR(PE_RESULT_UNKNOWN_ERROR, "Error from sceFiosArchiveMountSync() 0x%08x", err);

			SCE_DBG_ASSERT(sceFiosIsValidHandle(archive.m_fileHandle));
			return PE_RESULT_NO_ERROR;
		}

		// Description:
		// Unmounts the specified archive.
		// Arguments:
		// archive : The archive object that stores FIOS information.
		// Return Value List:
		// PE_RESULT_UNINITIALIZED_DATA : The archive is not mounted.
		// PE_RESULT_UNKNOWN_ERROR : A FIOS error occurred.
		// PE_RESULT_NO_ERROR : Archive unmounted successfully.
		PResult PFIOSInterfacePSP2::UnmountArchive(PFIOSArchive &archive)
		{
			SceFiosFH fileHandle = archive.m_fileHandle;
			if(!fileHandle)
				return PHYRE_SET_LAST_ERROR(PE_RESULT_UNINITIALIZED_DATA, "The archive has not been mounted");
			SceInt32 err = sceFiosArchiveUnmountSync(NULL, fileHandle);
			archive.m_fileHandle = 0;
			if(err != SCE_FIOS_OK)
				return PHYRE_SET_LAST_ERROR(PE_RESULT_UNKNOWN_ERROR, "Error from closeFileSync() 0x%08x", err);
			return PE_RESULT_NO_ERROR;
		}

		// Description:
		// Terminates the use of FIOS.
		void PFIOSInterfacePSP2::Terminate()
		{
			sceFiosTerminate();
		}

	} //! End of PSamplesCommon Namespace
} //! End of Phyre Namespace
