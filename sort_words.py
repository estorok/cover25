import functools

# This script takes an input file and produces a 
# sorted output file according to a lexicographic 
# ordering, using reverse character frequency to rank 
# letter orders within the alphabet.

# The frequency rank was determined counting only 
# the characters that appeared in 5-letter words 
# with no duplicate characters, with anagrams removed.
# Given character c, rank[ord(c) - ord('a')] is the order of c 
# in the reverse frequency order alphabet, e.g. since the 
# most frequent character is 'a', rank[0] comes last in the 
# alphabet at place 26, and is 'greater' than all other characters 
# for the purpose of writing a comparator.
rank = [26,8,15,14,25,6,10,13,23,3,9,18,12,19,22,11,1,21,24,17,20,5,7,2,16,4,0]

# I define a comparator that implements the character frequency rank 
# order defined above, which returns some negative number
# when word1 < word2, a positive number when word1 > word2, and 
# 0 when word1 == word2.
def compare(word1, word2):
  for i in range(min(len(word1), len(word2)) - 1):
    w1i = ord(word1[i]) - ord('a')
    w2i = ord(word2[i]) - ord('a')
    if rank[w1i] < rank[w2i]:
      return -1
    elif rank[w1i] > rank[w2i]:
      return 1
  return len(word1) - len(word2)

words = []
with open("words_alpha.txt", 'r') as f:
  for line in f:
    words.append(line)

words.sort(key=functools.cmp_to_key(compare))
with open("sorted_alpha.txt", 'w') as f:
  for word in words:
    f.write(word)
