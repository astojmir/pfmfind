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


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include "misclib.h"

/********************************************************************/
/*                                                                  */
/*                     Exception handling                           */
/*                                                                  */
/********************************************************************/

EXCEPTION *FSexcept(int code, const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);

  FS_EXCEPTION->code = code;
  vsnprintf(FS_EXCEPTION->msg, MSG_SIZE, fmt, ap);
  va_end(ap);
  return FS_EXCEPTION;
}


/********************************************************************/
/*                                                                  */
/*      Error checking wrappers to malloc, calloc and realloc       */
/*                                                                  */
/********************************************************************/

void *callocec(long int number, long int size)
{
  void *memp;

  if (number <= 0)
    Throw FSexcept(NEG_MEM_REQ, "Negative memory request.");
  memp=calloc(number, size);
  if (memp==NULL)
    Throw FSexcept(NO_MEM, "No Memory!");
  return(memp);
}

void *mallocec(long int size)
{
  void *memp;

  if (size <= 0)
    Throw FSexcept(NEG_MEM_REQ, "Negative memory request.");
  memp=malloc(size);
  if (memp==NULL)
    Throw FSexcept(NO_MEM, "No Memory!");
  return(memp);
}

void *reallocec(void *pt, long int size)
{
  void *memp;

  if (size <= 0)
    Throw FSexcept(NEG_MEM_REQ, "Negative memory request.");
  memp=realloc(pt, size);
  if (memp==NULL)
    Throw FSexcept(NO_MEM, "No Memory!");
  return(memp);
}



/********************************************************************/
/*                                                                  */
/*      Error checking wrappers to fgets, fread and fwrite          */
/*                                                                  */
/********************************************************************/

void fgets_(char *s, int size, FILE *stream)
{
  char *d = fgets(s, size, stream);
  if (d==NULL && ferror(stream)) {
    Throw FSexcept(IO_ERR, "Problem reading file (fgets).");
  }
}

void fread_(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  size_t count = fread(ptr, size, nmemb, stream);
  if (count != nmemb) {
    Throw FSexcept(IO_ERR, "Problem reading file (fread).");
  }
}

void fwrite_(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
  size_t count = fwrite(ptr, size, nmemb, stream);
  if (count != nmemb) {
    Throw FSexcept(IO_ERR, "Problem writing to file.");
  }
}

/********************************************************************/
/*                                                                  */
/*                    Unix path processing                          */
/*                                                                  */
/********************************************************************/

void path_split(const char *p, char **h, char **t)
{
  /* Equivalent to Python os.path.split except that we don't strip
     multiple '/'
  */

  int L = strlen(p);
  const char *s = p + L - 1;

  while (*s != '/' && s >= p) s--;

  if (s >= p) {
    /* Using calloc thus guaranteeing null-termination */
    *t = callocec(1, p + L - s);
    memcpy(*t, s+1, p + L - s - 1);

    if (s == p) { /* path starts with '/' */
      *h = callocec(1, 2);
      **h = '/';
    }
    else {
      *h = callocec(1, s - p + 1);
      memcpy(*h, p, s - p);
    }
  }
  else if (s < p) {
    *t = strdup(p);
    *h = callocec(1, 1);
  }
}

char *path_join(const char *h, const char *t)
{
  /* Equivalent to Python os.path.join */

  char *p;
  int m = strlen(h);
  int n = strlen(t);

  if (*t == '/')
    p = strdup(t);
  else if (m == 0 || *(h+m-1) == '/') {
    p = callocec(1, m+n+1);
    memcpy(p, h, m);
    memcpy(p+m, t, n);
  }
  else {
    p = callocec(1, m+n+2);
    memcpy(p, h, m);
    *(p+m) = '/';
    memcpy(p+m+1, t, n);
  }
  return p;
}

/********************************************************************/
/*                                                                  */
/*                    I/O                                           */
/*                                                                  */
/********************************************************************/

void fwrite_string(char *s, FILE *fp)
{
  int len = strlen(s) + 1;
  fwrite_(&len, sizeof(int), 1, fp);
  fwrite_(s, sizeof(char), len, fp);
}

void fread_string(char **s, FILE *fp)
{
  int len;
  fread_(&len, sizeof(int), 1, fp);
  *s = mallocec(len);
  fread_(*s, sizeof(char), len, fp);
}


/* Byte-order-independent I/O of binary arrays */

