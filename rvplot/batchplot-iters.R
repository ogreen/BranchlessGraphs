# This script generates:
#
# figs/Time-vs-iters--sv--All.pdf
# figs/Instructions-vs-iters--sv--All.pdf
# figs/Branches-vs-iters--sv--All.pdf
# figs/Mispredictions-vs-iters--sv--All.pdf
# figs/HD--Time-vs-iters--sv--All.pdf
# figs/HD--Instructions-vs-iters--sv--All.pdf
# figs/HD--Branches-vs-iters--sv--All.pdf
# figs/HD--Mispredictions-vs-iters--sv--All.pdf
#

SAVE.PDF <- TRUE

COMP <- "sv"
for (HD in c (FALSE, TRUE)) {
  for (METRIC in c ("Time", "Mispredictions", "Branches", "Instructions")) {
    source ("plot-iters.R")
  }
}

COMP <- "bfs"
for (HD in c (FALSE, TRUE)) {
  for (METRIC in c ("Time", "Mispredictions", "Branches", "Instructions")) {
    source ("plot-iters.R")
  }
}

# eof

