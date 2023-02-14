import re
import sys
import math


def read_data(fname, bin_width_ms):
    samples_by_bin_num = {}
    all_durations_us = []
    with open(fname) as f:
        comment_re = re.compile(r"^\s*#.*$")
        cnt = 0

        for line in f:
            # check if comment
            if comment_re.match(line):
                continue

            fields = line.split()
            duration_ns = int(fields[2])
            duration_ms = duration_ns / 1000000.0
            bin_num = int(duration_ms / bin_width_ms)
            if bin_num not in samples_by_bin_num:
                samples_by_bin_num[bin_num] = 0
            samples_by_bin_num[bin_num] += 1
            all_durations_us.append(duration_ns / 1000.0)

            # update status
            cnt += 1
            if 0 == cnt % 10000:
                print("\r{:>8} lines read\r".format(cnt),
                      end="", file=sys.stderr)

        print("\r{:>8} lines read".format(cnt), file=sys.stderr)
    return samples_by_bin_num


def print_data(var_name, data_dict, xmin, xmax, bin_width, cnt_total):
    print("${} <<EOD".format(var_name))
    for bin_num in range(int(xmin / bin_width),
                         int(xmax / bin_width)):
        bin_lower_bound = bin_num * bin_width
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
bin_width_ms = 0.001

xmin_ms = 40.05
xmax_ms = 40.10

print("hwmon...", file=sys.stderr)
# has no acc -> use sensor
hwmon_data = read_data("results/hwmon_sensorjitter.dat", bin_width_ms)
cnt_total_hwmon = sum(hwmon_data.values())
print_data("hwmon", hwmon_data, xmin_ms, xmax_ms, bin_width_ms, cnt_total_hwmon)

print("occ...", file=sys.stderr)
occ_data = read_data("results/occ_sensorjitter.dat", bin_width_ms)
cnt_total_occ = sum(occ_data.values())
print_data("occ", occ_data, xmin_ms, xmax_ms, bin_width_ms, cnt_total_occ)

print("occ optimized...", file=sys.stderr)
occ_optimized_data = read_data("results/occ_optimized_sensorjitter.dat", bin_width_ms)
cnt_total_occ_optimized = sum(occ_optimized_data.values())
print_data("occ_optimized", occ_optimized_data, xmin_ms, xmax_ms, bin_width_ms, cnt_total_occ_optimized)

xmax_bin_num = xmax_ms / bin_width_ms
threshold_cnt_hwmon = get_cnt_gt(hwmon_data, xmax_bin_num)
threshold_cnt_occ = get_cnt_gt(occ_data, xmax_bin_num)
threshold_cnt_occ_optimized = get_cnt_gt(occ_optimized_data, xmax_bin_num)
print("# hwmon cnt > {} ms: {} ({} %)".
      format(xmax_ms,
             threshold_cnt_hwmon,
             100 * threshold_cnt_hwmon / cnt_total_hwmon))
print("#   occ cnt > {} ms: {} ({} %)".
      format(xmax_ms,
             threshold_cnt_occ,
             100 * threshold_cnt_occ / cnt_total_occ))
print("#   occ optimized cnt > {} ms: {} ({} %)".
      format(xmax_ms,
             threshold_cnt_occ_optimized,
             100 * threshold_cnt_occ_optimized / cnt_total_occ_optimized))

ymax_percent = max(
        max(hwmon_data.values()),
        max(occ_data.values()),
        max(occ_optimized_data.values()))


# from experience: ticks every 2 percent
ymax_tick_delta = 100
ymax_next_tick = int(ymax_tick_delta * 
                     math.ceil(ymax_percent / ymax_tick_delta))

print("""
set multiplot layout 2,1
#set ylabel "Number of Samples (\\\\#)"

# note: global label for x axis, "set xlabel" is foreach plot
set label "Interval (ms, bin width {}\\\\,µs)" at screen 0.5,0.05 center front
set label "Number of Samples (\\\\#)" at screen 0.05,0.5 center rotate by 90

set lmargin at screen 0.2
set rmargin at screen 0.9

set key inside top right

set xrange [{}:{}]
set format x "%.2f"
set yrange [0:{}]
set ytics {}

# xtics only on final plot
unset xtics

set style fill solid 1.0 noborder

set tmargin at screen 0.9
set bmargin at screen 0.7
plot $occ   u 1:2 w fillsteps lc 1 fs solid title 'OCC'

set tmargin at screen 0.65
set bmargin at screen 0.45
plot $occ_optimized u 1:2 w fillsteps lc 3 fs solid title 'OCC optimized'

set xtics
set tmargin at screen 0.4
set bmargin at screen 0.2
plot $hwmon u 1:2 w fillsteps lc 2 fs solid title 'hwmon'

unset multiplot
""".format(bin_width_ms * 1000,
           xmin_ms, xmax_ms,
           ymax_next_tick,
           ymax_tick_delta))
