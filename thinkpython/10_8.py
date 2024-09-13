import random

def has_duplicates(t):
  
  # make a copy of t to avoid modifying the parameter
  s = t[:]
  s.sort()

  for i in range(len(s) - 1):
    if s[i] == s[i + 1]:
      return True
  
  return False

def random_bdays(n):
  t = []
  for i in range(n):
    bday = random.randint(1, 365)
    t.append(bday)
  return t

def count_matches(num_students, num_simulations):
  count = 0

  for i in range(num_simulations):
    t = random_bdays(num_students)
    if has_duplicates(t):
      count += 1
  
  return count

def main():
  num_students = 23
  num_simulations = 1000
  count = count_matches(num_students, num_simulations)

  print("{:<20} {:<20} {:<10}".format("num_simulations", "num_students", "ratio"))
  print("{:<20} {:<20} {:<10}".format(num_simulations, num_students, count/num_simulations))

if __name__ == '__main__':
    main()