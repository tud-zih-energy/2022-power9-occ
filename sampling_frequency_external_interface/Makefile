CXX=g++
PROGRAMS=sample_occ sample_hwmon sample_occ_optimized postprocess
DUMPS=results/hwmon_dump.bin results/occ_dump.bin results/occ_optimized.bin
RESULTS=results/hwmon_raw.dat results/hwmon_sensorjitter.dat results/hwmon_accjitter.dat results/occ_raw.dat results/occ_sensorjitter.dat results/occ_accjitter.dat results/occ_optimized_raw.dat results/occ_optimized_sensorjitter.dat results/occ_optimized_accjitter.dat
PLOTS=results/hist_value_interval.gp results/hist_readout_separation.gp results/means.txt

all: plots
plots: $(PLOTS)
results: $(RESULTS)

clean:
	rm -rf $(PROGRAMS)

unpack:
	find results -name '*.xz' -exec xz -f -k -d '{}' ';'

.PHONY: clean unpack

programs: $(PROGRAMS)
postprocess: postprocess.cc
%: %.cc
	$(CXX) -std=c++11 -o $@ $< -Ofast -lm

results/hwmon_dump.bin: sample_hwmon
	# ensure expected sensor assignment
	test `cat /sys/class/hwmon/hwmon0/power11_label` "=" "System"
	mkdir -p results
	./sample_hwmon /sys/class/hwmon/hwmon0/power11_input ./results/hwmon_dump.bin

results/occ_dump.bin: sample_occ
	mkdir -p results
	./sample_occ /sys/firmware/opal/exports/occ_inband_sensors ./results/occ_dump.bin

results/occ_optimized_dump.bin: sample_occ_optimized
	mkdir -p results
	./sample_occ_optimized /sys/firmware/opal/exports/occ_inband_sensors ./results/occ_optimized_dump.bin

results/%_raw.dat results/%_sensorjitter.dat results/%_accjitter.dat: postprocess results/%_dump.bin
	./postprocess results/$*_dump.bin results/$*_raw.dat results/$*_sensorjitter.dat results/$*_accjitter.dat

results/hist_value_interval.gp: $(RESULTS) hist_value_interval.py
	python3 hist_value_interval.py > ./results/hist_value_interval.gp

results/hist_readout_separation.gp: $(RESULTS) hist_readout_separation.py
	python3 hist_readout_separation.py > ./results/hist_readout_separation.gp

results/means.txt: $(RESULTS) get_overhead_means.R get_valuechange_means.sh
	rm -f results/means.txt
	./get_valuechange_means.sh >> results/means.txt
	R -s -q --vanilla < get_overhead_means.R >> results/means.txt


