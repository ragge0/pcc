/*	$Id$	*/
/*
 * Copyright(C) Caldera International Inc. 2001-2002. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code and documentation must retain the above
 * copyright notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditionsand the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 * 	This product includes software developed or owned by Caldera
 *	International, Inc.
 * Neither the name of Caldera International, Inc. nor the names of other
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * USE OF THE SOFTWARE PROVIDED FOR UNDER THIS LICENSE BY CALDERA
 * INTERNATIONAL, INC. AND CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL CALDERA INTERNATIONAL, INC. BE LIABLE
 * FOR ANY DIRECT, INDIRECT INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OFLIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/types.h>

#ifndef MKEXT
#include "external.h"
#else
typedef unsigned int bittype; /* XXX - for basicblock */
#define	BIT2BYTE(a)	(((a) + 31) / 32)
#endif
#include "manifest.h"

/* cookies, used as arguments to codgen */
#define FOREFF	01		/* compute for effects only */
#define INAREG	02		/* compute into a register */
#define INBREG	04		/* compute into a register */
#define INCREG	010		/* compute into a register */
#define INDREG	020		/* compute into a register */
#define	INREGS	(INAREG|INBREG|INCREG|INDREG)
#define FORCC	040		/* compute for condition codes only */
#define QUIET	0100		/* tell geninsn() to not complain if fail */
#define INTEMP	010000		/* compute into a temporary location */
#define FORREW	040000		/* search the table for a rewrite rule */
#define INEREG	0x10000		/* compute into a register, > 16 bits */
#define INFREG	0x20000		/* compute into a register, > 16 bits */
#define INGREG	0x40000		/* compute into a register, > 16 bits */

/*
 * OP descriptors,
 * the ASG operator may be used on some of these
 */
#define OPSIMP	010000		/* +, -, &, |, ^ */
#define OPCOMM	010002		/* +, &, |, ^ */
#define OPMUL	010004		/* *, / */
#define OPDIV	010006		/* /, % */
#define OPUNARY	010010		/* unary ops */
#define OPLEAF	010012		/* leaves */
#define OPANY	010014		/* any op... */
#define OPLOG	010016		/* logical ops */
#define OPFLOAT	010020		/* +, -, *, or / (for floats) */
#define OPSHFT	010022		/* <<, >> */
#define OPLTYPE	010024		/* leaf type nodes (e.g, NAME, ICON, etc.) */

/* shapes */
#ifdef NEWSHAPES
struct shape {
	int op;
	CONSZ left;
	CONSZ right;
};
extern struct shape *shlst[];
struct shape **o2nsh(int osh);
int n2osh(struct shape **);
extern struct shape shareg, shbreg, shcreg, shdreg, shereg, shfreg, shgreg;
extern struct shape shoreg, shname, shicon, shone, shzero;
#define	SHTY	struct shape **
#else
#define	o2nsh(osh)	osh
#define	n2osh(sh)	sh
#define	SHTY	int
#endif

#define SANY	01		/* same as FOREFF */
#define SAREG	02		/* same as INAREG */
#define SBREG	04		/* same as INBREG */
#define SCREG	010		/* same as INCREG */
#define SDREG	020		/* same as INDREG */
#define SCC	040		/* same as FORCC */
#define SNAME	0100
#define SCON	0200
#define SFLD	0400
#define SOREG	01000
#define STARNM	02000
#define STARREG	04000
#define SWADD	040000
#define SPECIAL	0100000
#define SZERO	SPECIAL
#define SONE	(SPECIAL|1)
#define SMONE	(SPECIAL|2)
#define SCCON	(SPECIAL|3)	/* -256 <= constant < 256 */
#define SSCON	(SPECIAL|4)	/* -32768 <= constant < 32768 */
#define SSOREG	(SPECIAL|5)	/* non-indexed OREG */
#define	MAXSPECIAL	(SPECIAL|5)
#define SEREG	0x10000		/* same as INEREG */
#define SFREG	0x20000		/* same as INFREG */
#define SGREG	0x40000		/* same as INGREG */

/* These are used in rstatus[] in conjunction with SxREG */
#define	TEMPREG	01000
#define	PERMREG	02000
#define	PREMREG	06000

/* tshape() return values */
#define	SRNOPE	0		/* Cannot match any shape */
#define	SRDIR	1		/* Direct match */
#define	SROREG	2		/* Can convert into OREG */
#define	SRREG	3		/* Must put into REG */

