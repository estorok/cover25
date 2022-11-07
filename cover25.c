// This program finds sets of 5 english words that 
// have 5 letters each and no letters in common.
// written by Ethan Torok

#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Number of threads to use
// Change NUM_THREADS to 2-3x the number of logical CPUs on the local system
#define NUM_THREADS (24)

// Fixed allocation sizes
// From prior analysis of the data, there are 5977
// candidate words to search, so 8000 is enough room for all of them.
// The anagram table uses 26^5 indices to store the 
// presence or absence of any 5-letter word. 11881377 > 26^5 
// so this number is large enough.
#define WORD_LIST_SIZE 8000
#define ANAGRAM_TABLE_SIZE 11881377

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

// This struct holds the function arguments needed 
// for a thread to begin a depth-first search.
//
// ascii_word_vector: array of 5-letter words
//                    with no duplicate letters
// bit_word_vector: array of bitwise representations of words
// size: length of each word vector
// slice: the section number (out of NUM_THREADS) the thread 
//        is responsible for searching. the start and end of the
//        slice are calculated in start_parallel_recurse
//        with respect to size and NUM_THREADS
// The int32 at bit_word_vector[i] corresponds to 
// the 5 bytes at ascii_word_vector[i].
typedef struct search_args {
  uint64_t *ascii_word_vector;
  uint32_t *bit_word_vector;
  uint32_t size;
  uint32_t slice;
} search_args;

// returns true if the 5 first elements of x are unique, else false
// x: points to the start of a 5-byte english word
uint8_t unique(uint8_t *x) {
  // For a fixed length 5 string, making 9 comparisons is 
  // faster than sorting (9 comparisons) and then making 4 comparisons.
  return (x[0] ^ x[1]) &&
         (x[0] ^ x[2]) &&
         (x[0] ^ x[3]) &&
         (x[0] ^ x[4]) &&
         (x[1] ^ x[2]) &&
         (x[1] ^ x[3]) &&
         (x[1] ^ x[4]) &&
         (x[2] ^ x[3]) &&
         (x[2] ^ x[4]) &&
         (x[3] ^ x[4]);
}

// prints the 5 characters at x to standard out, followed by a space
// x: points to the start of a 5-byte english word
void print_word(uint64_t *x) {
  for (uint8_t i = 0; i < 5; ++i) {
    printf("%c", ((uint8_t*) (x))[i]);
  }
  printf(" ");
}

// returns x such that 0 <= x <= ANAGRAM_TABLE_SIZE and 
//         (a == b) iff (hash(a) == hash(b))
// bit_rep: bitwise representation of a 5-letter word, 
//          with exactly 5 of the 26 rightmost bits set
uint32_t hash(uint32_t bit_rep) {
  uint32_t accum = 0;
  // Consider a word to be a base-26 number with 5 digits.
  // Order does not matter, so check set bits in alphabetical order 
  // and construct the hash from highest place value to lowest.
  for (uint8_t i = 0; i < 26; ++i) {
    if (bit_rep & (0x1 << i)) {
      accum = 26 * accum + i;
    }
  }
  return accum;
}

// returns the bitwise representation of a 5-letter word, 
//         with exactly 5 of the 26 rightmost bits set
// buf: points to the start of a 5-byte english word
//      with no repeating letters
uint32_t bit_rep(uint8_t *buf) {
  uint32_t m = 0;
  for (uint8_t i = 0; i < 5; ++i) {
    m |= (0x1 << (buf[i] - 'a'));
  }
  return m;
}

