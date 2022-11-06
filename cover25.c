#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>


#define NPROC (24)

uint8_t rev_freq_ab[26] = 
 {26,8,15,14,25,6,10,13,23,3,9,18,12,19,22,11,1,21,24,17,20,5,7,2,16,4};
//a  b c  d  e  f g  h  i  j k l  m  n  o  p  q r  s  t  u  v w x y  z

pthread_t t[NPROC];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct rbt_args {
  uint64_t *w_vec;
  uint32_t *flat_vec;
  uint32_t size;
  uint32_t slice;
} rbt_args;

uint8_t uniq(uint8_t *x) {
  // whether or not x is 5 unique elements given it is length 5
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

// returns true when all unique, must be sorted beforehand
uint8_t uniq_sorted(uint8_t *x) {
  return (x[0] != x[1]) &&
         (x[1] != x[2]) &&
         (x[2] != x[3]) &&
         (x[3] != x[4]);
}

void sort(uint8_t *word) {
  // insertion sort
  uint8_t tmp;
  uint8_t i, j;
  for (i = 1; i < 5; ++i) {
    tmp = word[i];
    for (j = i; (j > 0) && (tmp < word[j - 1]); --j){
      word[j] = word[j - 1];
    }
    word[j] = tmp;
  }
}

void prnt(uint64_t *word) {
  uint8_t i;

  for (i = 0; i < 5; ++i) {
    printf("%c", ((uint8_t*) (word))[i]);
  }
  printf(" ");
}

uint32_t hash(uint32_t mask) {
  uint32_t accum;
  uint8_t i;
  
  accum = 0;
  for (i = 0; i < 26; ++i) {
    if (mask & (0x1 << i)) {
      accum = 26 * accum + i;
    }
  }
  return accum;
  // return 26 * (26 * (26 * (26 * (x[0] - 'a') + x[1] - 'a') + x[2] - 'a') + x[3] - 'a') + x[4] - 'a';
}

uint32_t mask(uint8_t *buf) {
  uint32_t m;
  uint8_t i;

  m = 0;
  for (i = 0; i < 5; ++i) {
    m |= (0x1 << (buf[i] - 'a'));
  }
  return m;
}

void get_freqs(uint32_t *flat_vec, uint32_t n, uint32_t *freqs) {
  uint32_t i;
  uint32_t j;
  for (i = 0; i < n; ++i)
    for (j = 0; j < 26; ++j) 
      freqs[j] += ((flat_vec[i] & (0x1 << j)) >> j);
}

uint32_t l5_words(uint64_t *w_vec, uint32_t *flat_vec) {
  FILE *f;
  uint32_t vec_ptr;
  uint8_t *gram_tbl;
  uint8_t buf[32];

  f = fopen("words_alpha.txt", "r");
  gram_tbl = calloc(sizeof (uint8_t), 11881377); // = 26^5 indices
  vec_ptr = 0;
  buf[5] = 0;
  while (fgets(buf, 31, f)) {
    if (buf[5] == '\r' 
        && uniq(buf)
        && gram_tbl[hash(mask(buf))] == 0 ) {
      memcpy(w_vec + vec_ptr, buf, 5);
      flat_vec[vec_ptr] = mask(buf);
      gram_tbl[hash(mask(buf))] = 1;
      ++vec_ptr;
    }
    buf[5] = 0;
  }
  return vec_ptr;
}

// *st: array of indices
void find_covers(uint64_t *w_vec, 
                 uint32_t *flat_vec, 
                 uint32_t n, 
                 uint32_t i,
                 uint8_t depth,
                 uint32_t accum,
                 uint32_t *st, 
                 uint32_t slice_end) {
  uint32_t j;
  if (depth == 5) {
    pthread_mutex_lock(&lock);
    for (depth = 0; depth < 5; ++depth) {
      prnt(w_vec + st[depth]);
    }
    printf("\n");
    pthread_mutex_unlock(&lock);
  } else {
    for (j = i; j < (depth == 0 ? slice_end : n); ++j) {
      st[depth] = j;
      if (!(accum & (flat_vec[j])))
        find_covers(w_vec, flat_vec, n, j + 1, depth + 1, 
                    accum | flat_vec[j], st, slice_end);
    }
  }
}

void *start_parallel_recurse(void *arg) {
  uint32_t start;
  uint32_t end;
  rbt_args *rbt;
  uint32_t st[5] = {0, 0, 0, 0, 0};

  rbt = (rbt_args *) arg;
  // calculate slice window
  start = (rbt->slice) * ((rbt->size) / NPROC);
  end = (rbt->slice == NPROC - 1) ? 
         rbt->size : 
         ((1 + (rbt->slice)) * ((rbt->size) / NPROC));
  find_covers(rbt->w_vec, 
              rbt->flat_vec,
              rbt->size,
              start,
              0,
              0,
              st,
              end);
  return 0;
}

int main() {
  uint64_t *w_vec;
  uint32_t *flat_vec;
  uint32_t count;
  uint8_t i;
  rbt_args rbt[NPROC];
  // uint32_t *freqs;
  // uint32_t stack[] = {0, 0, 0, 0, 0};

  w_vec = malloc(sizeof (uint64_t) * 8000);
  flat_vec = calloc(sizeof (uint32_t), 8000);
  count = l5_words(w_vec, flat_vec);

  // freqs = calloc(sizeof (uint32_t), 26);
  // get_freqs(flat_vec, count, freqs);
  // printf("count=%d\n", count);
  // for (i = 0; i < 26; ++i) {
  //   printf("%c:%d\n", i + 'a', freqs[i]);
  // }
  for (i = 0; i < NPROC; ++i) {
    rbt[i].w_vec = w_vec;
    rbt[i].flat_vec = flat_vec;
    rbt[i].size = count;
    rbt[i].slice = i;
    pthread_create(&(t[i]), NULL, start_parallel_recurse, &rbt[i]);
  }
  for (i = 0; i < NPROC; ++i) {
    pthread_join(t[i], NULL);
  }
  free(w_vec);
  free(flat_vec);
}
