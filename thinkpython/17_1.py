
class Time:
  """ represent the time of day 
  attr: seconds
  """

  def __init__(self, hour = 0, minute = 0, second = 0):

    minutes = hour * 60 + minute
    self.seconds = minutes * 60 + second

  def __str__(self):
    minutes, second = divmod(self.seconds, 60)
    hour, minute = divmod(minutes, 60)
    return '%.2d:%.2d:%.2d' % (hour, minute, second)

  def print_time(self):
    print(str(self))
  
  def time_to_int(self):
    return self.seconds

  def is_after(self, other):
    return self.seconds > other.seconds

  def __add__(self, other):
    if isinstance(other, Time):
      return self.add_time(other)
    else:
      return self.increment(other)

  def __radd__(self, other):
    return self.__add__(other)

  def add_time(self, other):
    assert self.is_valid() and other.is_valid()
    seconds = self.seconds + other.seconds
    return int_to_time(seconds)
  
  def increment(self, seconds):
    seconds += self.seconds
    return int_to_time(seconds)

  def is_valid(self):
    return self.seconds >= 0 and self.seconds < 24 * 60 * 60
  

def int_to_time(seconds):
  return Time(0, 0, seconds)


def main():
  start = Time(9, 45, 00)
  start.print_time()

  end = start.increment(1337)
  #end = start.increment(1337, 460)
  end.print_time()

  print('is end after start?')
  print(end.is_after(start))

  print('using __str__')
  print(start, end)

  start = Time(9, 45)
  duration = Time(1, 35)
  print(start + duration)
  print(start + 1337)
  print(1337 + start)

  print('example of polymorphism')
  t1 = Time(7, 43)
  t2 = Time(7, 41)
  t3 = Time(7, 37)
  total = sum([t1, t2, t3])
  print(total)

if __name__ == '__main__':
  main()