import json
import os

test_cases = {}
counter = 0

feb_26 = './logs/feb-26'
for test_case in os.listdir(feb_26):
    test_cases[test_case] = float(0)
    station_root = os.path.join(feb_26, test_case)
    for station in os.listdir(station_root):
        log_root = os.path.join(station_root, station)
        for log_file_name in os.listdir(log_root):
            with open(os.path.join(log_root, log_file_name)) as log_file:
                for raw_line in log_file:
                    fields = raw_line.rstrip('\n').split(',')
                    try:
                        test_cases[test_case] += float(fields[2])
                        counter += 1
                    except ValueError:
                        print("value error", os.path.join(log_root, log_file_name), fields)
                    except IndexError:
                        print("index error", os.path.join(log_root, log_file_name), fields)

    test_cases[test_case] /= counter
    counter = 0

output_file = open('parsed-delays.txt', 'w+')
output_file.write(json.dumps(test_cases, sort_keys=True, indent=2, separators=(',', ':')))
output_file.close()
