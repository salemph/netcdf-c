/*********************************************************************
 *   Copyright 2018, UCAR/Unidata
 *   See netcdf/COPYRIGHT file for copying and redistribution conditions.
 *   $Header: /upc/share/CVS/netcdf-3/ncgen/util.c,v 1.4 2010/04/14 22:04:59 dmh Exp $
 *********************************************************************/

#include "includes.h"

/* Track primitive symbol instances (initialized in ncgen.y) */
Symbol* primsymbols[PRIMNO];

char*
append(const char* s1, const char* s2)
{
    int len = (s1?strlen(s1):0)+(s2?strlen(s2):0);
    char* result = (char*)ecalloc(len+1);
    result[0] = '\0';
    if(s1) strcat(result,s1);
    if(s2) strcat(result,s2);
    return result;
}


unsigned int
chartohex(char c)
{
    switch (c) {
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
            return (c - '0');
        case 'A': case 'B': case 'C':
        case 'D': case 'E': case 'F':
            return (c - 'A') + 0x0a;
        case 'a': case 'b': case 'c':
        case 'd': case 'e': case 'f':
            return (c - 'a') + 0x0a;
    }
    return 0;
}

/*
 * For generated Fortran, change 'e' to 'd' in exponent of double precision
 * constants.
 */
void
expe2d(
    char *cp)			/* string containing double constant */
{
    char *expchar = strrchr(cp,'e');
    if (expchar) {
	*expchar = 'd';
    }
}

/* Returns non-zero if n is a power of 2, 0 otherwise */
int
pow2(
     int n)
{
  int m = n;
  int p = 1;

  while (m > 0) {
    m /= 2;
    p *= 2;
  }
  return p == 2*n;
}


/*
 * Remove trailing zeros (after decimal point) but not trailing decimal
 * point from ss, a string representation of a floating-point number that
 * might include an exponent part.
 */
void
tztrim(
    char *ss			/* returned string representing dd */
    )
{
    char *cp, *ep;

    cp = ss;
    if (*cp == '-')
      cp++;
    while(isdigit((int)*cp) || *cp == '.')
      cp++;
    if (*--cp == '.')
      return;
    ep = cp+1;
    while (*cp == '0')
      cp--;
    cp++;
    if (cp == ep)
      return;
    while (*ep)
      *cp++ = *ep++;
    *cp = '\0';
    return;
}

static void
clearSpecialdata(Specialdata* data)
{
    if(data == NULL) return;
    reclaimdatalist(data->_Fillvalue);
    if(data->_ChunkSizes)
        efree(data->_ChunkSizes);
    if(data->_Filters) {
	int i;
	for(i=0;i<data->nfilters;i++) {
	    NC_H5_Filterspec* f = data->_Filters[i];
	    ncaux_h5filterspec_free(f);
	}
	efree(data->_Filters);
    }
    if(data->_Codecs)
        efree(data->_Codecs);
}

void
freeSymbol(Symbol* sym)
{
    if(sym == NULL) return;
    switch (sym->objectclass) {
    case NC_VAR:
	clearSpecialdata(&sym->var.special);
	listfree(sym->var.attributes);
	break;
    case NC_TYPE:
	if(sym->typ.econst)
	    reclaimconstant(sym->typ.econst);
	if(sym->typ._Fillvalue)
	    reclaimdatalist(sym->typ._Fillvalue);
	break;
    case NC_GRP:
        if(sym->file.filename)
	    efree(sym->file.filename);
	break;
    default: break;
    }
    /* Universal */
    if(sym->name) efree(sym->name);
    if(sym->fqn) efree(sym->fqn);
    listfree(sym->prefix);
    if(sym->data)
        reclaimdatalist(sym->data);
    listfree(sym->subnodes);
    efree(sym);
}

char* nctypenames[17] = {
"NC_NAT",
"NC_BYTE", "NC_CHAR", "NC_SHORT", "NC_INT",
"NC_FLOAT", "NC_DOUBLE",
"NC_UBYTE", "NC_USHORT", "NC_UINT",
"NC_INT64", "NC_UINT64",
"NC_STRING",
"NC_VLEN", "NC_OPAQUE", "NC_ENUM", "NC_COMPOUND"
};

char* nctypenamesextend[9] = {
"NC_GRP", "NC_DIM", "NC_VAR", "NC_ATT", "NC_TYPE",
"NC_ECONST","NC_FIELD", "NC_ARRAY","NC_PRIM"
};

