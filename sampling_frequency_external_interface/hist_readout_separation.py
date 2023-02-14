import re
import sys


def read_data(fname, bin_width_us):
    samples_by_bin_num = {}
    with open(fname) as f:
        comment_re = re.compile(r"^\s*#.*$")
        last_timestamp_ns = None
        cnt = 0

        for line in f:
            # check if comment
            if comment_re.match(line):
                continue

            fields = line.split()
            timestamp_ns = int(fields[0])

            if last_timestamp_ns is not None:
                delta_ns = timestamp_ns - last_timestamp_ns
                delta_us = delta_ns / 1000.0
                bin_num = int(delta_us / bin_width_us)
                if bin_num not in samples_by_bin_num:
                    samples_by_bin_num[bin_num] = 0
                samples_by_bin_num[bin_num] += 1

            last_timestamp_ns = timestamp_ns

            # update status
            cnt += 1
            if 0 == cnt % 10000:
                print("\r{:>8} lines read\r".format(cnt),
                      end="", file=sys.stderr)

        print("\r{:>8} lines read".format(cnt), file=sys.stderr)
    return samples_by_bin_num


def print_data(var_name, data_dict, xmin_us, xmax_us, bin_width_us, cnt_total):
    print("${} <<EOD".format(var_name))
    for bin_num in range(int(xmin_us / bin_width_us),
                         int(xmax_us / bin_width_us)):
        bin_lower_bound = bin_num * bin_width_us
        if bin_num in data_dict:
            bin_value = data_dict[bin_num]
        else:
            bin_value = 0

        print("{}\t{}".format(bin_lower_bound, bin_value))
    print("EOD")


def get_cnt_gt(data_dict, threshold):
    """
    return number of data points >threshold
    """
    return sum(map(lambda item: item[1],
                   filter(lambda item: item[0] > threshold,
                          data_dict.items())))


# "bin_num" is the bin number:
# lower_bound = bin_width * bin
bin_width_us = 0.1

xmin_us = 0
xmax_us = 20

print("hwmon...", file=sys.stderr)
hwmon_data = read_data("results/hwmon_raw.dat", bin_width_us)
cnt_total = sum(hwmon_data.values())
print_data("hwmon", hwmon_data, xmin_us, xmax_us, bin_width_us, cnt_total)

print("occ...", file=sys.stderr)
occ_data = read_data("results/occ_raw.dat", bin_width_us)
cnt_total = sum(occ_data.values())
print_data("occ", occ_data, xmin_us, xmax_us, bin_width_us, cnt_total)

print("occ optimized...", file=sys.stderr)
occ_optimized_data = read_data("results/occ_optimized_raw.dat", bin_width_us)
cnt_total = sum(occ_optimized_data.values())
print_data("occ_optimized", occ_optimized_data, xmin_us, xmax_us, bin_width_us, cnt_total)

xmax_bin_num = xmax_us / bin_width_us
threshold_cnt_hwmon = get_cnt_gt(hwmon_data, xmax_bin_num)
threshold_cnt_occ = get_cnt_gt(occ_data, xmax_bin_num)
threshold_cnt_occ_optimized = get_cnt_gt(occ_optimized_data, xmax_bin_num)
print("# hwmon cnt > {} us: {} ({} %)".
      format(xmax_us,
             threshold_cnt_hwmon,
             100 * threshold_cnt_hwmon / cnt_total))
print("#   occ cnt > {} us: {} ({} %)".
      format(xmax_us,
             threshold_cnt_occ,
             100 * threshold_cnt_occ / cnt_total))
print("#   occ optimized cnt > {} us: {} ({} %)".
      format(xmax_us,
             threshold_cnt_occ_optimized,
             100 * threshold_cnt_occ_optimized / cnt_total))

# note: four slashes in python to pass two escaping stages
print("""
set title "Readout Latency of OCC Interfaces"
set xlabel "Readout Latency (µs, bin width {}\\\\,µs)"
set ylabel "Number of Samples (\\\\#)"
set format y "$%g \\\\times 10^6$"

set key inside top right
set xrange [0:20]

set style fill solid 1.0 noborder

plot $occ           u 1:($2/1000000) w fillsteps lc 1 fs solid title 'OCC',\\
     $hwmon         u 1:($2/1000000) w fillsteps lc 2 fs solid title 'hwmon',\\
     $occ_optimized u 1:($2/1000000) w fillsteps lc 3 fs solid title 'OCC optimized'
""".format(bin_width_us))
