/********************************************************************

  fragdist.c

  Given a sequence (fasta) database and length of fragments, 
  constructs a histogram of the frequency of occurrence of each unique
  fragment. It also compares the total number of unique fragments to
  the theoretical number of fragments based on alphabet size.

  The AVL tree is used to store unique sequences.

 ********************************************************************/

#include "misclib.h"
#include "fastadb.h"
#include "avl.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>

#define DEFAULT_SEG_LENGTH (2 << 22)
#define DEFAULT_SEQ_LENGTH 2048
#define MAX_BINS 500


struct unique_fragment
  {
    char *seq;
    ULINT len;
    ULINT counter;
  };

/* Comparison functions */

/* Here we assume the fragments have the same length */
static 
int compare_frags(const void *pa, const void *pb, void *param)
{
  const struct unique_fragment *a = pa;
  const struct unique_fragment *b = pb;
  
  return memcmp(a->seq, b->seq, a->len);
}

/* Here we check length */
static 
int compare_seqs(const void *pa, const void *pb, void *param)
{
  const struct unique_fragment *a = pa;
  const struct unique_fragment *b = pb;

  if (a->len > b->len)
    return 1;
  else if (a->len < b->len)
    return -1;
  else
    {
      return memcmp(a->seq, b->seq, a->len);
    }
}

/* Destructor function - frees the items */
static
void free_fragment(void *pfrag, void *param)
{
  free(pfrag);
}

/* Load tree functions */

static
struct avl_table *load_fragment_tree(SEQUENCE_DB *s_db, ULINT m, 
				     ULINT *total_frags)
{
  ULINT i;
  char *pseq;
  char *pend;
  struct unique_fragment *frag;
  struct unique_fragment **tree_item;
  struct avl_table *tree;

  /* Load tree */
  *total_frags = 0;
  tree = avl_create(compare_frags, NULL, NULL);
  frag = mallocec(sizeof(struct unique_fragment));
  for (i=0; i < s_db->no_seq; i++)
    {
      pseq=s_db->seq[i].start;
      pend=s_db->seq[i].start + s_db->seq[i].len - m + 1;
      while (pseq < pend)
 	{
	  (*total_frags)++;
	  frag->seq = pseq;
	  frag->len = m;
	  frag->counter = 0;
	  tree_item = (struct unique_fragment **) 
	    avl_probe (tree, frag);	  
	  (*tree_item)->counter++;
	  if ((*tree_item) == frag)
	    frag = mallocec(sizeof(struct unique_fragment));
	  pseq++;
	}
    }
  return tree;
}

static 
struct avl_table *load_sequence_tree(SEQUENCE_DB *s_db)
{ 
  ULINT i;
  struct unique_fragment *frag;
  struct unique_fragment **tree_item;
  struct avl_table *tree;

  /* Load tree */
  tree = avl_create(compare_seqs, NULL, NULL);
  frag = mallocec(sizeof(struct unique_fragment));
  for (i=0; i < s_db->no_seq; i++)
    {
      frag->seq = s_db->seq[i].start;
      frag->len = s_db->seq[i].len;
      frag->counter = 0;
      tree_item = (struct unique_fragment **) 
	avl_probe (tree, frag);
      (*tree_item)->counter++;     
      if ((*tree_item) == frag)
	frag = mallocec(sizeof(struct unique_fragment));
    }
  return tree;
}

/* Count fragments */

static
ULINT *count_fragments(struct avl_table *tree, ULINT *max_bins)
{
  struct unique_fragment *frag;
  struct avl_traverser *traverser;
  ULINT *freq;
  ULINT old_max_bins = MAX_BINS;

  *max_bins = 1;
  traverser = mallocec(sizeof(struct avl_traverser));
  /* Traverse to generate histogram */
  freq = (ULINT *) callocec(old_max_bins, sizeof(ULINT));
  frag = avl_t_first(traverser, tree);

  while (frag != NULL)
    {
      *max_bins = frag->counter >= *max_bins ? 
	         (frag->counter+1) : *max_bins;
      
      if (*max_bins > old_max_bins)
	{
	  freq = reallocec(freq, (*max_bins)*sizeof(ULINT));
	  memset(freq+old_max_bins, 0, (*max_bins -
		 old_max_bins)*sizeof(ULINT));	      
	  old_max_bins = *max_bins;
	}
      (freq[frag->counter])++;
      frag = avl_t_next(traverser);
    }
  return freq;
} 

