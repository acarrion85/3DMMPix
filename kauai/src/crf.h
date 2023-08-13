/* Copyright (c) Microsoft Corporation.
   Licensed under the MIT License. */

/* Copyright (c) Microsoft Corporation.
   Licensed under the MIT License. */

/***************************************************************************
    Author: ShonK
    Project: Kauai
    Reviewed:
    Copyright (c) Microsoft Corporation

    Chunky resource file management. A ChunkyResourceFile is a cache wrapped around a
    chunky file. A ChunkyResourceManager is a list of CRFs. A BACO is an object that
    can be cached in a ChunkyResourceFile. An RCA is an interface that ChunkyResourceFile and ChunkyResourceManager both
    implement (are a super set of).

***************************************************************************/
#ifndef CHRES_H
#define CHRES_H

typedef class ChunkyResourceFile *PChunkyResourceFile;
typedef ChunkNumber RSC;
const RSC rscNil = 0L;

// chunky resource entry priority
enum
{
    crepToss,
    crepTossFirst,
    crepNormal,
};

/***************************************************************************
    Base cacheable object.  All cacheable objects must be based on BACO.
***************************************************************************/
typedef class BACO *PBACO;
#define BACO_PAR BASE
#define kclsBACO 'BACO'
class BACO : public BACO_PAR
{
    RTCLASS_DEC
    ASSERT
    MARKMEM

  private:
    // These fields are owned by the ChunkyResourceFile
    PChunkyResourceFile _pcrf; // The BACO has a ref count on this iff !_fAttached
    ChunkTag _ctg;
    ChunkNumber _cno;
    long _crep : 16;
    long _fAttached : 1;

    friend class ChunkyResourceFile;

  protected:
    BACO(void);
    ~BACO(void);

  public:
    virtual void Release(void);

    virtual void SetCrep(long crep);
    virtual void Detach(void);

    ChunkTag Ctg(void)
    {
        return _ctg;
    }
    ChunkNumber Cno(void)
    {
        return _cno;
    }
    PChunkyResourceFile Pcrf(void)
    {
        return _pcrf;
    }
    long Crep(void)
    {
        return _crep;
    }

    // Many objects know how big they are, and how to write themselves to a
    // chunky file.  Here are some useful prototypes so that the users of those
    // objects don't need to know what the actual class is.
    virtual bool FWrite(PDataBlock pblck);
    virtual bool FWriteFlo(PFLO pflo);
    virtual long CbOnFile(void);
};

/***************************************************************************
    Chunky resource cache - this is a pure virtual class that supports
    the crf and crm classes.
***************************************************************************/
// Object reader function - must handle ppo == pvNil, in which case, the
// *pcb should be set to an estimate of the size when read.
typedef bool FNRPO(PChunkyResourceFile pcrf, ChunkTag ctg, ChunkNumber cno, PDataBlock pblck, PBACO *ppbaco, long *pcb);
typedef FNRPO *PFNRPO;

typedef class RCA *PRCA;
#define RCA_PAR BASE
#define kclsRCA 'RCA'
class RCA : public RCA_PAR
{
    RTCLASS_DEC

  public:
    virtual tribool TLoad(ChunkTag ctg, ChunkNumber cno, PFNRPO pfnrpo, RSC rsc = rscNil, long crep = crepNormal) = 0;
    virtual PBACO PbacoFetch(ChunkTag ctg, ChunkNumber cno, PFNRPO pfnrpo, bool *pfError = pvNil, RSC rsc = rscNil) = 0;
    virtual PBACO PbacoFind(ChunkTag ctg, ChunkNumber cno, PFNRPO pfnrpo, RSC rsc = rscNil) = 0;
    virtual bool FSetCrep(long crep, ChunkTag ctg, ChunkNumber cno, PFNRPO pfnrpo, RSC rsc = rscNil) = 0;
    virtual PChunkyResourceFile PcrfFindChunk(ChunkTag ctg, ChunkNumber cno, RSC rsc = rscNil) = 0;
};

/***************************************************************************
    Chunky resource file.
***************************************************************************/
#define ChunkyResourceFile_PAR RCA
#define kclsChunkyResourceFile 'CRF'
class ChunkyResourceFile : public ChunkyResourceFile_PAR
{
    RTCLASS_DEC
    ASSERT
    MARKMEM

  protected:
    struct CRE
    {
        PFNRPO pfnrpo;    // object reader
        long cactRelease; // the last time this object was released
        BACO *pbaco;      // the object
        long cb;          // size of data
    };

    PCFL _pcfl;
    PGL _pglcre; // sorted by (cki, pfnrpo)
    long _cbMax;
    long _cbCur;
    long _cactRelease;

