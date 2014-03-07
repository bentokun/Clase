/* SCE CONFIDENTIAL
PhyreEngine(TM) Package 3.7.0.0
* Copyright (C) 2013 Sony Computer Entertainment Inc.
* All Rights Reserved.
*/

#ifndef PHYRE_SAMPLES_COMMON_SCRIPTING_H
#define PHYRE_SAMPLES_COMMON_SCRIPTING_H

namespace Phyre
{
	namespace PCommon
	{
		Phyre::PResult TryReloadScriptedComponentScript(Phyre::PGameplay::PScriptedComponent *sc, const Phyre::PChar *mediaPath);
		const Phyre::PScripting::PScript *FindScriptInClustersByName(const Phyre::PSharray<Phyre::PCluster*>& clusters, const Phyre::PChar *name);

	} //! End of PCommon Namespace
} //! End of Phyre Namespace

#endif //! PHYRE_SAMPLES_COMMON_SCRIPTING_H

