/* Copyright (c) Microsoft Corporation.
   Licensed under the MIT License. */

/***************************************************************************

    tatr.cpp: Theater class

    Primary Author: ******
    Review Status: REVIEWED - any changes to this file must be reviewed!

    A TATR (theater) is similar to a Studio, but has no UI and is used for
    playback only.

***************************************************************************/
#include "studio.h"
ASSERTNAME

RTCLASS(TATR)

const long kcmhlTheater = 0x10000;

BEGIN_CMD_MAP(TATR, CommandHandler)
ON_CID_GEN(cidTheaterLoad, &TATR::FCmdLoad, pvNil)
ON_CID_GEN(cidTheaterPlay, &TATR::FCmdPlay, pvNil)
ON_CID_GEN(cidTheaterStop, &TATR::FCmdStop, pvNil)
ON_CID_GEN(cidTheaterRewind, &TATR::FCmdRewind, pvNil)
END_CMD_MAP_NIL()

/***************************************************************************
    Create a new TATR
***************************************************************************/
PTATR TATR::PtatrNew(long kidParent)
{
    PTATR ptatr;

    ptatr = NewObj TATR(HidUnique());
    if (pvNil == ptatr)
        return pvNil;
    if (!ptatr->_FInit(kidParent))
    {
        ReleasePpo(&ptatr);
        return pvNil;
    }
    AssertPo(ptatr, 0);
    return ptatr;
}

/***************************************************************************
    Initialize the TATR
***************************************************************************/
bool TATR::_FInit(long kidParent)
{
    AssertBaseThis(0);

    _kidParent = kidParent;

    if (!vpcex->FAddCmh(this, kcmhlTheater))
        return fFalse;

    return fTrue;
}

/***************************************************************************
    Clean up and delete this theater
***************************************************************************/
TATR::~TATR(void)
{
    AssertBaseThis(0);
    ReleasePpo(&_pmvie);
#ifdef BUG1907
    vptagm->ClearCache(sidNil, ftagmFile); // Clear content out of HD cache
                                           // Note: could clear out the RAM cache too, but I'm keeping this change
                                           // as small as possible.
#endif                                     // BUG1907
}

/***************************************************************************
    Load a new movie into the theater
***************************************************************************/
bool TATR::FCmdLoad(PCommand pcmd)
{
    AssertThis(0);
    AssertVarMem(pcmd);

    PMovieClientCallbacks pmcc;
    Filename fni;
    PMovie pmvie = pvNil;
    PGraphicsObject pgob;
    GraphicsObjectBlock gcb;

    pmcc = NewObj MovieClientCallbacks(kdxpWorkspace, kdypWorkspace, kcbStudioCache);
    if (pvNil == pmcc)
        goto LFail;

    vpapp->GetPortfolioDoc(&fni);
    if (fni.Ftg() != kftg3mm)
    {
        Bug("Portfolio's Filename has bad FileType in theater");
        goto LFail;
    }

    vpappb->BeginLongOp();
#ifdef BUG1907
    // Close up the previous movie, if any
    if (_pmvie != pvNil)
    {
        if (!_pmvie->PmvuCur()->FCloseDoc(fFalse))
            goto LFail;
        ReleasePpo(&_pmvie);
    }
    vptagm->ClearCache(sidNil, ftagmFile); // Clear content out of HD cache
                                           // Note: could clear out the RAM cache too, but I'm keeping this change
                                           // as small as possible.
#endif                                     // BUG1907
    pmvie = Movie::PmvieNew(vpapp->FSlowCPU(), pmcc, &fni, cnoNil);
    if (pmvie == pvNil)
        goto LFail;
    ReleasePpo(&pmcc);

    // Create a new MovieView (with PddgNew()) as a child GraphicsObject of _kidParent.
    // Make it invisible until we get a play command
    pgob = vpapp->Pkwa()->PgobFromHid(_kidParent);
    if (pvNil == pgob)
    {
        Bug("Couldn't find gob for TATR's kidParent");
        goto LFail;
    }
    gcb.Set(HidUnique(), pgob);
    if (pmvie->PddgNew(&gcb) == pvNil)
        goto LFail;

    if (pmvie->Cscen() > 0)
    {
        if (!pmvie->FSwitchScen(0))
            goto LFail;
    }

#ifndef BUG1907
    // Close up the previous movie, if any
    if (_pmvie != pvNil)
    {
        if (!_pmvie->PmvuCur()->FCloseDoc(fFalse))
            goto LFail;
        ReleasePpo(&_pmvie);
    }
#endif //! BUG1907
    _pmvie = pmvie;

    vpcex->EnqueueCid(cidTheaterLoadCompleted, pvNil, pvNil, fTrue);
    return fTrue;
LFail:
    ReleasePpo(&pmcc);
    ReleasePpo(&pmvie);
    vpcex->EnqueueCid(cidTheaterLoadCompleted, pvNil, pvNil, fFalse);
    return fTrue;
}

/***************************************************************************
    Play the current movie.  Also makes the MovieView visible, if it was hidden.
***************************************************************************/
bool TATR::FCmdPlay(PCommand pcmd)
{
    AssertThis(ftatrMvie); // make sure we have a movie
    AssertVarMem(pcmd);

    PMovieView pmvu;
    RC rcAbs;
    RC rcRel;

    if (pvNil == _pmvie) // Prevent crash if _pmvie is pvNil (although it
        return fTrue;    // shouldn't ever be if this function is called)

    pmvu = _pmvie->PmvuCur();
    pmvu->GetPos(&rcAbs, &rcRel);
    rcRel.Set(0, 0, krelOne, krelOne);
    pmvu->SetPos(&rcAbs, &rcRel);

    if (!_pmvie->FPlaying())
        _pmvie->Play();

    return fTrue;
}

/***************************************************************************
    Stop the current movie
***************************************************************************/
bool TATR::FCmdStop(PCommand pcmd)
{
    AssertThis(ftatrMvie); // make sure we have a movie
    AssertVarMem(pcmd);

    if (pvNil == _pmvie) // Prevent crash if _pmvie is pvNil (although it
        return fTrue;    // shouldn't ever be if this function is called)

    if (_pmvie->FPlaying())
        _pmvie->Play(); // Play() stops the movie if it's currently playing

    return fTrue;
}

/***************************************************************************
    Rewind the current movie
***************************************************************************/
bool TATR::FCmdRewind(PCommand pcmd)
{
    AssertThis(ftatrMvie); // make sure we have a movie
    AssertVarMem(pcmd);

    if (pvNil == _pmvie) // Prevent crash if _pmvie is pvNil (although it
        return fTrue;    // shouldn't ever be if this function is called)

    if (_pmvie->Cscen() > 0)
    {
        StopAllMovieSounds();
        if (_pmvie->FSwitchScen(0))
            _pmvie->Pscen()->FGotoFrm(_pmvie->Pscen()->NfrmFirst());
    }
    return fTrue;
}

#ifdef DEBUG
/***************************************************************************
    Assert the validity of the TATR.
***************************************************************************/
void TATR::AssertValid(ulong grf)
{
    TATR_PAR::AssertValid(fobjAllocated);
    if (grf & ftatrMvie)
        AssertPo(_pmvie, 0);
}

/***************************************************************************
    Mark memory used by the TATR
***************************************************************************/
void TATR::MarkMem(void)
{
    AssertThis(0);
    TATR_PAR::MarkMem();
    MarkMemObj(_pmvie);
}
#endif // DEBUG
