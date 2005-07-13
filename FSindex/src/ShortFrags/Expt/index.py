"""
Wrapper around FSindex library.
"""

from ShortFrags.Expt import FS
from ShortFrags.Expt.db import db
from ShortFrags.Expt.hit_list import HitList


FS_BINS = FS.FS_BINS
SUFFIX_ARRAY = FS.SUFFIX_ARRAY
SEQ_SCAN = FS.SEQ_SCAN

SARRAY = FS.SARRAY
DUPS_ONLY = FS.DUPS_ONLY
FULL_SCAN = FS.FULL_SCAN

def _get_db(I):
    return db(FS.Index_s_db_get(I), new=False, own=False) 

def _get_ix_data(I):
    return FS.Index_get_data(I)

class FSIndex(object):
    def __init__(self, filename, sepn=None, use_sa=1, print_flag=0):
        self.thisown = 0
        if sepn == None:
            sepn = []
        self.this = FS.new_Index(filename, sepn, use_sa, print_flag)
        self.thisown = 1
        self.__dict__.update(FS.Index_get_data(self))

    def __del__(self):
        if self.thisown:
            FS.delete_Index(self)

    def save(self, filename):
        return FS.Index_save(self, filename)

    def __str__(self):
        return FS.Index___str__(self)

    def seq2bin(self, seq):
        return FS.Index_seq2bin(self, seq)

    def print_bin(self, bin, options=1):
        return FS.Index_print_bin(self, bin, options)

    def print_stats(self, options=3):
        return FS.Index_print_stats(self, options)

    def get_bin_size(self, bin):
        return FS.Index_get_bin_size(self, bin)

    def get_unique_bin_size(self, bin):
        return FS.Index_get_unique_bin_size(self, bin)

    def rng_srch(self, qseq, M, rng, stype=FS_BINS,
                 ptype=SARRAY, qdef=""):
        

        hits_dict = FS.Index_rng_srch(self, qseq, M, rng, M.conv_type,
                                      stype, ptype, qdef)
        return HitList(hits_dict)

    def kNN_srch(self, qseq, M, kNN, stype=FS_BINS,
                 ptype=SARRAY, qdef=""):
        hits_dict = FS.Index_kNN_srch(self, qseq, M, kNN,
                                      stype, ptype, qdef)
        return HitList(hits_dict)

    def threaded_search(self, srch_args):
        results = FS.Index_threaded_search(self, srch_args)
        return [HitList(r) for r in results]

    db = property(_get_db)
    ix_data = property(_get_ix_data)