/* Print results */

static
void print_results(struct avl_table *tree, ULINT *freq, ULINT bins,
		   SEQUENCE_DB *s_db, char *db_file, ULINT m,
		   ULINT total_frags, ULINT a_size) 
{
  ULINT i, j;

   /* Results:
     - database
     - m
     - total number of sequences
     - total number of fragments
     - total number of unique fragments (percent)
     - theoretical number of fragments based on alpahbet size
     - proportion of actual/theoritical (i.e. sparsity)
     - log2 of number of unique fragments
     - Histogram of frequency vs multiplicity; */

  printf("******* fragdist Results *******\n");
  printf("Database: %s\n", db_file);
  printf("Total sequences: %ld\n", s_db->no_seq);
  if (m > 0)
    printf("Fragment length: %ld\n", m);
  else
    printf("Fragment length: (full sequences)\n");
  printf("Total fragments: %ld\n", total_frags);
  for (i=1, j=0; i < bins; i++)
    j+= freq[i]*i;
  printf("Counted fragments: %ld\n",j);
  printf("Unique fragments: %d (%.4f %%)\n", (int) avl_count(tree), 
	 100.0 * (float) avl_count(tree) / (float) total_frags);
  printf("Alphabet size: %ld\n", a_size);
  if (m > 0)
    {
      printf("Theoretical fragments: %.4e\n", 
	     pow((double) a_size, (double) m));
      printf("'Dataset sparsity': %8.4le\n",
	     (double)avl_count(tree)/  
	     pow((double) a_size, (double) m));
      printf("'Apparent alphabet size': %.4lf\n",
	     pow((double) avl_count(tree), 1.0/(double)m));
    }
  printf("Histogram: \n\n");

  printf("%12s %10s %10s %10s\n", "Multiplicity", "Frequency",
	 "Total Pts.", "Percentage"); 
  for (i=1; i < bins; i++)
    if (freq[i] > 0)
      printf("%6ld%6s %10ld %10ld %10.4f\n", i, "",freq[i], 
	     freq[i]*i, ((float)freq[i]*i *100.00) 
	     / (float) j);
}

int main(int argc, char **argv)
{
  SEQUENCE_DB *s_db;                  /* Sequence database storage */
  char *db_file;

  ULINT frag_len = 0;
  ULINT a_size = 20;
  ULINT total_frags;
  struct avl_table *tree;

  ULINT bins;
  ULINT *freq;

  fastadb_arg fastadb_argt[5];
  fastadb_argv_t fastadb_argv[5];

  fastadb_argt[0] = ACCESS_TYPE;
  fastadb_argt[1] = RETREIVE_DEFLINES;
  fastadb_argt[2] = NONE;
  fastadb_argv[0].access_type = MEMORY;
  fastadb_argv[1].retrieve_deflines = YES;

  /* Process command line options */

  if (argc >= 2)
    {  
      db_file = argv[1];
      if (argc >= 3)
	frag_len = atoi(argv[2]);
      if (argc >= 4)
	a_size = atoi(argv[3]);      
    }
  else
    {
      fprintf(stderr,"Insufficient arguments \n");
      fprintf(stderr,"Usage: %s database [length alphabet]\n", argv[0]); 
      exit(EXIT_FAILURE);
    }

  /* Load the sequence database */
  fprintf(stdout, "  Loading Sequence Database\n");
  s_db = fastadb_open(db_file, fastadb_argt, fastadb_argv); 

  /* Load tree */
  fprintf(stdout, "  Loading Search Tree\n");
  if (frag_len == 0)
    {
      tree = load_sequence_tree(s_db);
      total_frags = s_db->no_seq;
    }
  else
    tree = load_fragment_tree(s_db, frag_len, &total_frags);

  /* Count fragments */
  fprintf(stdout, "  Counting Fragments\n");
  freq = count_fragments(tree, &bins);

  /* Print Results */
  print_results(tree, freq, bins, s_db, db_file, frag_len,
		total_frags, a_size);

  /* Clean up */
  avl_destroy (tree, free_fragment);
  free(freq);
  exit(EXIT_SUCCESS);
}

