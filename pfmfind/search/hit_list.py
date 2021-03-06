#
# Copyright (C) 2004-2006 Victoria University of Wellington
#
# This file is part of the PFMFind module.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2,
# or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#


"""
Provides classes to define a hit and construct a 
string to print its information.

Hits are fragments from the searched database.

B{Exceptions}:
    - KeyError

B{Classes}:
    - Hit  
        - Defines a dictionary for each hit [a fragment of the sequence].
    - HitList  
        - Provides the list of hits and the functions to manipulate them.

B{Functions}:
    - description(hits, query_seq, qs=0):
        - Return a printable string providing full details for a list 
	of hits.
    - summary(hits, show_rank=0, defline_width=57, rank_offset=0)
        - A list of hit class instances to be displayed.
    
"""


from cStringIO import StringIO

class Hit(object):

    """
    Defines a dictionary for each hit [a fragment of the sequence].
    
    For each hit a dictionary is defined.  Each class 
    contains a list of dictionaries for individual hits. 
    The dictionary for each hit provides the following 
    attributes:
    
        - B{seq_id} 
	     - Sequence description.
        - B{seq_to} 
	     - End of sequence. 
        - B{seq_from}
	     - Starting point for sequence.
        - B{dist}
	     - Distance.
        - B{sim} 
             - Similarity score.   

    Therefore, an instance of Hit will contain all of the above 
    attributes plus the following ones which are assigned 
    dynamically and for which an instance of IndexedDb must 
    exist and have a sequence database loaded.
    
        - B{defline}
            - Description of the hit.
        - B{accession}
	    - Key to uniquely identify protein.
 
    B{Exceptions:}
        - KeyError
    
    """     
    def __init__(self, dict):
        """
	Constructor, assigns all non dynamic attributes.
	See class description for details on which attributes
	are dynamic.
	"""
        self.__dict__ = dict

    keywords = property(fget=lambda hit : [])
#    defline = property(fget=lambda hit : hit.accession)


# Functions for printing hits (as dictionaries) and
# their lists

def summary(hits, show_rank=0, defline_width=57,
            rank_offset=0):
    """
    Construct a printable string from a list of 
    hit class instances.
    
        @param hits: The hit to be displayed.
        @param show_rank: Display the rank of the hits if true.
	Default value 0.
	@param defline_width: Width of the description.
	@param rank_offset: Control how much space is between
	the rank and the hit.  Default value 0.
	
        @return: String providing summary.
    """
	
    file_str = StringIO()

    if show_rank:
        line_func = lambda i,  w, defline, dist, sim, pvalue, Evalue:\
                        '%4d. %-*.*s %4d %4d %8.2e %8.2e' % (i, w, w, defline,
                                                             dist, sim, pvalue, Evalue)   
        if len(hits) > 1:
            file_str.write('%4.4s  %-*.*s %4.4s %4.4s %8.8s %8.8s\n' %\
                           ('Rank', defline_width, defline_width,
                            'Description', 'Dist', 'Sim', 'Pvalue', 'Evalue'))
    else:
        line_func = lambda i,  w, defline, dist, sim, pvalue, Evalue:\
                    '%-*.*s %4d %4d %8.2e %8.2e' % (w, w, defline,
                                        dist, sim, pvalue, Evalue)   
        if len(hits) > 1:
            file_str.write('%-*.*s %4.4s %4.4s %8.8s %8.8s\n' %\
                           (defline_width, defline_width,
                            'Description', 'Dist', 'Sim', 'Pvalue', 'Evalue'))

    for j, ht in enumerate(hits):
        defline = ht.defline
        if len(defline) > defline_width:
            defline = defline[:defline_width-3] + '...'
        if 'pvalue' in ht.__dict__:
            pval = ht.pvalue
            Eval = ht.Evalue
        else:
            pval = 0.0
            Eval = 0.0

        file_str.write(line_func(j+rank_offset, defline_width,
                                 defline, ht.dist, ht.sim, pval, Eval))
        if len(hits) > 1: file_str.write('\n')
    return file_str.getvalue()

def description(hits, query_seq, qs=0, get_offsets=False):
    """
    Return a printable string providing full details for a 
    list of hits.
    
    @param hits: A list of hits to be displayed.
    @param query_seq: A query sequence.
    @param qs:  Starting offset of query fragment in B{query_seq}. The 
    length of the query fragment is the same as the length of the hit.
    
    @return: String providing details of hits.
    """
    file_str = StringIO()
    lines = 0
    offsets = []

    for i,ht in enumerate(hits):
        offsets.append(lines)
        defline = ht.defline
        a = qs
        b = qs + len(query_seq)
        file_str.write('%s\n' % defline)
        lines += 1

        if hasattr(ht, 'taxon'):
            file_str.write('  Taxon: %s\n' % ht.taxon)
            lines += 1

        if hasattr(ht, 'features'):
            for ft, kws in ht.features.iteritems():
                file_str.write('  %s:' % ft)
                for k in kws[:-1]:
                    file_str.write(' %s,' % k)
                file_str.write(' %s\n' % kws[-1])
                lines += 1

        file_str.write('dist = %d  sim = %d\n' % (ht.dist,
                                                    ht.sim))
        file_str.write('Query: %5d %s %5d\n' % (a, query_seq, b))
        file_str.write('Sbjct: %5d %s %5d\n' % (ht.seq_from,
                                                ht.sequence,
                                                ht.seq_to,
                                                ))
        file_str.write('\n')
        lines += 4

    if get_offsets:
        return file_str.getvalue(), offsets
    else:
        return file_str.getvalue()
    
