/* SCE CONFIDENTIAL
PhyreEngine(TM) Package 3.7.0.0
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef PHYRE_SAMPLES_COMMON_FIOS_PSP2_H
#define PHYRE_SAMPLES_COMMON_FIOS_PSP2_H

namespace Phyre
{
	namespace PSamplesCommon
	{
		// Description:
		// Contains information about archives mounted with FIOS on PlayStation(R)Vita.
		class PFIOSArchivePSP2
		{
		public:
			PFIOSArchivePSP2();
			SceFiosFH m_fileHandle;	// The FIOS file handle.
		};

		// Description:
		// Provides an interface for initializing and terminating FIOS on PlayStation(R)Vita.
		class PFIOSInterfacePSP2
		{
		public:
			static PResult Initialize(const PFIOSInterfaceParams &fiosInterfaceParams);
			static PResult MountArchive(const PChar *name, PFIOSArchive &archive);
			static PResult UnmountArchive(PFIOSArchive &archive);
			static void Terminate();
		};

	} //! End of PSamplesCommon Namespace
} //! End of Phyre Namespace

#endif //! PHYRE_SAMPLES_COMMON_FIOS_PSP2_H
