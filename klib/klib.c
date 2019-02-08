// #include "/usr/local/include/csound/csdl.h"
#include "csdl.h"
// #include "/usr/local/include/csound/csound.h"
#include "khash.h"

/*

  This file implements opcodes for a hashtable (dict)

  * A dict can be created by a note or by instr 0.
  * lifetime: either tied to the lifetime of the note, or it can
  survive the note until dict_free is called later. When a dict
  is not tied to a note, its handle can be passed around.
  By default, a dict is local (is freed with the end of the note)
  * key can be either a string or an integer
  * value can be either be a string or a float (maybe in the future also audio and arrays?)
  * the type of the hashtable is indicated via a string code:
  idict dict_new "sf"  -> key of type string, value of type float
  idict dict_new "is"  -> key:int, value:string
  Possible types are: sf, ss, is, if

  Implemented opcodes:

  dict_new

    ihandle dict_new Stype [, iglobal]

    Stype: the type definition, a 2-char string defining the type of
           the key and the value
           s = string
           i = int
           f = float

           for keys, only s and i are allowed
           for values, only s and f

    idict dict_new "sf"    ; create a hashtable of type string -> number
                           ; hashtable is freed at note end
    idict dict_new "if", 1 ; create a hashtable of type int -> number
                           ; hahstable is global, survives the note

  dict_free

    dict_free idict [, iwhen]

    frees the hashtable either at init time or at the end of the note
    (similar to ftfree)
    This is needed when passing a hashtable from one instrument to another

    iwhen = 0  -> free now
    iwhen = 1  -> free at the end of this note

  dict_set

    dict_set idict, Skey, kvalue        ; k-time
    dict_set idict, kkey, kvalue        ; k-time
    dict_set idict, ikey, ivalue        ; i-time

    Set a key:value pair

    idict dict_new "sf"
    dict_set idict, "key", kvalue

  dict_get

    Get the value at a given key. For string keys, an empty string
    is returned when the key is not found. For int keys, either 0
    or a default value given by the user are returned when the
    key is not found.

    kvalue dict_get idict, "key" [, kdefault]

  dict_del

    dict_del idict, Skey
    dict_del idict, kkey
    dict_del idict, ikey

  dict_type

    itype dict_type idict
    ktype dict_type kdict

    Returns the type definition as an integer, or 0 if the hashtable does not exist
    11 = int -> float
    12 = int -> string
    21 = string -> float
    22 = string -> string

  All opcodes work at k-time
  The hashtables with int-keys work also at i-time whenever key and value are
  of i-type (for both set and get actions)
  
  TODO:
  * implement easy initialization of dicts, something like
  idict dict_new "sf", 0, "key1", value1, "key2", value2, ...

*/

#define STR_OVERALLOC 24
#define KHASH_STRKEY_MAXSIZE 63
#define KHASH_STRVALUE_MAXSIZE 256
#define HANDLES_INITIAL_SIZE 200

#define min(x, y) (((x) < (y)) ? (x) : (y))
#define max(x, y) (((x) > (y)) ? (x) : (y))


#define INITERR(m) (csound->InitError(csound, "%s", m))
#define PERFERR(m) (csound->PerfError(csound, &(p->h), "%s", m))
#define PERFERRFMT(fmt, ...) (csound->PerfError(csound, &(p->h), fmt, __VA_ARGS__))
#define DEBUG(a) do { printf("%s\n", a); fflush(stdout); } while (0)


static
char* cs_strdup(CSOUND* csound, char* str) {
    size_t len;
    char* retVal;

    if (str == NULL) return NULL;

    len = strlen(str);
    retVal = csound->Malloc(csound, len + 1);

    if (len > 0) {
      strncpy(retVal, str, len);
    }
    retVal[len] = '\0';
    return retVal;
}

static
char *cs_strdup_overalloc(CSOUND *csound, char *str, int32_t overalloc) {
    size_t len;
    char* retVal;

    if (str == NULL) return NULL;

    len = strlen(str);
    
    retVal = csound->Malloc(csound, len + 1 + overalloc);

    if (len > 0) {
      strncpy(retVal, str, len);
    }
    retVal[len] = '\0';
    return retVal;
}

static inline
void strncpy0(char *dest, const char *src, size_t n) {
    strncpy(dest, src, n);
    dest[n] = '\0';
}

/*
static
char* strensure(CSOUND *csound, char *s, int minsize, int overalloc) {
    if((int)sizeof(s) < minsize) {
        s = csound->ReAlloc(csound, s, minsize + overalloc);
    }
    return s;
}
*/

// float=1, str=2
const int khStrFloat = 21;
const int khStrStr   = 22;
const int khIntStr   = 12;
const int khIntFloat = 11;

// setup khash to handle string key with MYFLT payload
KHASH_MAP_INIT_STR(khStrFloat, MYFLT);
KHASH_MAP_INIT_STR(khStrStr, char*);
KHASH_MAP_INIT_INT(khIntFloat, MYFLT);
KHASH_MAP_INIT_INT(khIntStr, char*);

typedef struct {
    CSOUND *csound;
    int khtype;
    uint64_t counter;
    void *khashptr;
    int isglobal;
    void *mutex_;
    spin_lock_t lock;
} KHASH_HANDLE;

typedef struct {
    CSOUND *csound;
    int32_t maxhandles;
    KHASH_HANDLE *handles;
    void *mutex_;
} KHASH_GLOBALS;

// ihandle hashtab_new "type", isglobal=0
typedef struct {
    OPDS h;
    // outputs
    MYFLT *ihandleidx;

    // inputs
    STRINGDAT *keyvaltype;
    MYFLT *isglobal;

    // internal
    KHASH_GLOBALS *globals;
    int khtype;
} HASHTAB_NEW;

