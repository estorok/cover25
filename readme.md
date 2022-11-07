## Introduction

The challenge is [to find 5 English words, 5 letters long each, that cover 25 out of 26 letters of the English alphabet](https://youtu.be/_-AfhLQfb6w)
("how many guesses can you make in Wordle 
without repeating letters").

An initial brute-force solution by Matt Parker, 
written in Python, found 538 solutions 
in about 30 days' worth of computing time.

This recursive solution, written in C, finds 
the same 538 solutions in less than a minute.

## Overview of approach and optimizations

I fill an array with only the 5-letter words 
from an existing dictionary, `words_alpha.txt`.

### Bitwise representation

Words are stored as both their ASCII and 
26-bit representation.

    Example bitwise representation for "jacks"
    uint32_i: 00000000000001000000011000000101
                           |       ||      | |
    encode  : ------zyxwvutsrqponmlkjihgfedcba

If a word contains repeat letters, it is not stored. 
Therefore, all the words in consideration contain 
5 different letters out of the 26 letter English 
alphabet. I set the corresponding bits in a 32-bit 
integer to form an alternate flat representation of 
the word.

If a word is an anagram of a previous word, it is also 
not stored. Anagrams are tracked in a hash set 
using the 26-bit (`uint32_i`) representation 
of the word to create the hash in a way that it will 
be the same for every anagram of the same five letters.

### Recursive backtracking

I do a depth-first search, trying each of the words 
that have no overlap with the set of currently selected
words. When a fifth word that fits is found, 
a solution is output. When a word does not fit 
at a given position, I advance to the next word for 
that position.

### Bitwise word overlap checking

To check that the character sets of the 
currently selected words and the next word 
do not overlap, we can take their bitwise intersection 
using the `&` operator. Then we can take a bitwise 
union with the `|` operator to continue the 
depth first search with the new character set.
This operation is extremely fast and is the 
primary speedup in operation compared to 
Matt Parker's original solution.

### Parallelization and synchronization

This code uses POSIX threads (pthreads) to leverage 
multiprocessor capabilities for a substantial speedup.
`NPROC` is hardcoded in the source and should be 
changed to 2-3x the number of 
logical processors available locally. 
Each thread receives an equal size
section of words to begin a depth first search with.
When a thread finds a solution, it acquires a 
mutex before printing to `STDOUT`, then releases the 
mutex. This prevents interleaved printing.

## Further optimizations

The file `sort_words.py` is a Python script 
to produce a dictionary with words sorted 
lexicographically by reverse character frequency.
I thought that this could lead to an improvement in backtracking 
speed, as the widest branching of recursive calls 
happens at the deepest part of the tree instead 
of the shallowest. When I tried the same search algorithm 
using the new sort order, however, performance decreased, 
so I reverted the change.

## Dependencies, building and running

Requires `gcc`, a C standard library like `glibc`, 
and pthreads.

Type `make` to produce the binary. `./cover25` to run.

The python script `verify.py` checks that no lines have
duplicate letters.

    $ ./cover25 | tee answers.txt
    $ python3 verify.py
