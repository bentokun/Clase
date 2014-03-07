
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 *                                                                             *
 *  Copyright © 2014+ ESNE, Estudios Superiores Internacionales SL             *
 *  All Rights Reserved                                                        *
 *                                                                             *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include <Phyre.h>
#include <Framework/PhyreFramework.h>

using namespace Phyre;
using namespace PFramework;

#include "Main.hpp"
#include "Assets.h"

static Main main;           // PApplication instance

Main::Main()
{
}

PResult Main::prePhyreInit ()
{
    return (PApplication::prePhyreInit());  // required
}

PResult Main::initApplication (PChar ** , PInt32 )
{
    return (PE_RESULT_NO_ERROR);
}

PResult Main::initScene ()
{
    return (PE_RESULT_NO_ERROR);
}

PResult Main::animate ()
{
    return (PE_RESULT_NO_ERROR);
}

PResult Main::exitScene ()
{
    return (PE_RESULT_NO_ERROR);
}

PResult Main::exitApplication ()
{
    return (PE_RESULT_NO_ERROR);
}

PResult Main::handleInputs ()
{
    return (PApplication::handleInputs());  // recommended
}

PResult Main::render ()
{
    return (PApplication::handleInputs());
}

PResult Main::resize ()
{
    return (PApplication::resize());    // required
}
