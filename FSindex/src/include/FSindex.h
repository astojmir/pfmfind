#ifndef _FS_INDEX_H
#define _FS_INDEX_H

#include "misclib.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fastadb.h"
#include "partition.h"
#include "smatrix.h"
#include "pmatrix.h"
#include "hit_list.h"
#ifdef USE_MPATROL
#include <mpatrol.h>
#endif


/********************************************************************/    
/********************************************************************/    
/***                                                              ***/
/***               PROTOTYPES                                     ***/ 
/***                                                              ***/
/********************************************************************/    
/********************************************************************/    



/********************************************************************/    
/*                                                                  */
/*                     FS_HASH_TABLE module                         */ 
/*                                                                  */
/********************************************************************/    

typedef struct
{
  ULINT no_bins;
  ULINT no_seqs;
  ULINT max_seqs_per_bin;
  ULINT *freq_seqs_per_bin;
  ULINT *bin_size;
  ULINT *max_bin_size;
  SEQ_index_t **bin;
  SEQ_index_t largest_bin;
  SEQ_index_t *heap;
} FS_HASH_TABLE_t;

/* Main constructor */
FS_HASH_TABLE_t *FS_HASH_TABLE_create(ULINT no_bins, ULINT def_size); 

/* Destructor */
void FS_HASH_TABLE_destroy(FS_HASH_TABLE_t *FS_HT);

/* Insertion */
void FS_HASH_TABLE_insert_seq(FS_HASH_TABLE_t *FS_HT, ULINT i,
			      SEQ_index_t seq_i);
void FS_HASH_TABLE_resize(FS_HASH_TABLE_t *FS_HT);

/* Element access */
ULINT FS_HASH_TABLE_get_no_seqs(FS_HASH_TABLE_t *FS_HT, ULINT i);
SEQ_index_t FS_HASH_TABLE_retrieve_seq(FS_HASH_TABLE_t *FS_HT, ULINT i, 
				       ULINT j);

/* Reading, writing to file */
int FS_HASH_TABLE_write(FS_HASH_TABLE_t *FS_HT, FILE *stream);
FS_HASH_TABLE_t *FS_HASH_TABLE_read(FILE *stream);

/* Statistics */
void FS_HASH_TABLE_print_stats(FS_HASH_TABLE_t *FS_HT, FILE *stream,
			       FS_PARTITION_t *FS_partition, 
			       ULINT frag_len);

/********************************************************************/    
/*                                                                  */
/*                     FS_INDEX module                              */ 
/*                                                                  */
/********************************************************************/    
#if 0
typedef struct
{
  char *db_name;
  char *matrix;
  char *alphabet; 
  char separator;
  ULINT frag_len;
  ULINT db_no_frags;
  SEQUENCE_DB *s_db;
  FS_PARTITION_t *ptable;
  SCORE_MATRIX_t *S;
  SCORE_MATRIX_t *D;
  FS_HASH_TABLE_t *HT;
} FS_INDEX_t;
#endif

extern int FS_INDEX_VERBOSE;
extern int FS_INDEX_PRINT_BAR;

typedef void FS_INDEX_process_func(FS_SEQ_t FS_neighbour);

/* These functions should convert the cutoff value to distances for
   bin search */

typedef int FS_INDEX_range_convert_func(SCORE_MATRIX_t *S0, 
					BIOSEQ *query0,
					int cutoff);	

/* Main constructor */
void FS_INDEX_create(const char *database, ULINT flen,
		     const char *abet, 
		     const char sepchar);

/* Destructor */
void FS_INDEX_destroy(void);

/* Load, Save */
int FS_INDEX_save(const char *filename);
int FS_INDEX_load(const char *filename);

/* Print */
void FS_INDEX_print_stats(FILE *stream, ULINT count, double dtime); 


/* Get / Set members */
SEQUENCE_DB *FS_INDEX_get_database(void);
FS_PARTITION_t *FS_INDEX_get_ptable(void);
int FS_INDEX_get_frag_len(void);


/* Main general range search */
int FS_INDEX_search(HIT_LIST_t *hit_list0, BIOSEQ *query0, 
		    SCORE_MATRIX_t *S0, SCORE_MATRIX_t *D0,
		    int cutoff, FS_INDEX_process_func *pfunc0, 
		    FS_INDEX_range_convert_func *cfunc);

/* Other Searches */
int FS_INDEX_kNN_search(HIT_LIST_t *hit_list0, BIOSEQ *query0,
			SCORE_MATRIX_t *S0, SCORE_MATRIX_t *D0,
			ULINT k, int V0,
			FS_INDEX_process_func *pfunc0, 
			FS_INDEX_range_convert_func *cfunc);


/* Process bin functions */

FS_INDEX_process_func FS_INDEX_QD_process_bin;
FS_INDEX_process_func FS_INDEX_S_process_bin;
FS_INDEX_process_func FS_INDEX_count_only_process_bin;

