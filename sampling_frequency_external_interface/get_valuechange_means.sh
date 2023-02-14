#!/bin/sh
set -eu

which datamash >/dev/null 2>&1 || (echo "[!!] datamash not found" && exit 1)

echo "# separation of value changes"
echo "#  all in ns"
echo "###"
echo ""
echo "hwmon (sensor)"
echo -n "  mean of <=60ms: "
cat results/hwmon_sensorjitter.dat | awk '{if($3 <= 60000000){print $3}}' | datamash mean 1
echo -n "            mean: "
cat results/hwmon_sensorjitter.dat | sed -e '/[[:space:]]*#/d' | datamash mean 3

echo "occ (acc)"
echo -n "  mean of <=60ms: "
cat results/occ_accjitter.dat | awk '{if($3 <= 60000000){print $3}}' | datamash mean 1
echo -n "            mean: "
cat results/occ_accjitter.dat | sed -e '/[[:space:]]*#/d' | datamash mean 3

echo "occ (sensor)"
echo -n "  mean of <=60ms: "
cat results/occ_sensorjitter.dat | awk '{if($3 <= 60000000){print $3}}' | datamash mean 1
echo -n "            mean: "
cat results/occ_sensorjitter.dat | sed -e '/[[:space:]]*#/d' | datamash mean 3

echo "occ optimized (acc)"
echo -n "  mean of <=60ms: "
cat results/occ_optimized_accjitter.dat | awk '{if($3 <= 60000000){print $3}}' | datamash mean 1
echo -n "            mean: "
cat results/occ_optimized_accjitter.dat | sed -e '/[[:space:]]*#/d' | datamash mean 3

echo "occ optimized (sensor)"
echo -n "  mean of <=60ms: "
cat results/occ_optimized_sensorjitter.dat | awk '{if($3 <= 60000000){print $3}}' | datamash mean 1
echo -n "            mean: "
cat results/occ_optimized_sensorjitter.dat | sed -e '/[[:space:]]*#/d' | datamash mean 3
