source ("rvplot2-inc.R")

BATCH <- TRUE
SAVE.PDF <- TRUE

for (ARCH in as.vector (ARCHS.ALL.MAP)) {
  if ("ALGS" %in% ls ()) { rm ("ALGS") }
  if ("CODES" %in% ls ()) { rm ("CODES") }
  source ("explore-cpi-lasso.R")
}

# These graphs have at least 5 iterations for any (algorithm, implementation):
#GRAPHS <- c ("astro-ph", "auto", "coAuthorsDBLP", "cond-mat-2003", "cond-mat-2005", "coPapersDBLP", "ldoor", "power")
#
# These graphs have at least 6 iterations for any (algorithm, implementation):
#GRAPHS <- c ("auto", "coAuthorsDBLP", "cond-mat-2003", "cond-mat-2005", "coPapersDBLP", "ldoor", "power")
#
# These graphs have at least 7 iterations for any (algorithm, implementation):
#GRAPHS <- c ("auto", "coAuthorsDBLP", "coPapersDBLP", "ldoor", "power")
GRAPHS <- c ("auto", "coAuthorsDBLP", "coPapersDBLP", "ldoor")

FIT.PER.GRAPH <- TRUE
for (ARCH in as.vector (ARCHS.ALL.MAP)) {
  if ("ALGS" %in% ls ()) { rm ("ALGS") }
  if ("CODES" %in% ls ()) { rm ("CODES") }
  source ("explore-cpi-lasso.R")
}

# eof