/* Conversion functions */
FS_INDEX_range_convert_func FS_INDEX_identity_convert;
FS_INDEX_range_convert_func FS_INDEX_S2QD_convert;


/* Experiments */
HIT_LIST_t *SSCAN_QD_search(SEQUENCE_DB *s_db, const char *matrix, 
			    BIOSEQ *query, ULINT D_cutoff);
#if 0
int FS_INDEX_has_neighbour(FS_INDEX_t *FS_index, BIOSEQ *query, 
			   ULINT D_cutoff);
#endif
int SSCAN_has_neighbour(SEQUENCE_DB *s_db,  SCORE_MATRIX_t *D, 
			FS_PARTITION_t *ptable,
			BIOSEQ *query, ULINT D_cutoff);


/********************************************************************/    
/*                                                                  */
/*                     PROFILE BASED SEARCHES                       */ 
/*                                                                  */
/********************************************************************/    
typedef void FS_INDEX_profile_process_func(FS_SEQ_t FSneighbour); 

typedef int FS_INDEX_profile_range_convert_func(POS_MATRIX *PS0, 
						BIOSEQ *query0,
						int cutoff);	



int FS_INDEX_profile_search(HIT_LIST_t *hit_list0, BIOSEQ *query0, 
			    POS_MATRIX *PS0, POS_MATRIX *PD0, 
			    ULINT Pfrom0, ULINT Pto0, int cutoff, 
			    FS_INDEX_profile_process_func *ppfunc0, 
			    FS_INDEX_range_convert_func *cfunc);

/* Process bin functions */
FS_INDEX_profile_process_func FS_INDEX_profile_S_process_bin;
FS_INDEX_profile_process_func FS_INDEX_profile_D_process_bin;

/* Conversion functions */


#ifndef DEBUG

#ifndef MY_INLINE
#define MY_INLINE extern inline
#endif
#define FS_INDEX_INLINE

#else

#ifndef MY_INLINE
#define MY_INLINE 
#endif

#endif


#ifdef  FS_INDEX_INLINE
/********************************************************************/    
/********************************************************************/    
/***                                                              ***/
/***               INLINE FUNCTION DEFINITIONS                    ***/ 
/***                                                              ***/
/********************************************************************/    
/********************************************************************/    


/********************************************************************/    
/*                                                                  */
/*                     FS_HASH_TABLE module                         */ 
/*                                                                  */
/********************************************************************/    

/* Insertion */

MY_INLINE
void FS_HASH_TABLE_insert_seq(FS_HASH_TABLE_t *FS_HT, ULINT i,
			      SEQ_index_t seq_i)
{
  ULINT old_seqs_per_bin;
  FS_HT->freq_seqs_per_bin[FS_HT->bin_size[seq_i]]--;

  FS_HT->bin[seq_i][FS_HT->bin_size[seq_i]] = i;
  FS_HT->bin_size[seq_i]++;  
  if (FS_HT->bin_size[seq_i] >=  FS_HT->max_bin_size[seq_i])
    {
      FS_HT->max_bin_size[seq_i] *= 2;
      FS_HT->bin[seq_i] = reallocec(FS_HT->bin[seq_i], 
			FS_HT->max_bin_size[seq_i] *
				    sizeof(SEQ_index_t));   
    }
  FS_HT->no_seqs++;

  if (FS_HT->bin_size[seq_i] >= FS_HT->max_seqs_per_bin)
    {
      old_seqs_per_bin = FS_HT->max_seqs_per_bin;
      FS_HT->max_seqs_per_bin = FS_HT->bin_size[seq_i] + 1;       
      FS_HT->freq_seqs_per_bin = reallocec(FS_HT->freq_seqs_per_bin,
				   FS_HT->max_seqs_per_bin *
					   sizeof(ULINT)); 
      memset(FS_HT->freq_seqs_per_bin + old_seqs_per_bin, 0, 
	     (FS_HT->max_seqs_per_bin - old_seqs_per_bin) 
	     * sizeof(ULINT));
      FS_HT->largest_bin = seq_i;
    }
  FS_HT->freq_seqs_per_bin[FS_HT->bin_size[seq_i]]++;
}


/* Element access */

MY_INLINE
ULINT FS_HASH_TABLE_get_no_seqs(FS_HASH_TABLE_t *FS_HT, ULINT i)
{
  return FS_HT->bin_size[i];
}


MY_INLINE
SEQ_index_t FS_HASH_TABLE_retrieve_seq(FS_HASH_TABLE_t *FS_HT, ULINT i, 
				ULINT j)
{
  return FS_HT->bin[i][j];
}


/********************************************************************/    
/*                                                                  */
/*                     FS_INDEX module                              */ 
/*                                                                  */
/********************************************************************/    

#endif   



#endif   