void swrite_int32(void *ptr, size_t nmemb, unsigned char *s)
{
  int *x = ptr;
  int *y = x + nmemb;
  for (; x < y; x++) {
    *s++ = (unsigned char) *x & 0xFF;
    *s++ = (unsigned char) (*x >> 8) & 0xFF;
    *s++ = (unsigned char) (*x >> 16) & 0xFF;
    *s++ = (unsigned char) (*x >> 24) & 0xFF;
  }
}

void sread_int32(void *ptr, size_t nmemb, unsigned char *s)
{
  int *x = ptr;
  int *y = x + nmemb;
  for (; x < y; x++) {
    *x = ((int) *s++);
    *x |= ((int) *s++) << 8;
    *x |= ((int) *s++) << 16;
    *x |= ((int) *s++) << 24;
  }
}


void fwrite_int32(void *ptr, size_t nmemb, FILE *stream)
{
  int *x = ptr;
  int *y = x + nmemb;

  for (; x < y; x++) {
    fputc(*x & 0xFF, stream);            /* write lowest byte */
    fputc((*x >> 8) & 0xFF, stream);     /* write 2nd byte */
    fputc((*x >> 16) & 0xFF, stream);    /* write 3rd byte */
    fputc((*x >> 24) & 0xFF, stream);    /* write highest byte */
  }
}

void fread_int32(void *ptr, size_t nmemb, FILE *stream)
{
  int *x = ptr;
  int *y = x + nmemb;

  for (; x < y; x++) {
    *x = fgetc(stream) & 0xFF;            /* read lowest byte */
    *x |= (fgetc(stream) & 0xFF) << 8;    /* read 2nd byte */
    *x |= (fgetc(stream) & 0xFF) << 16;   /* read 3rd byte */
    *x |= (fgetc(stream) & 0xFF) << 24;   /* read highest byte */
  }
}


/********************************************************************/
/*                                                                  */
/*                Comparison functions for qsort                    */
/*                                                                  */
/********************************************************************/

int compare_int(const void *M1, const void *M2)
{
  const int *i1 = M1;
  const int *i2 = M2;
  if (*i1 > *i2)
    return 1;
  else if (*i1 < *i2)
    return -1;
  else
    return 0;
}

int compare_char(const void *M1, const void *M2)
{
  const char *i1 = M1;
  const char *i2 = M2;
  if (*i1 > *i2)
    return 1;
  else if (*i1 < *i2)
    return -1;
  else
    return 0;
}

int compare_dbl(const void *M1, const void *M2)
{
  const double *i1 = M1;
  const double *i2 = M2;
  if (*i1 > *i2)
    return 1;
  else if (*i1 < *i2)
    return -1;
  else
    return 0;
}

/********************************************************************/
/*                                                                  */
/*                Check validity of strings                         */
/*                                                                  */
/********************************************************************/

int check_word_alphabet(const char *word, int word_len,
			const char *alphabet, int alphabet_len)
{
  /* Checks if all letters from word belong to alphabet.
     alphabet must be sorted using compare_char */
  int i;

  for (i=0; i < word_len; i++, word++)
    if (bsearch(word, alphabet, alphabet_len, 1,
		compare_char) == NULL)
      return 0;

  return 1;
}

/********************************************************************/
/*                                                                  */
/*                Print into buffer that can grow                   */
/*                                                                  */
/********************************************************************/

#define ABSPRINTF_BUF 256
void absprintf(char **buf, int *size, int *len, const char *fmt, ...)
{
  va_list ap;
  int remaining_size;
  int n;

  if (*buf == NULL) {
    *buf = mallocec(ABSPRINTF_BUF);
    *size = ABSPRINTF_BUF;
    *len = 0;
  }

  while (1) {
      /* Try to print in the allocated space. */
      remaining_size = *size - *len;
      va_start(ap, fmt);
      n = vsnprintf (*buf+*len, remaining_size, fmt, ap);
      va_end(ap);
      /* If that worked, get out. */
      if (n > -1 && n < remaining_size) {
	  *len += n;
	  break;
      }
      /* Else try again with more space. */
      if (n > -1)    /* glibc 2.1 */
	  *size += max(ABSPRINTF_BUF, n+1-remaining_size);
      else           /* glibc 2.0 */
	  *size += ABSPRINTF_BUF;
      *buf = reallocec(*buf, *size);
  }
}
