# quick note:
# normally, i'd use python for these plots.
# however, the available libraries suck:
# matplotlib does not allow to have two input to a single node (only through graphical tricks)
# plotly does not generate useful node position, and requires you to compute them yourself if you need custom positioning (which is the entire point of the library)
# ggsankey reaches the needed sweet spot of configurability and automization.

library(ggsankey)
library(ggplot2)
library(dplyr)

data_all <- read.csv("data/it01_run03.tsv", sep="\t")

# extract row with largest bulk power consumption
row <- data_all[
    data_all$occ_power_system_bulk.0.power_from_energy ==
    max(data_all$occ_power_system_bulk.0.power_from_energy),]

row_data <- data.frame(
    psu0=row$taurus.taurusml5.ps0.power,
    psu1=row$taurus.taurusml5.ps1.power,
    bulk=row$occ_power_system_bulk.0.power_from_energy,

    proc0=row$occ_power_proc.0.power_from_energy,
    vdd0=row$occ_power_proc_vdd.0.power_from_energy,
    vdn0=row$occ_power_proc_vdn.0.power_from_energy,
    mem0=row$occ_power_mem.0.power_from_energy,
    gpu0=row$occ_power_gpu.0.power_from_energy,

    proc1=row$occ_power_proc.1.power_from_energy,
    vdd1=row$occ_power_proc_vdd.1.power_from_energy,
    vdn1=row$occ_power_proc_vdn.1.power_from_energy,
    mem1=row$occ_power_mem.1.power_from_energy,
    gpu1=row$occ_power_gpu.1.power_from_energy)

cat("experiment nr: ", row$experiment, "\n",
    "bulk power: ", row_data$bulk, "W\n",
    "PSUs in power: ", row_data$psu0, "W,", row_data$psu1, "W\n")

# compute leftovers
total_psu_in <- row_data$psu0 + row_data$psu1
total_efficiency <- row_data$bulk / total_psu_in
row_data$loss <- total_psu_in - row_data$bulk

row_data$bulk_unaccounted <- row_data$bulk - row_data$proc0 - row_data$mem0 - row_data$gpu0 - row_data$proc1 - row_data$mem1 - row_data$gpu1
row_data$proc0_unaccounted <- row_data$proc0 - row_data$vdd0 - row_data$vdn0
row_data$proc1_unaccounted <- row_data$proc1 - row_data$vdd1 - row_data$vdn1

# approximate loss of PSUs, is not mesaured in trace
row_data$psu0_out <- total_efficiency * row_data$psu0
row_data$psu1_out <- total_efficiency * row_data$psu1
row_data$psu0_loss <- row_data$psu0 - row_data$psu0_out
row_data$psu1_loss <- row_data$psu1 - row_data$psu1_out

# recap on format: every row is a single connection connection
# (note: this creates an empty dataframe)
columns <- c("x", "next_x", "node", "next_node", "value", "label", "fill")
sankey_data <- data.frame(matrix(ncol=length(columns),
                                 nrow=0))
colnames(sankey_data) <- columns

# note: for drawing, *sinks* (no follow-up node) must come *before* intermediate nodes
# note: ggsankey draws vertically in the order of y axis, i.e., bottom to top. I want top to bottom, so sometimes, the ordering is reversed.
nodenames <- c("PDU Outlet B", "PDU Outlet A", # PDU outlets
               "PSU 1", "PSU 0", # PSU outputs
               "bulk", # system bulk
               "proc 1", "proc 0", # processors
               # vvv only sinks below this vvv
               "unaccounted", # PSU loss
               "gpu 1", "gpu 0", "mem 1", "mem 0", # direct OCC decendents
               "unaccounted", # bulk unaccounted
               "vdd 1", "vdn 1", "vdd 0", "vdn 0", # proc sub-powers
               "unaccounted") # unaccounted sub-powers

# 1: PDU
# 2: BMC
# 3: BMC + OCC
# 4: OCC
# 5: computed/inferred
reported_by <- c(1, 1, # PDU outlets
                 2, 2, # PSU outputs
                 3, # system bulk
                 4, 4, # processors
                 # vvv only sinks below this vvv
                 5, # PSU loss
                 4, 4, 4, 4, # direct OCC decendents
                 5, # bulk unaccounted
                 4, 4, 4, 4, # proc sub-powers
                 5) # unaccounted proc sub-powers

