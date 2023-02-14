import re


def get_time_per_bin(bin_width: float, time_s_min, time_s_max, filename: str):
    """
    returns time in s per bin
    """
    time_s_by_bin = {}

    comment_re = re.compile(r"^\s*#.*$")

    with open(filename) as sensor_file:
        # data starts always at 0s
        last_timestamp_s = 0
        for line in sensor_file:
            if comment_re.match(line):
                continue

            fields = line.split()
            timestamp_s = float(fields[0])
            power_w = float(fields[1])

            if timestamp_s < time_s_min or time_s_max > time_s_max:
                last_timestamp_s = timestamp_s
                continue

            bin_num = int(power_w/bin_width)
            if bin_num not in time_s_by_bin:
                time_s_by_bin[bin_num] = 0

            delta_t_s = timestamp_s - last_timestamp_s
            time_s_by_bin[bin_num] += delta_t_s

            last_timestamp_s = timestamp_s

    return time_s_by_bin


def print_hist(dataset_name, bin_width, time_s_by_bin):
    print("${} << EOD".format(dataset_name))

    bin_min = min(time_s_by_bin.keys())
    bin_max = max(time_s_by_bin.keys())

    # initialize at 0
    print("{}\t0".format((bin_min - 1) * bin_width))

    total_time_s = sum(time_s_by_bin.values())

    for bin_num in range(bin_min, bin_max + 1):
        lower_bound = bin_num * bin_width

        if bin_num in time_s_by_bin:
            value = time_s_by_bin[bin_num]
        else:
            value = 0

        print("{}\t{}".format(lower_bound, value))

    # make data drop to 0 after last bin
    bin_max_upper_bound = (bin_max + 1) * bin_width
    print("{}\t0".format(bin_max_upper_bound))

    print("EOD")
    print("")


bin_width = 2
frequencies = [499, 998, 1996, 2045]

for frequency in frequencies:
    sensor_time_s_by_bin = \
        get_time_per_bin(bin_width, 5, 15, "data/{}hz_sensor.dat".format(frequency))
    acc_time_s_by_bin = \
        get_time_per_bin(bin_width, 5, 15, "data/{}hz_derived.dat".format(frequency))

    print_hist("sensor_{}hz".format(frequency), bin_width, sensor_time_s_by_bin)
    print_hist("acc_{}hz".format(frequency), bin_width, acc_time_s_by_bin)

print("""
# puts x-label and y-label manually
set label "Power (W, 2\\\\,W bins)" at screen 0.50,0.02 center
set label "Duration (s)" at screen 0.01,0.5 center rotate by 90

set multiplot layout 4,1

set xzeroaxis
set xrange [130:340]

set lmargin at screen 0.1
set rmargin at screen 0.9

""".format(bin_width))

plot_num = 0

for frequency in frequencies:
    if 0 == plot_num:
        # first plot
        print("set key outside horizontal above")
    else:
        # other plot
        print("unset key")

    if plot_num <= 2:
        # first three plots
        print("""
        set xtics ("" 150, "" 200, "" 250, "" 300)
        set yrange [-2.5:2.5]
        set ytics ("2" 2, "1" 1, "0" 0, "1" -1, "2" -2)
        """)
    else:
        # last plot
        print("""
        set xtics 50
        set yrange [-10:3]
        set ytics ("2" 2, "0" 0, "2" -2, "4" -4, "6" -6, "8" -8)
        """)

    # compute position
    total_height = 0.2
    # 1- ... top is 1, bottom is 0
    # 0.1 ... margin
    # 0.5*total_height ... find center
    # total_height*plot_num ... offset for multiplot
    center = 1 - (0.1 + 0.5 * total_height + total_height * plot_num)
    top = center - total_height * .5 + 0.01
    bottom = center + total_height * .5 - 0.01
    label_y = center - 0.3 * total_height

    print("""
set tmargin at screen {top}
set bmargin at screen {bottom}
set label "{f}\\\\,Hz" at screen 0.85, {label_y} right
plot $sensor_{f}hz w fillsteps fs solid t "Direct Samples",\\
     $acc_{f}hz u 1:((-1 * $2)) w fillsteps fs solid t "Power From Energy"
    """.format(f=frequency, top=top, bottom=bottom, label_y=label_y))
    plot_num += 1

print("""
unset multiplot
""")