#define kh_get_val(kname, hash, key, defVal) ({k=kh_get(kname, hash, key);(k!=kh_end(hash)?kh_val(hash,k):defVal);})
// #define kh_set_val(kname, hash, key, val) ({int ret; k = kh_put(kname, hash,key,&ret); kh_value(hash,k) = val; ret;})

#define CHECK_HASHTAB_EXISTS(h) ({ if(h==NULL) {PERFERR(Str("dict: instance does not exist!")); return NOTOK; }})

#define CHECK_HASHTAB_TYPE(handletype, expectedtype)  \
    if(handletype != expectedtype) { \
        csound->InitError(csound, Str("Expected a dict of type '%s', got '%s'"), \
        intdef2stringdef(expectedtype), intdef2stringdef(handletype));            \
        return NOTOK; }

// #define CSOUND_MULTICORE

#ifdef CSOUND_MULTICORE
    #define LOCK(x) csound->LockMutex(x->mutex_)
    #define UNLOCK(x) csound->LockMutex(x->mutex_)
#else
    #define LOCK(x) do {} while(0)
    #define UNLOCK(x) do {} while(0)
#endif

static int32_t hashtab_reset(CSOUND *csound, KHASH_GLOBALS *g);
static int32_t _hashtab_free(CSOUND *csound, KHASH_GLOBALS *g, int32_t idx);

static
KHASH_GLOBALS* hashtab_globals(CSOUND *csound) {
    KHASH_GLOBALS *g = (KHASH_GLOBALS*)csound->QueryGlobalVariable(csound, "khashGlobals_");
    if(LIKELY(g != NULL))
        return g;
    int err = csound->CreateGlobalVariable(csound, "khashGlobals_", sizeof(KHASH_GLOBALS));
    if (err != 0) {
        INITERR(Str("hashtab: failed to allocate globals"));
        return NULL;
    };
    g = (KHASH_GLOBALS*)csound->QueryGlobalVariable(csound, "khashGlobals_");
    g->csound = csound;
    g->maxhandles = HANDLES_INITIAL_SIZE;
    g->handles = csound->Calloc(csound, sizeof(KHASH_HANDLE)*g->maxhandles);
    g->mutex_ = csound->Create_Mutex(0);
    csound->RegisterResetCallback(csound, (void*)g, (int32_t (*)(CSOUND*, void*))hashtab_reset);

    return g;
}

static
int32_t hashtab_getfreeslot(KHASH_GLOBALS *g) {
    for(int32_t i=0; i<g->maxhandles; i++) {
        if(g->handles[i].khashptr == NULL) {
            return i;
        }
    }
    // no free slots, we need to grow!
    CSOUND *csound = g->csound;
    int32_t numhandles = g->maxhandles * 2;
    LOCK(g);
    g->handles = csound->ReAlloc(csound, g->handles, sizeof(KHASH_HANDLE) * numhandles);
    if (g->handles == NULL) {
        return -1;
    }
    int32_t idx = g->maxhandles;
    g->maxhandles = numhandles;
    UNLOCK(g);
    return idx;
}

static
int32_t hashtab_reset(CSOUND *csound, KHASH_GLOBALS *g) {
    int32_t i;
    for(i = 0; i < g->maxhandles; i++) {
        if(g->handles[i].khashptr != NULL) {
            _hashtab_free(csound, g, i);
        }
        if(g->handles[i].mutex_ != NULL) {
            csound->DestroyMutex(g->handles[i].mutex_);
        }
    }
    csound->DestroyMutex(g->mutex_);
    csound->Free(csound, g->handles);
    csound->DestroyGlobalVariable(csound, "khashGlobals_");
    return OK;
}

static
int32_t hashtab_make(CSOUND *csound, int khtype, int isglobal) {
    KHASH_GLOBALS *g = hashtab_globals(csound);
    int32_t idx = hashtab_getfreeslot(g);
    if(idx < 0) {
        return -1;
    }
    void *hashtab = NULL;
    switch(khtype) {
    case khStrFloat:
        hashtab = (void *)kh_init(khStrFloat);
    case khStrStr:
        hashtab = (void *)kh_init(khStrStr);
    case khIntStr:
        hashtab = (void *)kh_init(khIntStr);
    case khIntFloat:
        hashtab = (void *)kh_init(khIntFloat);
    }
    KHASH_HANDLE *handle = &(g->handles[idx]);
    handle->khashptr = hashtab;
    handle->khtype = khtype;
    handle->counter = 0;
    handle->isglobal = isglobal;
    handle->mutex_ = csound->Create_Mutex(0);
    return idx;
}

static
int32_t _hashtab_free(CSOUND *csound, KHASH_GLOBALS *g, int32_t idx) {
    int khtype = g->handles[idx].khtype;
    khint_t k;
    char *strvalue;
    printf(">>>>>>>>>>>>> freeing idx: %d   type: %d\n", idx, khtype);
    KHASH_HANDLE *handle = &(g->handles[idx]);
    void *khashptr = (void *)handle->khashptr;
    handle->counter++;
    handle->khashptr = NULL;
    if(khtype == khStrFloat) {
        khash_t(khStrFloat) *h = khashptr;
        // we need to free all keys
        for (k = 0; k < kh_end(h); ++k) {
            if (kh_exist(h, k)) {
                csound->Free(csound, (char*)kh_key(h, k));
            }
        }
        kh_destroy(khStrFloat, h);
    } else if (khtype == khStrStr) {
        khash_t(khStrStr) *h = khashptr;
        g->handles[idx].khashptr = NULL;
        // we need to free all keys and values
        for (k = 0; k < kh_end(h); ++k) {
            if (kh_exist(h, k)) {
                strvalue = kh_val(h, k);
                if(strvalue != NULL)
                    csound->Free(csound, strvalue);
                csound->Free(csound, (char*)kh_key(h, k));
            }
        }
        kh_destroy(khStrStr, h);
    } else if (khtype == khIntFloat) {
        khash_t(khIntFloat) *h = khashptr;
        kh_destroy(khIntFloat, h);
    } else if (khtype == khIntStr) {
        khash_t(khIntStr) *h = khashptr;
        // we need to free all values
        for (k = 0; k < kh_end(h); ++k) {
            if (kh_exist(h, k)) {
                csound->Free(csound, kh_val(h, k));
            }
        }
        kh_destroy(khIntStr, h);
    } else {
        return NOTOK;
    }
    return OK;
}

