$hwmon <<EOD
40.049	0
40.050000000000004	0
40.051	0
40.052	0
40.053000000000004	0
40.054	0
40.055	0
40.056	0
40.057	0
40.058	0
40.059	0
40.06	0
40.061	0
40.062	0
40.063	0
40.064	0
40.065	2
40.066	0
40.067	1
40.068	3
40.069	1
40.07	7
40.071	30
40.072	60
40.073	111
40.074	140
40.075	167
40.076	121
40.077	87
40.078	52
40.079	13
40.08	0
40.081	2
40.082	1
40.083	1
40.084	1
40.085	0
40.086	0
40.087	0
40.088	0
40.089	0
40.09	0
40.091	0
40.092	0
40.093	0
40.094	0
40.095	0
40.096000000000004	0
40.097	0
40.098	0
40.099000000000004	0
EOD
$occ <<EOD
40.049	0
40.050000000000004	0
40.051	0
40.052	0
40.053000000000004	0
40.054	0
40.055	0
40.056	1
40.057	1
40.058	1
40.059	0
40.06	1
40.061	9
40.062	9
40.063	18
40.064	20
40.065	39
40.066	44
40.067	54
40.068	64
40.069	82
40.07	81
40.071	106
40.072	117
40.073	107
40.074	134
40.075	115
40.076	115
40.077	108
40.078	103
40.079	95
40.08	86
40.081	63
40.082	59
40.083	49
40.084	37
40.085	34
40.086	22
40.087	25
40.088	12
40.089	2
40.09	3
40.091	1
40.092	0
40.093	2
40.094	2
40.095	0
40.096000000000004	0
40.097	0
40.098	0
40.099000000000004	0
EOD
$occ_optimized <<EOD
40.049	0
40.050000000000004	0
40.051	0
40.052	0
40.053000000000004	0
40.054	0
40.055	0
40.056	0
40.057	0
40.058	0
40.059	0
40.06	0
40.061	0
40.062	0
40.063	0
40.064	0
40.065	1
40.066	1
40.067	4
40.068	1
40.069	1
40.07	6
40.071	32
40.072	50
40.073	74
40.074	87
40.075	115
40.076	73
40.077	79
40.078	44
40.079	16
40.08	4
40.081	4
40.082	3
40.083	2
40.084	2
40.085	0
40.086	0
40.087	0
40.088	0
40.089	0
40.09	0
40.091	0
40.092	0
40.093	0
40.094	0
40.095	0
40.096000000000004	0
40.097	0
40.098	0
40.099000000000004	0
EOD
# hwmon cnt > 40.1 ms: 376 (31.972789115646258 %)
#   occ cnt > 40.1 ms: 1008 (35.63096500530223 %)
#   occ optimized cnt > 40.1 ms: 340 (36.208732694355696 %)

set multiplot layout 2,1
#set ylabel "Number of Samples (\\#)"

# note: global label for x axis, "set xlabel" is foreach plot
set label "Interval (ms, bin width 1.0\\,µs)" at screen 0.5,0.05 center front
set label "Number of Samples (\\#)" at screen 0.05,0.5 center rotate by 90

set lmargin at screen 0.2
set rmargin at screen 0.9

set key inside top right

set xrange [40.05:40.1]
set format x "%.2f"
set yrange [0:200]
set ytics 100

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