// returns the number of words found. will only store and count 
// the first instance of each anagram found.
// ascii_word_vector: output parameter where 5-letter words
//                    with no duplicate letters will be stored
// bit_word_vector: output parameter where bitwise representation
//                  of word will be stored
// The int32 at bit_word_vector[i] corresponds to 
// the 5 bytes at ascii_word_vector[i].
// 
uint32_t get_length_5_words(uint64_t *ascii_word_vector, 
                            uint32_t *bit_word_vector) {
  FILE *f;
  uint32_t current_index = 0;
  uint8_t *anagram_table;
  uint8_t buf[64];
  // Upon analysis of the dictionary in use, the longest line 
  // was 31 characters long. So a 64 byte buffer is long enough.

  f = fopen("words_alpha.txt", "r");
  anagram_table = calloc(sizeof (uint8_t), ANAGRAM_TABLE_SIZE); // = 26^5 indices
  buf[5] = 0;
  while (fgets((char *) buf, 63, f)) {
    if (buf[5] == '\n'                                  // word is length 5
        && unique(buf)                                  // no repeat chars
        && anagram_table[hash(bit_rep(buf))] == 0 ) {   // not an anagram
      memcpy(ascii_word_vector + current_index, buf, 5);
      bit_word_vector[current_index] = bit_rep(buf);
      anagram_table[hash(bit_rep(buf))] = 1;
      ++current_index;
    }
    buf[5] = 0;
  }
  free(anagram_table);
  return current_index;
}

// Prints sets of 5 words that are each 5 letters long, and 
// cover 25 letters of the English alphabet.
//
// ascii_word_vector: array of 5-letter words
//                    with no duplicate letters
// bit_word_vector: array of bitwise representations of words
// n: the number of elements in each word vector
// i: index to begin searching at this depth
// depth: number of words currently chosen out of 5
// accum: int32 with bits from the chosen words' characters set
// chosen: list of chosen word indices
// slice_end: for depth 0, index+1 of final word to do DFS
//
// The int32 at bit_word_vector[i] corresponds to 
// the 5 bytes at ascii_word_vector[i].
void find_covers(uint64_t *ascii_word_vector, 
                 uint32_t *bit_word_vector, 
                 uint32_t n, 
                 uint32_t i,
                 uint8_t depth,
                 uint32_t accum,
                 uint32_t *chosen, 
                 uint32_t slice_end) {
  if (depth == 5) {
    // base case
    // critical section: acquire lock to prevent print interleavings
    pthread_mutex_lock(&lock);
    for (depth = 0; depth < 5; ++depth) {
      print_word(ascii_word_vector + chosen[depth]);
    }
    printf("\n");
    pthread_mutex_unlock(&lock);
  } else {
    for (uint32_t j = i; j < (depth == 0 ? slice_end : n); ++j) {
      chosen[depth] = j;
      if (!(accum & (bit_word_vector[j])))      // empty intersection
        find_covers(ascii_word_vector, bit_word_vector, 
                    n, j + 1, depth + 1, 
                    accum | bit_word_vector[j], // union of bitsets
                    chosen, slice_end);
    }
  }
}

// a pthread begins execution according to the supplied args.
// raw_arg: a search_args.
void *start_parallel_recurse(void *raw_arg) {
  uint32_t start;
  uint32_t end;
  search_args *arg;
  uint32_t chosen[5] = {0, 0, 0, 0, 0};

  arg = (search_args *) raw_arg;
  // calculate slice window. edge case for final slice
  start = (arg->slice) * ((arg->size) / NUM_THREADS);
  end = (arg->slice == NUM_THREADS - 1) ? 
         arg->size : 
         ((1 + (arg->slice)) * ((arg->size) / NUM_THREADS));
  find_covers(arg->ascii_word_vector, 
              arg->bit_word_vector,
              arg->size,
              start,
              0,
              0,
              chosen,
              end);
  return 0;
}

// this program takes no command line arguments
int main() {
  uint64_t *ascii_word_vector;
  uint32_t *bit_word_vector;
  uint32_t count;
  search_args args[NUM_THREADS];
  pthread_t thread[NUM_THREADS];

  ascii_word_vector = malloc(sizeof (uint64_t) * WORD_LIST_SIZE);
  bit_word_vector = calloc(sizeof (uint32_t), WORD_LIST_SIZE);
  count = get_length_5_words(ascii_word_vector, bit_word_vector);

  for (uint8_t i = 0; i < NUM_THREADS; ++i) {
    // set args and spin up threads
    args[i].ascii_word_vector = ascii_word_vector;
    args[i].bit_word_vector = bit_word_vector;
    args[i].size = count;
    args[i].slice = i;
    pthread_create(&(thread[i]), NULL, start_parallel_recurse, &args[i]);
  }
  for (uint8_t i = 0; i < NUM_THREADS; ++i) {
    pthread_join(thread[i], NULL);
  }
  free(ascii_word_vector);
  free(bit_word_vector);
  return 0;
}