static
int32_t hashtab_deinit_callback(CSOUND *csound, HASHTAB_NEW* p) {
    int idx = (int32_t)*p->ihandleidx;
    KHASH_GLOBALS *g = p->globals;
    if(g->handles[idx].khashptr == NULL) {
        // already freed?
        csound->Message(csound, "%s\n", Str("dict already freed"));
        return OK;
    }
    return _hashtab_free(csound, g, idx);
}

static
char* intdef2stringdef(int32_t intdef) {
    // convert an integer definition of a hashtab type A -> B
    // to its string representation
    switch(intdef) {
    case khIntFloat:
        return "if";
    case khIntStr:
        return "is";
    case khStrFloat:
        return "sf";
    case khStrStr:
        return "ss";
    }
    return NULL;
}

static
int32_t stringdef2intdef(STRINGDAT *s) {
    // convert a string definition of a hashtab type A->B to
    // its integer representation
    // returns -1 in case of error
    // for now, only 2-char definitions supported (maybe later will support arrays?)
    // NB: size counts also the \0 termination
    if(s->size != 3) {
        return -1;
    }
    char *stringdef = (char *)s->data;
    switch(stringdef[0]) {
    case 's':
        switch(stringdef[1]) {
        case 's':
            return khStrStr;
        case 'f':
            return khStrFloat;
        default:
            return -1;
        }
    case 'i':
        switch(stringdef[1]) {
        case 's':
            return khIntStr;
        case 'f':
            return khIntFloat;
        default:
            return -1;
        }
    default:
        return -1;
    }
}

static
int32_t hashtab_init(CSOUND *csound, HASHTAB_NEW *p) {
    p->globals = hashtab_globals(csound);
    int32_t khtype = stringdef2intdef(p->keyvaltype);
    if(khtype < 0) {
        INITERR(Str("dict: type definition not understood, expected sf, ss, if or is"));
        return NOTOK;
    }
    int32_t idx = hashtab_make(csound, khtype, *p->isglobal);
    if(idx < 0) {
        INITERR(Str("dict: failed to allocate the hashtable"));
        return NOTOK;
    }
    *p->ihandleidx = idx;
    p->khtype = khtype;
    if ((int32_t)*p->isglobal == 0) {
        // dict should not survive this note
        csound->RegisterDeinitCallback(csound, p, (int32_t (*)(CSOUND *, void*)) hashtab_deinit_callback);
    }
    return OK;
}

// ------------------------------------------------------
//                         SET
// ------------------------------------------------------

// hashtab_set ihandle, Skey, kvalue
typedef struct {
    OPDS h;
    // out
    MYFLT *ihandle;
    // in
    MYFLT *key;
    MYFLT *value;
    // internal
    KHASH_GLOBALS *globals;
    khint_t lastidx;
    int32_t lastkey;
    uint64_t counter;
} HASHTAB_SET_if;


static
int32_t hashtab_set_if_init(CSOUND *csound, HASHTAB_SET_if *p) {
    p->globals = hashtab_globals(csound);
    p->lastkey = -1;
    p->lastidx = -1;
    p->counter = 0;
    return OK;
}

static
int32_t hashtab_set_if(CSOUND *csound, HASHTAB_SET_if *p) {
    KHASH_GLOBALS *g = p->globals;
    int32_t idx = (int32_t)*p->ihandle;
    KHASH_HANDLE *handle = &(g->handles[idx]);
    khash_t(khIntFloat) *h = handle->khashptr;
    CHECK_HASHTAB_EXISTS(h);
    CHECK_HASHTAB_TYPE(handle->khtype, khIntFloat);
    int32_t key = *p->key;
    khint_t k;
    if(handle->counter == p->counter && key == p->lastkey) {
        k = p->lastidx;
    } else {
        int absent;    
        p->lastidx = k = kh_put(khIntFloat, h, key, &absent);
        if (absent) {
            if(handle->isglobal) {
                LOCK(handle);
                kh_key(h, k) = key;
                UNLOCK(handle);
            } else {
                kh_key(h, k) = key;
            }
            handle->counter++;
        }
        p->lastkey = key;
        p->counter = handle->counter;
    }
    kh_value(h, k) = *p->value;
    return OK;
}

static
int32_t hashtab_set_if_i(CSOUND *csound, HASHTAB_SET_if *p) {
    hashtab_set_if_init(csound, p);
    return hashtab_set_if(csound, p);
}

// hashtab_set ihandle, Skey, kvalue
typedef struct {
    OPDS h;
    // out
    MYFLT *ihandle;
    // in
    STRINGDAT *key;
    MYFLT *value;
    // internal
    KHASH_GLOBALS *khashglobals;
    uint64_t counter;
    khiter_t keyidx;
    int32_t lastkey_size;
    char lastkey_data[KHASH_STRKEY_MAXSIZE+1];
} HASHTAB_SET_sf;

static
int32_t hashtab_set_sf_init(CSOUND *csound, HASHTAB_SET_sf *p) {
    KHASH_GLOBALS *g = hashtab_globals(csound);
    p->khashglobals = g;
    p->lastkey_size = -1;
    p->keyidx = 0;
    p->counter = 0;
    p->lastkey_data[0] = '\0';
    return OK;
}

