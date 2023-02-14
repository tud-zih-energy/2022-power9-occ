load_durations_ns <- function(fname) {
    data_all <- read.csv(fname,
                         sep="\t",
                         header=TRUE)
    len <- length(data_all$X.timing_ns)
    return(data_all$X.timing_ns[2:len] - data_all$X.timing_ns[1:(len-1)])
}

dump_info <- function(description, durations_ns) {
    cat(description, "-- mean, sstdev\n ",
        mean(durations_ns),
        sd(durations_ns),
        "\n")
}

durations_ns_occ_vanilla <- load_durations_ns("results/occ_raw.dat")
durations_ns_hwmon <- load_durations_ns("results/hwmon_raw.dat")

# load occ optimized into dataframe (instead of vector to allow for grouping afterwards)
data_occ_optimized <- read.csv("results/occ_optimized_raw.dat",
                               sep="\t",
                               header=TRUE)
len <- length(data_occ_optimized$X.timing_ns)

occ_optimized <- data.frame(
    duration_ns=data_occ_optimized$X.timing_ns[2:len] - data_occ_optimized$X.timing_ns[1:(len - 1)],
    ping_xor_pong_active=("ping" == data_occ_optimized$source | "pong" == data_occ_optimized$source)[2:len])

occo_all_n <- length(occ_optimized$duration_ns)
occo_single_n <- length(occ_optimized$duration_ns[occ_optimized$ping_xor_pong_active])
occo_both_n <- length(occ_optimized$duration_ns[!occ_optimized$ping_xor_pong_active])

cat("all in ns\n")
cat("occ optimized all samples: ", occo_all_n, "#\n",
    "    single buffer active: ", occo_single_n, "#\n",
    "     both buffers active: ", occo_both_n, "#\n",
    ">>> fraction of both buffers active: ", occo_both_n / occo_all_n, "\n")

# dump all info
cat("duration of a single readout (overhead), all in ns\n")
dump_info("hwmon", durations_ns_hwmon)
dump_info("occ vanilla", durations_ns_occ_vanilla)
dump_info("occ optimized (all)", occ_optimized$duration_ns)
dump_info("occ optimized (single buffer active)", occ_optimized$duration_ns[occ_optimized$ping_xor_pong_active])
dump_info("occ optimized (both buffers active)", occ_optimized$duration_ns[!occ_optimized$ping_xor_pong_active])