char*
nctypename(nc_type nctype)
{
    char* s;
    if(nctype >= NC_NAT && nctype <= NC_COMPOUND)
	return nctypenames[nctype];
    if(nctype >= NC_GRP && nctype <= NC_PRIM)
	return nctypenamesextend[(nctype - NC_GRP)];
    if(nctype == NC_FILLVALUE) return "NC_FILL";
    if(nctype == NC_NIL) return "NC_NIL";
    const size_t s_size = 128;
    s = poolalloc(s_size);
    snprintf(s,s_size,"NC_<%d>",nctype);
    return s;
}

/* These are the augmented NC_ values (0 based from NC_GRP)*/
char* ncclassnames[9] = {
"NC_GRP", "NC_DIM", "NC_VAR", "NC_ATT",
"NC_TYP", "NC_ECONST", "NC_FIELD", "NC_ARRAY",
"NC_PRIM"
};

char*
ncclassname(nc_class ncc)
{
    char* s;
    if(ncc >= NC_NAT && ncc <= NC_COMPOUND)
	return nctypename((nc_type)ncc);
    if(ncc == NC_FILLVALUE) return "NC_FILL";
    if(ncc >= NC_GRP && ncc <= NC_PRIM)
	return ncclassnames[ncc - NC_GRP];
    const size_t s_size = 128;
    s = poolalloc(s_size);
    snprintf(s,s_size,"NC_<%d>",ncc);
    return s;
}

int ncsizes[17] = {
0,
1,1,2,4,
4,8,
1,2,4,
8,8,
sizeof(char*),
sizeof(nc_vlen_t),
0,0,0
};

int
ncsize(nc_type nctype)
{
    if(nctype >= NC_NAT && nctype <= NC_COMPOUND)
	return ncsizes[nctype];
    return 0;
}

int
hasunlimited(Dimset* dimset)
{
    int i;
    for(i=0;i<dimset->ndims;i++) {
	Symbol* dim = dimset->dimsyms[i];
	if(dim->dim.declsize == NC_UNLIMITED) return 1;
    }
    return 0;
}

/* return 1 if first dimension is unlimited*/
int
isunlimited0(Dimset* dimset)
{
   return (dimset->ndims > 0 && dimset->dimsyms[0]->dim.declsize == NC_UNLIMITED);
}


/* True only if dim[0] is unlimited all rest are bounded*/
/* or all are bounded*/
int
classicunlimited(Dimset* dimset)
{
    int i;
    int last = -1;
    for(i=0;i<dimset->ndims;i++) {
	Symbol* dim = dimset->dimsyms[i];
	if(dim->dim.declsize == NC_UNLIMITED) last = i;
    }
    return (last < 1);
}

/* True only iff no dimension is unlimited*/
int
isbounded(Dimset* dimset)
{
    int i;
    for(i=0;i<dimset->ndims;i++) {
	Symbol* dim = dimset->dimsyms[i];
	if(dim->dim.declsize == NC_UNLIMITED) return 0;
    }
    return 1;
}

int
signedtype(nc_type nctype)
{
    switch (nctype) {
    case NC_BYTE:
    case NC_SHORT:
    case NC_INT:
    case NC_INT64:
	return nctype;
    case NC_UBYTE: return NC_BYTE;
    case NC_USHORT: return NC_SHORT;
    case NC_UINT: return NC_INT;
    case NC_UINT64: return NC_INT64;
    default: break;
    }
    return nctype;
}

int
unsignedtype(nc_type nctype)
{
    switch (nctype) {
    case NC_UBYTE:
    case NC_USHORT:
    case NC_UINT:
    case NC_UINT64:
	return nctype;
    case NC_BYTE: return NC_UBYTE;
    case NC_SHORT: return NC_USHORT;
    case NC_INT: return NC_UINT;
    case NC_INT64: return NC_UINT64;
    default: break;
    }
    return nctype;
}

int
isinttype(nc_type nctype)
{
    return (nctype != NC_CHAR)
            && ((nctype >= NC_BYTE && nctype <= NC_INT)
	        || (nctype >= NC_UBYTE && nctype <= NC_UINT64));
}

int
isuinttype(nc_type t)
{
    return isinttype(t)
	   && t >= NC_UBYTE
           && t <= NC_UINT64
           && t != NC_INT64;
}

int
isfloattype(nc_type nctype)
{
    return (nctype == NC_FLOAT || nctype <= NC_DOUBLE);
}