static
int32_t hashtab_set_sf(CSOUND *csound, HASHTAB_SET_sf *p) {
    KHASH_GLOBALS *g = p->khashglobals;
    int32_t idx = (int32_t)*p->ihandle;
    KHASH_HANDLE *handle = &(g->handles[idx]);
    khash_t(khStrFloat) *h = handle->khashptr;
    CHECK_HASHTAB_EXISTS(h);
    CHECK_HASHTAB_TYPE(handle->khtype, khStrFloat);
    int absent;
    khiter_t k;
    char *key;
    // test fastpath
    if(p->counter == handle->counter &&
       p->lastkey_size == p->key->size &&
       strcmp(p->lastkey_data, p->key->data)==0) {
        kh_value(h, p->keyidx) = *p->value;
        return OK;
    }
    p->keyidx = k = kh_put(khStrFloat, h, p->key->data, &absent);
    if (absent) {
        if(p->key->size > KHASH_STRKEY_MAXSIZE) {
            PERFERRFMT(Str("dict: key is too long (%d > %d)"),
                       p->key->size, KHASH_STRKEY_MAXSIZE);
            return NOTOK;
        }
        key = cs_strdup(csound, p->key->data);
        if(handle->isglobal) {
            LOCK(handle); kh_key(h, k) = key; UNLOCK(handle);
        } else
            kh_key(h, k) = key;
        handle->counter++;
    }
    kh_value(h, k) = *p->value;
    p->lastkey_size = p->key->size;
    p->counter = handle->counter;
    return OK;
}

static
int32_t hashtab_set_sf_i(CSOUND *csound, HASHTAB_SET_sf *p) {
    hashtab_set_sf_init(csound, p);
    return hashtab_set_sf(csound, p);
}

// hashtab_set ihandle, Skey, kvalue
typedef struct {
    OPDS h;
    // out
    MYFLT *ihandle;
    // in
    STRINGDAT *key;
    STRINGDAT *value;

    // interval
    KHASH_GLOBALS *khashglobals;
    uint64_t counter;
    khiter_t keyidx;
    int32_t lastkey_size;
    char lastkey_data[KHASH_STRKEY_MAXSIZE+1];
} HASHTAB_SET_ss;

#define KHASH_KEY_MINSIZE 24

static
int32_t hashtab_set_ss_init(CSOUND *csound, HASHTAB_SET_ss *p) {
    p->khashglobals = hashtab_globals(csound);
    p->lastkey_size = -1;
    p->keyidx = 0;
    p->counter = 0;
    p->lastkey_data[0] = '\0';
    return OK;
}

static
int32_t hashtab_set_ss(CSOUND *csound, HASHTAB_SET_ss *p) {
    KHASH_GLOBALS *g = p->khashglobals;
    int32_t idx = (int32_t)*p->ihandle;
    KHASH_HANDLE *handle = &(g->handles[idx]);
    khash_t(khStrStr) *h = handle->khashptr;
    khint_t k;
    int absent;
    char *key, *value;
    CHECK_HASHTAB_EXISTS(h);
    CHECK_HASHTAB_TYPE(handle->khtype, khStrStr);
    // fastpath: dict was not changed and this key is unchanged
    if(p->counter == handle->counter &&
       p->key->size == p->lastkey_size &&   
       strcmp(p->key->data, p->lastkey_data)==0) {
        // last index is valid
        k = p->keyidx;
    } else {
        k = kh_put(khStrStr, h, p->key->data, &absent);
        if (absent) {
            if(p->key->size > KHASH_STRKEY_MAXSIZE) {
                PERFERRFMT(Str("dict: key too long (%d > %d)"),
                           p->key->size, KHASH_STRKEY_MAXSIZE);
                return NOTOK;
            }
            key = cs_strdup(csound, p->key->data);
            value = cs_strdup_overalloc(csound, p->value->data, STR_OVERALLOC);
            LOCK(handle);
            kh_key(h, k) = key;
            kh_value(h, k) = value;
            UNLOCK(handle);
            handle->counter++;
            return OK;   
        }
        strncpy0(p->lastkey_data, p->key->data, p->key->size);
    }
    value = kh_val(h, k);
    LOCK(handle);
    if ((int)sizeof(value) < p->value->size) {
        kh_value(h, k) = value = csound->ReAlloc(csound, value, p->value->size+STR_OVERALLOC);
    }
    strncpy0(value, p->value->data, p->value->size);
    UNLOCK(handle);
    return OK;
}

// hashtab_set ihandle, Skey, kvalue
typedef struct {
    OPDS h;
    // out
    MYFLT *ihandle;
    // in
    MYFLT *key;
    STRINGDAT *value;
    
    // internal
    KHASH_GLOBALS *globals;
    khint_t lastidx;
    int32_t lastkey;
    uint64_t counter;
} HASHTAB_SET_is;

static
int32_t hashtab_set_is_init(CSOUND *csound, HASHTAB_SET_is *p) {
    p->globals = hashtab_globals(csound);
    p->lastkey = -1;
    p->lastidx = -1;
    p->counter = 0;
    return OK;
}

static
int32_t hashtab_set_is(CSOUND *csound, HASHTAB_SET_is *p) {
    KHASH_GLOBALS *g = p->globals;
    int32_t idx = (int32_t)*p->ihandle;
    KHASH_HANDLE *handle = &(g->handles[idx]);
    khash_t(khIntStr) *h = handle->khashptr;
    CHECK_HASHTAB_EXISTS(h);
    CHECK_HASHTAB_TYPE(handle->khtype, khIntStr);
    int32_t key = *p->key;
    char *value;
    khint_t k;
    if(handle->counter == p->counter && key == p->lastkey) {
        k = p->lastidx;
    } else {
        int absent;    
        p->lastidx = k = kh_put(khIntStr, h, key, &absent);
        if (absent) {
            value = cs_strdup_overalloc(csound, p->value->data, STR_OVERALLOC);
            LOCK(handle);
            kh_key(h, k) = key;
            kh_value(h, k) = value;
            UNLOCK(handle);
            handle->counter++;
            return OK;
        }
        p->lastkey = key;
        p->counter = handle->counter;
    }
    value = kh_val(h, k);
    LOCK(handle);
    if ((int)sizeof(value) < p->value->size) {
        kh_value(h, k) = value = csound->ReAlloc(csound, value, p->value->size+STR_OVERALLOC);
    }
    strncpy0(value, p->value->data, p->value->size);
    UNLOCK(handle);
    return OK;
}

