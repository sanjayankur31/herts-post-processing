### File: generatePatterns.R
### Generate pattern files that I'll use as input for my simulation


NE <- 8000
NI <- 2000

patternPercentOfTotal <- 0.1
recallPercentOfPattern <- 0.50

numberOfRandomPatternFiles <- 60
patternFilePrefix <- "randomPatternFile-"
recallFilePrefix <- "recallPatternFile-"

for (filenumber in c(1:numberOfRandomPatternFiles))
{
    patternFileName <- sprintf("%s%08d%s",patternFilePrefix,filenumber,".pat")
    recallFileName <- sprintf("%s%08d%s",recallFilePrefix,filenumber,".pat")
    randomlist <- sample(0:NE,NE*patternPercentOfTotal,replace=F)
    recalllist <- sample(randomlist, NE*patternPercentOfTotal*recallPercentOfPattern,replace=F)

    writeLines(as.character(randomlist),patternFileName,sep="\n")
    writeLines(as.character(recalllist),recallFileName,sep="\n")
}
