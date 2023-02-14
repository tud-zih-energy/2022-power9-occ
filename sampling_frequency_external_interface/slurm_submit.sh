#!/bin/bash
#SBATCH --hint=multithread
#SBATCH --time=00:30:00
#SBATCH --ntasks=1
#SBATCH --nodes=1
#SBATCH --cpus-per-task=176
#SBATCH --exclusive
#SBATCH --partition=ml
#SBATCH --gres=gpu:0
#SBATCH --output=run_%j.out

set -euo pipefail

echo "running on $(hostname)"

module purge
module load modenv/ml
module load GCC

echo "Checking access rights for OCC inband sensors file (must be user-readable): "
export 'SENSOR_FILE=/sys/firmware/opal/exports/occ_inband_sensors'
ls -lah $SENSOR_FILE
test -r $SENSOR_FILE || (echo '[!!] sensor data unreadable: '"$SENSOR_FILE" && exit 1)

echo "checking sensor name (should be 'System'):"
# note that changing the file here is not sufficient,
# you gotta change it in the makefile too
LABEL=$(cat /sys/class/hwmon/hwmon0/power11_label)
echo $LABEL
test "$LABEL" = "System" || (echo '[!!] wrong sensor selected' && exit 1)

echo "Cleaning..."
make clean

echo "recording..."
make results

echo "Done -- create plots with 'make plots' where datamash is available"
