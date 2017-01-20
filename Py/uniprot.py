#!/usr/bin/env python3

import sys
import urllib.request





### This is a kind of convinience function that buffers print() output so we can manipulate it before
### it goes to stdout. (We can :%s/print1/print/g to see raw output for debugging instead.)
###
def print1(s):
    if "line_buffer" not in print1.__dict__:
        print1.line_buffer = []
    print1.line_buffer.append(s)


### Clean up the print1'd lines before actually printing them. This involves taking multiple lines of
### the form
###     RECORD    L    M
###     RECORD  M+1    N
### and collapsing those records to simply
###     RECORD    L    N
### as well as taking lines of the form
###     RECORD    P    P
### and collapsing them to simply
###     RECORD    P
### for human readability and to save space.
###
def print1_buffer_cleanup():
    sorted_buffer = sorted(print1.line_buffer, key = lambda s: int(s[30:39]))
    collapse      = [sorted_buffer[0]]
    output        = []
    for s in sorted_buffer[1:] + [' ' * 50]: # HACK. We append ' ' * 50 so that the below automatically
                                             # flushes the very last element (and leaves spaces in the
                                             # collapse buffer.
        # Check that this record is of the same type as all the entries in the collapse buffer, and begins
        # at the sequentially next residue to the last thing in that buffer. If not, flush the collapse buffer.
        if s[0:29] != collapse[0][0:29] or int(s[30:39]) != int(collapse[-1][40:49]) + 1:
            o = collapse[0][0:29] + ' ' + collapse[0][30:39]
            if collapse[0][30:39] != collapse[-1][40:49]: # If both 'to' and 'from' residues are the same, collapse.
                o += ' ' + collapse[-1][40:49]
            output.append(o)
            collapse = []
        # Append this record to the collapse buffer.
        collapse.append(s)
    print1.line_buffer = [] 
    return output


### Take a line m of a UniProtKB text page and print appropriately formatted sequence data to stdout
### if m represents part of a FASTA sequence.
###
def extract_sequence(m):
    # Static variables: reading controls whether we are treating all inputted lines as FASTA code, and
    # residue controls the currently outputting residue number.
    if "reading" not in extract_sequence.__dict__:
        extract_sequence.reading = False
    if "residue" not in extract_sequence.__dict__:
        extract_sequence.residue = 0
    # Manage the state of the extract_sequence function. 
    if m[0:2] == 'SQ': # Are we just about to begin reading FASTA code?
        extract_sequence.reading = True
        return
    if m[0:2] == '//': # Have we finished reading FASTA code?
        extract_sequence.reading = False
        return
    if extract_sequence.reading == True: # Are we currently reading FASTA code?
        lines = 0
        t = '{residue:>29} {start:>9} {end:>9}'
        for c in m.strip().replace(' ', ''):
            extract_sequence.residue += 1
            a = b = extract_sequence.residue
            ### Generally hydrophobic amino acids.
            ###
            # Glycine is very tiny and flexible.
            if c in 'G':
                print1(t.format(residue = 'GLYCINE', start = a, end = b))
            # Small nonpolar chains.
            elif c in 'AILMV':
                print1(t.format(residue = 'NONPOLAR-ALKYL', start = a, end = b))
            # Proline forms a ring with a later atom in its own backbone, so has a kind of double sidechain structure.
            elif c in 'P':
                print1(t.format(residue = 'PROLINE', start = a, end = b))
            # Large nonpolar rings.
            elif c in 'FW':
                print1(t.format(residue = 'NONPOLAR-AROMATIC', start = a, end = b))
            # Tyrosine can act hydrophobic or hydrophillic depending on its environment. 
            elif c in 'Y':
                print1(t.format(residue = 'TYROSINE', start = a, end = b))
            ### Generally hydrophilic amino acids.
            ###
            # Positively charged amino acids.
            elif c in 'RK':
                print1(t.format(residue = 'POLAR-BASIC', start = a, end = b))
            # Histidine can act charged (basic) or uncharged depending on its environment.
            elif c in 'H':
                print1(t.format(residue = 'HISTADINE', start = a, end = b))
            # Polar but noncharged amino acids. 
            elif c in 'STNQ':
                print1(t.format(residue = 'POLAR-NEUTRAL', start = a, end = b))
            # Cysteine and selenocysteine form disulfide bridges to become cystine and selenocystine as long as they
            # are in an oxidising environment -- typically extracellular.
            elif c in 'CU':
                print1(t.format(residue = 'CYSTEINE', start = a, end = b))
            # Negatively charged amino acids.
            elif c in 'DE':
                print1(t.format(residue = 'POLAR-ACIDIC', start = a, end = b))


