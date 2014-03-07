/* SCE CONFIDENTIAL
PhyreEngine(TM) Package 3.7.0.0
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef PHYRE_SAMPLES_COMMON_DIALOGS_H
#define PHYRE_SAMPLES_COMMON_DIALOGS_H

#if defined(PHYRE_PLATFORM_PS3) || defined(PHYRE_PLATFORM_PSP2)

// Description:
// Compiler define enabled for platforms that provide their own dialog support.
#define PHYRE_SAMPLES_COMMON_DIALOGS_SUPPORTED

#endif //! defined(PHYRE_PLATFORM_PS3) || defined(PHYRE_PLATFORM_PSP2)

#ifdef PHYRE_SAMPLES_COMMON_DIALOGS_SUPPORTED

namespace Phyre
{
	namespace PCommon
	{

		// Description:
		// Represents the base class for dialogs.
		// The PDialogBase class exposes the enums used to define the interface for dialogs and the types of button that the user can select.
		class PDialogBase
		{
		public:

			// Description:
			// Defines the types of dialog that can be shown.
			enum PDialogType
			{
				PE_DIALOG_OK	= 0,	// A dialog with only an OK option.
				PE_DIALOG_YESNO	= 1,	// A dialog with a Yes and No option.
			};

			// Description:
			// Defines the types of button that the user can press.
			enum PButtonType
			{
				PE_DIALOG_BUTTON_NONE		= 0,	// No button was selected.
				PE_DIALOG_BUTTON_INVALID  	= 1,	// An invalid button was selected.
				PE_DIALOG_BUTTON_OKYES		= 2,	// The OK or Yes button was selected.
				PE_DIALOG_BUTTON_NO			= 3,	// The No button was selected.
				PE_DIALOG_BUTTON_ESCAPE  	= 4		// The Escape option was selected.
			};

		protected:

			volatile PButtonType	m_buttonPressed;	// The button that was pressed before this dialog was closed.
			volatile bool			m_closed;			// A flag indicating that this dialog is closed.

		public:

			// Description:
			// The constructor for the PDialogBase class.
			PDialogBase();
			void render();
		};

#ifdef PHYRE_PLATFORM_PS3

		// Description:
		// Represents the base class for dialogs on PlayStation(R)3.
		class PDialogPS3 : public PDialogBase
		{
		protected:

			static void ButtonCallback(int buttonType, void *userdata);
			PResult initialize(const PChar *message, PDialogType type);
			PResult terminate(bool waitForExit);

		public:

			void render();
		};

		// Description:
		// Represents the base class for progress bar dialogs on PlayStation(R)3.
		class PProgressBarDialogPS3 : public PDialogPS3
		{
			PUInt32 m_previousValue;	// The previously displayed percentage value since cellMsgDialogProgressBarInc() requires the increment.

		protected:

			PProgressBarDialogPS3();
			PResult initialize(const PChar *message);
			void update(PUInt32 percent);
		};

#endif //! PHYRE_PLATFORM_PS3

#ifdef PHYRE_PLATFORM_PSP2

		// Description:
		// Represents the base class for dialogs on PlayStation(R)Vita.
		class PDialogPSP2 : public PDialogBase
		{
		protected:
			PResult initialize(const PChar *message, PDialogType type);
			PResult terminate(bool waitForExit);
			void render();
		};

		// Description:
		// Represents the base class for progress bar dialogs on PlayStation(R)Vita.
		class PProgressBarDialogPSP2 : public PDialogPSP2
		{
		protected:
			PResult initialize(const PChar *message);
			void update(PUInt32 percent);
		};

#endif //! PHYRE_PLATFORM_PSP2

		// Description:
		// Represents the cross platform interface for progress bar dialogs on PlayStation(R)3 and PlayStation(R)Vita.
		class PProgressBarDialog : public PHYRE_PLATFORM_IMPLEMENTATION(PProgressBarDialog)
		{
		public:
			PProgressBarDialog();
			PResult initialize(const PChar *message);
			void render();
			void update(PUInt32 percent);
			PResult terminate(bool waitForExit);
		};

		// Description:
		// Represents the cross platform interface for showing a dialog on PlayStation(R)3 and PlayStation(R)Vita.
		class PDialog : public PHYRE_PLATFORM_IMPLEMENTATION(PDialog)
		{
		public:
			static PResult ShowDialog(const PChar *message, PDialogType type, PButtonType *button);
		};

	} //! End of PCommon Namespace
} //! End of Phyre Namespace

#endif //! PHYRE_SAMPLES_COMMON_DIALOGS_SUPPORTED

#endif //! PHYRE_SAMPLES_COMMON_DIALOGS_H