sankey_add_conn <- function(x, next_x, node, next_node, val, final=FALSE) {
    sankey_data[nrow(sankey_data) + 1,] <<- c(x, next_x, node, next_node, val, NA, NA)

    if (final) {
        sankey_data[nrow(sankey_data) + 1,] <<- c(next_x, NA, next_node, NA, val, NA, NA)
    }

    # is partly redundant, but i don't care about efficiency here
    sankey_data$label <<- sapply(sankey_data$node,
                                 function(x){nodenames[[1 + x]]})
    sankey_data$fill <<- sapply(sankey_data$node,
                                function(x){reported_by[[1 + x]]})
}

# add actual data

# PDU -> PSU
sankey_add_conn(0, 1, 1, 3, row_data$psu0)
sankey_add_conn(0, 1, 0, 2, row_data$psu1)

# PSU -> [bulk, loss]
sankey_add_conn(1, 2, 3, 4, row_data$psu0_out)
sankey_add_conn(1, 2, 2, 4, row_data$psu1_out)

sankey_add_conn(1, 2, 3, 7, row_data$psu0_loss, TRUE)
sankey_add_conn(1, 2, 2, 7, row_data$psu1_loss, TRUE)

# bulk subdivision
sankey_add_conn(2, 3, 4, 6, row_data$proc0)
sankey_add_conn(2, 3, 4, 5, row_data$proc1)

sankey_add_conn(2, 3, 4, 9, row_data$gpu0, TRUE)
sankey_add_conn(2, 3, 4, 8, row_data$gpu1, TRUE)
sankey_add_conn(2, 3, 4, 11, row_data$mem0, TRUE)
sankey_add_conn(2, 3, 4, 10, row_data$mem1, TRUE)

sankey_add_conn(2, 3, 4, 12, row_data$bulk_unaccounted, TRUE)

# proc sub-powers
sankey_add_conn(3, 4, 5, 13, row_data$vdd1, TRUE)
sankey_add_conn(3, 4, 5, 14, row_data$vdn1, TRUE)

sankey_add_conn(3, 4, 6, 15, row_data$vdd0, TRUE)
sankey_add_conn(3, 4, 6, 16, row_data$vdn0, TRUE)

sankey_add_conn(3, 4, 6, 17, row_data$proc0_unaccounted, TRUE)
sankey_add_conn(3, 4, 5, 17, row_data$proc1_unaccounted, TRUE)

reporters <- c("PDU", "BMC", "both BMC and OCC", "OCC", "computed")
color_by_reporter <- c("#E8D4F7", "#700CBC", "#AE0D7A", "#EA202C", "#34090C")

plot_sankey <- ggplot(sankey_data,
                      aes(x = x,
                          next_x = next_x,
                          node = node,
                          next_node = next_node,
                          value = value,
                          label = label,
                          fill = factor(fill))) +
    geom_sankey(node.color = "black",
                node.alpha = 1,
                flow.fill = "gray",
                flow.color = "black",
                flow.alpha = .6) +
    geom_sankey_label(fill = NA, # do not include box/fill: latex changes font, and then the box size does not match anymore
                      label.size = NA,
                      hjust = 0,
                      position = position_nudge(x = .04)) + 
    xlim(-.15, 4.8) +
    scale_fill_manual(values = color_by_reporter,
                      name = "Reported by",
                      labels = reporters) +
    theme_sankey(base_size = 9) +
    theme(legend.position = "bottom",
          legend.title = element_text(size = 11),
          legend.text = element_text(size = 11),
          axis.text = element_blank(),
          axis.title = element_blank())




if (!interactive()) {
    require(tikzDevice)
    options(tikzLatexPackages = c(
                getOption("tikzLatexPackages"),
                 "\\usepackage{libertine}"))
    tikz("plots/domains/domains.tex",
        width = 9,
        height = 2.5,
        standAlone = TRUE) # create stand-alone PDF for inclusion with includegraphics

    print(plot_sankey)

    dev.off()
}
