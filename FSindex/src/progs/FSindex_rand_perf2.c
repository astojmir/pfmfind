#include "misclib.h"
#include "fastadb.h"
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include "FSindex.h"
#include "hit_list.h"


int FS_PARTITION_VERBOSE = 0;
int SCORE_MATRIX_VERBOSE = 0;
int FS_INDEX_VERBOSE = 0;
int FS_INDEX_PRINT_BAR = 1;

int main(int argc, char **argv)
{
  BIOSEQ query;
  const char *index_file;
  ULINT no_runs;
  ULINT i;
  ULINT len;
  const char *out_file = NULL;

  int K0;
  int K1;
  int step;
  FILE *stream = stdout;
  ULINT one_percent;
  SCORE_MATRIX_t *D;
  char *matrix_full;
  char *matrix_base;
  char *matrix_dir;
  int no_args = 6;
  FS_PARTITION_t *ptable;
  SCORE_MATRIX_t *S;
  HIT_LIST_t *hit_list;
  FS_HASH_TABLE_t *HT;
  int eps;
  SEQ_index_t s;

  if (argc < no_args+1)
    {
      fprintf(stderr,"Insufficient arguments \n");
      fprintf(stderr,"Usage: %s index_file matrix_file"
	      " no_runs start_eps step_size end_eps <out_file>\n", argv[0]);  
      exit(EXIT_FAILURE);
    }
  index_file = argv[1];
  matrix_full = argv[2]; 
  no_runs = atoi(argv[3]);
  K0 = atoi(argv[4]);
  step = atoi(argv[5]);
  K1 = atoi(argv[6]);

 if (argc >= no_args+2)
    {
      out_file = argv[7];
      if ((stream = fopen(out_file, "w")) == NULL)
	{
	  fprintf(stderr, "Unable to open %s\n ", out_file);
	  exit(EXIT_FAILURE);
	} 
    } 

  FS_INDEX_load(index_file);
  ptable = FS_INDEX_get_ptable();
  S = SCORE_MATRIX_create(matrix_full, ptable); 
  D = SCORE_MATRIX_S_2_Dquasi(S);
  split_base_dir(matrix_full, &matrix_base, &matrix_dir);  
  len = FS_INDEX_get_frag_len();

  hit_list = HIT_LIST_create(&query, FS_INDEX_get_database(), 
			     matrix_base, 0);
  if (no_runs < 100)
    one_percent = 1;
  else
    one_percent = no_runs/100;


  fprintf(stream, "***** FSsearch random runs: %s"
	  " database ******\n", FS_INDEX_get_db_name());
  fprintf(stream, "No runs: %ld\n", no_runs);
  HT = FS_INDEX_get_hash_table();
  fprintf(stream, "Total fragments: %ld\n", FS_HASH_TABLE_get_total_seqs(HT));
  fprintf(stream, "Total bins: %ld\n", FS_HASH_TABLE_get_no_bins(HT));
  fprintf(stream, "Clustering: %s\n", FS_index_get_alphabet());
  fprintf(stream, "Matrix: %s\n", matrix_base);

 
  srand48(time(NULL)); 
  fastadb_init_Ffrags(FS_INDEX_get_database(), 10);

  if (K0 < 0)
    K1 = 0;

  while (K0 <= K1)
    {
      fprintf(stdout,"\nk = %d\n", K0);
      fprintf(stream,"\nk = %d\n", K0);
      fprintf(stream, "%*.*s %6.6s %10.10s %10.10s %10.10s"
	      " %10.10s %10.10s %10.10s %10.10s %10.10s %10.10s\n",
	      (int) len , (int) len, "query", "eps", "FS_visited",
	      "FS_hits", "FS_ratio", "s_visited", "s_hits", "s_ratio",
	      "u_visited", "u_hits", "u_ratio");   
      for (i = 0; i < no_runs; i++)
	{
	  s = HT->heap[lrand48()%FS_HASH_TABLE_get_total_seqs(HT)];

	  fastadb_get_Ffrag(FS_INDEX_get_database(), 10, &query, s);
	  eps = K0;
	  HIT_LIST_reset(hit_list, &query, FS_INDEX_get_database(),
			 matrix_base, eps);
	  FS_INDEX_search(hit_list, &query, S, D, eps,
			  FS_INDEX_QD_process_bin, 
			  FS_INDEX_identity_convert);

	  fprintf(stream, "%*.*s %6d %10ld %10ld %10.2f %10ld"
		  " %10ld %10.2f %10ld %10ld %10.2f\n", 
		  (int) len , (int) len, query.start,
		  hit_list->range, hit_list->FS_seqs_visited,
		  hit_list->FS_seqs_hits, 
		  (double) hit_list->FS_seqs_visited / hit_list->FS_seqs_hits, 
		  hit_list->seqs_visited, hit_list->seqs_hits,
		  (double) hit_list->seqs_visited / hit_list->seqs_hits,
		  hit_list->useqs_visited, hit_list->useqs_hits,
		  (double) hit_list->useqs_visited / hit_list->useqs_hits);

	  /* Print progress bar */
	  printbar(stdout, i+1, one_percent, 50);  
	}
      K0 += step;
    }
  fprintf(stdout,"\n");
  if (out_file != NULL)
    fclose(stream);
  return EXIT_SUCCESS;
}