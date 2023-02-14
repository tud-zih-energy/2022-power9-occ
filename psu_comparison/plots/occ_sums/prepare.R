data_raw <- read.csv("./data/it01_run03.tsv", sep="\t", header=TRUE)


occ_sum <- data.frame(
    reported = data_raw$occ_power_system_bulk.0.power_from_energy,
    recalculated =
        data_raw$occ_power_gpu.0.power_from_energy +
        data_raw$occ_power_gpu.1.power_from_energy + 
        data_raw$occ_power_mem.0.power_from_energy +
        data_raw$occ_power_mem.1.power_from_energy +
        data_raw$occ_power_proc.0.power_from_energy +
        data_raw$occ_power_proc.1.power_from_energy,
    kernel = data_raw$experiment)

# compute if there is memory used
mem_kernels <- c(8, 14, 13, # mem read, copy, write
                 7) # matmul

# note: cast to int for compatibility
occ_sum$uses_mem <- 1 * (occ_sum$kernel %in% mem_kernels)

# compute difference
mae <- mean(abs(occ_sum$recalculated - occ_sum$reported))
mape <- 100 * mean(abs((occ_sum$recalculated - occ_sum$reported)/occ_sum$reported))
print(paste("MAE: ", mae, "W"))
print(paste("MAPE: ", mape, "%"))

#plot(x = occ_sum$reported,
#     y = occ_sum$recalculated,
#     # color +1, b/c 0 is "transparent"
#     col = 1 + occ_sum$uses_mem,
#     pch = 16,
#     )
#legend("topleft",
#       legend = c("uses mem", "no mem"),
#       pch = 16,
#       col = 1 + c(1, 0),
#       )
#lines(x=450:800,
#      y=450:800)


# dump data
write.table(occ_sum,
            file="./plots/occ_sums/occ_sums.dat",
            quote=FALSE,
            sep=" ",
            row.names=FALSE)
             
