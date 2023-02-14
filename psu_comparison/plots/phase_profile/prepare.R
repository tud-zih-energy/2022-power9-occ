data_raw <- read.csv("./data/it01_run03.tsv", sep="\t", header=TRUE)


input_vs_occ_sum <- data.frame(
    input_sum = data_raw$taurus.taurusml5.ps0.power + data_raw$taurus.taurusml5.ps1.power,
#    occ_sum = data_raw$occ_power_gpu.0.power_from_energy +
#       data_raw$occ_power_gpu.1.power_from_energy + 
#       data_raw$occ_power_mem.0.power_from_energy +
#       data_raw$occ_power_mem.1.power_from_energy +
#       data_raw$occ_power_proc.0.power_from_energy +
#       data_raw$occ_power_proc.1.power_from_energy)
    occ_sum = data_raw$occ_power_system_bulk.0.power_from_energy)

# quadratic fit
input_vs_occ_sum$occ_sum2 <- input_vs_occ_sum$occ_sum^2
model_linear <- lm(input_sum ~ occ_sum,
                   data=input_vs_occ_sum)
model <- lm(input_sum ~ occ_sum + occ_sum2,
            data=input_vs_occ_sum)

print(coefficients(model))
print(coefficients(model_linear))

# verify fit
prediction <- predict(model,
                      list(occ_sum=input_vs_occ_sum$occ_sum,
                           occ_sum2=input_vs_occ_sum$occ_sum2))
mae <- mean(abs(input_vs_occ_sum$input_sum - prediction))
mape <-  100 * mean(abs((input_vs_occ_sum$input_sum - prediction)/input_vs_occ_sum$input_sum))
print(paste("MAE: ", mae, "W"))
print(paste("MAPE: ", mape, "%"))

prediction_linear <- predict(model_linear,
                             list(occ_sum=input_vs_occ_sum$occ_sum))
mae_linear <- mean(abs(input_vs_occ_sum$input_sum - prediction_linear))
mape_linear <-  100 * mean(abs((input_vs_occ_sum$input_sum - prediction_linear)/input_vs_occ_sum$input_sum))
print(paste("(lin) MAE: ", mae_linear, "W"))
print(paste("(lin) MAPE: ", mape_linear, "%"))

# compute overall efficiency
efficiency_total <- mean(100 * input_vs_occ_sum$occ_sum / input_vs_occ_sum$input_sum);
print(paste("avg efficiency: ", efficiency_total, "%"))

# dump data
input_vs_occ_sum$occ_sum2 <- NULL
write.table(input_vs_occ_sum,
            file="./plots/phase_profile/input_vs_occ_reported_sum.dat",
            quote=FALSE,
            sep=" ",
            row.names=FALSE)

# dump fit
watts <- seq(0.8 * min(input_vs_occ_sum$occ_sum),
             1.2 * max(input_vs_occ_sum$occ_sum))
watts2 <- watts^2
fit <- predict(model, list(occ_sum=watts, occ_sum2=watts2))
fit_data <- data.frame(
    occ_sum=watts,
    input_sum=fit)
write.table(fit_data,
            file="./plots/phase_profile/input_vs_occ_reported_sum_fit.dat",
            quote=FALSE,
            sep=" ",
            row.names=FALSE)
             
