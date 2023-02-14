raw/run04:
	cd raw && sha256sum -c checksums.txt
	cd raw && (pv run04.tar.xz | tar -xJf-)

extract: raw/run04
	cd data && python ../extract_from_trace.py ../$(shell find ./raw -name traces.otf2)
	# accidentally generated
	rm data/1hz_sensor.dat data/1hz_derived.dat

clean:
	rm -rf raw/run04
	rm -f data/*.dat
.PHONY: clean
