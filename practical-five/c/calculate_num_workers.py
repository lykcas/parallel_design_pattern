import sys
import math

data_size=int(sys.argv[1])
serial_threshold=int(sys.argv[2])

current_data_size=data_size
level=0
while (current_data_size >= serial_threshold):
	current_data_size/=2
	level+=1
number_workers=int(math.ceil(2**level/2*2))
print(str(number_workers))