/* find*() return values */
#define	FRETRY	-2
#define	FFAIL	-1

/* INTEMP is carefully not conflicting with shapes */

/* types */
#define TCHAR		01	/* char */
#define TSHORT		02	/* short */
#define TINT		04	/* int */
#define TLONG		010	/* long */
#define TFLOAT		020	/* float */
#define TDOUBLE		040	/* double */
#define TPOINT		0100	/* pointer to something */
#define TUCHAR		0200	/* unsigned char */
#define TUSHORT		0400	/* unsigned short */
#define TUNSIGNED	01000	/* unsigned int */
#define TULONG		02000	/* unsigned long */
#define TPTRTO		04000	/* pointer to one of the above */
#define TANY		010000	/* matches anything within reason */
#define TSTRUCT		020000	/* structure or union */
#define	TLONGLONG	040000	/* long long */
#define	TULONGLONG	0100000	/* unsigned long long */
#define	TLDOUBLE	0200000	/* long double; exceeds 16 bit */
#define	TFTN		0400000	/* function pointer; exceeds 16 bit */

/* reclamation cookies */
#define RNULL		0	/* clobber result */
#define RLEFT		01
#define RRIGHT		02
#define RESC1		04
#define RESC2		010
#define RESC3		020
#define RDEST		040
#define RESCC		04000
#define RNOP		010000	/* DANGER: can cause loops.. */

enum { cNREG = 1, cNTL, cNTR, cNSL, cNSR, cNL, cNR, cNOL, cNOR,
	cNEVER, cNRES, cNTEMP, cNREW };
#define	NEEDS(...)	(char []){ __VA_ARGS__, 0 }
#define	NEEDADD(x)	(x == cNREG ? 3 : x < cNSL ? 1 : 2)
#define	NREG(cl,num)	cNREG, CLASS ## cl, num
#define	NTL		cNTL
#define	NTR		cNTR
#define	NSL(cl)		cNSL, CLASS ## cl
#define	NSR(cl)		cNSR, CLASS ## cl
#define	NLEFT(reg)	cNL, reg
#define	NRIGHT(reg)	cNR, reg
#define	NOLEFT(reg)	cNOL, reg
#define	NORIGHT(reg)	cNOR, reg
#define	NEVER(reg)	cNEVER, reg
#define	NRES(reg)	cNRES, reg
#define	NTEMP(num)	cNTEMP, num
#define	NREWRITE	cNREW, 0

#define MUSTDO		010000	/* force register requirements */
#define NOPREF		020000	/* no preference for register assignment */

#define	isreg(p)	(p->n_op == REG || p->n_op == TEMP)

extern	int fregs;

/* code tables */
extern	struct optab {
	int	op;
	int	visit;
	SHTY	lshape;
	int	ltype;
	SHTY	rshape;
	int	rtype;
	char 	*needs;
	int	rewrite;
	char	*cstring;
} table[];

struct p2env;
#define	NRESC 4
extern	NODE resc[];
extern	int p2autooff, p2maxautooff;

extern	NODE
	*talloc(void),
	*eread(void),
	*mklnode(int, CONSZ, int, TWORD),
	*mkbinode(int, NODE *, NODE *, TWORD),
	*mkunode(int, NODE *, int, TWORD),
	*getlr(NODE *p, int);

