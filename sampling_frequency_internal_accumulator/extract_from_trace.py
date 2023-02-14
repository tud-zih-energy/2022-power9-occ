import otf2
import sys

def print_bar(now, total, length=20, char_done='#', char_todo='.'):
    cnt_done = int(length * now / total)
    cnt_todo = length - cnt_done
    print('\r[{}{}]\r'.format(char_done * cnt_done, char_todo * cnt_todo), end='', file=sys.stderr)

def write_to_dat(filename, data, header=''):
    with open(filename, 'w') as f:
        if header != '':
            f.write('#{}\n'.format(header))
        for line in data:
            f.write('{}\n'.format(' '.join(map(str, line))))
        

if len(sys.argv) < 2:
    print('expected: {} FILE'.format(sys.argv[0]), file=sys.stderr)
    exit(1)

fname = sys.argv[1]
print('reading from {}'.format(fname), file=sys.stderr)

with otf2.reader.open(fname) as trace:
    print('found {} metrics, searching...'.format(len(trace.definitions.metric_members)), file=sys.stderr)
    metric_pwr_sensor = None
    metric_pwr_from_energy = None
    metric_freq = None

    for metric in trace.definitions.metric_members:
        if 'occ_power_system' == metric.name:
            metric_pwr_sensor = metric
        if 'occ_power_system_from_energy' == metric.name:
            metric_pwr_from_energy = metric
        if 'Utility' == metric.name:
            # utility metric contains the frequency of the workload in Hz
            metric_freq = metric

        # names after rework
        if 'occ_power_proc.0.power_from_energy' == metric.name:
            metric_pwr_from_energy = metric
        if 'occ_power_proc.0.direct_sample' == metric.name:
            metric_pwr_sensor = metric

    if not metric_pwr_sensor or not metric_pwr_from_energy or not metric_freq:
        print('could not find all required metrics in trace', file=sys.stderr)
        print('Utility:         {}'.format('OK' if metric_freq else 'MISSING'), file=sys.stderr)
        print('Power (Sensor):  {}'.format('OK' if metric_pwr_sensor else 'MISSING'), file=sys.stderr)
        print('Power (Derived): {}'.format('OK' if metric_pwr_from_energy else 'MISSING'), file=sys.stderr)
        exit(0)

    # locations, that could contain one of the events above
    interesting_locations = []

    for location in trace.definitions.locations:
        if otf2.LocationType.METRIC == location.type:
            interesting_locations.append(location)
        if otf2.LocationType.CPU_THREAD == location.type and 'OMP thread 7' == location.name:
            # only use one thread for utility metric (it is contained in all threads)
            interesting_locations.append(location)

    print('will examine {} out of {} locations'.format(len(interesting_locations), len(trace.definitions.locations)), file=sys.stderr)

    data_points = trace.events(interesting_locations)
    print('recorded {} data points total'.format(len(data_points)), file=sys.stderr)
    processed = 0

    current_freq = None
    timestamp_base = None
    stored_pwr_sensor = []
    stored_pwr_from_energy = []

    for location, event in data_points:
        processed += 1
        if 0 == processed % 1000:
            print_bar(processed, len(data_points))
        if not isinstance(event, otf2.events.Metric):
            continue

        if isinstance(event.metric, otf2.definitions.MetricClass) and metric_freq in event.metric.members:
            if current_freq:
                # flush current experiment
                write_to_dat('{}hz_sensor.dat'.format(current_freq), stored_pwr_sensor, 'time(s) power(W)')
                write_to_dat('{}hz_derived.dat'.format(current_freq), stored_pwr_from_energy, 'time(s) power(W)')

            # start new section
            stored_pwr_sensor = []
            stored_pwr_from_energy = []
            current_freq = event.values[0]
            timestamp_base = event.time

        elif isinstance(event.metric, otf2.definitions.MetricInstance):
            if current_freq:
                if metric_pwr_sensor in event.metric.metric_class.members:
                    stored_pwr_sensor.append(((event.time - timestamp_base) / trace.timer_resolution, event.values[0]))
                if metric_pwr_from_energy in event.metric.metric_class.members:
                    stored_pwr_from_energy.append(((event.time - timestamp_base) / trace.timer_resolution, event.values[0]))

    print_bar(len(data_points), len(data_points))
    print('', file=sys.stderr)

