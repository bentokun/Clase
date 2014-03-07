/* SCE CONFIDENTIAL
PhyreEngine(TM) Package 3.7.0.0
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#include <Phyre.h>
#include <Geometry/PhyreGeometry.h>
#include <Gameplay/PhyreGameplay.h>
#include <Scripting/PhyreScripting.h>

namespace Phyre
{
	namespace PCommon
	{

		using namespace PScripting;

		//!
		//! Script helper functions
		//!

		// Description:
		// Attempts to reload a scripted component script.
		// Arguments:
		// sc - The scripted component.
		// mediaPath - The media path.
		// Return Value List:
		// PE_RESULT_OBJECT_NOT_FOUND - The scripted component argument was NULL.
		// PE_RESULT_NO_ERROR - The scripted component script was reloaded successfully.
		// Other - An error from a sub method.
		PResult TryReloadScriptedComponentScript(PGameplay::PScriptedComponent *sc, const PChar *mediaPath)
		{
			// Check it's present.
			if (!sc)
				return PE_RESULT_OBJECT_NOT_FOUND;

			// Get the suspended state
			bool wasSuspended = !sc->getIsRunning();

			// Suspend the script if it wasn't already.
			if (!wasSuspended)
				sc->suspendScript();

			// Close the old context and remove from scheduler
			PHYRE_TRY(sc->uninitializeScript());

			// Build a string to the path
			PStringBuilder path(mediaPath);

			// Load the requested script
			PHYRE_TRY(ReloadScript(path.c_str(), sc->getScript()));

			// Add back into scheduler - using the new context (overrides the default)
			PHYRE_TRY(sc->initializeScript());

			// If it was suspended put it back into the same state.
			if (wasSuspended)
				sc->suspendScript();

			// Message
			PHYRE_PRINTF("RELOADED SCRIPT: %s\n", sc->getScript()->m_sourceName.c_str());

			return PE_RESULT_NO_ERROR;
		}

		// Description:
		// Finds a script by name in an array of clusters.
		// Arguments:
		// clusters - The clusters to search in.
		// name - The name of the script.
		// Returns:
		// A pointer to the matching PScript object if one was found; otherwise NULL is returned.
		const PScript *FindScriptInClustersByName(const PSharray<PCluster*>& clusters, const PChar *name)
		{
			const size_t nameLen = strlen(name);
			for (PUInt32 l=0; l < clusters.getCount(); ++l)
			{
				PCluster& cluster = *clusters[l];
				for (PCluster::PObjectIteratorOfType<PScript> it(cluster); it; ++it)
				{
					const PScript *script= &*it;
					const PChar *path = script->m_sourceName.c_str();
					if (path)
					{
						const size_t pathLen = strlen(path);
						if (pathLen >= nameLen)
						{
							// To match the end of the path must match the name to find.
							PChar const *start = path + pathLen- nameLen;
							if (PHYRE_STRNICMP(name, start, strlen(name) - 1) == 0)
								return script;
						}
					}
				}
			}
			return (const PScript *)0;
		}

	} //! End of PCommon Namespace
} //! End of Phyre Namespace