void eoftn(struct interpass_prolog *);
void prologue(struct interpass_prolog *);
void e2print(NODE *p, int down, int *a, int *b);
void myoptim(struct interpass *);
void cbgen(int op, int label);
int match(NODE *p, int cookie);
int acceptable(struct optab *);
int special(NODE *, int);
int setasg(NODE *, int);
int setuni(NODE *, int);
int sucomp(NODE *);
int nsucomp(NODE *);
int setorder(NODE *);
int geninsn(NODE *, int cookie);
void adrput(FILE *, NODE *);
void comperr(char *str, ...);
void genregs(NODE *p);
void ngenregs(struct p2env *);
NODE *store(NODE *);
struct interpass *ipnode(NODE *);
void deflab(int);
void rmove(int, int, TWORD);
int rspecial(struct optab *, int);
void printip(struct interpass *pole);
int findops(NODE *p, int);
int findasg(NODE *p, int);
int finduni(NODE *p, int);
int findumul(NODE *p, int);
int findleaf(NODE *p, int);
int relops(NODE *p);
#ifdef FINDMOPS
int findmops(NODE *p, int);
int treecmp(NODE *p1, NODE *p2);
#endif
void offstar(NODE *p, int shape);
int gclass(TWORD);
void lastcall(NODE *);
void myreader(struct interpass *pole);
int oregok(NODE *p, int sharp);
void myormake(NODE *);
int *livecall(NODE *);
void prtreg(NODE *);
char *prcook(int);
int myxasm(struct interpass *ip, NODE *p);
int xasmcode(char *s);
int freetemp(int k);
NODE *storenode(TWORD t, int k);
void storemod(NODE *, int k, int reg);
int rewfld(NODE *p);
void canon(NODE *);
void mycanon(NODE *);
void oreg2(NODE *p, void *);
int shumul(NODE *p, int);
NODE *deluseless(NODE *p);
int getlab2(void);
int tshape(NODE *, int);
void conput(FILE *, NODE *);
int shtemp(NODE *p);
int ttype(TWORD t, int tword);
void expand(NODE *, int, char *);
void hopcode(int, int);
void adrcon(CONSZ);
void zzzcode(NODE *, int);
void insput(NODE *);
void upput(NODE *, int);
int tlen(NODE *p);
int setbin(NODE *);
int notoff(TWORD, int, CONSZ, char *);
int fldexpand(NODE *, int, char **);
int flshape(NODE *p);
int ncnt(char *);
char *hasneed(char *, int);
char *hasneed2(char *, int, int);
void mainp2(void);

extern	char *rnames[];

extern int classmask(int), tclassmask(int);
extern void cmapinit(void);
extern int aliasmap(int adjclass, int regnum);
extern int regK[];
#define	CLASSA	1
#define	CLASSB	2
#define	CLASSC	3
#define	CLASSD	4
#define	CLASSE	5
#define	CLASSF	6
#define	CLASSG	7

/* used when parsing xasm codes */
#define	XASMVAL(x)	((x) & 0377)		/* get val from codeword */
#define	XASMVAL1(x)	(((x) >> 16) & 0377)	/* get val from codeword */
#define	XASMVAL2(x)	(((x) >> 24) & 0377)	/* get val from codeword */
#define	XASMASG		0x100	/* = */
#define	XASMCONSTR	0x200	/* & */
#define	XASMINOUT	0x400	/* + */
#define XASMALL		(XASMASG|XASMCONSTR|XASMINOUT)
#define	XASMISINP(cw)	(((cw) & XASMASG) == 0)		/* input operand */
#define	XASMISOUT(cw)	((cw) & (XASMASG|XASMINOUT))	/* output operand */

/* routines to handle double indirection */
#ifdef R2REGS
void makeor2(NODE *p, NODE *q, int, int);
int base(NODE *);
int offset(NODE *p, int);
#endif

extern	int lineno;
extern	int ndebug;
extern	int b2debug, c2debug, e2debug, f2debug, g2debug, o2debug;
extern	int r2debug, s2debug, t2debug, u2debug, x2debug;

extern	int dope[];	/* a vector containing operator information */
extern	char *opst[];	/* a vector containing names for ops */

#ifdef PCC_DEBUG

static inline int
optype(int o)
{
	if (o >= MAXOP+1)
		cerror("optype");
	return (dope[o]&TYFLG);
}

static inline int
asgop(int o)
{
	if (o >= MAXOP+1)
		cerror("asgop");
	return (dope[o]&ASGFLG);
}

static inline int
logop(int o)
{
	if (o >= MAXOP+1)
		cerror("logop");
	return (dope[o]&LOGFLG);
}

static inline int
callop(int o)
{
	if (o >= MAXOP+1)
		cerror("callop");
	return (dope[o]&CALLFLG);
}

#else

#define optype(o)	(dope[o]&TYFLG)
#define asgop(o)	(dope[o]&ASGFLG) 
#define logop(o)	(dope[o]&LOGFLG)
#define callop(o)	(dope[o]&CALLFLG)

#endif

	/* macros for doing double indexing */
#define R2PACK(x,y,z)	(0200*((x)+1)+y+040000*z)
#define R2UPK1(x)	((((x)>>7)-1)&0177)
#define R2UPK2(x)	((x)&0177)
#define R2UPK3(x)	(x>>14)
#define R2TEST(x)	((x)>=0200)

