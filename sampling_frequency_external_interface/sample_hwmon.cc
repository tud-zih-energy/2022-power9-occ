#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <math.h>
#include <chrono>
#include <vector>
#include <fstream>
#include <endian.h>
#include <string>
#include <thread>
#include <iostream>

#include "common.h"

using std::vector;
using namespace std;

int main(int argc, char *argv[])
{
    if (argc < 3) {
        std::cerr << argv[0] << " HWMONFILE OUTFILE" << std::endl;
        return 1;
    }

    int hwmon_fd = open(argv[1], O_RDONLY);
	if (hwmon_fd < 0) {
		printf("Failed to open: %s\n", argv[1]);
		return -1;
	}

    std::ofstream samples_file(argv[2], std::ios::binary);
    if (!samples_file.is_open()) {
        cout << "failed to open " << argv[2] << endl;
        return -1;
    }


    uint64_t samples_cnt;
    uint64_t max_samples_cnt = 1 << 24;

    vector<uint64_t> samples_sensor, samples_acc;
    vector<sample_source_t> samples_sources;
    vector<chrono::time_point<chrono::high_resolution_clock>> samples_time;
    samples_sensor.resize(max_samples_cnt);
    samples_time.resize(max_samples_cnt);
    samples_acc.resize(max_samples_cnt, 1337);
    samples_sources.resize(max_samples_cnt, sample_source_t::hwmon);

    cout << "init done, recording...";
    cout.flush();
    
    chrono::time_point<chrono::high_resolution_clock> start = chrono::high_resolution_clock::now();

    for (samples_cnt = 0; samples_cnt < max_samples_cnt; samples_cnt++) {
        lseek(hwmon_fd, 0, SEEK_SET);
        samples_time[samples_cnt] = chrono::high_resolution_clock::now();
        char buf[64];
        int rc = read(hwmon_fd, buf, sizeof(buf));
        if (rc <= 0) {
            printf("Failed to read data\n");
            return -1;
        }

        uint64_t sensor = atoll(buf);
        samples_sensor[samples_cnt] = sensor;
    }

    cout << "OK" << endl;
    cout << "saving...";
    cout.flush();

    size_t write_ok = 0;
    for (uint64_t i = 0; i < max_samples_cnt; i++) {
        uint64_t time_since_start = chrono::duration_cast<chrono::nanoseconds>(samples_time[i] - start).count();
        samples_file.write((char*) &time_since_start, sizeof(time_since_start));
        samples_file.write((char*) &samples_sensor[i], sizeof(time_since_start));
        samples_file.write((char*) &samples_acc[i], sizeof(time_since_start));
        samples_file.write((char*) &samples_sources[i], sizeof(time_since_start));
        
        if (samples_file.fail()) {
            cout << "FAIL" << endl;
            cout << "Writing failed on record #" << i << endl;
            cout << "aborting." << endl;
            return 1;
        }
    }

    cout << "OK" << endl;

    cout << "recorded " << samples_time.size() << " samples in " << chrono::duration_cast<chrono::milliseconds>(samples_time[samples_cnt - 1] - start).count() << "ms" << endl;
}
