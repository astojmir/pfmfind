/*
 * Copyright (C) 2004-2006 Victoria University of Wellington
 *
 * This file is part of the PFMFind module.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "FSindex.h"
#include "hit_list.h"
#include "misclib.h"

#define MAX_RUNS 500
#define MAX_LINES 10
#define BUF_SIZE 1023

EXCEPTION FSexcept_array[1];
EXCEPTION *except;
struct exception_context the_exception_context[1];

/* The same as FSindex_rng_perf but threaded */


int main(int argc, char **argv)
{
  const char *index_file;
  char *matrix_full;
  char *input_file = NULL;

  time_t time0;
  time_t time1;
  double dt;

  int n = 0;        /* Number of runs */
  int N = MAX_RUNS;
  char *cp;

  SRCH_ARGS *sargs;

  
  int M = MAX_LINES;
  int m = 0;
  char **out_files;
  char **query_files;
  int l;
  int *sfunc_type;
  int *pfunc_type;

  int i, j;

  FILE *fp;
  FILE *outstream = stdout;
  FILE *instream = stdin;

  SCORE_MATRIX *S;
  SCORE_MATRIX *D;
  HIT_LIST_t *HL = NULL;
  FSINDX *I;
  char buffer[BUF_SIZE+1];

  int no_args = 2;

  if (argc < no_args+1)
    {
      fprintf(stderr,"Insufficient arguments \n");
      fprintf(stderr,"Usage: %s index_file matrix_file [input_file]\n", argv[0]);  
      fprintf(stderr,"Input file contains on each line the query file path"
	      "\nand the path of output file"
	      "separated by one (or more) blanks\n");
      exit(EXIT_FAILURE);
    }
  index_file = argv[1];
  matrix_full = argv[2];

  if (argc == no_args + 2) input_file = argv[3];

  Try {
    /* Load input data:
       Each line of input_file is of the form: query_file sfunc_type pfunc_type output_file
       where query_file contains at each line at least the fields sequence and radius.
       These are meant to come from running the kNN routines.
    */
    if (input_file != NULL) { 
      if((instream = fopen(input_file, "r")) == NULL)
	Throw FSexcept(FOPEN_ERR, "Could not open the file %s.",
		       input_file);
    }
    else instream = stdin; 

    query_files = callocec(M, sizeof(char *));
    out_files = callocec(M, sizeof(char *));
    sfunc_type = callocec(M, sizeof(int));
    pfunc_type = callocec(M, sizeof(int));
	
    while (fgets(buffer, BUF_SIZE, instream)) {
      if (buffer[0] == '#') continue;
      if (m >= M) {
	M += MAX_LINES;
	query_files = reallocec(query_files, M * sizeof(char *));
	out_files = reallocec(out_files, M * sizeof(char *));
	sfunc_type = reallocec(sfunc_type, M * sizeof(int));
	pfunc_type = reallocec(pfunc_type, M * sizeof(int));
      }
      l = strlen(buffer);
      buffer[l-1] = '\0';
      /* Too much but safe */
      query_files[m] = mallocec(l+1);
      out_files[m] = mallocec(l+1);
      sscanf(buffer, "%s %d %d %s", query_files[m], 
	     sfunc_type+m, pfunc_type+m, out_files[m]);
      m++;
    }
    if (input_file != NULL) 
      fclose(instream);

    /* Load matrix */
    S = SCORE_MATRIX_from_file(matrix_full);
    S->set_conv_type(S, QUASI);
    D = S->matrix_conv(S, NULL, 0);

    /* Load index */
    I = FS_INDEX_load(index_file);

    /* Allocate data arrays */
    sargs = mallocec(N * sizeof(SRCH_ARGS));

    for (i=0; i < m; i++) {
      printf("%s\n", query_files[i]);

      /* Load queries and ranges from each query_file */
      n=0;
      if((fp = fopen(query_files[i], "r")) == NULL)
	Throw FSexcept(FOPEN_ERR, "Could not open the file %s.",
		       query_files[i]);

      while (fgets(buffer, BUF_SIZE, fp)) {
	if (buffer[0] == '#') continue;
	if (n >= N) {
	  N += MAX_RUNS;
	  sargs = reallocec(sargs, N * sizeof(SRCH_ARGS));
	}
	buffer[strlen(buffer)-1] = '\0'; /* Removes '\n' */

	/* We assume fields are separated by ' ' */
	cp = strchr(buffer, ' ');
	*cp = '\0';
	sargs[n].query.start = strdup(buffer);
	sargs[n].query.len = strlen(sargs[n].query.start);
	sargs[n].query.id.defline = "Query sequence";
	sargs[n].M = D;
	sscanf(cp+1, "%d", &(sargs[n].range));
	sargs[n].kNN = -1;
	sargs[n].stype = sfunc_type[i];
	sargs[n].ptype = pfunc_type[i];
	n++;
      }
      fclose(fp);

      /* Parallel search */
      time0 = time(NULL);
      HL = FSINDX_threaded_srch(I, sargs, n);
      time1 = time(NULL);
      dt = difftime(time1, time0);

      if (out_files[i] != NULL) { 
	if((outstream = fopen(out_files[i], "w")) == NULL)
	  Throw FSexcept(FOPEN_ERR, "Could not open the file %s.",
			 out_files[i]);
      }
      else outstream = stdout; 

      fprintf(outstream, "# time = %.2f usecs\n", dt);
      for (j=0; j < n; j++) {
	HIT_LIST_sort_incr(HL+j);
	fprintf(outstream, "%s %5d %5ld %9ld %9ld %9ld %9ld %12ld %12ld\n",
		sargs[j].query.start,
		HL[j].hits[HL[j].actual_seqs_hits-1].dist,
		HL[j].actual_seqs_hits,		
		HL[j].FS_seqs_visited,
		HL[j].FS_seqs_hits,
		HL[j].seqs_visited,
		HL[j].seqs_hits,
		HL[j].useqs_visited,
		HL[j].useqs_hits);
	HIT_LIST_cleanup(HL+j);
	free(sargs[j].query.start);
      }
      free(HL);
      HL = NULL;

      if (out_files[i] != NULL)
	fclose(outstream);
    }
    FS_INDEX_destroy(I);
    for (i=0; i < m; i++) {
      free(query_files[i]);
      free(out_files[i]);
    }
    free(query_files);
    free(out_files);
    free(sfunc_type);
    free(pfunc_type);
    free(sargs);
    SCORE_MATRIX_del(S);
    SCORE_MATRIX_del(D);
  }
  Catch(except) {
    fprintf(stderr, "Error %d: %s\n", except->code, except->msg);  
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
