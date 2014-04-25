#======================================================================
# Use this script to plot the misprediction counts relative to Oded's
# lower and upper bounds analysis.

source ("rvplot2-inc.R")
source ("nnlm-inc.R")
library (GGally)

# Load data, if not already available
assign.if.undef ("All.data", load.perfdata.many ())
All.algs <- unique (get.all.colvals (All.data, "Algorithm"))
All.codes <- unique (get.all.colvals (All.data, "Implementation"))

#======================================================================
# Prompt user for platform

cat ("\n")

assign.if.undef ("BATCH", FALSE)
assign.if.undef ("SAVE.PDF", FALSE)

if (!BATCH) {
  prompt.any.if.undef ("ARCHS", keyword="architectures", ARCHS.ALL.MAP)
  prompt.if.undef ("ALG", keyword="algorithms", All.algs)
  prompt.any.if.undef ("CODES", keyword="implementations", All.codes)
} else {
  assign.if.undef ("ARCHS", as.vector (unlist (ARCHS.ALL.MAP)))
  assign.if.undef ("ALG", "SV")
  assign.if.undef ("CODES", as.vector (unlist (CODES.ALL.MAP)))
}

# Check above configuration parameters
stopifnot (all (ARCHS %in% ARCHS.ALL.MAP))
stopifnot ((length (ALG) == 1) & (ALG %in% ALGS.ALL.MAP))
stopifnot (all (CODES %in% CODES.ALL.MAP))

#======================================================================
# Determine output(s)

outfile.suffix <- get.file.suffix (ARCHS, ALG, CODES)
outfilename.mpb <- sprintf ("figs2/explore-mispred-bounds--%s.pdf", outfile.suffix)

cat (sprintf ("Output files%s:\n", if (SAVE.PDF) " [saving...]" else if (BATCH) "[*NOT* saving]" else "" ))
cat (sprintf ("  Misprediction bounds: %s\n", outfilename.mpb))

#======================================================================
# Preprocess data

Df <- get.perfdf (All.data, ARCHS, ALG, CODES)
Df <- subset (Df, Graph %in% GRAPHS.CONCISE) # Show data for only some graphs
Vars <- get.perfdf.var.info (Df, All.data)

cat ("Aggregating totals ...\n")
Df.tot <- total.perfdf (Df, Vars)

cat ("Extracting mispredictions ...\n")
Mispreds <- Df.tot[, c ("Architecture", "Algorithm", "Implementation", "Graph", "Mispredictions", "Vertices")]

#======================================================================
# Plot

alg.tag.display <- ALGS.FANCY.MAP[[ALG]]

# Annotate the data a little
Mispreds.plot <- transform (Mispreds
                            , X=gsub ("Branch-", "Branch-\n", Implementation)
                            , Y=Mispredictions / Vertices)

# HACK: Reorder implementation labs
Mispreds.plot$X <- with (Mispreds.plot, factor (X, levels=rev (levels (X))))

Q.mpb <- qplot (X, Y, data=Mispreds.plot, geom="bar", stat="identity"
            , fill=X, facets=Architecture ~ Graph)
Q.mpb <- Q.mpb + geom_hline (yintercept=1, colour="black", linetype="dashed") # add y=1 reference line
if (ALG == "SV") {
  Q.mpb <- Q.mpb + geom_hline (yintercept=3, colour="black", linetype="dashed") # add y=3 reference line
}
Q.mpb <- Q.mpb + xlab ("")
Q.mpb <- Q.mpb + ylab ("")
Q.mpb <- add.title.optsub (Q.mpb, ggtitle
                           , main=sprintf ("%s Branch Mispredictions", alg.tag.display)
                           , sub="(relative to lower-bound, at y=1)")
Q.mpb <- Q.mpb + theme (legend.position="none") # hide legend
Q.mpb <- set.hpcgarage.colours (Q.mpb)
Q.mpb <- set.hpcgarage.fill (Q.mpb)

Q.mpb.display <- set.all.font.sizes (Q.mpb, base=10)
Q.mpb.pdf <- set.all.font.sizes (Q.mpb, base=12)

if (!BATCH) {
  do.mpb <- prompt.yes.no ("\nDisplay misprediction bounds? ")
  if (do.mpb) {
    setDevPage.portrait ()
    print (Q.mpb)
    pause.for.enter ()
  }
  do.mpb.pdf <- prompt.yes.no (sprintf ("\nSave plot to '%s'? ", outfilename.mpb))
} else {
  do.mpb.pdf <- SAVE.PDF
}

if (do.mpb.pdf) {
  cat (sprintf ("\n--> Writing '%s' ... ", outfilename.mpb))
  setDevPage.portrait.pdf (outfilename.mpb, l=10)
  print (Q.mpb.pdf)
  dev.off ()
  cat ("done!\n\n")
}

#======================================================================
# eof