// -----------------------------------------------------
//                      DEL
// -----------------------------------------------------

typedef struct {
    OPDS h;
    MYFLT *ihandle;
    STRINGDAT *key;
} HASHTAB_DEL_s;

static
int32_t hashtab_del_s(CSOUND *csound, HASHTAB_DEL_s *p) {
    KHASH_GLOBALS *g = hashtab_globals(csound);
    int32_t idx = (int32_t)*p->ihandle;
    KHASH_HANDLE *handle = &(g->handles[idx]);
    int32_t khtype = handle->khtype;
    khiter_t k;
    if(khtype == khStrFloat) {
        khash_t(khStrFloat) *h = handle->khashptr;
        CHECK_HASHTAB_EXISTS(h);
        k = kh_get(khStrFloat, h, p->key->data);
        if(k != kh_end(h)) {
            // key exists, free key str and remove item
            LOCK(handle);
            csound->Free(csound, (char*)kh_key(h, k));
            kh_del(khStrFloat, h, k);
            handle->counter++;
            UNLOCK(handle);
        }
    } else if(khtype == khStrStr) {
        khash_t(khStrStr) *h = g->handles[idx].khashptr;
        CHECK_HASHTAB_EXISTS(h);
        k = kh_get(khStrStr, h, p->key->data);
        if(k != kh_end(h)) {
            // key exists, free key str and value, remove item
            LOCK(handle);
            csound->Free(csound, (char*)kh_key(h, k));
            csound->Free(csound, (char*)kh_val(h, k));
            kh_del(khStrStr, h, k);
            handle->counter++;
            UNLOCK(handle);
        }
    }
    return OK;
}

typedef struct {
    OPDS h;
    MYFLT *ihandle;
    MYFLT *key;
} HASHTAB_DEL_i;

static
int32_t hashtab_del_i(CSOUND *csound, HASHTAB_DEL_i *p) {
    KHASH_GLOBALS *g = hashtab_globals(csound);
    int32_t idx = (int32_t)*p->ihandle;
    KHASH_HANDLE *handle = &(g->handles[idx]);
    int32_t khtype = handle->khtype;
    khiter_t k;
    int key = (int)*p->key;
    if(khtype == khIntFloat) {
        khash_t(khIntFloat) *h = g->handles[idx].khashptr;
        CHECK_HASHTAB_EXISTS(h);
        k = kh_get(khIntFloat, h, key);
        if(k != kh_end(h)) {
            // key exists, remove item
            kh_del(khIntFloat, h, k);
            handle->counter++;
        }
    } else if(khtype == khIntStr) {
        khash_t(khIntStr) *h = g->handles[idx].khashptr;
        CHECK_HASHTAB_EXISTS(h);
        k = kh_get(khIntStr, h, key);
        if(k != kh_end(h)) {
            // key exists, free value, remove item
            LOCK(handle);
            csound->Free(csound, (char*)kh_val(h, k));
            kh_del(khIntStr, h, k);
            UNLOCK(handle);
            handle->counter++;

        }
    } else {
        PERFERRFMT(Str("dict: wrong type, expected type 'if' or 'is', got %s"),
            intdef2stringdef(khtype));
        return NOTOK;
    }
    return OK;
}

// ------------------------------------------------------
//                         GET
// ------------------------------------------------------

// kvalue hashtab_get ihandle, kkey, kdefault=0
typedef struct {
    OPDS h;
    MYFLT *kout;
    // inputs
    MYFLT *ihandle;
    MYFLT *key;
    MYFLT *defaultvalue;
    // internal
    KHASH_GLOBALS *khashglobals;
    uint64_t counter;
    khiter_t keyidx;
    int lastkey;
} HASHTAB_GET_if;

static
int32_t hashtab_get_if_init(CSOUND *csound, HASHTAB_GET_if *p) {
    p->khashglobals = hashtab_globals(csound);
    p->lastkey = -1;
    p->keyidx = 0;
    p->counter = 0;
    return OK;
}

static
int32_t hashtab_get_if(CSOUND *csound, HASHTAB_GET_if *p) {
    KHASH_GLOBALS *g = p->khashglobals;
    int32_t idx = (int32_t)*p->ihandle;
    KHASH_HANDLE *handle = &(g->handles[idx]);
    khash_t(khIntFloat) *h = handle->khashptr;
    CHECK_HASHTAB_EXISTS(h);
    CHECK_HASHTAB_TYPE(handle->khtype, khIntFloat);
    khiter_t k;
    int32_t key = (int32_t)*p->key;
    if(p->counter == handle->counter &&        // fast path
       p->lastkey == key) {
        k = p->keyidx;
        *p->kout = kh_val(h, k);
        return OK;
    } 
    p->keyidx = k = kh_get(khIntFloat, h, key);
    *p->kout = k != kh_end(h) ? kh_val(h, k) : *p->defaultvalue;
    p->counter = handle->counter;
    p->lastkey = key;
    return OK;
}

static
int32_t hashtab_get_if_i(CSOUND *csound, HASHTAB_GET_if *p) {
    hashtab_get_if_init(csound, p);
    return hashtab_get_if(csound, p);
}

// kvalue hashtab_get ihandle, Skey (can be changed), kdefault=0
typedef struct {
    OPDS h;
    MYFLT *kout;
    // inputs
    MYFLT *ihandle;
    STRINGDAT *key;
    MYFLT *defaultvalue;
    // internal
    KHASH_GLOBALS *khashglobals;
    uint64_t counter;
    khiter_t keyidx;
    int32_t lastkey_size;
    char lastkey_data[KHASH_STRKEY_MAXSIZE+1];
} HASHTAB_GET_sf;

