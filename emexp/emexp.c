#include "/usr/local/include/csound/csdl.h"
// #include <csdl.h>

#define MSG(m) (csound->Message(csound, Str(m)))
#define INITERR(m) (csound->InitError(csound, m))
#define PERFERROR(m) (csound->PerfError(csound, &(p->h), "%s", m))

/*

  iout[]  vecview ifn, istart, iend

  An array view into a function table. The returned array does not
  own the memory, it is just a view (an alias) into the memory
  allocated by the function table

 */


typedef struct {
    OPDS h;
    ARRAYDAT *out;
    MYFLT *ifn, *istart, *iend;
    FUNC * ftp;
    int size;
} TABALIAS;

int
tabalias_deinit(CSOUND *csound, void *voidp) {
    TABALIAS *p = voidp;
    p->out->data = NULL;
    p->out->sizes[0] = 0;
    return OK;
}

static int
tabalias_init(CSOUND *csound, TABALIAS *p) {
    FUNC *ftp;
    ftp = csound->FTFind(csound, p->ifn);
    if (UNLIKELY(ftp == NULL))
        return NOTOK;
    uint32_t tabsize = ftp->flen;
    int start = *p->istart;
    int end   = *p->iend;
    if(end == 0)
        end = tabsize;
    p->size = end - start;
    p->ftp = ftp;
    if (tabsize < end) {
        return INITERR(Str("tabalias: end is bigger than the length of the table"));
    }
    if(p->out->data == NULL) {
        CS_VARIABLE* var = p->out->arrayType->createVariable(csound, NULL);
        p->out->arrayMemberSize = var->memBlockSize;
    } else {
        return INITERR(Str("tabalias: Array variable already initialized!\n"));
    }
    if(p->out->sizes == NULL) {
        p->out->sizes = (int*)csound->Malloc(csound, sizeof(int));
    }
    p->out->data = ftp->ftable + start;
    p->out->dimensions = 1;
    p->out->sizes[0] = p->size;
    csound->RegisterDeinitCallback(csound, p, tabalias_deinit);
    return OK;
}


/*

  kout[]  vecview  kin[], istart, iend

  An array view into another array, useful to operate on a row
  of a large 2D array.

 */


typedef struct {
    OPDS h;
    ARRAYDAT *out, *in;
    MYFLT *istart, *iend;
    MYFLT *dataptr;
    int size;
} ARRAYVIEW;

int
arrayview_deinit(CSOUND *csound, void *voidp) {
    ARRAYVIEW *p = voidp;
    p->out->data = NULL;
    p->out->sizes[0] = 0;
    return OK;
}

static int32_t
arrayview_init(CSOUND *csound, ARRAYVIEW *p) {
    int end   = *p->iend;
    int start = *p->istart;
    if(p->in->dimensions > 1)
        return INITERR(Str("A view can only be taken from a 1D array"));
    if(p->out->data == NULL) {
        CS_VARIABLE* var = p->out->arrayType->createVariable(csound, NULL);
        p->out->arrayMemberSize = var->memBlockSize;
    }
    if(end == 0)
        end = p->in->sizes[0];
    p->out->data = p->dataptr = p->in->data + start;
    p->out->dimensions = 1;
    p->out->sizes    = (int*)csound->Malloc(csound, sizeof(int));
    p->out->sizes[0] = p->size = end - start;
    csound->RegisterDeinitCallback(csound, p, arrayview_deinit);
    return OK;
}


#define S(x) sizeof(x)

static OENTRY localops[] = {
    { "vecview", S(TABALIAS),  0, 1, "k[]", "ioo", (SUBR)tabalias_init},
    { "vecview", S(ARRAYVIEW), 0, 1, "k[]", ".[]oo", (SUBR)arrayview_init},
};

LINKAGE
