#======================================================================
# Use this script to plot and compare instruction mixes, given a
# platform.

source ("rvplot2-inc.R")
source ("nnlm-inc.R")
library (GGally)

# Load data, if not already available
assign.if.undef ("All.data", load.perfdata.many ())
All.codes <- unique (get.all.colvals (All.data, "Implementation"))

#======================================================================
# Prompt user for platform

cat ("\n")

assign.if.undef ("BATCH", FALSE)
assign.if.undef ("SAVE.PDF", FALSE)

prompt.if.undef ("ARCH", keyword="architectures", ARCHS.ALL.MAP)

if (!BATCH) {
  prompt.any.if.undef ("ALGS", keyword="algorithms", unique (All.data[[ARCH]]$Algorithm))
  prompt.any.if.undef ("CODES", keyword="implementations", unique (All.data[[ARCH]]$Implementation))
} else {
  assign.if.undef ("ALGS", as.vector (unlist (ALGS.ALL.MAP)))
  assign.if.undef ("CODES", as.vector (unlist (CODES.ALL.MAP)))
}

# Check above configuration parameters
stopifnot ((length (ARCH) == 1) & (ARCH %in% ARCHS.ALL.MAP))
stopifnot (all (ALGS %in% ALGS.ALL.MAP))
stopifnot (all (CODES %in% CODES.ALL.MAP))

#======================================================================
# Determine output(s)

outfile.suffix <- get.file.suffix (ARCH, ALGS, CODES)

use.branch.based <- ("Branch-based" %in% CODES)
norm.tag <- if (use.branch.based) "--norm_bb" else ""

outfilename.imix <- sprintf ("figs2/explore-imix--%s%s.pdf", outfile.suffix, norm.tag)

cat (sprintf ("Output files%s:\n", if (SAVE.PDF) " [saving...]" else if (BATCH) "[*NOT* saving]" else "" ))
cat (sprintf ("  Instruction mix: %s\n", outfilename.imix))

#======================================================================
# Preprocess data

Df.arch <- get.perfdf.arch (All.data, ARCH, ALGS, CODES)
Vars.arch <- get.perfdf.var.info (Df.arch, All.data)

# Normalize by instructions per iteration _of the branchy code_, if available
Inst.norm <- get.perfdf.norm (Df.arch, Vars.arch, by="Instructions", use.branch.based=use.branch.based)
Df.arch.per.inst <- normalize.perfdf (Df.arch, Vars.arch, Inst.norm)

#======================================================================
# Plot

# Put data into a form that can be plotted with facets. The target
# output data frame of this step is 'Instructions.only'.
FV <- split.df.by.colnames (Df.arch.per.inst, Vars.arch$Index)
F.arch.per.inst <- flatten.keyvals.df (FV$A, FV$B)
Vars.arch.plot <- c (Vars.arch$Load, Vars.arch$Store, "Branches", "Mispredictions")
Instructions.only <- subset (F.arch.per.inst, Key %in% Vars.arch.plot)
Instructions.only$Key <- with (Instructions.only, factor (Key, levels=Vars.arch.plot))

# Compute median of medians, to use as reference lines
F.1 <- subset (F.arch.per.inst, Key %in% Vars.arch.plot)
Select.1 <- c (Vars.arch$Select.fit, "Key")
Medians.1 <- ddply (F.1, Select.1, summarise, Median.1=median (Value))
Select.2 <- setdiff (Select.1, "Graph")
Medians.2 <- ddply (Medians.1, Select.2, summarise, Median.2=median (Median.1))


norm.title <- if (use.branch.based) ", relative to Branch-based" else ""

Q.imix <- ggplot (Instructions.only, aes (x=Graph, y=Value, colour=Key))
Q.imix <- Q.imix + geom_boxplot ()
Q.imix <- Q.imix + geom_hline (data=Medians.2, aes (yintercept=Median.2, colour=Key), linetype="solid", alpha=0.5)
Q.imix <- Q.imix + theme (legend.position="bottom")
Q.imix <- Q.imix + facet_grid (Algorithm ~ Implementation)
Q.imix <- Q.imix + theme (axis.text.x=element_text (angle=30, hjust=1), axis.ticks=element_blank ())
Q.imix <- Q.imix + scale_y_continuous (breaks=gen_ticks_linear (Instructions.only$Value, step=gen.stepsize.auto (Instructions.only$Value)$scaled), labels=percent)
Q.imix <- Q.imix + xlab ("")
Q.imix <- Q.imix + ylab ("")
Q.imix <- add.title.optsub (Q.imix, ggtitle
                            , sprintf ("Instruction mix on %s%s", ARCH, norm.title)
                            , "(Distributions shown are taken over iterations)")
Q.imix <- set.hpcgarage.fill (Q.imix)
Q.imix <- set.hpcgarage.colours (Q.imix, name="Instruction type: ")

Q.imix.display <- set.all.font.sizes (Q.imix, base=10)
Q.imix.pdf <- set.all.font.sizes (Q.imix, base=12)

if (!BATCH) {
  do.imix <- prompt.yes.no ("\nDisplay instruction mix? ")
  if (do.imix) {
    setDevHD ()
    print (Q.imix.display)
    cat (sprintf ("\nSee also the 'instruction mix' plot (might be in a different window).\n"))
    pause.for.enter ()
  }
  do.imix.pdf <- prompt.yes.no (sprintf ("\nSave plot to '%s'? ", outfilename.imix))
} else {
  do.imix.pdf <- SAVE.PDF
}

if (do.imix.pdf) {
  cat (sprintf ("--> Writing '%s' ... ", outfilename.imix))
  setDevHD.pdf (outfilename.imix, l=18)
  print (Q.imix.pdf)
  dev.off ()
  cat ("done!\n\n")
}

# eof