static
int32_t hashtab_get_sf_init(CSOUND *csound, HASHTAB_GET_sf *p) {
    p->khashglobals = hashtab_globals(csound);
    p->lastkey_size = -1;
    p->keyidx = 0;
    p->counter = 0;
    int32_t idx = (int32_t)*p->ihandle;
    CHECK_HASHTAB_TYPE(p->khashglobals->handles[idx].khtype, khStrFloat);
    return OK;
}

static
int32_t hashtab_get_sf(CSOUND *csound, HASHTAB_GET_sf *p) {
    KHASH_GLOBALS *g = p->khashglobals;
    int32_t idx = (int32_t)*p->ihandle;
    KHASH_HANDLE *handle = &(g->handles[idx]);
    int32_t khtype = handle->khtype;
    if(khtype != khStrFloat) {
        PERFERR(Str("dict should be of type Str -> float"));
        return NOTOK;
    }
    khash_t(khStrFloat) *h = handle->khashptr;
    if(h == NULL) {
        PERFERR("dict instance does not exist!");
        return NOTOK;
    }
    khiter_t k;
    // test fast path
    if(p->counter == handle->counter &&         \
       p->key->size == p->lastkey_size &&       \
       memcmp(p->key->data, p->lastkey_data, p->lastkey_size)==0) {
       //strcmp(p->key->data, p->lastkey_data)==0) {
        k = p->keyidx;
        *p->kout = kh_val(h, k);
        return OK;
    } 
    p->keyidx = k = kh_get(khStrFloat, h, p->key->data);
    p->lastkey_size = p->key->size;
    strncpy0(p->lastkey_data, p->key->data, p->key->size);
    *p->kout = k != kh_end(h) ? kh_val(h, k) : *p->defaultvalue;
    p->counter = handle->counter;
    return OK;
}

static
int32_t hashtab_get_sf_i(CSOUND *csound, HASHTAB_GET_sf *p) {
    hashtab_get_sf_init(csound, p);
    return hashtab_get_sf(csound, p);
}

/*
static
int32_t hashtab_get_sf_slow(CSOUND *csound, HASHTAB_GET_sf *p) {
    KHASH_GLOBALS *g = p->khashglobals;
    int32_t idx = (int32_t)*p->ihandle;
    KHASH_HANDLE *handle = &(g->handles[idx]);
    int32_t khtype = handle->khtype;
    if(khtype != khStrFloat) {
        PERFERR(Str("dict should be of type Str -> float"));
        return NOTOK;
    }
    khash_t(khStrFloat) *h = handle->khashptr;
    CHECK_HASHTAB_EXISTS(h);
    khiter_t k = kh_get(khStrFloat, h, p->key->data);
    *p->kout = k != kh_end(h) ? kh_val(h, k) : *p->defaultvalue;
    return OK;
}
*/

// kvalue hashtab_get ihandle, Skey (can be changed)
typedef struct {
    OPDS h;
    // out
    STRINGDAT *Sout;
    // inputs
    MYFLT *ihandle;
    STRINGDAT *key;
    // internal
    KHASH_GLOBALS *khashglobals;
    uint64_t counter;
    khiter_t keyidx;
    int32_t lastkey_size;
    char lastkey_data[KHASH_STRKEY_MAXSIZE+1];

} HASHTAB_GET_ss;

static
int32_t hashtab_get_ss_init(CSOUND *csound, HASHTAB_GET_ss *p) {
    p->khashglobals = hashtab_globals(csound);
    p->keyidx = 0;
    p->counter = 0;
    p->lastkey_size = 0;
    p->lastkey_data[0] = '\0';
    return OK;
}

static
int32_t hashtab_get_ss(CSOUND *csound, HASHTAB_GET_ss *p) {    
    // if the key is not found, an empty string is returned
    KHASH_GLOBALS *g = p->khashglobals;
    int32_t idx = (int32_t)*p->ihandle;
    KHASH_HANDLE *handle = &(g->handles[idx]);
    char *value;
    int32_t valuesize;
    khash_t(khStrStr) *h = g->handles[idx].khashptr;
    CHECK_HASHTAB_EXISTS(h);
    CHECK_HASHTAB_TYPE(handle->khtype, khStrStr);
    char *data = p->Sout->data;
    khiter_t k;
    // test fast path
    if(p->counter == handle->counter &&
       p->lastkey_size == p->key->size &&
       strcmp(p->lastkey_data, p->key->data)==0) {
        k = p->keyidx;
    } else {
        k = kh_get(khStrStr, h, p->key->data);
        if(k == kh_end(h)) {   // key not found
            data[0] = '\0';
            p->Sout->size = 0;
            return OK;
        }
        p->keyidx = k;
        p->counter = handle->counter;
        strncpy0(p->lastkey_data, p->key->data, p->key->size);
        p->lastkey_size = p->key->size;
    } 
    value = kh_val(h, k);
    valuesize = strlen(value);
    if ((int)sizeof(data) < valuesize) {
        p->Sout->data = data = csound->ReAlloc(csound, data, valuesize+STR_OVERALLOC);
    }
    strncpy0(data, value, valuesize);
    p->Sout->size = valuesize;
    return OK;
}


// kvalue hashtab_get ihandle, kkey, kdefault=0
typedef struct {
    OPDS h;
    STRINGDAT *Sout;
    // inputs
    MYFLT *ihandle;
    MYFLT *key;

    // internal
    KHASH_GLOBALS *khashglobals;
    uint64_t counter;
    khiter_t keyidx;
    int lastkey;
} HASHTAB_GET_is;

