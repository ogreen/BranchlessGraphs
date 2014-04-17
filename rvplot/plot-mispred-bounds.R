X <- with (Summary, data.frame (V.hat=Vs.tot.bry, M=Mispreds.tot.bry, Alg="Branch-based", Arch=Arch, Graph=Graph)) ; X <- rbind (X, with (Summary, data.frame (V.hat=Vs.tot.brl, M=Mispreds.tot.brl, Alg="Branch-avoiding", Arch=Arch, Graph=Graph)))

qplot (Alg, M/V.hat, data=X, geom="bar", stat="identity", fill=Alg, shape=Graph, facets=Arch ~ Graph) + geom_hline (yintercept=1, colour="black") + theme (legend.position="none")

