import sys
import time
from typing import List

def problem_one(input: List[int]) -> int:
  prev = input[0]
  counter = 0
  for val in input:
    if val > prev:
      counter += 1
    prev = val

  return counter


def problem_two(input: List[int]) -> int:
  counter = 0

  for index, val in enumerate(input):
    if index < 3:
      continue

    curr = val + input[index - 1] + input[index - 2]
    prev = input[index - 1] + input[index - 2] + input[index - 3]
    if curr > prev:
      counter += 1

  return counter



ints = []
with open(sys.argv[1]) as f:
  lines = f.readlines()
  for l in lines:
    ints.append(int(l))

start = time.time_ns()

p1 = problem_one(ints)
p2 = problem_two(ints)

end = time.time_ns()

print(f"Problem 1: {p1}")
print(f"Problem 2: {p2}")
print(f"Completed in {end - start} ns.")