int
isclassicprim(nc_type nctype)
{
    return    (nctype >= NC_BYTE && nctype <= NC_DOUBLE)
	;
}

int
isclassicprimplus(nc_type nctype)
{
    return    (nctype >= NC_BYTE && nctype <= NC_DOUBLE)
	   || (nctype == NC_STRING)
	;
}

int
isprim(nc_type nctype)
{
    return    (nctype >= NC_BYTE && nctype <= NC_STRING)
	;
}

int
isprimplus(nc_type nctype)
{
    return    (nctype >= NC_BYTE && nctype <= NC_STRING)
	   || (nctype == NC_ECONST)
	   || (nctype == NC_OPAQUE)
	 ;
}

void
collectpath(Symbol* grp, List* grpstack)
{
    while(grp != NULL) {
        listpush(grpstack,(void*)grp);
	grp = grp->container;
    }
}


#ifdef USE_NETCDF4
/* Result is pool'd*/
char*
prefixtostring(List* prefix, char* separator)
{
    int slen=0;
    int plen;
    int i;
    char* result;
    if(prefix == NULL) return pooldup("");
    plen = prefixlen(prefix);
    if(plen == 0) { /* root prefix*/
	slen=0;
        /* slen += strlen(separator);*/
        slen++; /* for null terminator*/
        result = poolalloc(slen);
        result[0] = '\0';
	/*strcat(result,separator);*/
    } else {
        for(i=0;i<plen;i++) {
	    Symbol* sym = (Symbol*)listget(prefix,i);
            slen += (strlen(separator)+strlen(sym->name));
	}
        slen++; /* for null terminator*/
        result = poolalloc(slen);
        result[0] = '\0';
        for(i=0;i<plen;i++) {
	    Symbol* sym = (Symbol*)listget(prefix,i);
            strcat(result,separator);
	    strcat(result,sym->name); /* append "/<prefix[i]>"*/
	}
    }
    return result;
}
#endif

/* Result is pool'd*/
char*
fullname(Symbol* sym)
{
#ifdef USE_NETCDF4
    char* s1;
    char* result;
    char* prefix;
    prefix = prefixtostring(sym->prefix,PATHSEPARATOR);
    s1 = poolcat(prefix,PATHSEPARATOR);
    result = poolcat(s1,sym->name);
    return result;
#else
    return nulldup(sym->name);
#endif
}

int
prefixeq(List* x1, List* x2)
{
    Symbol** l1;
    Symbol** l2;
    int len,i;
    if((len=listlength(x1)) != listlength(x2)) return 0;
    l1=(Symbol**)listcontents(x1);
    l2=(Symbol**)listcontents(x2);
    for(i=0;i<len;i++) {
        if(strcmp(l1[i]->name,l2[i]->name) != 0) return 0;
    }
    return 1;
}

List*
prefixdup(List* prefix)
{
    List* dupseq;
    int i;
    if(prefix == NULL) return listnew();
    dupseq = listnew();
    listsetalloc(dupseq,listlength(prefix));
    for(i=0;i<listlength(prefix);i++) listpush(dupseq,listget(prefix,i));
    return dupseq;
}

/*
Many of the generate routines need to construct
heap strings for short periods. Remembering to
free such space is error prone, so provide a
pseudo-GC to handle these short term requests.
The idea is to have a fixed size pool
tracking malloc requests and automatically
releasing when the pool gets full.
*/

/* Max number of allocated pool items*/
#define POOLMAX 100

static char* pool[POOLMAX];
static int poolindex = -1;
#define POOL_DEFAULT 256

char*
poolalloc(size_t length)
{
    if(poolindex == -1) { /* initialize*/
	memset((void*)pool,0,sizeof(pool));
	poolindex = 0;
    }
    if(poolindex == POOLMAX) poolindex=0;
    if(length == 0) length = POOL_DEFAULT;
    if(pool[poolindex] != NULL) efree(pool[poolindex]);
    pool[poolindex] = (char*)ecalloc(length);
    return pool[poolindex++];
}

char*
pooldup(const char* s)
{
    char* sdup = poolalloc(strlen(s)+1);
    strncpy(sdup,s,(strlen(s)+1));
    return sdup;
}

char*
poolcat(const char* s1, const char* s2)
{
    int len1, len2;
    char* cat;
    if(s1 == NULL && s2 == NULL) return NULL;
    len1 = (s1?strlen(s1):0);
    len2 = (s2?strlen(s2):0);
    cat = poolalloc(len1+len2+1);
    cat[0] = '\0';
    if(s1 != NULL) strcat(cat,s1);
    if(s2 != NULL) strcat(cat,s2);
    return cat;
}

