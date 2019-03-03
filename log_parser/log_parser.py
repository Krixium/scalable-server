import json

SOCKET = 0
TIMESTAMP = 1
EVENT = 2
VALUE = 3


def parse_log(f):
    current_timestamp = 0
    current = {"new": 0, "snd": 0, "rcv": 0}
    total = {"new": 0, "snd": 0, "rcv": 0}
    counts = {"new": [], "snd": [], "rcv": []}
    progress = 0
    total_progress = f[0]

    with open(f[1]) as opened_file:
        for raw_line in opened_file:

            # show progress
            progress += 1
            print(f[1], progress, total_progress)

            # read line and split into array
            line = raw_line.rstrip('\n').split(',')

            # if its the next second
            if float(line[TIMESTAMP]) > current_timestamp + 1000:
                for key, value in current.items():
                    if value > 0:
                        total[key] += value
                        counts[key].append((current_timestamp, value))
                        current[key] = 0
                current_timestamp = float(line[TIMESTAMP])

            if line[EVENT] == 'new':
                current['new'] += 1
            elif line[EVENT] == 'snd':
                if int(line[VALUE]) > 0:
                    current['snd'] += 1
            elif line[EVENT] == 'rcv':
                if int(line[VALUE]) > 0:
                    current['rcv'] += 1
            else:
                print("invalid event logged")
                print(line)

    # dump remaining data in current to total and counts
    for key, value in current.items():
        if value > 0:
            total[key] += value
            counts[key].append((current_timestamp, value))

    return total, counts


output_file = open('parsed-logs.txt', 'w+', buffering=0)
files = [
    (19748, './logs/server-logs/epoll-1000-10.log'),
    (701726, './logs/server-logs/epoll-1000-1000.log'),
    (194174, './logs/server-logs/epoll-10000-10.log'),
    (13140234, './logs/server-logs/epoll-10000-1000.log'),
    (589842, './logs/server-logs/epoll-30000-10.log'),
    (42357900, './logs/server-logs/epoll-30000-1000.log'),
    (20600, './logs/server-logs/select-1000-10.log'),
    (1960991, './logs/server-logs/select-1000-1000.log'),
    (187779, './logs/server-logs/select-10000-10.log'),
    (9144582, './logs/server-logs/select-10000-1000.log'),
    (465628, './logs/server-logs/select-30000-10.log'),
    (15725870, './logs/server-logs/select-30000-1000.log')
]

for file in files:
    total_result, total_count = parse_log(file)
    output_file.write(file[1] + '\n')
    output_file.write("total:\n")
    output_file.write(json.dumps(total_result, sort_keys=True, indent=2, separators=(',', ':')))
    output_file.write('\n')
    output_file.write("count:\n")
    output_file.write(json.dumps(total_count, sort_keys=True, indent=2, separators=(',', ':')))
    output_file.write('\n')
    output_file.write("\n\n\n")


output_file.close()
