### File: generatePatterns.R
### Generate pattern files that I'll use as input for my simulation


NE <- 8000
NI <- 2000

sparsityE <- 0.1
recallsize <- 0.25

numberOfRandomPatternFiles <- 100
patternFilePrefix <- "randomPatternFile-"
recallFilePrefix <- "recallPatternFile-"

for (filenumber in c(1:numberOfRandomPatternFiles))
{
    patternFileName <- sprintf("%s%08d%s",patternFilePrefix,filenumber,".pat")
    recallFileName <- sprintf("%s%08d%s",recallFilePrefix,filenumber,".pat")
    randomlist <- sample(0:NE,NE*sparsityE,replace=F)
    recalllist <- sample(randomlist, NE*sparsityE*recallsize,replace=F)

    writeLines(as.character(randomlist),patternFileName,sep="\n")
    writeLines(as.character(recalllist),recallFileName,sep="\n")
}
