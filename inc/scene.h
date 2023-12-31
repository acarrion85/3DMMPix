/* Copyright (c) Microsoft Corporation.
   Licensed under the MIT License. */

/***************************************************************************

    THIS IS A CODE REVIEWED FILE

    Basic scene classes:
        Scene (Scene)

            BASE ---> Scene

        Scene Actor Undo Object (SceneActorUndo)

            BASE ---> UndoBase ---> MovieUndo ---> SceneActorUndo

***************************************************************************/

#ifndef SCEN_H
#define SCEN_H

//
// Undo object for actor operations
//
typedef class SceneActorUndo *PSceneActorUndo;

#define SceneActorUndo_PAR MovieUndo

// Undo types
enum
{
    utAdd = 0x1,
    utDel,
    utRep,
};

#define kclsSceneActorUndo 'SUNA'
class SceneActorUndo : public SceneActorUndo_PAR
{
    RTCLASS_DEC
    MARKMEM
    ASSERT

  protected:
    PActor _pactr;
    long _ut; // Tells which type of undo this is.
    SceneActorUndo(void)
    {
    }

  public:
    static PSceneActorUndo PsunaNew(void);
    ~SceneActorUndo(void);

    void SetActr(PActor pactr)
    {
        _pactr = pactr;
    }
    void SetType(long ut)
    {
        _ut = ut;
    }

    virtual bool FDo(PDocumentBase pdocb);
    virtual bool FUndo(PDocumentBase pdocb);
};

//
// Different reasons for pausing in a scene
//
enum WIT
{
    witNil,
    witUntilClick,
    witUntilSnd,
    witForTime,
    witLim
};

//
// Functionality that can be turned off and on.  If the bit
// is set, it is disabled.
//
enum
{
    fscenSounds = 0x1,
    fscenPauses = 0x2,
    fscenTboxes = 0x4,
    fscenActrs = 0x8,
    fscenPosition = 0x10,
    fscenAction = 0x20,
    fscenCams = 0x40,
    fscenAll = 0xFFFF
};

typedef struct SSE *PSSE;
typedef struct TAGC *PTAGC;

typedef class Scene *PScene;

//
// Notes:
//
//	This assumes that struct SND contains at least,
//		- Everything necessary to play the sound.
//
//	This assumes that struct TBOX contains at least,
//		- Everything necessary to display the text.
//		- Enumerating through text boxes in a scene is not necessary.
//

#define Scene_PAR BASE
#define kclsScene 'SCEN'
class Scene : public Scene_PAR
{
    RTCLASS_DEC
    MARKMEM
    ASSERT

  public:
    PDynamicArray selected_actors;

  protected:
    typedef struct SEV *PSEV;

    //
    // These variables keep track of the internal frame numbers.
    //
    long _nfrmCur;   // Current frame number
    long _nfrmLast;  // Last frame number in scene.
    long _nfrmFirst; // First frame number in scene.

    //
    // Frames with events in them.  This stuff works as follows.
    //   _isevFrmLim is the index into the GeneralGroup of a sev with nfrm > nCurFrm.
    //
    PGeneralGroup _pggsevFrm;   // List of events that occur in frames.
    long _isevFrmLim; // Next event to process.

    //
    // Global information
    //
    STN _stnName;         // Name of this scene
    PDynamicArray _pglpactr;        // List of actors in the scene.
    PDynamicArray _pglptbox;        // List of text boxes in the scene.
    PGeneralGroup _pggsevStart;     // List of frame independent events.
    PMovie _pmvie;         // Movie this scene is a part of.
    PBackground _pbkgd;         // Background for this scene.
    ulong _grfscen;       // Disabled functionality.
    PActor _pactrSelected; // Currently selected actor, if any
    // PActor _pactrSelected2; // Currently selected actor, if any
    PTBOX _ptboxSelected; // Currently selected tbox, if any
    TRANS _trans;         // Transition at the end of the scene.
    PMBMP _pmbmp;         // The thumbnail for this scene.
    PSSE _psseBkgd;       // Background scene sound (starts playing
                          // at start time even if snd event is
                          // earlier)
    long _nfrmSseBkgd;    // Frame at which _psseBkgd starts
    TAG _tagBkgd;         // Tag to current Background

  protected:
    Scene(PMovie pmvie);
    ~Scene(void);

