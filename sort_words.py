import functools

rank = [26,8,15,14,25,6,10,13,23,3,9,18,12,19,22,11,1,21,24,17,20,5,7,2,16,4,0]
print(len(rank))
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
