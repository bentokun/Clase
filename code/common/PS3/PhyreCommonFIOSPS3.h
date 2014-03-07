/* SCE CONFIDENTIAL
PhyreEngine(TM) Package 3.7.0.0
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef PHYRE_SAMPLES_COMMON_FIOS_PS3_H
#define PHYRE_SAMPLES_COMMON_FIOS_PS3_H

namespace cell
{
	namespace fios
	{
		//! Forward declarations:
		class overlay;
	} //! End of fios namespace.
} //! End of cell namespace.

namespace Phyre
{
	namespace PSamplesCommon
	{
		// Description:
		// Contains information about archives mounted with FIOS on PlayStation(R)3.
		class PFIOSArchivePS3
		{
		public:
			PFIOSArchivePS3();
			cell::fios::filehandle *m_fileHandle; // The FIOS file handle.
		};

		// Description:
		// Provides an interface for initializing and terminating FIOS on PlayStation(R)3.
		class PFIOSInterfacePS3
		{
		public:
			static PResult Initialize(const PFIOSInterfaceParams &fiosInterfaceParams);
			static PResult MountArchive(const PChar *name, PFIOSArchive &archive);
			static PResult UnmountArchive(PFIOSArchive &archive);
			static void Terminate();

			static cell::fios::overlay *GetPS3Overlay();
		};

	} //! End of PSamplesCommon Namespace
} //! End of Phyre Namespace

#endif //! PHYRE_SAMPLES_COMMON_FIOS_PS3_H
