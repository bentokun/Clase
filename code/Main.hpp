
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\
 *                                                                             *
 *  Copyright © 2014+ ESNE, Estudios Superiores Internacionales SL             *
 *  All Rights Reserved                                                        *
 *                                                                             *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef MAIN_HEADER
#define MAIN_HEADER

    class Main : public PApplication
    {
    public:

        Main();

    protected:

        PResult prePhyreInit    ();
        PResult initApplication (PChar ** programArguments, PInt32 argumentCount);
        PResult initScene       ();
        PResult animate         ();
        PResult exitScene       ();
        PResult exitApplication ();
        PResult handleInputs    ();
        PResult render          ();
        PResult resize          ();

    };

#endif