    ChunkyResourceFile(PCFL pcfl, long cbMax);
    bool _FFindCre(ChunkTag ctg, ChunkNumber cno, PFNRPO pfnrpo, long *picre);
    bool _FFindBaco(PBACO pbaco, long *picre);
    bool _FPurgeCb(long cbPurge, long crepLast);

  public:
    ~ChunkyResourceFile(void);
    static PChunkyResourceFile PcrfNew(PCFL pcfl, long cbMax);

    virtual tribool TLoad(ChunkTag ctg, ChunkNumber cno, PFNRPO pfnrpo, RSC rsc = rscNil, long crep = crepNormal);
    virtual PBACO PbacoFetch(ChunkTag ctg, ChunkNumber cno, PFNRPO pfnrpo, bool *pfError = pvNil, RSC rsc = rscNil);
    virtual PBACO PbacoFind(ChunkTag ctg, ChunkNumber cno, PFNRPO pfnrpo, RSC rsc = rscNil);
    virtual bool FSetCrep(long crep, ChunkTag ctg, ChunkNumber cno, PFNRPO pfnrpo, RSC rsc = rscNil);
    virtual PChunkyResourceFile PcrfFindChunk(ChunkTag ctg, ChunkNumber cno, RSC rsc = rscNil);

    long CbMax(void)
    {
        return _cbMax;
    }
    void SetCbMax(long cbMax);

    PCFL Pcfl(void)
    {
        return _pcfl;
    }

    // These APIs are intended for BACO use only
    void BacoDetached(PBACO pbaco);
    void BacoReleased(PBACO pbaco);
};

/***************************************************************************
    Chunky resource manager - a list of CRFs.
***************************************************************************/
typedef class ChunkyResourceManager *PChunkyResourceManager;
#define ChunkyResourceManager_PAR RCA
#define kclsChunkyResourceManager 'CRM'
class ChunkyResourceManager : public ChunkyResourceManager_PAR
{
    RTCLASS_DEC
    ASSERT
    MARKMEM

  protected:
    PGL _pglpcrf;

    ChunkyResourceManager(void)
    {
    }

  public:
    ~ChunkyResourceManager(void);
    static PChunkyResourceManager PcrmNew(long ccrfInit);

    virtual tribool TLoad(ChunkTag ctg, ChunkNumber cno, PFNRPO pfnrpo, RSC rsc = rscNil, long crep = crepNormal);
    virtual PBACO PbacoFetch(ChunkTag ctg, ChunkNumber cno, PFNRPO pfnrpo, bool *pfError = pvNil, RSC rsc = rscNil);
    virtual PBACO PbacoFind(ChunkTag ctg, ChunkNumber cno, PFNRPO pfnrpo, RSC rsc = rscNil);
    virtual bool FSetCrep(long crep, ChunkTag ctg, ChunkNumber cno, PFNRPO pfnrpo, RSC rsc = rscNil);
    virtual PChunkyResourceFile PcrfFindChunk(ChunkTag ctg, ChunkNumber cno, RSC rsc = rscNil);

    bool FAddCfl(PCFL pcfl, long cbMax, long *piv = pvNil);
    long Ccrf(void)
    {
        AssertThis(0);
        return _pglpcrf->IvMac();
    }
    PChunkyResourceFile PcrfGet(long icrf);
};

/***************************************************************************
    An object (BACO) wrapper around a generic HQ.
***************************************************************************/
#define GHQ_PAR BACO
typedef class GHQ *PGHQ;
#define kclsGHQ 'GHQ'
class GHQ : public GHQ_PAR
{
    RTCLASS_DEC
    ASSERT
    MARKMEM

  public:
    HQ hq;

    GHQ(HQ hqT)
    {
        hq = hqT;
    }
    ~GHQ(void)
    {
        FreePhq(&hq);
    }

    // An object reader for a GHQ.
    static bool FReadGhq(PChunkyResourceFile pcrf, ChunkTag ctg, ChunkNumber cno, PDataBlock pblck, PBACO *ppbaco, long *pcb);
};

/***************************************************************************
    A BACO wrapper around a generic object.
***************************************************************************/
#define CABO_PAR BACO
typedef class CABO *PCABO;
#define kclsCABO 'CABO'
class CABO : public CABO_PAR
{
    RTCLASS_DEC
    ASSERT
    MARKMEM

  public:
    BASE *po;

    CABO(BASE *poT)
    {
        po = poT;
    }
    ~CABO(void)
    {
        ReleasePpo(&po);
    }
};

#endif //! CHRES_H
