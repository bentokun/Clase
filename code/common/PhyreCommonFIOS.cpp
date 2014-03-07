/* SCE CONFIDENTIAL
PhyreEngine(TM) Package 3.7.0.0
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <Phyre.h>
#include "PhyreCommonFIOS.h"

#ifdef PHYRE_SAMPLES_COMMON_FIOS_SUPPORTED

#ifdef PHYRE_PLATFORM_PS3

#include "PS3/PhyreSamplesCommonFIOSPS3.cpp"

#endif //! PHYRE_PLATFORM_PS3

#ifdef PHYRE_PLATFORM_PSP2

#include "PSP2/PhyreSamplesCommonFIOSPSP2.cpp"

#endif //! PHYRE_PLATFORM_PSP2

using namespace Phyre;

namespace Phyre
{
	namespace PCommon
	{
		//
		// PFIOSInterfaceParams implementation
		//

		// Description:
		// The constructor for the PFIOSInterfaceParams class.
		PFIOSInterfaceParams::PFIOSInterfaceParams()
			: m_titleID(NULL)
			, m_gameDataPath(NULL)
			, m_enableHostFileAccess(true)
		{
		}

		//
		// PFIOSInterface implementation
		//

		// Description:
		// Initializes FIOS based on the specified parameters.
		// Arguments:
		// fiosInterfaceParams : The parameters to be used for initializing FIOS.
		// Returns:
		// The result of the platform implementation.
		PResult PFIOSInterface::Initialize(const PFIOSInterfaceParams &fiosInterfaceParams)
		{
			return PHYRE_PLATFORM_IMPLEMENTATION(PFIOSInterface)::Initialize(fiosInterfaceParams);
		}

		// Description:
		// Mounts an archive with the specified name.
		// Arguments:
		// name : The name of the archive file to mount.
		// archive : The archive object to store FIOS information.
		// Returns:
		// The result of the platform implementation.
		PResult PFIOSInterface::MountArchive(const PChar *name, PFIOSArchive &archive)
		{
			return PHYRE_PLATFORM_IMPLEMENTATION(PFIOSInterface)::MountArchive(name, archive);
		}

		// Description:
		// Unmounts the specified archive.
		// Arguments:
		// archive : The archive object that stores FIOS information.
		// Returns:
		// The result of the platform implementation.
		PResult PFIOSInterface::UnmountArchive(PFIOSArchive &archive)
		{
			return PHYRE_PLATFORM_IMPLEMENTATION(PFIOSInterface)::UnmountArchive(archive);
		}

		// Description:
		// Terminates the use of FIOS.
		void PFIOSInterface::Terminate()
		{
			PHYRE_PLATFORM_IMPLEMENTATION(PFIOSInterface)::Terminate();
		}

	} //! End of PCommon Namespace
} //! End of Phyre Namespace

#endif //! PHYRE_SAMPLES_COMMON_FIOS_SUPPORTED