    //
    // Event stuff
    //
    bool _FPlaySev(PSEV psev, void *qvVar, ulong grfscen); // Plays a single scene event.
    bool _FUnPlaySev(PSEV psev, void *qvVar);              // Undoes a single scene event.
    bool _FAddSev(PSEV psev, long cbVar, void *pvVar);     // Adds scene event to the current frame.
    void _MoveBackFirstFrame(long nfrm);

    //
    // Dirtying stuff
    //
    void _MarkMovieDirty(void);
    void _DoPrerenderingWork(bool fStartNow); // Does any prerendering for _nfrmCur
    void _EndPrerendering(void);              // Stops prerendering

    //
    // Make actors go to a specific frame
    //
    bool _FForceActorsToFrm(long nfrm, bool *pfSoundInFrame = pvNil);
    bool _FForceTboxesToFrm(long nfrm);

    //
    // Thumbnail routines
    //
    void _UpdateThumbnail(void);

  public:
    //
    // Create and destroy
    //
    static Scene *PscenNew(PMovie pmvie);                      // Returns pvNil if it fails.
    static Scene *PscenRead(PMovie pmvie, PChunkyResourceFile pcrf, ChunkNumber cno); // Returns pvNil if it fails.
    bool FWrite(PChunkyResourceFile pcrf, ChunkNumber *pcno);                       // Returns fFalse if it fails, else the cno written.
    static void Close(PScene *ppscen);                        // Public destructor
    void RemActrsFromRollCall(bool fDelIfOnlyRef = fFalse);  // Removes actors from movie roll call.
    bool FAddActrsToRollCall(void);                          // Adds actors from movie roll call.

    //
    // Tag collection
    //
    static bool FAddTagsToTagl(PChunkyFile pcfl, ChunkNumber cno, PTAGL ptagl);

    //
    // Frame functions
    //
    bool FPlayStartEvents(bool fActorsOnly = fFalse); // Play all one-time starting scene events.
    void InvalFrmRange(void);                         // Mark the frame count dirty
    bool FGotoFrm(long nfrm);                         // Jumps to an arbitrary frame.
    long Nfrm(void)                                   // Returns the current frame number
    {
        return (_nfrmCur);
    }
    long NfrmFirst(void) // Returns the number of the first frame in the scene.
    {
        return (_nfrmFirst);
    }
    long NfrmLast(void) // Returns the number of the last frame in the scene.
    {
        return (_nfrmLast);
    }
    bool FReplayFrm(ulong grfscen); // Replay events in this scene.

    //
    // Undo accessor functions
    //
    void SetNfrmCur(long nfrm)
    {
        _nfrmCur = nfrm;
    }

    //
    // Edit functions
    //
    void SetMvie(PMovie pmvie); // Sets the associated movie.
    void GetName(PSTN pstn)    // Gets name of current scene.
    {
        *pstn = _stnName;
    }
    void SetNameCore(PSTN pstn) // Sets name of current scene.
    {
        _stnName = *pstn;
    }
    bool FSetName(PSTN pstn); // Sets name of current scene, and undo
    bool FChopCore(void);     // Chops off the rest of the scene.
    bool FChop(void);         // Chops off the rest of the scene and undo
    bool FChopBackCore(void); // Chops off the rest of the scene, backwards.
    bool FChopBack(void);     // Chops off the rest of the scene, backwards, and undo.

    //
    // Transition functions
    //
    void SetTransitionCore(TRANS trans) // Set the final transition to be.
    {
        _trans = trans;
    }
    bool FSetTransition(TRANS trans); // Set the final transition to be and undo.
    TRANS Trans(void)
    {
        return _trans;
    } // Returns the transition setting.
    // These two operate a specific Scene chunk rather than a Scene in memory
    static bool FTransOnFile(PChunkyResourceFile pcrf, ChunkNumber cno, TRANS *ptrans);
    static bool FSetTransOnFile(PChunkyResourceFile pcrf, ChunkNumber cno, TRANS trans);

    //
    // State functions
    //
    void Disable(ulong grfscen) // Disables functionality.
    {
        _grfscen |= grfscen;
    }
    void Enable(ulong grfscen) // Enables functionality.
    {
        _grfscen &= ~grfscen;
    }
    long GrfScen(void) // Currently disabled functionality.
    {
        return _grfscen;
    }
    bool FIsEmpty(void); // Is the scene empty?