class HitList(list):

    """
    Provides the list of hits.
    
    The class may be used to print query details, print a full summary, 
    print full details for all hits, and to print performance statistics.  
    The class also provides the functions necessary to sort and print 
    the results by priority, similarity score, distance, 
    sequence(alphabetical and sequence id.
    
    """
    
    def __init__(self, dict):
        """
	Constructor, assigns the dictionary providing the search 
	related information.  For each hit in the hits dictionary 
	B{Hit} is called to create a list of B{Hit} instances. The 
	information provided in the hits dictionary includes the 
	following attributes:
	    - B{query_seq}
	        - The query fragment.
	    - B{query_def}
	        - The query description.
	    - B{conv_type}
	        - Matrix conversion type.
	    - B{sim_range}
	        - Similarity score range.
	    - B{dist_range}
	        - Distance range.
	    - B{kNN}
	        - Nearest neighbor cutoff.
	    - B{bins_visited}
	        - Bins checked.
	    - B{bins_hit}
	        - Accepted bins.
	    - B{frags_visited}
	        - Number of checked fragments.
	    - B{frags_hit} 
	        - Number of accepted fragments.
	    - B{search_time}
	        - Time to complete the search.
	    - B{hits}
	        - A list holding individual dictionaries for each hit.  
		For the attributes provided in the dictionary of 
		each hit please refer to the help for class B{Hit}.
	"""
        HL = [Hit(ht) for ht in dict['hits']] 
        del dict['hits']
        list.__init__(self, HL)
        self.__dict__ = dict
	
    def __str__(self):
        return self.print_str()

    def header_str(self):
        """
        Returns a printable string with header information.
        """

        file_str = StringIO()
        file_str.write("***** Query Parameters *****\n")
        file_str.write("Query fragment: %s\n" % self.query_seq)
        file_str.write("Query decription: %s\n" % self.query_def)
        if hasattr(self, 'matrix_name'):
            file_str.write("Score matrix: %s\n" % self.matrix_name)
        file_str.write("Matrix conversion type: %s\n" % self.conv_type)
        file_str.write("Distance range: %d\n" % self.dist_range)
        file_str.write("Similarity score range: %d\n" % self.sim_range)
        file_str.write("Nearest neighbours cutoff: %d\n" % self.kNN)
        file_str.write("Actual neighbours: %d\n\n" % len(self))
        return file_str.getvalue()

    def summary_str(self):
        """
        Returns a printable string with full summary.
        """

        file_str = StringIO()
        file_str.write("***** Summary *****\n")
        file_str.write(summary(self, 1))
        file_str.write('\n')
        return file_str.getvalue()

    def full_str(self, qs=0, get_offsets=False):
        """
        Returns a printable string with full details for all hits.
	
	@param qs:  Starting offset of query fragment in B{query_seq}. The 
        length of the query fragment is the same as the length of the hit.
	@return: String with full details for all hits.
        """

        file_str = StringIO()
        file_str.write("***** Full Details *****\n")
        if get_offsets:
            ds, offsets = description(self, self.query_seq, qs, get_offsets)
            file_str.write(ds)
        else:
            file_str.write(description(self, self.query_seq, qs))
        file_str.write('\n')
        if get_offsets:
            return file_str.getvalue(), offsets
        else:
            return file_str.getvalue()

    def perf_str(self):
        """
        Returns a printable string with performance statistics.
        """

        has_bins = hasattr(self, 'bins_visited') and hasattr(self, 'bins_hit')
        has_frags = hasattr(self, 'frags_visited') and hasattr(self, 'frags_hit')
        has_ufrags =  hasattr(self, 'unique_frags_visited') and \
                     hasattr(self, 'unique_frags_hit')
        has_time = hasattr(self, 'search_time')

        if not (has_bins or has_frags or has_ufrags): return ""

        file_str = StringIO()
        file_str.write("***** Index Performance *****\n")

        if has_bins:
            file_str.write("Number of checked bins : %d\n" % self.bins_visited)
            file_str.write("Number of accepted bins : %d\n" % self.bins_hit)
            if self.bins_visited != 0:
                file_str.write("Accepted out of generated bins:  %.2f %%\n\n" \
                               % (100.0 * self.bins_hit / self.bins_visited))

        if has_frags:
            file_str.write("Number of checked fragments : %d\n" % self.frags_visited)
            file_str.write("Number of accepted fragments : %d\n" % self.frags_hit)
            if self.frags_visited != 0:
                file_str.write("Accepted out of generated fragments:  %.2f %%\n\n" \
                               % (100.0 * self.frags_hit / self.frags_visited))

        if has_ufrags:
            if self.unique_frags_visited:
                file_str.write("Number of checked distinct fragments : %d\n" %\
                               self.unique_frags_visited)
                file_str.write("Number of accepted distinct fragments : %d\n" %\
                               self.unique_frags_hit)
                file_str.write("Accepted out of generated distinct fragments:  %.2f %%\n\n" \
                               % (100.0 * self.unique_frags_hit / self.unique_frags_visited))

        if has_time:
            file_str.write("Search time: %.2f sec.\n" % self.search_time)

        return file_str.getvalue()
    
    def print_str(self, qs=0, get_offsets=False):
        """
	Returns a printable string with the header, summary, full details, 
	and performance statistics.
	
	@param qs: Starting offset of query fragment in B{query_seq}. The 
        length of the query fragment is the same as the length of the hit.
	"""

        file_str = StringIO()
        file_str.write(self.header_str())
        file_str.write(self.summary_str())
        if get_offsets:
            ds, offsets = self.full_str(qs, get_offsets)
            file_str.write(ds)
        else:
            file_str.write(self.full_str(qs))
        file_str.write(self.perf_str())

        if get_offsets:
            return file_str.getvalue(), offsets
        else:
            return file_str.getvalue()
        
    
    def _sort_hits(self, incr, attribs):
        """
        General hits sort - pass attributes in order
        of priority.
        """

        tmp = [[eval('ht.%s' % k) for k in attribs] + [i] for i, ht in enumerate(self)]
        tmp.sort()
        if not incr:
            tmp.reverse()
        n = len(attribs)
        htmp = [self[a[n]] for a in tmp]
        self[:] = htmp
        
    def sort_by_similarity(self, incr=True):
        """
        Sort hits by similarity score.
        """

        attribs = ['sim',
                   'accession',
                   'seq_from',
                   'seq_to',
                   ]
        self._sort_hits(incr, attribs)
        
    def sort_by_distance(self, incr=True):
        """
        Sort hits by distance.
        """
        
        attribs = ['dist',
                   'accession',
                   'seq_from',
                   'seq_to',
                   ]
        self._sort_hits(incr, attribs)

    def sort_by_distance_only(self, incr=True):
        """
        Sort hits solely by distance.
        """
        
        attribs = ['dist',
                   ]
        self._sort_hits(incr, attribs)

    def sort_by_seq(self, incr=True):
        """
        Sort hits by sequence (alphabetical).
        """

        attribs = ['sequence',
                   'accession',
                   'seq_from',
                   'seq_to',
                   ]
        self._sort_hits(incr, attribs)

    def sort_by_seqid(self, incr=True):
        """
        Sort hits by sequence id.
        """
        
        attribs = ['accession',
                   'seq_from',
                   'seq_to',
                   ]
        self._sort_hits(incr, attribs)

    def get_seqs(self):
        """
	Extract sequences of all hits from the dictionary.
	
	@return: Sequences from hits.
	"""
        seqs = [ht.sequence for ht in self]
        return seqs

    def get_deflines(self):
        """
	Extract descriptions of all hits from the dictionary.
	
	@return: Descriptions from hits.
	"""
        deflines = [ht.defline for ht in self]
        return deflines




