#======================================================================
# Use this script to plot and compare instruction mixes, given a
# platform.

source ("rvplot2-inc.R")
source ("nnlm-inc.R")
library (GGally)

Data <- load.perfdata.many ()
All.codes <- unique (get.all.colvals (Data, "Implementation"))

#======================================================================
# Prompt user for platform

cat ("\n")

prompt.if.undef ("ARCH", keyword="architectures", ARCHS.ALL.MAP)
prompt.any.if.undef ("ALGS", keyword="algorithms", unique (Data[[ARCH]]$Algorithm))
prompt.any.if.undef ("CODES", keyword="implementations", unique (Data[[ARCH]]$Implementation))

assign.if.undef ("ANALYSIS.VARS", NULL)
assign.if.undef ("BATCH", FALSE)
assign.if.undef ("SAVE.PDF", FALSE)

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

Df.arch <- get.perfdf.arch (Data, ARCH, ALGS, CODES)
Vars.arch <- get.perfdf.var.info (Df.arch, Data)

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

# Compute medians, to use as reference lines


norm.title <- if (use.branch.based) ", relative to Branch-based" else ""

Q.imix <- ggplot (Instructions.only, aes (x=Graph, y=Value, colour=Key))
Q.imix <- Q.imix + geom_boxplot ()
Q.imix <- Q.imix + theme (legend.position="bottom")
Q.imix <- Q.imix + facet_grid (Algorithm ~ Implementation)
Q.imix <- Q.imix + theme (axis.text.x=element_text (angle=30, hjust=1), axis.ticks=element_blank ())
Q.imix <- Q.imix + scale_y_continuous (breaks=gen_ticks_linear (Instructions.only$Value, step=gen.stepsize.auto (Instructions.only$Value)$scaled), labels=percent)
Q.imix <- Q.imix + xlab ("")
Q.imix <- Q.imix + ylab ("")
Q.imix <- add.title.optsub (Q.imix, ggtitle
                            , sprintf ("Instruction mix%s", norm.title)
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

    do.imix.pdf <- prompt.yes.no (sprintf ("\nSave plot to '%s'? ", outfilename.imix))
    if (do.imix.pdf) {
      cat ("--> Writing ... ")
      setDevHD.pdf (outfilename.imix, l=18)
      print (Q.imix.pdf)
      dev.off ()
      cat ("done!\n\n")
    }
  }
}

# eof
