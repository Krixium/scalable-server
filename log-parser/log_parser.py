SOCKET = 0
TIMESTAMP = 1
EVENT = 2
VALUE = 3


def import_file(f):
    print('importing lines')
    tmp = []
    lines = [line.rstrip('\n') for line in open(f)]
    for line in lines:
        tmp.append(line.split(','))

    print('finished importing lines')
    return tmp


def parse_log(lines):
    current_timestamp = float(lines[0][TIMESTAMP])
    current = {
        "new": 0,
        "snd": 0,
        "rcv": 0,
        "snd-data": 0,
        "rcv-data": 0,
    }
    total = {
        "new": 0,
        "snd": 0,
        "rcv": 0,
        "snd-data": 0,
        "rcv-data": 0
    }
    counts = {
        "new": [],
        "snd": [],
        "rcv": [],
        "snd-data": [],
        "rcv-data": []
    }
    progress = 0
    total_progress = len(lines)

    print('parsing lines')
    for line in lines:
        # show progress
        print(progress, total_progress)
        progress += 1
        # if its the next second
        if float(line[TIMESTAMP]) > current_timestamp + 1000:
            for key, value in current.items():
                # add current values to total
                total[key] += value
                # save the timestamp, count pair
                counts[key].append((current_timestamp, value))
                # set current back to 0
                current[key] = 0
            # set new timestamp
            current_timestamp = float(line[TIMESTAMP])

        if line[EVENT] == 'new':
            current['new'] += 1
        elif line[EVENT] == 'snd':
            if int(line[VALUE]) > 0:
                current['snd'] += 1
                current['snd-data'] += int(line[VALUE])
        elif line[EVENT] == 'rcv':
            if int(line[VALUE]) > 0:
                current['rcv'] += 1
                current['rcv-data'] += int(line[VALUE])
        else:
            print("invalid event logged")

    # dump remaining data in current to total and counts
    for key, value in current.items():
        # add current values to total
        total[key] += value
        # save the timestamp, count pair
        counts[key].append((current_timestamp, value))

    print('finished parsing lines')
    return total, counts


# set this to relative path of log file
file = ''

print(parse_log(import_file(file)))
print(file)

