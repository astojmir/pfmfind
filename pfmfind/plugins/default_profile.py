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




from cStringIO import StringIO

from pfmfind.search.DirichletMix import DirichletMix
from pfmfind.search.DirichletMix import freq_counts
from pfmfind.search.DirichletMix import henikoff_weights
from pfmfind.search.DirichletMix import BKGRND_PROBS as bg_dict
from pfmfind.search.DirichletInfo import get_mix
from pfmfind.search.DirichletInfo import NAMES
from pfmfind.search.matrix import POSITIONAL


iteration = True
arg_list = [('Scale', 'real', 2.0),
            ('Weighting', ['None', 'Henikoff'], 'Henikoff'),
            ('Regulariser', NAMES, 'recode3.20comp'),
            ]

def _get_matrix_counts(HL, scale, weight_type, dirichlet_type):

    seqs = HL.get_seqs()

    # Calculate sequence weights
    DM = get_mix(dirichlet_type)
    bcounts = DM.block_counts(seqs)

    if weight_type == 'None':
        weights = [1.0]*len(seqs)
        wcounts = bcounts
    elif weight_type == 'Henikoff':
        weights = henikoff_weights(seqs, DM.alphabet, bcounts)
        wcounts = DM.block_counts(seqs, weights)

    wprobs = DM.block_probs(wcounts)
    bkgrnd = DM.aa_vector(bg_dict)

    PM = DM.block2pssm(DM.block_log_odds(wprobs, bkgrnd, scale),
                       HL.query_seq)
    PM.name = 'PSSM'
    PM.module = __name__
    matrix_type = POSITIONAL
    ctype = 0

    return PM, matrix_type, ctype, bcounts, weights, wcounts, wprobs


def get_matrix(HL, scale, weight_type, dirichlet_type):

    if not len(HL):
        return None, 0, 0
    return _get_matrix_counts(HL, scale, weight_type,
                              dirichlet_type)[0:3]


def print_info(HL, scale, weight_type, dirichlet_type):

    if not len(HL):
        return "Too few hits to construct PSSM"

    if weight_type is None:
        return ""

    seqs = HL.get_seqs()
    deflines = HL.get_deflines()

    PM, matrix_type, ctype, bcounts, weights, wcounts, wprobs = \
        _get_matrix_counts(HL, scale, weight_type, dirichlet_type)

    DM = get_mix(dirichlet_type)
    file_str = StringIO()
    file_str.write('***** ALIGNMENT *****\n')
    for i in range(len(seqs)):
        file_str.write('%8.4f %s %s\n' % (weights[i], seqs[i],
                                          deflines[i]))
    file_str.write('\n***** COUNTS *****\n')
    file_str.write(DM.print_block_data(bcounts))
    file_str.write('\n***** WEIGHTED COUNTS *****\n')
    file_str.write(DM.print_block_data(wcounts, 5, 1, 'float'))
    file_str.write('\n***** DIRICHLET MIXTURE PROBABILITIES *****\n')
    bprobs = DM.block_probs(wcounts)
    file_str.write(DM.print_block_data(bprobs, 6, 4, 'float'))
    file_str.write("\n"+ str(PM))
    return file_str.getvalue()