static
int32_t hashtab_get_is_init(CSOUND *csound, HASHTAB_GET_is *p) {
    p->khashglobals = hashtab_globals(csound);
    p->lastkey = -1;
    p->keyidx = 0;
    p->counter = 0;
    return OK;
}

static
int32_t hashtab_get_is(CSOUND *csound, HASHTAB_GET_is *p) {
    KHASH_GLOBALS *g = p->khashglobals;
    int32_t idx = (int32_t)*p->ihandle;
    KHASH_HANDLE *handle = &(g->handles[idx]);
    khash_t(khIntStr) *h = handle->khashptr;
    CHECK_HASHTAB_EXISTS(h);
    CHECK_HASHTAB_TYPE(handle->khtype, khIntStr);
    khiter_t k;
    int32_t key = (int32_t)*p->key;
    if(p->counter == handle->counter && p->lastkey == key) {  // fast path
        k = p->keyidx;
    } else {
        k = kh_get(khIntStr, h, key);
        if(k == kh_end(h)) {   // key not found
            p->Sout->data[0] = '\0';
            p->Sout->size = 0;
            return OK;
        }
        p->keyidx = k;
        p->counter = handle->counter;
        p->lastkey = key;
    } 
    char *value = kh_val(h, k);
    int32_t valuesize = strlen(value);
    if ((int)sizeof(p->Sout->data) < valuesize) {
        p->Sout->data = csound->ReAlloc(csound, p->Sout->data, valuesize+STR_OVERALLOC);
    }
    strncpy0(p->Sout->data, value, valuesize);
    p->Sout->size = valuesize;
    return OK;
}


// ------------------------------
//           FREE
// -------------------------------

/*

  dict_free ihandle, iwhen (0=at init time, 1=end of note)

  modelled after ftfree
  
  dict_free can only be called with a global hashtable (isglobal=1)
  
*/
typedef struct {
    OPDS h;
    MYFLT *ihandle;
    MYFLT *iwhen;
} HASHTAB_FREE;

static
int32_t hashtab_free_callback(CSOUND *csound, HASHTAB_FREE *p) {
    int32_t idx = (int32_t)*p->ihandle;
    KHASH_GLOBALS *g = hashtab_globals(csound);
    if(g->handles[idx].khashptr == NULL) {
        PERFERR(Str("dict free: instance does not exist!"));
        return NOTOK;
    }
    return _hashtab_free(csound, g, idx);
}

static
int32_t hashtab_free(CSOUND *csound, HASHTAB_FREE *p) {
    // modeled after ftfree - works at init time 
    // only global hashtables can be freed in this way. for local hashtables, the
    // canonical way is to free them at the end of the note
    int32_t idx = (int32_t)*p->ihandle;
    KHASH_GLOBALS *g = hashtab_globals(csound);
    KHASH_HANDLE *handle = &(g->handles[idx]);
    if(handle->khashptr == NULL) {
        PERFERR(Str("dict free: instance does not exist"));
        return NOTOK;
    }
    if(handle->isglobal == 0) {
        PERFERR(Str("dict free: only global dicts can be freed"));
        return NOTOK;
    }
    if(*p->iwhen == 0) {
        return _hashtab_free(csound, g, idx);
    }
    // free at the end of the note
    csound->RegisterDeinitCallback(csound, p, (int32_t (*)(CSOUND *, void*)) hashtab_free_callback);
    return OK;
}

// -----------------------------------
//            PRINT
// -----------------------------------

// dict_print ihandle, [ktrig]

typedef struct {
    OPDS h;
    MYFLT *ihandle;
    MYFLT *ktrig;
    KHASH_GLOBALS *globals;
    MYFLT lasttrig;
} HASHTAB_PRINT;

#define HASHTAB_PRINT_MAXLINE 1024

static
int32_t _hashtab_print(CSOUND *csound, HASHTAB_PRINT *p, KHASH_HANDLE *handle) {
    int khtype = handle->khtype;
    khint_t k;
    int32_t charswritten = 0;
    const int32_t linelength = 80;    
    char currline[HASHTAB_PRINT_MAXLINE];
    if(khtype == khIntFloat) {
        khash_t(khIntFloat) *h = handle->khashptr;
        for(k = kh_begin(h); k != kh_end(h); ++k) {
            if(!kh_exist(h, k)) continue;            
            charswritten += sprintf(currline+charswritten, "%d: %.5f", kh_key(h, k), kh_value(h, k));
            if(charswritten < linelength) {
                currline[charswritten++] = '\t';
            } else {
                currline[charswritten+1] = '\0';
                csound->MessageS(csound, CSOUNDMSG_ORCH, "%s\n", (char*)currline);
                charswritten = 0;
            }
        }
    } else if(khtype == khIntStr) {
        khash_t(khIntStr) *h = handle->khashptr;
        for(k = kh_begin(h); k != kh_end(h); ++k) {
            if(!kh_exist(h, k)) continue;            
            charswritten += sprintf(currline+charswritten, "%d: %s", kh_key(h, k), kh_val(h, k));
            if(charswritten < linelength) {
                currline[charswritten++] = '\t';
            } else {
                currline[charswritten+1] = '\0';
                csound->MessageS(csound, CSOUNDMSG_ORCH, "%s\n", (char*)currline);
                charswritten = 0;
            }
        }
    } else if(khtype == khStrFloat) {
        khash_t(khStrFloat) *h = handle->khashptr;
        for(k = kh_begin(h); k != kh_end(h); ++k) {
            if(!kh_exist(h, k)) continue;                      
            charswritten += sprintf(currline+charswritten, "%s: %.5f", kh_key(h, k), kh_val(h, k));
            if(charswritten < linelength) {
                currline[charswritten++] = '\t';
            } else {
                currline[charswritten+1] = '\0';
                csound->MessageS(csound, CSOUNDMSG_ORCH, "%s\n", (char*)currline);
                charswritten = 0;
            }
        }
    } else if(khtype == khStrStr) {
        khash_t(khStrStr) *h = handle->khashptr;
        for(k = kh_begin(h); k != kh_end(h); ++k) {
            if(!kh_exist(h, k)) continue;                      
            charswritten += sprintf(currline+charswritten, "%s: %s", kh_key(h, k), kh_val(h, k));
            if(charswritten < linelength) {
                currline[charswritten++] = '\t';
            } else {
                currline[charswritten+1] = '\0';
                csound->MessageS(csound, CSOUNDMSG_ORCH, "%s\n", (char*)currline);
                charswritten = 0;
            }
        }
    } else {
        PERFERR(Str("dict: format not supported"));
        return NOTOK;
    }
    // last line
    if(charswritten > 0) {
        currline[charswritten] = '\0';
        csound->MessageS(csound, CSOUNDMSG_ORCH, "%s\n", (char*)currline);
    }
    return OK;
}