/*
 * Layout of findops() return value:
 *      bit 0 whether left shall go into a register.
 *      bit 1 whether right shall go into a register.
 *      bit 2 entry is only used for side effects.
 *      bit 3 if condition codes are used.
 *
 * These values should be synced with FOREFF/FORCC.
 */
#define ISMOPS		001
#define RREG		002
#define	RVEFF		004
#define	RVCC		010
#define DORIGHT		020
#define	SCLASS(v,x)	((v) |= ((x) << 5))
#define	TCLASS(x)	(((x) >> 5) & 7)
#define	TBSH		8
#define TBLIDX(idx)	((idx) >> TBSH)
#define MKIDX(tbl,mod)	(((tbl) << TBSH) | (mod))

#ifndef	BREGS
#define	BREGS	0
#define	TBREGS	0
#endif
#define	REGBIT(x) (1 << (x))
#ifndef PERMTYPE
#define	PERMTYPE(a)	(INT)
#endif

/* Flags for the dataflow code */
/* do the live/dead analysis */
#define DO_LIVEDEAD  0x01
/* compute avail expressions */
#define DO_AVAILEXPR 0x02
/* Do an update on the live/dead. One variable only */
#define DO_UPDATELD  0x04
/* Do an update on available expressions, one variable has changed */
#define DO_UPDATEEX  0x08

void emit(struct interpass *);
void optimize(struct p2env *);

struct basicblock {
	DLIST_ENTRY(basicblock) bbelem;
	SLIST_HEAD(, cfgnode) parents;	/* CFG - parents to this node */
	SLIST_HEAD(, cfgnode) child;	/* Children, usually max 2 of them */
	int bbnum;	/* this basic block number */
	unsigned int dfnum; /* DFS-number */
	unsigned int dfparent; /* Parent in DFS */
	unsigned int semi;
	unsigned int ancestor;
	unsigned int idom;
	unsigned int samedom;
	bittype *bucket;
	bittype *df;
	bittype *dfchildren;
	bittype *Aorig;
	bittype *Aphi;
	SLIST_HEAD(, phiinfo) phi;

	bittype *gen, *killed, *in, *out;	/* Liveness analysis */

	struct interpass *first; /* first element of basic block */
	struct interpass *last;  /* last element of basic block */
};

struct labelinfo {
	struct basicblock **arr;
	int size;
	unsigned int low;
};

struct bblockinfo {
	int size;
	struct basicblock **arr;
};

struct varinfo {
	struct pvarinfo **arr;
	SLIST_HEAD(, varstack) *stack;
	int size;
	int low;
};

struct pvarinfo {
	struct pvarinfo *next;
	struct basicblock *bb;
	TWORD n_type;
};

struct varstack {
	SLIST_ENTRY(varstack) varstackelem;
	int tmpregno;
};


struct cfgnode {
	SLIST_ENTRY(cfgnode) cfgelem;
	SLIST_ENTRY(cfgnode) chld;
	struct basicblock *bblock;
};

struct phiinfo {
	SLIST_ENTRY(phiinfo) phielem;
	int tmpregno;
	int newtmpregno;
	TWORD n_type;
	int size;
	int *intmpregno;
};

/*
 * Description of the pass2 environment.
 * There will be only one of these structs.  It is used to keep
 * all state descriptions during the compilation of a function
 * in one place.
 */
struct p2env {
	struct interpass ipole;			/* all statements */
	struct interpass_prolog *ipp, *epp;	/* quick references */
	struct bblockinfo bbinfo;
	struct labelinfo labinfo;
	struct basicblock bblocks;
	int nbblocks;
#define NIPPREGS        BIT2BYTE(MAXREGS)/sizeof(bittype)
	bittype p_regs[NIPPREGS];	/* Bitmask of registers to save */
};

extern struct p2env p2env;

/*
 * C compiler second pass extra defines.
 */
#define PHI (MAXOP + 1)		/* Used in SSA trees */
#define SPECON (MAXOP + 2)

enum {
	ATTR_P2_FIRST = ATTR_MI_MAX + 1,
	ATTR_P2STRUCT,
#ifdef ATTR_P2_TARGET
	ATTR_P2_TARGET,
#endif
	ATTR_P2_MAX
};
