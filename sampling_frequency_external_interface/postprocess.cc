#include <iostream>
#include <fstream>
#include <vector>
#include <endian.h>
#include <limits>
#include <cmath>
#include <cstdlib>
#include <unordered_set>
#include <cassert>

#include "common.h"

using namespace std;

struct value_gap {
    /// delta to previous value
    uint64_t delta = 0;
    /// current value
    uint64_t value = 0;
    /// this value was held for ... ns
    uint64_t duration_ns = 0;
    /// this value was measured in ... samples continuously
    uint64_t cnt_samples = 0;
};
typedef struct value_gap value_gap;

uint64_t get_smallest_diff(const vector<uint64_t>& v) {
    uint64_t previous = v.front();
    uint64_t smallest_diff = numeric_limits<uint64_t>::max();
    
    for (const uint64_t e : v) {
        if (e != previous) {
            uint64_t diff = previous - e;
            if (e > previous) {
                diff = e - previous;
            }
            if (diff < smallest_diff) {
                smallest_diff = diff;
            }
        }

        previous = e;
    }

    return smallest_diff;
}

vector<value_gap> get_value_gaps(const vector<uint64_t>& timings_ns, const vector<uint64_t>& values) {
    vector<value_gap> gaps;
    assert(timings_ns.size() == values.size());

    uint64_t last_value_index = 0;
    bool first = true;

    for (size_t i = 0; i < timings_ns.size(); i++) {
        if (values[i] != values[last_value_index]) {
            if (0 != last_value_index) {
                // skip for first value
                value_gap gap;
                gap.value = values[last_value_index];
                gap.duration_ns = timings_ns[i] - timings_ns[last_value_index];
                gap.cnt_samples = i - last_value_index;
                gap.delta = abs((int64_t)(values[last_value_index] - values[last_value_index - 1]));

                gaps.push_back(gap);
            }

            last_value_index = i;
        }
    }

    return gaps;
}

#define TO_FP(f)    ((f >> 8) * pow(10, ((int8_t)(f & 0xFF))))

int main(int argc, char** argv) {
    if (argc < 5) {
        cout << "Usage: " << argv[0] << " INFILE OUTFILE_RAW OUTFILE_SENSORJITTER OUTFILE_ACCJITTER" << endl;
        return 1;
    }

    ifstream infile(argv[1], std::ios::binary);
    if (!infile.is_open()) {
        cout << "couldn't open: " << argv[1] << endl;
        return 1;
    }

    ofstream out_raw(argv[2]), out_sensor(argv[3]), out_acc(argv[4]);
    if (!out_raw.is_open()) {
        cout << "couldn't open: " << argv[2] << endl;
        return 1;
    }
    if (!out_sensor.is_open()) {
        cout << "couldn't open: " << argv[3] << endl;
        return 1;
    }
    if (!out_acc.is_open()) {
        cout << "couldn't open: " << argv[4] << endl;
        return 1;
    }

    vector<uint64_t> timings_ns, samples, acc;
    vector<sample_source_t> sources;

    cout << "loading..." << flush;
    bool broken_record = false;

    while (!infile.eof()) {
        uint64_t timing, sample, acc_single, source;
        infile.read((char*) &timing, sizeof(timing));
        infile.read((char*) &sample, sizeof(sample));
        infile.read((char*) &acc_single, sizeof(acc_single));
        infile.read((char*) &source, sizeof(source));

        if (infile.eof()) {
            // TODO better check
            cout << "WARN" << endl;
            cout << "encountered broken record, discarding" << endl;
            broken_record = true;
            break;
        }

        timings_ns.push_back(timing);
        samples.push_back(sample);
        acc.push_back(acc_single);
        //freqs.push_back(TO_FP((uint32_t)le64toh(freq_le)));
        sources.push_back(static_cast<sample_source_t>(source));
    }
    if (!broken_record) {
        cout << "OK" << endl;
    }
    cout << "read " << samples.size() << " samples" << endl;

    cout << "sanity check..." << flush;
    bool timings_ascending = true;
    for (size_t i = 1; i < timings_ns.size(); i++) {
        if (timings_ns[i - 1] > timings_ns[i]) {
            timings_ascending = false;
            cout << "ERROR at i=" << i << endl;
            break;
        }
    }
    if (timings_ascending) {
        cout << "OK" << endl;
    } else {
        cout << endl;
        cout << "=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=" << endl;
        cout << endl;
        cout << "/!\\ sanity check failed, use data with caution /!\\" << endl;
        cout << endl;
        cout << "=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=*=" << endl;
        cout << endl;
    }

    cout << "analyzing" << flush;
    uint64_t smallest_sample_diff = get_smallest_diff(samples);
    cout << "." << flush;

    uint64_t smallest_acc_diff = get_smallest_diff(acc);
    cout << "." << flush;

    uint64_t smallest_timings_diff = get_smallest_diff(timings_ns);
    cout << "." << flush;

    unordered_set<sample_source_t> source_values(sources.begin(), sources.end());
    cout << "." << flush;

    auto sample_gaps = get_value_gaps(timings_ns, samples);
    cout << "." << flush;
    auto acc_gaps = get_value_gaps(timings_ns, acc);
    cout << "." << flush;

    cout << "OK" << endl;
    cout << endl;
    cout << "#samples:             " << samples.size() << endl;
    cout << "#sample changes:      " << sample_gaps.size() << endl;
    cout << "#acc changes:         " << acc_gaps.size() << endl;
    cout << "smallest value diff:  " << smallest_sample_diff << endl;
    cout << "smallest acc diff:    " << smallest_acc_diff << endl;
    cout << "smallest timing diff: " << smallest_timings_diff << "ns" << endl;
    cout << "#source values:       " << source_values.size() << endl;

    cout << endl;
    cout << "writing" << flush;

    out_raw << "#timing_ns\tsensor\tacc\tsource\n";
    for (size_t i = 0; i < timings_ns.size(); i++) {
        out_raw << timings_ns[i] << "\t"
                << samples[i] << "\t"
                << acc[i] << "\t"
                << sample_source_to_str(sources[i]) << "\n";
    }
    cout << "." << flush;

    out_sensor << "#sensor_value\tdelta_value\tduration_ns\tduration_samples\n";
    for (const auto& g : sample_gaps) {
        out_sensor << g.value << "\t"
                   << g.delta << "\t"
                   << g.duration_ns << "\t"
                   << g.cnt_samples << "\n";
    }
    cout << "." << flush;

    out_acc << "#acc_value\tdelta_value\tduration_ns\tduration_samples\n";
    for (const auto& g : acc_gaps) {
        out_acc << g.value << "\t"
                << g.delta << "\t"
                << g.duration_ns << "\t"
                << g.cnt_samples << "\n";
    }
    cout << "." << flush;
    
    cout << "OK" << endl;
    return 0;
}
