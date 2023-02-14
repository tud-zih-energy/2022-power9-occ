/*
 * based on p9_inband_sensors.c
 */

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
#include <cstring>
#include <stdexcept>
#include <endian.h>

#include <string>
#include <iostream>

#include "common.h"

using std::vector;
using namespace std;

/*
 * OCC Sensor Data
 *
 * OCC sensor data will use BAR2 (OCC Common is per physical drawer).
 * Starting address is at offset 0x00580000 from BAR2 base address.
 * Maximum size is 1.5MB.
 *
 * -------------------------------------------------------------------------
 * | Start (Offset from |    End    | Size       |Description           |
 * | BAR2 base address) |        |       |               |
 * -------------------------------------------------------------------------
 * |    0x00580000      |  0x005A57FF   |150kB     |OCC 0 Sensor Data Block|
 * |    0x005A5800      |  0x005CAFFF   |150kB       |OCC 1 Sensor Data Block|
 * |        :        |    :    |  :       |        :          |
 * |    0x00686800    |  0x006ABFFF   |150kB       |OCC 7 Sensor Data Block|
 * |    0x006AC000    |  0x006FFFFF   |336kB     |Reserved           |
 * -------------------------------------------------------------------------
 *
 *
 * OCC N Sensor Data Block Layout (150kB)
 *
 * The sensor data block layout is the same for each OCC N. It contains
 * sensor-header-block, sensor-names buffer, sensor-readings-ping buffer and
 * sensor-readings-pong buffer.
 *
 * ----------------------------------------------------------------------------
 * | Start (Offset from OCC |   End       | Size |Description              |
 * | N Sensor Data Block)   |           |      |                  |
 * ----------------------------------------------------------------------------
 * |    0x00000000        |  0x000003FF  |1kB   |Sensor Data Header Block   |
 * |    0x00000400        |  0x0000CBFF  |50kB  |Sensor Names              |
 * |    0x0000CC00        |  0x0000DBFF  |4kB   |Reserved              |
 * |    0x0000DC00        |  0x00017BFF  |40kB  |Sensor Readings ping buffer|
 * |    0x00017C00        |  0x00018BFF  |4kB   |Reserved              |
 * |    0x00018C00        |  0x00022BFF  |40kB  |Sensor Readings pong buffer|
 * |    0x00022C00        |  0x000257FF  |11kB  |Reserved              |
 * ----------------------------------------------------------------------------
 *
 * Sensor Data Header Block : This is written once by the OCC during
 * initialization after a load or reset. Layout is defined in 'struct
 * occ_sensor_data_header'
 *
 * Sensor Names : This is written once by the OCC during initialization after a
 * load or reset. It contains static information for each sensor. The number of
 * sensors, format version and length of each sensor is defined in
 * 'Sensor Data Header Block'. Format of each sensor name is defined in
 * 'struct occ_sensor_name'. The first sensor starts at offset 0 followed
 * immediately by the next sensor.
 *
 * Sensor Readings Ping/Pong Buffer:
 * There are two 40kB buffers to store the sensor readings. One buffer that
 * is currently being updated by the OCC and one that is available to be read.
 * Each of these buffers will be of the same format. The number of sensors and
 * the format version of the ping and pong buffers is defined in the
 * 'Sensor Data Header Block'.
 *
 * Each sensor within the ping and pong buffers may be of a different format
 * and length. For each sensor the length and format is determined by its
 * 'struct occ_sensor_name.structure_type' in the Sensor Names buffer.
 *
 * --------------------------------------------------------------------------
 * | Offset | Byte0 | Byte1 | Byte2 | Byte3 | Byte4 | Byte5 | Byte6 | Byte7 |
 * --------------------------------------------------------------------------
 * | 0x0000 |Valid  |           Reserved                    |
 * |        |(0x01) |                                |
 * --------------------------------------------------------------------------
 * | 0x0008 |            Sensor Readings                    |
 * --------------------------------------------------------------------------
 * |    :   |                :                    |
 * --------------------------------------------------------------------------
 * | 0xA000 |                     End of Data                    |
 * --------------------------------------------------------------------------
 *
 */

#define MAX_OCCS			8
#define MAX_CHARS_SENSOR_NAME		16
#define MAX_CHARS_SENSOR_UNIT		4

