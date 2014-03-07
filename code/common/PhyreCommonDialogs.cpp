/* SCE CONFIDENTIAL
PhyreEngine(TM) Package 3.7.0.0
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <Phyre.h>
#include <Rendering/PhyreRendering.h>
#include "PhyreCommonDialogs.h"

#ifdef PHYRE_PLATFORM_PSP2

#include <message_dialog.h>

#endif //! PHYRE_PLATFORM_PSP2

#ifdef PHYRE_PLATFORM_PS3

#include <sysutil/sysutil_msgdialog.h>
PHYRE_PRAGMA_COMMENT_LIB(sysutil_stub)

#endif //! PHYRE_PLATFORM_PS3

#ifdef PHYRE_SAMPLES_COMMON_DIALOGS_SUPPORTED

using namespace Phyre;
using namespace PRendering;

namespace Phyre
{
	namespace PCommon
	{

		//
		// PDialogBase implementation
		//

		// Description:
		// The constructor for the PDialogBase class.
		PDialogBase::PDialogBase()
			: m_buttonPressed(PE_DIALOG_BUTTON_NONE)
			, m_closed(true)
		{
		}

		// Description:
		// The default render implementation for a dialog.
		// Renders the dialog immediately and flips buffers using a render interface.
		// Note:
		// Since this method uses the render interface directly, any renderers should flushed and synchronized before calling this method.
		// This method should not be used while rendering any other content since it clears the screen and only renders the dialog.
		void PDialogBase::render()
		{
			PRenderInterfaceLock lock;
			PRenderInterface &renderInterface = lock.getRenderInterface();
			renderInterface.setClearColor((PUInt32)0);
			renderInterface.beginScene(PRenderInterfaceBase::PE_CLEAR_COLOR_BUFFER_BIT | PRenderInterfaceBase::PE_CLEAR_DEPTH_BUFFER_BIT | PRenderInterfaceBase::PE_CLEAR_STENCIL_BUFFER_BIT);
			renderInterface.endScene();
			renderInterface.flip();
		}

#ifdef PHYRE_PLATFORM_PS3

		//
		// PDialogPS3 implementation
		//

		// Description:
		// The callback to be used by system when a button is pressed.
		// Arguments:
		// buttonType - The button type pressed by the user. From CellMsgDialogButtonType.
		// userdata - The application-defined data specified when the callback function was registered
		void PDialogPS3::ButtonCallback(int buttonType, void *userdata)
		{
			PDialogPS3 *dialogBasePS3 = static_cast<PDialogPS3 *>(userdata);
			switch(buttonType)
			{
			default: // Fall through
			case CELL_MSGDIALOG_BUTTON_NONE:	dialogBasePS3->m_buttonPressed = PE_DIALOG_BUTTON_NONE;		break;
			case CELL_MSGDIALOG_BUTTON_INVALID:	dialogBasePS3->m_buttonPressed = PE_DIALOG_BUTTON_INVALID;	break;
			case CELL_MSGDIALOG_BUTTON_OK:		dialogBasePS3->m_buttonPressed = PE_DIALOG_BUTTON_OKYES;	break;
			//case CELL_MSGDIALOG_BUTTON_YES:	dialogBasePS3->m_buttonPressed = PE_DIALOG_BUTTON_OKYES;	break; // Same as CELL_MSGDIALOG_BUTTON_OK
			case CELL_MSGDIALOG_BUTTON_NO:		dialogBasePS3->m_buttonPressed = PE_DIALOG_BUTTON_NO;		break;
			case CELL_MSGDIALOG_BUTTON_ESCAPE:	dialogBasePS3->m_buttonPressed = PE_DIALOG_BUTTON_ESCAPE;	break;
			};
			dialogBasePS3->m_closed = true;
		}

		// Description:
		// Initializes this dialog for a specific type and message.
		// Arguments:
		// message : The message to display with the dialog.
		// type : The type of dialog to open.
		// Return Value List:
		// PE_RESULT_UNKNOWN_ERROR : A platform specific error occurred.
		// PE_RESULT_NO_ERROR: This dialog was initialized successfully.
		PResult PDialogPS3::initialize(const PChar *message, PDialogType type)
		{
			m_closed = false;
			PUInt32 typeFlags;
			switch(type)
			{
			default: // Fall through
			case PE_DIALOG_OK:		typeFlags = CELL_MSGDIALOG_TYPE_BUTTON_TYPE_OK;		break;
			case PE_DIALOG_YESNO:	typeFlags = CELL_MSGDIALOG_TYPE_BUTTON_TYPE_YESNO;	break;
			};
			PInt32 res = cellMsgDialogOpen2(
				typeFlags
				| CELL_MSGDIALOG_TYPE_SE_TYPE_NORMAL
				| CELL_MSGDIALOG_TYPE_DISABLE_CANCEL_ON,
				message,
				ButtonCallback,
				this,
				NULL);
			if (res != 0)
				return PHYRE_SET_LAST_ERROR(PE_RESULT_UNKNOWN_ERROR, "Error from cellMsgDialogOpen2 = 0x%08x", res);
			return PE_RESULT_NO_ERROR;
		}

		// Description:
		// Terminates this dialog.
		// Arguments:
		// waitForExit : A flag to indicate that this method should wait for the dialog to close.
		// Return Value List:
		// PE_RESULT_UNKNOWN_ERROR : A platform specific error occurred.
		// PE_RESULT_NO_ERROR: This dialog was terminated successfully.
		PResult PDialogPS3::terminate(bool waitForExit)
		{
			if(!m_closed)
			{
				PInt32 ret = cellMsgDialogClose(0.0f);
				if(ret != 0)
					return PHYRE_SET_LAST_ERROR(PE_RESULT_UNKNOWN_ERROR, "Error from cellMsgDialogClose = 0x%08x", ret);
				if(waitForExit)
				{
					while(!m_closed)
						render();
				}
			}
			return PE_RESULT_NO_ERROR;
		}

		// Description:
		// The PlayStation(R)3 render implementation for a dialog.
		// Also calls cellSysutilCheckCallback() to ensure that the dialog is rendered.
		void PDialogPS3::render()
		{
			cellSysutilCheckCallback();
			PDialogBase::render();
		}

		//
		// PProgressBarDialogPS3 implementation
		//

		// Description:
		// The constructor for the PProgressBarDialogPS3 class.
		PProgressBarDialogPS3::PProgressBarDialogPS3()
			: m_previousValue(0)
		{
		}

		// Description:
		// Initializes this progress dialog with a specific message.
		// Arguments:
		// message : The message to display with the progress dialog.
		// Return Value List:
		// PE_RESULT_UNKNOWN_ERROR : A platform specific error occurred.
		// PE_RESULT_NO_ERROR: This dialog was initialized successfully.
		PResult PProgressBarDialogPS3::initialize(const PChar *message)
		{
			m_closed = false;
			PInt32 res = cellMsgDialogOpen2(
				CELL_MSGDIALOG_TYPE_SE_TYPE_NORMAL
				| CELL_MSGDIALOG_TYPE_BUTTON_TYPE_NONE
				| CELL_MSGDIALOG_TYPE_DISABLE_CANCEL_ON
				| CELL_MSGDIALOG_TYPE_DEFAULT_CURSOR_NONE
				| CELL_MSGDIALOG_TYPE_PROGRESSBAR_SINGLE,
				message ? message : "Please wait.",
				ButtonCallback,
				static_cast<PDialogPS3 *>(this),
				NULL);

			if (res != 0)
				return PHYRE_SET_LAST_ERROR(PE_RESULT_UNKNOWN_ERROR, "Error from cellMsgDialogOpen2 = 0x%08x", res);
			return PE_RESULT_NO_ERROR;
		}

		// Description:
		// Updates the percentage shown.
		// Arguments:
		// percent : The percentage to display.
		void PProgressBarDialogPS3::update(PUInt32 percent)
		{
			if(percent > m_previousValue)
			{
				cellMsgDialogProgressBarInc(CELL_MSGDIALOG_PROGRESSBAR_INDEX_SINGLE, percent - m_previousValue);
				m_previousValue = percent;
			}
		}

#endif //! PHYRE_PLATFORM_PS3

#ifdef PHYRE_PLATFORM_PSP2

		//
		// PDialogPSP2 implementation
		//

		// Description:
		// Initializes this dialog for a specific type and message.
		// Arguments:
		// message : The message to display with the dialog.
		// type : The type of dialog to open.
		// Return Value List:
		// PE_RESULT_UNKNOWN_ERROR : A platform specific error occurred.
		// PE_RESULT_NO_ERROR: This dialog was initialized successfully.
		PResult PDialogPSP2::initialize(const PChar *message, PDialogType type)
		{
			m_closed = false;
			SceMsgDialogButtonType buttonType;
			switch(type)
			{
			default: // Fall through
			case PE_DIALOG_OK:		buttonType = SCE_MSG_DIALOG_BUTTON_TYPE_OK;		break;
			case PE_DIALOG_YESNO:	buttonType = SCE_MSG_DIALOG_BUTTON_TYPE_YESNO;	break;
			};

			SceMsgDialogParam				msgParam;
			SceMsgDialogUserMessageParam	userMsgParam;
			sceMsgDialogParamInit(&msgParam);
			msgParam.mode = SCE_MSG_DIALOG_MODE_USER_MSG;
			memset(&userMsgParam, 0, sizeof(SceMsgDialogUserMessageParam));
			msgParam.userMsgParam = &userMsgParam;
			msgParam.userMsgParam->msg = reinterpret_cast<const SceChar8 *>(message);
			msgParam.userMsgParam->buttonType = buttonType;

			msgParam.commonParam.infobarParam = NULL;
			msgParam.commonParam.dimmerColor = NULL;
			msgParam.commonParam.bgColor = NULL;

			PInt32 res = sceMsgDialogInit(&msgParam);
			if (res != 0)
				return PHYRE_SET_LAST_ERROR(PE_RESULT_UNKNOWN_ERROR, "Error from sceMsgDialogInit = 0x%08x", res);
			return PE_RESULT_NO_ERROR;
		}

		// Description:
		// Terminates this dialog.
		// Arguments:
		// waitForExit : A flag to indicate that this method should wait for the dialog to close.
		// Return Value List:
		// PE_RESULT_UNKNOWN_ERROR : A platform specific error occurred.
		// PE_RESULT_NO_ERROR: This dialog was terminated successfully.
		PResult PDialogPSP2::terminate(bool waitForExit)
		{
			SceCommonDialogStatus dialogStatus = sceMsgDialogGetStatus();
			if(dialogStatus == SCE_COMMON_DIALOG_STATUS_RUNNING)
			{
				SceInt32 ret = sceMsgDialogClose();
				if(ret != 0)
					return PHYRE_SET_LAST_ERROR(PE_RESULT_UNKNOWN_ERROR, "Error from sceMsgDialogClose = 0x%08x", ret);
				dialogStatus = sceMsgDialogGetStatus();
			}
			if(waitForExit && dialogStatus == SCE_COMMON_DIALOG_STATUS_RUNNING)
			{
				while(sceMsgDialogGetStatus() != SCE_COMMON_DIALOG_STATUS_FINISHED)
					render();
				SceInt32 ret = sceMsgDialogTerm();
				if(ret != 0)
					return PHYRE_SET_LAST_ERROR(PE_RESULT_UNKNOWN_ERROR, "Error from sceMsgDialogTerm = 0x%08x", ret);
				render(); // Clear the screen
			}
			return PE_RESULT_NO_ERROR;
		}

		// Description:
		// The PlayStation(R)Vita render implementation for a dialog.
		// Also calls sceMsgDialogGetResult() to check the status of the dialog.
		void PDialogPSP2::render()
		{
			if(sceMsgDialogGetStatus() == SCE_COMMON_DIALOG_STATUS_FINISHED)
			{
				// Get message dialog result
				SceMsgDialogResult   s_commonDialogResult;
				memset(&s_commonDialogResult, 0, sizeof(SceMsgDialogResult));
				PInt32 res = sceMsgDialogGetResult(&s_commonDialogResult);
				if (res != SCE_OK)
				{
					(void)PHYRE_SET_LAST_ERROR(PE_RESULT_UNKNOWN_ERROR, "sceMsgDialogGetResult = 0x%08x", res);
				}
				else
				{
					switch(s_commonDialogResult.buttonId)
					{
					default: // Fall through
					case SCE_MSG_DIALOG_BUTTON_ID_INVALID:	m_buttonPressed = PE_DIALOG_BUTTON_NONE;	break;
					case SCE_MSG_DIALOG_BUTTON_ID_OK:		m_buttonPressed = PE_DIALOG_BUTTON_OKYES;	break;
					// case SCE_MSG_DIALOG_BUTTON_ID_YES:	m_buttonPressed = PE_DIALOG_BUTTON_OKYES;	break; // Same as SCE_MSG_DIALOG_BUTTON_ID_OK
					case SCE_MSG_DIALOG_BUTTON_ID_NO:		m_buttonPressed = PE_DIALOG_BUTTON_NO;		break;
					};
				}
				m_closed = true;
			}
			PDialogBase::render();
		}

		//
		// PProgressBarDialogPSP2 implementation
		//

		// Description:
		// Initializes this progress dialog with a specific message.
		// Arguments:
		// message : The message to display with the progress dialog.
		// Return Value List:
		// PE_RESULT_UNKNOWN_ERROR : A platform specific error occurred.
		// PE_RESULT_NO_ERROR: This dialog was initialized successfully.
		PResult PProgressBarDialogPSP2::initialize(const PChar *message)
		{
			m_closed = false;

			SceMsgDialogProgressBarParam dialogProgressBarDialogParam;
			memset(&dialogProgressBarDialogParam, 0, sizeof(SceMsgDialogProgressBarParam));
			dialogProgressBarDialogParam.barType = SCE_MSG_DIALOG_PROGRESSBAR_TYPE_PERCENTAGE;
			if(message)
				dialogProgressBarDialogParam.msg = reinterpret_cast<const SceChar8 *>(message);
			else
				dialogProgressBarDialogParam.sysMsgParam.sysMsgType = SCE_MSG_DIALOG_SYSMSG_TYPE_WAIT;

			SceMsgDialogParam msgParam;
			sceMsgDialogParamInit(&msgParam);
			msgParam.mode = SCE_MSG_DIALOG_MODE_PROGRESS_BAR;
			msgParam.progBarParam = &dialogProgressBarDialogParam;
			msgParam.commonParam.infobarParam = NULL;
			msgParam.commonParam.dimmerColor = NULL;
			msgParam.commonParam.bgColor = NULL;

			SceInt32 res = sceMsgDialogInit(&msgParam);
			if (res != SCE_OK)
				return PHYRE_SET_LAST_ERROR(PE_RESULT_UNKNOWN_ERROR, "Error from sceMsgDialogInit = 0x%08x", res);
			return PE_RESULT_NO_ERROR;
		}

		// Description:
		// Updates the percentage shown.
		// Arguments:
		// percent : The percentage to display.
		void PProgressBarDialogPSP2::update(PUInt32 percent)
		{
			sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, percent);
		}

#endif //! PHYRE_PLATFORM_PSP2

		//
		// PProgressBarDialog implementation
		//

		// Description:
		// The constructor for the PProgressBarDialog class.
		PProgressBarDialog::PProgressBarDialog()
			: PHYRE_PLATFORM_IMPLEMENTATION(PProgressBarDialog)()
		{
		}

		// Description:
		// Initializes this progress dialog with a specific message.
		// Arguments:
		// message : The message to display with the progress dialog.
		// Return Value List:
		// PE_RESULT_UNKNOWN_ERROR : A platform specific error occurred.
		// PE_RESULT_NO_ERROR: This dialog was initialized successfully.
		PResult PProgressBarDialog::initialize(const PChar *message)
		{
			return PHYRE_PLATFORM_IMPLEMENTATION(PProgressBarDialog)::initialize(message);
		}

		// Description:
		// Renders the dialog.
		void PProgressBarDialog::render()
		{
			PHYRE_PLATFORM_IMPLEMENTATION(PProgressBarDialog)::render();
		}

		// Description:
		// Updates the percentage shown.
		// Arguments:
		// percent : The percentage to display.
		void PProgressBarDialog::update(PUInt32 percent)
		{
			PHYRE_PLATFORM_IMPLEMENTATION(PProgressBarDialog)::update(percent);
		}

		// Description:
		// Terminates this progress dialog.
		// Arguments:
		// waitForExit : A flag to indicate that this method should wait for the dialog to close.
		// Return Value List:
		// PE_RESULT_UNKNOWN_ERROR : A platform specific error occurred.
		// PE_RESULT_NO_ERROR: This dialog was terminated successfully.
		PResult PProgressBarDialog::terminate(bool waitForExit)
		{
			return PHYRE_PLATFORM_IMPLEMENTATION(PProgressBarDialog)::terminate(waitForExit);
		}

		//
		// PDialog implementation
		//

		// Description:
		// Shows a dialog.
		// Arguments:
		// message : The message to display with the dialog.
		// type : The type of dialog to open.
		// button : Optional. Output for the button that was pressed.
		// Return Value List:
		// PE_RESULT_UNKNOWN_ERROR : A platform specific error occurred.
		// PE_RESULT_NO_ERROR: This dialog was shown successfully.
		// Note:
		// Since this method uses the render() function, any renderers should flushed and synchronized before calling this method.
		PResult PDialog::ShowDialog(const PChar *message, PDialogType type, PButtonType *button)
		{
			PDialog dialog;
			PHYRE_TRY(dialog.initialize(message, type));
			while(!dialog.m_closed)
				dialog.render();
			PHYRE_TRY(dialog.terminate(true));
			if(button)
				*button = dialog.m_buttonPressed;
			return PE_RESULT_NO_ERROR;
		}

	} //! End of PCommon Namespace
} //! End of Phyre Namespace

#endif //! PHYRE_SAMPLES_COMMON_DIALOGS_SUPPORTED