### Take a line m of a UniProtKB text page and print appropriately formatted sequence data to stdout
### if m represents a secondary structure motif annotation of the sequence.
###
def extract_structure(m):
    # Check that this is a feature table (sequence annotation) record.
    if m[0:2] != 'FT':
        return
    # Check that the record has well-defined start and end points in residues.
    try:
        a, b = int(m[14:20]), int(m[21:27])
    except:
        return
    t = '{key:>29} {start:>9} {end:>9}'
    # Is this a beta-strand?
    if m[5:13].strip() == 'STRAND':
        return print1(t.format(key = 'BETA', start = a, end = b))
    # Is this a turn?
    elif m[5:13].strip() == 'TURN':
        return print1(t.format(key = 'TURN', start = a, end = b))
    # Is this a helix?
    elif m[5:13].strip() == 'HELIX':
        return print1(t.format(key = 'HELIX', start = a, end = b))
    return


### Take a line m of a UniProtKB text page and print appropriately formatted sequence data to stdout
### if m represents a protein topological domain (cellular location) motif annotation of the sequence.
###
def extract_domains(m):
    # Check that this is a feature table (sequence annotation) record.
    if m[0:2] != 'FT':
        return 0
    # Check that the record has well-defined start and end points in residues.
    try:
        a, b = int(m[14:20]), int(m[21:27])
    except:
        return 0
    t = '{key:>29} {start:>9} {end:>9}'
    # Is this a topological domain?
    if m[5:13].strip() == 'TOPO_DOM':
        # Cytoplasmic region?
        if 'CYTOPLASMIC' in m:
            return print1(t.format(key = 'CYTOPLASMIC', start = a, end = b))
        # Intramembrane region?
        elif 'INTERMEMBRANE' in m:
            return print1(t.format(key = 'INTERMEMBRANE', start = a, end = b))
        # Extracellular region?
        elif 'EXTRACELLULAR' in m:
            return print1(t.format(key = 'EXTRACELLULAR', start = a, end = b))
    # Is this a transmembrane domain?
    elif m[5:13].strip() == 'TRANSMEM':
        return print1(t.format(key = 'TRANSMEMBRANE', start = a, end = b))
    return 0


### Take a UniProt code, connect to UniProtKB, and yield an iterator of the lines in that code's text entry.
###
def code2pagelines(c):
    try:
        R = urllib.request.Request("http://www.uniprot.org/uniprot/{code}.txt".format(code = c))
        with urllib.request.urlopen(R) as f:
            for l in f:
                m = l.decode('utf-8').upper()
                yield m
    except:
        print('Invalid UniProt code: {code}'.format(code = c))
        sys.exit(-2)


### Take a list of the lines of a UniProtKB text entry and feed each line to the extract function specified.
###
def parse_page(page, extract):
    for l in page:
        extract(l)
    return print1_buffer_cleanup()





if len(sys.argv) != 3 or sys.argv[1] not in ['sequence', 'structure', 'domains', 'dump-all']:
    print('Usage: uniprot.py sequence|structure|domains|dump-all UniProt_code')
    sys.exit(-1)
    
e = sys.argv[1]
c = sys.argv[2]
if e == 'dump-all':
    page = [l for l in code2pagelines(c)]
    for extract, x in [(extract_sequence,  'kbseq'),
                       (extract_structure, 'kbstr'),
                       (extract_domains,   'kbdom')]:
        with open('{code}.{extension}'.format(code = c, extension = x), 'w') as f:
            for l in parse_page(page, extract):
                f.write(l + "\n")
    sys.exit(0)
else:
    if e == 'sequence':
        lines = parse_page(code2pagelines(c), extract_sequence)
    elif e == 'structure':
        lines = parse_page(code2pagelines(c), extract_structure)
    elif e == 'domains':
        lines = parse_page(code2pagelines(c), extract_domains)
    for l in lines:
        print(l)
    sys.exit(len(lines))