#define OCC_SENSOR_DATA_BLOCK_OFFSET	0x00580000
#define OCC_SENSOR_DATA_BLOCK_SIZE	0x00025800
//#define OCC_SENSOR_DATA_BLOCK_SIZE	5

enum occ_sensor_type {
	OCC_SENSOR_TYPE_GENERIC		= 0x0001,
	OCC_SENSOR_TYPE_CURRENT		= 0x0002,
	OCC_SENSOR_TYPE_VOLTAGE		= 0x0004,
	OCC_SENSOR_TYPE_TEMPERATURE	= 0x0008,
	OCC_SENSOR_TYPE_UTILIZATION	= 0x0010,
	OCC_SENSOR_TYPE_TIME		= 0x0020,
	OCC_SENSOR_TYPE_FREQUENCY	= 0x0040,
	OCC_SENSOR_TYPE_POWER		= 0x0080,
	OCC_SENSOR_TYPE_PERFORMANCE	= 0x0200,
};

enum occ_sensor_location {
	OCC_SENSOR_LOC_SYSTEM		= 0x0001,
	OCC_SENSOR_LOC_PROCESSOR	= 0x0002,
	OCC_SENSOR_LOC_PARTITION	= 0x0004,
	OCC_SENSOR_LOC_MEMORY		= 0x0008,
	OCC_SENSOR_LOC_VRM		= 0x0010,
	OCC_SENSOR_LOC_OCC		= 0x0020,
	OCC_SENSOR_LOC_CORE		= 0x0040,
	OCC_SENSOR_LOC_GPU		= 0x0080,
	OCC_SENSOR_LOC_QUAD		= 0x0100,
};

enum sensor_struct_type {
	OCC_SENSOR_READING_FULL		= 0x01,
	OCC_SENSOR_READING_COUNTER	= 0x02,
};


/**
 * struct occ_sensor_data_header -    Sensor Data Header Block
 * @valid:                When the value is 0x01 it indicates
 *                    that this header block and the sensor
 *                    names buffer are ready
 * @version:                Format version of this block
 * @nr_sensors:                Number of sensors in names, ping and
 *                    pong buffer
 * @reading_version:            Format version of the Ping/Pong buffer
 * @names_offset:            Offset to the location of names buffer
 * @names_version:            Format version of names buffer
 * @names_length:            Length of each sensor in names buffer
 * @reading_ping_offset:        Offset to the location of Ping buffer
 * @reading_pong_offset:        Offset to the location of Pong buffer
 * @pad/reserved:            Unused data
 */
struct occ_sensor_data_header {
	uint8_t valid;
	uint8_t version;
	uint16_t nr_sensors;
	uint8_t reading_version;
	uint8_t pad[3];
	uint32_t names_offset;
	uint8_t names_version;
	uint8_t name_length;
	uint16_t reserved;
	uint32_t reading_ping_offset;
	uint32_t reading_pong_offset;
} __attribute__((__packed__));

/**
 * struct occ_sensor_name -        Format of Sensor Name
 * @name:                Sensor name
 * @units:                Sensor units of measurement
 * @gsid:                Global sensor id (OCC)
 * @freq:                Update frequency
 * @scale_factor:            Scaling factor
 * @type:                Sensor type as defined in
 *                    'enum occ_sensor_typee
 * @location:                Sensor location as defined in
 *                    'enum occ_sensor_location'
 * @structure_type:            Indicates type of data structure used
 *                    for the sensor readings in the ping and
 *                    pong buffers for this sensor as defined
 *                    in 'enum sensor_struct_type'
 * @reading_offset:            Offset from the start of the ping/pong
 *                    reading buffers for this sensor
 * @sensor_data:            Sensor specific info
 * @pad:                Padding to fit the size of 48 bytes.
 */
struct occ_sensor_name {
	char name[MAX_CHARS_SENSOR_NAME];
	char units[MAX_CHARS_SENSOR_UNIT];
	uint16_t gsid;
	uint32_t freq;
	uint32_t scale_factor;
	uint16_t type;
	uint16_t location;
	uint8_t structure_type;
	uint32_t reading_offset;
	uint8_t sensor_data;
	uint8_t pad[8];
} __attribute__((__packed__));

