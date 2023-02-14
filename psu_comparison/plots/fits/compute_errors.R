data_raw_current <- read.csv("./data/it01_run03.tsv", sep="\t", header=TRUE)
powers_current <- data.frame(
    occ_reported = data_raw_current$occ_power_system_bulk.0.power_from_energy,
    occ_recalculated =
        data_raw_current$occ_power_gpu.0.power_from_energy +
        data_raw_current$occ_power_gpu.1.power_from_energy + 
        data_raw_current$occ_power_mem.0.power_from_energy +
        data_raw_current$occ_power_mem.1.power_from_energy +
        data_raw_current$occ_power_proc.0.power_from_energy +
        data_raw_current$occ_power_proc.1.power_from_energy,
    bmc = data_raw_current$taurus.taurusml5.power,
    psu = data_raw_current$taurus.taurusml5.ps0.power + data_raw_current$taurus.taurusml5.ps1.power)

analyze_profile <- function(powers, dataset_name){
    # compute square, b/c model will not
    powers$occ_reported2 <- powers$occ_reported ^ 2
    powers$occ_recalculated2 <- powers$occ_recalculated ^ 2

    # comparison w/o fit
    occ_reported_vs_occ_recalculated_mae <- mean(abs(powers$occ_recalculated - powers$occ_reported))
    occ_reported_vs_occ_recalculated_mape <- mean(abs(100 * (powers$occ_recalculated - powers$occ_reported)/powers$occ_reported))
    occ_reported_vs_bmc_mae <- mean(abs(powers$bmc - powers$occ_reported))
    occ_reported_vs_bmc_mape <- mean(abs(100 * (powers$bmc - powers$occ_reported)/powers$occ_reported))

    # comparison w/ fit

    # occ recalculated
    fit_psu_by_occ_recalculated_model <- lm(psu ~ occ_recalculated + occ_recalculated2,
                                            data=powers)
    fit_psu_by_occ_recalculated_prediction <- predict(fit_psu_by_occ_recalculated_model,
                                                    list(occ_recalculated=powers$occ_recalculated,
                                                        occ_recalculated2=powers$occ_recalculated2))
    fit_psu_by_occ_recalculated_mae <- mean(abs(powers$psu - fit_psu_by_occ_recalculated_prediction))
    fit_psu_by_occ_recalculated_mape <- mean(abs(100 * (powers$psu - fit_psu_by_occ_recalculated_prediction)/fit_psu_by_occ_recalculated_prediction))

    # occ reported
    fit_psu_by_occ_reported_model <- lm(psu ~ occ_reported + occ_reported2,
                                        data=powers)
    fit_psu_by_occ_reported_prediction <- predict(fit_psu_by_occ_reported_model,
                                                list(occ_reported=powers$occ_reported,
                                                    occ_reported2=powers$occ_reported2))
    fit_psu_by_occ_reported_mae <- mean(abs(powers$psu - fit_psu_by_occ_reported_prediction))
    fit_psu_by_occ_reported_mape <- mean(abs(100 * (powers$psu - fit_psu_by_occ_reported_prediction)/fit_psu_by_occ_reported_prediction))

    # occ reported vs occ recalculated
    fit_occ_reported_by_occ_recalculated_model <- lm(occ_reported ~ occ_recalculated + occ_recalculated2,
                                                    data=powers)
    fit_occ_reported_by_occ_recalculated_prediction <- predict(fit_occ_reported_by_occ_recalculated_model,
                                                            list(occ_recalculated=powers$occ_recalculated,
                                                                    occ_recalculated2=powers$occ_recalculated2))
    fit_occ_reported_by_occ_recalculated_mae <- mean(abs(powers$occ_reported - fit_occ_reported_by_occ_recalculated_prediction))
    fit_occ_reported_by_occ_recalculated_mape <- mean(abs(100 * (powers$occ_reported - fit_occ_reported_by_occ_recalculated_prediction)/fit_occ_reported_by_occ_recalculated_prediction))


    cat("
##########
 ", dataset_name, "
##########

=*= direct comparison =*=
OCC reported vs OCC recalculated
MAE:  ", occ_reported_vs_occ_recalculated_mae, "W
MAPE: ", occ_reported_vs_occ_recalculated_mape, "%
OCC reported vs BMC
MAE:  ", occ_reported_vs_bmc_mae, "W
MAPE: ", occ_reported_vs_bmc_mape, "%

=*= errors of *fit* =*=
fit PSU inputs based on OCC recalculated
MAE:  ", fit_psu_by_occ_recalculated_mae, "W
MAPE: ", fit_psu_by_occ_recalculated_mape, "%
fit PSU inputs based on OCC reported
MAE:  ", fit_psu_by_occ_reported_mae, "W
MAPE: ", fit_psu_by_occ_reported_mape, "%
fit OCC reported based on OCC recalculated
MAE:  ", fit_occ_reported_by_occ_recalculated_mae, "W
MAPE: ", fit_occ_reported_by_occ_recalculated_mape, "%
")
}

analyze_profile(powers_current, "current")