/* Result is malloc'd*/
unsigned char*
makebytestring(char* s, size_t* lenp)
{
    unsigned char* bytes;
    unsigned char* b;
    size_t slen = strlen(s); /* # nibbles */
    size_t blen = slen/2; /* # bytes */
    int i;

    ASSERT((slen%2) == 0);
    ASSERT(blen > 0);
    bytes = (unsigned char*)ecalloc(blen);
    b = bytes;
    for(i=0;i<slen;i+=2) {
	unsigned int digit1 = chartohex(*s++);
	unsigned int digit2 = chartohex(*s++);
	unsigned int byte = (digit1 << 4) | digit2;
	*b++ = byte;
    }
    if(lenp) *lenp = blen;
    return bytes;
}

int
getpadding(int offset, int alignment)
{
    int rem = (alignment==0?0:(offset % alignment));
    int pad = (rem==0?0:(alignment - rem));
    return pad;
}

static void
reclaimSymbols(void)
{
    int i;
    for(i=0;i<listlength(symlist);i++) {
        Symbol* sym = listget(symlist,i);
        freeSymbol(sym);
    }
}

void
cleanup()
{
    reclaimSymbols();
    listfree(symlist);
    listfree(grpdefs);
    listfree(dimdefs);
    listfree(attdefs);
    listfree(gattdefs);
    listfree(xattdefs);
    listfree(typdefs);
    listfree(vardefs);
    filldatalist->readonly = 0;
    freedatalist(filldatalist);
}

/* compute the total n-dimensional size as 1 long array;
   if stop == 0, then stop = dimset->ndims.
*/
size_t
crossproduct(Dimset* dimset, int start, int stop)
{
    size_t totalsize = 1;
    int i;
    for(i=start;i<stop;i++) {
	totalsize = totalsize * dimset->dimsyms[i]->dim.declsize;
    }
    return totalsize;
}

/* Do the "complement" of crossproduct;
   compute the total n-dimensional size of an array
   starting at 0 thru the 'last' array index.
   stop if we encounter an unlimited dimension
*/
size_t
prefixarraylength(Dimset* dimset, int last)
{
    return crossproduct(dimset,0,last+1);
}



#ifdef USE_HDF5
extern int H5Eprint1(FILE * stream);
#endif

void
check_err(const int stat, const int line, const char* file, const char* func)
{
    check_err2(stat,-1,line,file,func);
}

void check_err2(const int stat, const int cdlline, const int line, const char* file, const char* func)
{
    if (stat != NC_NOERR) {
	if(cdlline >= 0)
	    fprintf(stderr, "ncgen: cdl line %d; %s\n", cdlline, nc_strerror(stat));
	else
	    fprintf(stderr, "ncgen: %s\n", nc_strerror(stat));
	fprintf(stderr, "\t(%s:%s:%d)\n", file,func,line);
#ifdef USE_HDF5
	H5Eprint1(stderr);
#endif
	fflush(stderr);
        finalize_netcdf(1);
    }
}

/**
Find the index of the first unlimited
dimension at or after 'start'.
If no unlimited exists, return |dimset|
*/
int
findunlimited(Dimset* dimset, int start)
{
    for(;start<dimset->ndims;start++) {
	if(dimset->dimsyms[start]->dim.isunlimited)
	    return start;
    }
    return dimset->ndims;
}

/**
Find the index of the last unlimited
dimension.
If no unlimited exists, return |dimset|
*/
int
findlastunlimited(Dimset* dimset)
{
    int i;
    for(i=dimset->ndims-1;i>=0;i--) {
	if(dimset->dimsyms[i]->dim.isunlimited)
	    return i;
    }
    return dimset->ndims;
}

/**
Count the number of unlimited dimensions.
*/
int
countunlimited(Dimset* dimset)
{
    int i, count;
    for(count=0,i=dimset->ndims-1;i>=0;i--) {
	if(dimset->dimsyms[i]->dim.isunlimited)
	    count++;
    }
    return count;
}

/* Return standard format string */
const char *
kind_string(int kind)
{
    switch (kind) {
    case 1: return "classic";
    case 2: return "64-bit offset";
    case 3: return "netCDF-4";
    case 4: return "netCDF-4 classic model";
    default:
	derror("Unknown format index: %d\n",kind);
    }
    return NULL;
}