/**
 * struct occ_sensor_record -        Sensor Reading Full
 * @gsid:                Global sensor id (OCC)
 * @timestamp:                Time base counter value while updating
 *                    the sensor
 * @sample:                Latest sample of this sensor
 * @sample_min:                Minimum value since last OCC reset
 * @sample_max:                Maximum value since last OCC reset
 * @csm_min:                Minimum value since last reset request
 *                    by CSM (CORAL)
 * @csm_max:                Maximum value since last reset request
 *                    by CSM (CORAL)
 * @profiler_min:            Minimum value since last reset request
 *                    by profiler (CORAL)
 * @profiler_max:            Maximum value since last reset request
 *                    by profiler (CORAL)
 * @job_scheduler_min:            Minimum value since last reset request
 *                    by job scheduler(CORAL)
 * @job_scheduler_max:            Maximum value since last reset request
 *                    by job scheduler (CORAL)
 * @accumulator:            Accumulator for this sensor
 * @update_tag:                Count of the number of ticks that have
 *                    passed between updates
 * @pad:                Padding to fit the size of 48 bytes
 */
struct occ_sensor_record {
	uint16_t gsid;
	uint64_t timestamp;
	uint16_t sample;
	uint16_t sample_min;
	uint16_t sample_max;
	uint16_t csm_min;
	uint16_t csm_max;
	uint16_t profiler_min;
	uint16_t profiler_max;
	uint16_t job_scheduler_min;
	uint16_t job_scheduler_max;
	uint64_t accumulator;
	uint32_t update_tag;
	uint8_t pad[8];
} __attribute__((__packed__));

/**
 * struct occ_sensor_counter -        Sensor Reading Counter
 * @gsid:                Global sensor id (OCC)
 * @timestamp:                Time base counter value while updating
 *                    the sensor
 * @accumulator:            Accumulator/Counter
 * @sample:                Latest sample of this sensor (0/1)
 * @pad:                Padding to fit the size of 24 bytes
 */
struct occ_sensor_counter {
	uint16_t gsid;
	uint64_t timestamp;
	uint64_t accumulator;
	uint8_t sample;
	uint8_t pad[5];
} __attribute__((__packed__));

enum sensor_attr {
	SENSOR_SAMPLE,
	SENSOR_ACCUMULATOR,
};

/// all offsets required to read one sensor
struct occ_sensor_offsets {
    /// one byte: is ping valid
    uint64_t ping_valid;
    /// one byte: is pong valid
    uint64_t pong_valid;
    /// sensor offset to ping/pong address
    uint64_t sensor_offset;
};

struct occ_sensor_offsets get_pwrsys_offsets(void *buf)
{
	struct occ_sensor_data_header *hb;
	struct occ_sensor_name *md;
	int i = 0;

	hb = (struct occ_sensor_data_header *)(uint64_t)buf;
	md = (struct occ_sensor_name *)((uint64_t)hb +
			be32toh(hb->names_offset));

	for (i = 0; i < be16toh(hb->nr_sensors); i++) {
		uint32_t offset =  be32toh(md[i].reading_offset);

		if (be16toh(md[i].type) ==OCC_SENSOR_TYPE_POWER) {
            if (string("PWRSYS") == string(md[i].name)) {
                struct occ_sensor_offsets offsets = {
                    .ping_valid = be32toh(hb->reading_ping_offset),
                    .pong_valid = be32toh(hb->reading_pong_offset),
                    .sensor_offset = offset,
                };
                return offsets;
            }
		}
	}

    throw std::runtime_error("PWRSYS not found");
}