static
int32_t hashtab_print_i(CSOUND *csound, HASHTAB_PRINT *p) {
    KHASH_GLOBALS *g = hashtab_globals(csound);
    int idx = (int)*p->ihandle;
    KHASH_HANDLE *handle = &(g->handles[idx]);
    _hashtab_print(csound, p, handle);
    return OK;
}

static
int32_t hashtab_print_k_init(CSOUND *csound, HASHTAB_PRINT *p) {
    p->globals = hashtab_globals(csound);
    p->lasttrig = 0;
    return OK;
}

static
int32_t hashtab_print_k(CSOUND *csound, HASHTAB_PRINT *p) {
    KHASH_GLOBALS *g = p->globals;
    int idx = (int)*p->ihandle;
    KHASH_HANDLE *handle = &(g->handles[idx]);
    int32_t trig = *p->ktrig;
    if(handle->khashptr == NULL) {
        PERFERR(Str("dict does not exist"));
        return NOTOK;
    } 
    if(trig == -1 || (trig > 0 && trig > p->lasttrig)) {
        p->lasttrig = trig;
        _hashtab_print(csound, p, handle);
    }
    return OK;
}


typedef struct {
    OPDS h;
    MYFLT *out;
    MYFLT *ihandle;
} HASHTAB_GETTYPE;


static
int32_t hashtab_gettype(CSOUND *csound, HASHTAB_GETTYPE *p) {
    KHASH_GLOBALS *g = hashtab_globals(csound);
    int idx = (int)*p->ihandle;
    KHASH_HANDLE *handle = &(g->handles[idx]);
    *p->out = handle->khashptr == NULL ? 0 : handle->khtype;
    return OK;
}



#define S(x) sizeof(x)

static OENTRY localops[] = {
    { "dict_new", S(HASHTAB_NEW), 0, 1, "i", "So", (SUBR)hashtab_init },
    { "dict_free",S(HASHTAB_FREE),   0, 1, "", "io",   (SUBR)hashtab_free},
    
    { "dict_set", S(HASHTAB_SET_sf), 0, 3, "",  "iSk",  (SUBR)hashtab_set_sf_init, (SUBR)hashtab_set_sf },
    { "dict_set", S(HASHTAB_SET_sf), 0, 1, "",  "iSi",  (SUBR)hashtab_set_sf_i},    
    { "dict_get", S(HASHTAB_GET_sf), 0, 3, "k", "iSO", (SUBR)hashtab_get_sf_init, (SUBR)hashtab_get_sf },
    { "dict_get", S(HASHTAB_GET_sf), 0, 1, "i", "iSo", (SUBR)hashtab_get_sf_i},
    
    { "dict_set", S(HASHTAB_SET_ss), 0, 3, "",  "iSS",  (SUBR)hashtab_set_ss_init, (SUBR)hashtab_set_ss },
    { "dict_get", S(HASHTAB_GET_ss), 0, 3, "S", "iS",  (SUBR)hashtab_get_ss_init, (SUBR)hashtab_get_ss },

    { "dict_set", S(HASHTAB_SET_if), 0, 3, "",  "ikk",  (SUBR)hashtab_set_if_init, (SUBR)hashtab_set_if },
    { "dict_set", S(HASHTAB_SET_if), 0, 1, "",  "iii",  (SUBR)hashtab_set_if_i},
    { "dict_get", S(HASHTAB_GET_if), 0, 3, "k", "ikO", (SUBR)hashtab_get_if_init, (SUBR)hashtab_get_if },
    { "dict_get", S(HASHTAB_GET_if), 0, 1, "k", "iio", (SUBR)hashtab_get_if_i},

    { "dict_set", S(HASHTAB_SET_is), 0, 3, "",  "ikS",  (SUBR)hashtab_set_is_init, (SUBR)hashtab_set_is },
    { "dict_get", S(HASHTAB_GET_is), 0, 3, "S", "ik",  (SUBR)hashtab_get_is_init, (SUBR)hashtab_get_is },
 
    { "dict_del", S(HASHTAB_DEL_i),   0, 2, "", "ik",   NULL, (SUBR)hashtab_del_i },
    { "dict_del", S(HASHTAB_DEL_i),   0, 1, "", "ii",   NULL, (SUBR)hashtab_del_i },
    
    { "dict_del", S(HASHTAB_DEL_s),   0, 2, "", "iS",   NULL, (SUBR)hashtab_del_s },

    { "dict_print", S(HASHTAB_PRINT), 0, 1, "", "i",  (SUBR)hashtab_print_i},
    { "dict_print", S(HASHTAB_PRINT), 0, 3, "", "ik", (SUBR)hashtab_print_k_init, (SUBR)hashtab_print_k},

    { "dict_type", S(HASHTAB_GETTYPE), 0, 1, "i", "i", (SUBR)hashtab_gettype },
    { "dict_type", S(HASHTAB_GETTYPE), 0, 2, "k", "k", NULL, (SUBR)hashtab_gettype },
    
};

LINKAGE
