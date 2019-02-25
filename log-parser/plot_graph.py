import matplotlib.pyplot as plt
import numpy as np
import math


def ceil_power_of_10(n):
    exp = math.log(n, 10)
    exp = math.ceil(exp)
    return 10**exp


def replace_abs_with_rel(arr):
    for i in range(0, len(arr)):
        arr[i] = (i, arr[i][1])


def generate_range(lst, steps):
    minimum = 0
    maximum = max(lst)
    step = ceil_power_of_10(max(lst)) / steps
    return np.arange(minimum, maximum + step, step)


# place from parser here
data = {}


for key, value in data.items():
    replace_abs_with_rel(value)


newList = list(zip(*data['new']))
rcvList = list(zip(*data['rcv']))
sndList = list(zip(*data['snd']))

plt.subplot(311)
plt.title('Accepts/Second')
plt.plot(*zip(*data['new']))
plt.grid(True)
plt.xlabel('Time(s)')
plt.ylabel('Accepts')
plt.yticks(generate_range(newList[1], 10))

plt.subplot(312)
plt.title('Reads/Seconds')
plt.plot(*zip(*data['rcv']))
plt.grid(True)
plt.xlabel('Time(s)')
plt.ylabel('Reads')
plt.yticks(generate_range(rcvList[1], 10))

plt.subplot(313)
plt.title('Sends/Second')
plt.plot(*zip(*data['snd']))
plt.grid(True)
plt.xlabel('Time(s)')
plt.ylabel('Sends')
plt.yticks(generate_range(sndList[1], 10))

plt.show()