void read_sensor(int fd, const struct occ_sensor_offsets& offsets, struct occ_sensor_record* sensor,
                 sample_source_t* source_ptr) {
    // determine if ping/pong are valid

    uint8_t ping_valid, pong_valid;
    lseek(fd, offsets.ping_valid, SEEK_SET);
    if (1 != read(fd, &ping_valid, 1)) {
        throw std::runtime_error("could not read ping valid flag");
    }
    lseek(fd, offsets.pong_valid, SEEK_SET);
    if (1 != read(fd, &pong_valid, 1)) {
        throw std::runtime_error("could not read pong valid flag");
    }

    if (!ping_valid && !pong_valid) {
        throw std::runtime_error("neither ping nor pong valid");
    }

    if (ping_valid && pong_valid) {
        // determine which sensor is newer
        struct occ_sensor_record ping_sensor, pong_sensor;
        lseek(fd, offsets.ping_valid + offsets.sensor_offset, SEEK_SET);
        if (sizeof(ping_sensor) != read(fd, &ping_sensor, sizeof(ping_sensor))) {
            throw std::runtime_error("could not read ping sensor");
        }
        lseek(fd, offsets.pong_valid + offsets.sensor_offset, SEEK_SET);
        if (sizeof(pong_sensor) != read(fd, &pong_sensor, sizeof(pong_sensor))) {
            throw std::runtime_error("could not read pong sensor");
        }

        if (be64toh(ping_sensor.timestamp) > be64toh(pong_sensor.timestamp)) {
            memcpy(sensor, &ping_sensor, sizeof(ping_sensor));
            *source_ptr = sample_source_t::both_use_ping;
        } else {
            memcpy(sensor, &pong_sensor, sizeof(ping_sensor));
            *source_ptr = sample_source_t::both_use_pong;
        }
        return;
    }

    if (ping_valid) {
        lseek(fd, offsets.ping_valid + offsets.sensor_offset, SEEK_SET);
        *source_ptr = sample_source_t::ping;
    } else {
        lseek(fd, offsets.pong_valid + offsets.sensor_offset, SEEK_SET);
        *source_ptr = sample_source_t::pong;
    }

    if (sizeof(struct occ_sensor_record) != read(fd, sensor, sizeof(struct occ_sensor_record))) {
        throw std::runtime_error("could not read OCC sensor");
    }
}

int main(int argc, char *argv[])
{
	void *buf;
	int fd, rc, bytes;

    if (argc < 3) {
        std::cerr << argv[0] << " INFILE OUTFILE" << std::endl;
        return 1;
    }

	//fd = open("/sys/firmware/opal/exports/occ_inband_sensors", O_RDONLY);
	fd = open(argv[1], O_RDONLY);
	if (fd < 0) {
		printf("Failed to open: %s\n", argv[1]);
		return -1;
	}

    std::ofstream samples_file(argv[2], std::ios::binary);
    if (!samples_file.is_open()) {
        cout << "failed to open " << argv[2] << endl;
        return -1;
    }

	buf = malloc(OCC_SENSOR_DATA_BLOCK_SIZE);
	if (!buf) {
		printf("Failed to allocate tmp buffer\n");
		return -1;
	}

    chrono::time_point<chrono::high_resolution_clock> start = chrono::high_resolution_clock::now();

    uint64_t samples_cnt;
    uint64_t max_samples_cnt = 1 << 24;

    vector<uint64_t> samples_sensor, samples_acc;
    vector<sample_source_t> samples_sources;
    vector<chrono::time_point<chrono::high_resolution_clock>> samples_time;
    samples_sensor.resize(max_samples_cnt);
    samples_acc.resize(max_samples_cnt);
    samples_time.resize(max_samples_cnt);
    samples_sources.resize(max_samples_cnt);

    cout << "reading offsets...";
    cout.flush();

    lseek(fd, 0, SEEK_SET);
    for (rc = bytes = 0; bytes < OCC_SENSOR_DATA_BLOCK_SIZE; bytes += rc) {
        rc = read(fd, (uint8_t*) buf + bytes, OCC_SENSOR_DATA_BLOCK_SIZE - bytes);
        if (!rc || rc < 0)
            return 2;
    }
    // get offset of a pwr sensor
    auto offset = get_pwrsys_offsets(buf);
    cout << "OK" << endl;

    cout << "init done, recording...";
    cout.flush();

    for (samples_cnt = 0; samples_cnt < max_samples_cnt; samples_cnt++) {
        struct occ_sensor_record occ_sensor;
        read_sensor(fd, offset, &occ_sensor, &samples_sources[samples_cnt]);

        samples_time[samples_cnt] = chrono::high_resolution_clock::now();

        samples_sensor[samples_cnt] = be16toh(occ_sensor.sample);
        samples_acc[samples_cnt] = be64toh(occ_sensor.accumulator);
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
