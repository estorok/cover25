import sys
from string import ascii_lowercase as alc

# To test my solution for obvious errors, 
# this script checks that no line of output contains 
# duplicate characters.
# The script also tells the missing letter in each line of output.
#with open("answers.txt", 'r') as f:
for line in sys.stdin:
  stripped = line.replace(" ", "")
  processed = "".join(sorted(stripped))
  for c in alc:
    if c not in processed:
      print(f'{c} is the letter missing from {line}', end='')
  for i in range(len(processed) - 1):
    if processed[i] == processed[i+1]:
      print(f'line {line} failed; contains multiple {processed[i]}.')