    //
    // Actor functions
    //
    bool FAddActrCore(Actor *pactr); // Adds an actor to the scene at current frame.
    bool FAddActr(Actor *pactr);     // Adds an actor to the scene at current frame, and undo
    void RemActrCore(long arid);    // Removes an actor from the scene.
    bool FRemActr(long arid);       // Removes an actor from the scene, and undo
    PActor PactrSelected(void)       // Returns selected actor
    {
        return _pactrSelected;
    }
    void SelectActr(Actor *pactr);                      // Sets the selected actor
    bool ActorIsSelected(Actor *pactor);
    void SelectAllActors();
    void SelectMultipleActors(Actor *pactr, bool toggle_selection);
    void DeselectMultipleActors();
    PActor PactrFromPt(long xp, long yp, long *pibset); // Gets actor pointed at by the mouse.
    PDynamicArray PglRollCall(void)                              // Return a list of all actors in scene.
    {
        return (_pglpactr);
    } // Only to be used by the movie-class
    void HideActors(void);
    void ShowActors(void);
    PActor PactrFromArid(long arid); // Finds a current actor in this scene.
    long Cactr(void)
    {
        return (_pglpactr == pvNil ? 0 : _pglpactr->IvMac());
    }

    //
    // Sound functions
    //
    bool FAddSndCore(bool fLoop, bool fQueue, long vlm, long sty, long ctag,
                     PTAG prgtag); // Adds a sound to the current frame.
    bool FAddSndCoreTagc(bool fLoop, bool fQueue, long vlm, long sty, long ctagc, PTAGC prgtagc);
    bool FAddSnd(PTAG ptag, bool fLoop, bool fQueue, long vlm, long sty); // Adds a sound to the current frame, and undo
    void RemSndCore(long sty);                                            // Removes the sound from current frame.
    bool FRemSnd(long sty);                             // Removes the sound from current frame, and undo
    bool FGetSnd(long sty, bool *pfFound, PSSE *ppsse); // Allows for retrieval of sounds.
    void PlayBkgdSnd(void);
    bool FQuerySnd(long sty, PDynamicArray *pgltagSnd, long *pvlm, bool *pfLoop);
    void SetSndVlmCore(long sty, long vlmNew);
    void UpdateSndFrame(void);
    bool FResolveAllSndTags(ChunkNumber cnoScen);

    //
    // Text box functions
    //
    bool FAddTboxCore(PTBOX ptbox);   // Adds a text box to the current frame.
    bool FAddTbox(PTBOX ptbox);       // Adds a text box to the current frame.
    bool FRemTboxCore(PTBOX ptbox);   // Removes a text box from the scene.
    bool FRemTbox(PTBOX ptbox);       // Removes a text box from the scene.
    PTBOX PtboxFromItbox(long itbox); // Returns the ith tbox in this frame.
    PTBOX PtboxSelected(void)         // Returns the tbox currently selected.
    {
        return _ptboxSelected;
    }
    void SelectTbox(PTBOX ptbox); // Selects this tbox.
    void HideTboxes(void);        // Hides all text boxes.
    long Ctbox(void)
    {
        return (_pglptbox == pvNil ? 0 : _pglptbox->IvMac());
    }

    //
    // Pause functions
    //
    bool FPauseCore(WIT *pwit, long *pdts); // Adds\Removes a pause to the current frame.
    bool FPause(WIT wit, long dts);         // Adds\Removes a pause to the current frame, and undo

    //
    // Background functions
    //
    bool FSetBkgdCore(PTAG ptag, PTAG ptagOld); // Sets the background for this scene.
    bool FSetBkgd(PTAG ptag);                   // Sets the background for this scene, and undo
    Background *Pbkgd(void)
    {
        return _pbkgd;
    }                                               // Gets the background for this scene.
    bool FChangeCamCore(long icam, long *picamOld); // Changes camera viewpoint at current frame.
    bool FChangeCam(long icam);                     // Changes camera viewpoint at current frame, and undo
    PMBMP PmbmpThumbnail(void);                     // Returns the thumbnail.
    bool FGetTagBkgd(PTAG ptag);                    // Returns the tag for the background for this scene

    //
    // Movie functions
    //
    PMovie Pmvie()
    {
        return (_pmvie);
    } // Get the parent movie

    //
    // Mark scene as dirty
    //
    void MarkDirty(bool fDirty = fTrue); // Mark the scene as changed.

    //
    // Clipboard type functions
    //
    bool FPasteActrCore(PActor pactr); // Pastes actor into current frame
    bool FPasteActr(PActor pactr);     // Pastes actor into current frame and undo

    //
    // Playing functions
    //
    bool FStartPlaying(void); // For special behavior when playback starts
    void StopPlaying(void);   // Used to clean up after playback has stopped.
};

#endif //! SCEN_H
