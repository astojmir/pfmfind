#ifndef _FS_INDEX_H
#define _FS_INDEX_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "fastadb.h"
#include "partition.h"

#include "smatrix.h"
#include "hit_list.h"
#include "misclib.h"


/********************************************************************/    
/********************************************************************/    
/***                                                              ***/
/***               PROTOTYPES                                     ***/ 
/***                                                              ***/
/********************************************************************/    
/********************************************************************/    

/********************************************************************/    
/*                                                                  */
/*                     UFRAG                                        */ 
/*                                                                  */
/********************************************************************/    

typedef union
{
  SEQ_index_t *p; /* Pointer to the array of fragments */
  SEQ_index_t s;  /* Only fragment */
} FRAG_HNDL; 

typedef struct
{
  int n;
  FRAG_HNDL frag; 
} UFRAG;

typedef struct 
{
  ULINT offset;
  int dist;
} HIT;

struct FSINDX_s;

typedef struct FSSRCH_s
{
  struct FSINDX_s *FSI;    /* Index                                 */
  char *base;              /* Start of sequence heap                */
  int eps;                 /* Cutoff value                          */
  int kNN;                 /* Number of nearest neighbours          */
  HIT_LIST_t *HL;          /* Search results                        */

  void (*ifunc) (struct FSSRCH_s *, uint32_t, int);
                           /* Hit insertion function                */
  void (*pfunc) (struct FSSRCH_s *, FS_SEQ_t); 
                           /* Bin scan function pter                */

  int qlen;                /* Length of query sequence              */
  int len;                 /* len = min(qlen, FSI->m)               */
  FS_SEQ_t qbin;           /* Query bin                             */
  int Mlen;                /* Number of coordinates                 */
  int **M;                 /* Scoring matrix, profile               */
  int *cdist;              /* Cumulative distances                  */
  int *nbpt;               /* Number of different partitions        */
  int *nbpt_closest;       /* Distance to the closest partition     */
  long **nbpt_diff;        /* FS sequence difference                */
  int **nbpt_dist;         /* Distances to different clusters       */
  int bincr;               /* Number of bins to search              */

  int *sa;                 /* Points to 'suffix array'              */
  uint8_t *lcpx;           /* Points to lcp array                   */

  HIT *hits;               /* Temporary hits                        */
  int hits_size;           /* Size of hits_size                     */
  int hits_len;            /* Number of hits stored in hits         */
  int no_hits;             /* True number of hits stored            */
  struct pqueue *p_queue;  /* Priority queue                        */
} FSSRCH; 

/********************************************************************/    
/*                                                                  */
/*                     FS_INDEX module                              */ 
/*                                                                  */
/********************************************************************/    
#if ! defined (__unix__) 
#undef THREADS
#endif

#ifndef THREADS
#define THREADS 1
#endif

typedef enum {FS_BINS, SUFFIX_ARRAY, SEQ_SCAN} SFUNC_TYPE;
typedef enum {SARRAY, DUPS_ONLY, FULL_SCAN} PFUNC_TYPE;

typedef struct FSINDX_s
{
/* FSindex variables - essentially constant throughout all searches
   with the same index .*/ 
  char *db_name;           /* Name of the database                  */
  char *index_name;        /* Name of the index                     */
  SEQUENCE_DB *s_db;       /* Pointer to the fasta database         */
  FS_TABLE *ptable;        /* Partition table                       */

  uint32_t m;              /* Maximum length of indexed fragments   */
  double dtime;            /* Creation time                         */
  uint32_t db_no_frags;    /* Number of fragments in full database  */
  uint32_t no_bins;        /* Number of bins                        */

  uint32_t no_seqs;        /* Number of indexed fragments           */
  uint32_t no_useqs;       /* Number of indexed unique fragments    */
  uint32_t shist_len;      /* Length of shist                       */
  uint32_t *shist;         /* Histogram of bin sizes                */
  uint32_t *uhist;         /* Histogram of bin sizes - unique seqs  */
  uint32_t uhist_len;      /* Length of uhist                       */

  uint32_t binL;           /* Largest bin                           */
  uint32_t uL;             /* Largest bin - unique seqs             */

  /* FSSRCH - search specific variables (one per thread) */
  FSSRCH FSS[THREADS];

  /* Suffix and lcp array */ 
  int *sa;               /* Suffix array of the sequence database */
  uint8_t *lcpx;   /* Longest common prefix                 */
  uint32_t sa_size;           /* Size of sa                            */
  int use_sa;

  /* Bins and its lcp */
  int *oa;               /* Offset array - bin order              */
  uint32_t *bins;        /* Bin start offset                      */
  uint8_t *lcpb;         /* Longest common prefix array for oa     */

} FSINDX;

FSINDX *FS_INDEX_create(const char *database, uint32_t len, const char **sepn, 
			int use_sa, int print_flag);
void FS_INDEX_destroy(FSINDX *FSI);

int FS_INDEX_save(FSINDX *FSI, const char *filename);
FSINDX *FS_INDEX_load(const char *filename);


/* Printing Functions */

char *FS_INDEX_sprint_stats(FSINDX *FSI, int options);
void FS_INDEX_fprint_stats(FSINDX *FSI, FILE *fp, int options);
char *FS_INDEX_sprint_bin(FSINDX *FSI, FS_SEQ_t bin, int options);
void FS_INDEX_fprint_bin(FSINDX *FSI, FS_SEQ_t bin, FILE *fp,
			 int options);

/* Search functions */

HIT_LIST_t *FSINDX_rng_srch(FSINDX *FSI, BIOSEQ *query, SCORE_MATRIX *M,
			    int range, int conv_type, HIT_LIST_t *HL,
			    SFUNC_TYPE stype, PFUNC_TYPE ptype);

HIT_LIST_t *FSINDX_kNN_srch(FSINDX *FSI, BIOSEQ *query, SCORE_MATRIX *M,
			    int kNN, HIT_LIST_t *HL, SFUNC_TYPE stype, 
			    PFUNC_TYPE ptype);

/* Threaded search */

typedef struct
{
  BIOSEQ query;
  SCORE_MATRIX *M;
  int range;
  int kNN;
  SFUNC_TYPE stype; 
  PFUNC_TYPE ptype;  
} SRCH_ARGS;

HIT_LIST_t *FSINDX_threaded_srch(FSINDX *FSI, SRCH_ARGS *args, int n);


/* Access functions */
uint32_t FS_INDEX_get_bin_size(FSINDX *FSI, FS_SEQ_t bin);
uint32_t FS_INDEX_get_unique_bin_size(FSINDX *FSI, FS_SEQ_t bin);
BIOSEQ *FS_INDEX_get_seq(FSINDX *FSI, ULINT bin, ULINT seq);

#define FS_INDEX_get_ptable(FSI) \
        ((FSI)->ptable)

#define FS_INDEX_get_database(FSI) \
        ((FSI)->s_db)

#define FS_INDEX_get_frag_len(FSI) \
        ((FSI)->m)

#define FS_INDEX_get_no_bins(FSI) \
        ((FSI)->no_bins)

#endif   