def _compare_dicts(d1, d2, skip_attribs=[]):
    attribs = {}
    
    for k in d1.keys():
        if k in skip_attribs:
            continue
        if k in d2: 
            if d1[k] != d2[k]:
                attribs[k] = (d1[k], d2[k])
        else:
            attribs[k] = (d1[k], None)

    for k in d2.keys():
        if k in skip_attribs:
            continue
        if k in d1: 
            if d1[k] != d2[k]:
                attribs[k] = (d1[k], d2[k])
        else:
            attribs[k] = (None, d2[k])

    return attribs
    


def compare_hit_lists(HL1, HL2):
    HL1.sort_by_distance()
##    HL2.sort_by_distance()

    # Check attributes
    d1 = HL1.__dict__
    d2 = HL2.__dict__
    skip_attribs = ['search_time', 'frags_visited',
               'query_def', 'frags_hit',
               'unique_frags_hit', 'unique_frags_visited',
               'bins_visited', 'bins_hit', 'matrix']

    # Floats never compare equal and its too tedious
    # to round and then compare
    skip_attribs1 = ['defline', 'pvalue', 'Evalue']

    attribs = _compare_dicts(d1, d2, skip_attribs)

    if len(HL1) != len(HL2): return (False, (attribs, None))

    htdiffs = []
    all_ok = True
    for i in xrange(len(HL1)):
        d1 = HL1[i].__dict__
        d2 = HL2[i].__dict__
        ha = _compare_dicts(d1, d2, skip_attribs1)
        if len(ha) > 0: all_ok = False
        htdiffs.append(ha)

    all_ok =  all_ok and (len(attribs) == 0) 

    return (all_ok, (attribs, htdiffs))
