/* SCE CONFIDENTIAL
PhyreEngine(TM) Package 3.7.0.0
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef PHYRE_SAMPLES_COMMON_FIOS_H
#define PHYRE_SAMPLES_COMMON_FIOS_H

#if defined(PHYRE_PLATFORM_PS3) || defined(PHYRE_PLATFORM_PSP2)

// Description:
// Compiler define enabled for platforms that provide FIOS support.
#define PHYRE_SAMPLES_COMMON_FIOS_SUPPORTED

#endif //! defined(PHYRE_PLATFORM_PS3) || defined(PHYRE_PLATFORM_PSP2)

#ifdef PHYRE_SAMPLES_COMMON_FIOS_SUPPORTED

namespace Phyre
{
	namespace PCommon
	{
		// Description:
		// Contains the parameters for initializing FIOS.
		class PFIOSInterfaceParams
		{
		public:
			PFIOSInterfaceParams();
			const PChar *m_titleID;				// The title ID for the application. If unset, defaults to "PHYRETST".
			const PChar *m_gameDataPath;		// The path to game data.
			bool		m_enableHostFileAccess;	// Enables access to files on the host while debugging.
												// Defaults to true and should be false for release packages.
		};

		class PFIOSArchive;

	} //! End of PCommon Namespace
} //! End of Phyre Namespace

#ifdef PHYRE_PLATFORM_PS3
#include "PS3/PhyreSamplesCommonFIOSPS3.h"
#endif //! PHYRE_PLATFORM_PS3

#ifdef PHYRE_PLATFORM_PSP2
#include "PSP2/PhyreSamplesCommonFIOSPSP2.h"
#endif //! PHYRE_PLATFORM_PSP2

namespace Phyre
{
	namespace PCommon
	{
		// Description:
		// Contains information about archives mounted with FIOS.
		class PFIOSArchive : public PHYRE_PLATFORM_IMPLEMENTATION(PFIOSArchive)
		{
		public:
		};

		// Description:
		// Provides a common interface for initializing and terminating FIOS.
		class PFIOSInterface : public PHYRE_PLATFORM_IMPLEMENTATION(PFIOSInterface)
		{
		public:
			static PResult Initialize(const PFIOSInterfaceParams &fiosInterfaceParams);
			static PResult MountArchive(const PChar *name, PFIOSArchive &archive);
			static PResult UnmountArchive(PFIOSArchive &archive);
			static void Terminate();
		};

	} //! End of PCommon Namespace
} //! End of Phyre Namespace

#endif //! PHYRE_SAMPLES_COMMON_FIOS_SUPPORTED

#endif //! PHYRE_SAMPLES_COMMON_FIOS_H
