# This file takes a file with all the spikes from the simulation as input
# Copyright 2014 Ankur Sinha - a.sinha2@herts.ac.uk

# Not required
#require(data.table)
library(foreach)
library(doParallel)

strt <- Sys.time ()

# Globals
numberNeuronsE <- as.numeric(8000)
numberNeuronsE.rows <- as.numeric(100)
numberNeuronsE.cols <- as.numeric(80)
numberNeuronsI <- as.numeric(2000)
numberNeuronsI.rows <- as.numeric(100)
numberNeuronsI.cols <- as.numeric(20)

# Helper functions that might come in handy
# Define a printf function
printf <- function(...) invisible(print(sprintf(...)))
reindex <- function (x) (x - numberNeuronsE)
rotate <- function(x) t(apply(x, 2, rev))

# Other functions
mainworkfunction <- function (time, spikeDataE, spikeDataI ) 
{
    if (!is.na(as.numeric(time)))
    {
        #formattedTime <- sprintf("%.6f", as.numeric(time))
        formattedOutputTime <- sprintf("%013.6f", as.numeric(time))
        outputFile <- paste(paste(timestamp, "-", sep = ""), formattedOutputTime, sep = "")
    
        matrixE <- doYourThing(time, spikeDataE, TRUE, output)
        matrixI <- doYourThing(time, spikeDataI, FALSE, output)
        #write(matrixE, ncolumns = numberNeuronsE.cols, file = "matrix.e.rate", sep  = " ")
        #write(matrixI, ncolumns = numberNeuronsI.cols, file = "matrix.i.rate", sep  = " ")

        print("MatrixE dimensions are:")
        print(dim(matrixE))
        outputFileNameE <- paste(output, ".merged.e.rate", sep = "")
        write(matrixE, ncolumns = numberNeuronsE.cols, file = outputFileNameE, sep  = " ")
        print(paste("Merged matrix file written to: ", outputFileNameE))

        print("MatrixI dimensions are:")
        print(dim(matrixI))
        outputFileNameI <- paste(output, ".merged.i.rate", sep = "")
        write(matrixI, ncolumns = numberNeuronsI.cols, file = outputFileNameI, sep  = " ")
        print(paste("Merged matrix file written to: ", outputFileNameI))

        matrixFinal <- cbind(matrixE, matrixI)

        # Flip vertically
        #matrixFinalFlipped <- apply(matrixFinal, 2, rev)
        matrixFinalResult <- (t(matrixFinal))

        outputFileNameMerged <- paste(output, ".merged.rate", sep = "")
        # Flip vertically
        #write(rotate(matrixFinal), ncolumns = (numberNeuronsI.cols + numberNeuronsE.cols), file = outputFileNameMerged, sep  = " ")
        write(matrixFinalResult, ncolumns = (numberNeuronsI.cols + numberNeuronsE.cols), file = outputFileNameMerged, sep  = " ")
        print(paste("Merged matrix file written to: ", outputFileNameMerged))
    }
}

doYourThing <- function(time, spikeData, excitatory = TRUE, outputfile)
{
    if (excitatory){
        rows <- numberNeuronsE.rows
        cols <- numberNeuronsE.cols
        colname <- "NeuronsE"
        numberNeurons <- numberNeuronsE
        outputFileNameMultiline <- paste(outputfile, ".e.rate-multiline", sep = "")
    }
    else
    {
        rows <- numberNeuronsI.rows
        cols <- numberNeuronsI.cols
        colname <- "NeuronsI"
        numberNeurons <- numberNeuronsI
        outputFileNameMultiline <- paste(outputfile, ".i.rate-multiline", sep = "")
    }

    ## Find the starting and end of my bin
    print("Calculating indices for the data to be analysed")
    startIndexV <- which(abs(spikeData[,"Time"]-(time-1))==min(abs(spikeData[,"Time"]-(time-1))))
    startIndex <- min(startIndexV)
    print(paste("Start index is: ", startIndex))
    print(paste("Value at this point is:", spikeData[startIndex,"Time"]))

    endIndexV <- which(abs(spikeData[,"Time"]-time)==min(abs(spikeData[,"Time"]-time)))
    endIndex <- max(endIndexV)
    print(paste("End index is: ", endIndex))
    print(paste("Value at this point is:", spikeData[endIndex,"Time"]))
    #
    ##Obtain working set
    print("Extracting working set.")
    # This is one long row
    workingSet <- spikeData[c(startIndex:endIndex),colname]
    print(paste("Number of lines in working set are: ",length(workingSet)))
    # Again one long row with column names as the neuron names
    frequenciesWithoutMissingNeurons <- table(workingSet)

    # If inhibitory, the neuron numbering appears to start from numberE + 1, so I need to substract that value
    # Correct - one long row of neuron names
    frequenciesWithoutMissingNeuronsNames <- as.numeric(names(frequenciesWithoutMissingNeurons))


    #This data will not have neurons with 0 firing rate
    # Correct - a matrix with n rows and two cols - neuron number and firing rate count
    workingSetWithFrequencies <- cbind(as.numeric(frequenciesWithoutMissingNeuronsNames),as.numeric(frequenciesWithoutMissingNeurons))
    dim(workingSetWithFrequencies)

    # Add neurons with 0 spikes in this period
    tempAllNeurons <- matrix(c(1:numberNeurons))
    completeNeuronSetWithFrequencies <- merge(tempAllNeurons, workingSetWithFrequencies, all.x = TRUE)
    colnames(completeNeuronSetWithFrequencies) <- c(colname, "Frequency")
    # All correct till here
    completeNeuronSetWithFrequencies[is.na(completeNeuronSetWithFrequencies)] <- 0

    # I need this for the histogram files where it needs to be sorted
    onlyFrequencies <- sort(completeNeuronSetWithFrequencies[,"Frequency"], FALSE)
    # One long row
    print("Length of only Frequency vector are:")
    print(length(onlyFrequencies))
    print("Written frequency file for histograms.")
    write(onlyFrequencies, ncolumns = 1, file = outputFileNameMultiline, sep  = " ")

    onlyFrequenciesMatrix <- matrix(completeNeuronSetWithFrequencies[,"Frequency"], nrow = cols, ncol = rows)
    print("Dimensions of Frequency matrix are:")
    print(dim(onlyFrequenciesMatrix))

    # Flip horizontally
    onlyFrequenciesMatrixFlipped <- apply(onlyFrequenciesMatrix, 1, rev)
    # Flip vertically
    onlyFrequenciesMatrixFlippedReversed <- apply(onlyFrequenciesMatrixFlipped, 2, rev)
    print("Dimensions of matrix after flips are:")
    print(dim(onlyFrequenciesMatrixFlippedReversed))
    # transpose;
    onlyFrequenciesMatrixFlippedReversedAgain <- t(onlyFrequenciesMatrixFlippedReversed)
    result <- apply(onlyFrequenciesMatrixFlippedReversedAgain, 1, rev)
    print("Dimensions of matrix being returned are:")
    print(dim(result))
    return (result)

}

usage <- function () {
    print("spikes2plot.R - takes the spike raster dump file and generates matrix files to be given to gnuplot")
    print("Usage: spikes2plot.R <present directory name which is also the prefix for all files> <times to generate matrices for>")
    print("This script isn't meant to be called directly. It should be called via the main generator script.")
}

#Get command line args
args <- commandArgs(TRUE)

# Spike file name
timestamp <- args[1]
filenameE <- paste(timestamp, ".e.ras", sep = "")
filenameI <- paste(timestamp, ".i.ras", sep = "")


timeToSnapshot <- args[2:length(args)]


readFunctions <- list(
                      readE <- function(){
                          # Read in my spike data - all of it - E
                          print(paste("Working on file: ",filenameE, sep=" "))
                          # Calculate the number of lines
                          wcCommand=paste("wc -l", filenameE, sep=" ")
                          OnlyLinesE=paste(wcCommand, "| cut -d ' ' -f 1", sep=" ")
                          print(OnlyLinesE)
                          TotalLinesE=as.numeric(system(OnlyLinesE, intern=TRUE))
                          print(paste("Total lines in E input file: ", TotalLinesE))
                          # Import into R
                          spikeDataE <- as.matrix(read.table(filenameE, nrows = TotalLinesE, colClasses = "numeric"))
                          print("Read E spike data file.")
                          colnames(spikeDataE) <- c("Time", "NeuronsE")
                          print("Colnames of spikeDataE are:")
                          colnames (spikeDataE)
                          print("Dimensions of spikeDataE are:")
                          dim(spikeDataE)
                          return (spikeDataE)
                      },
                      readI <- function() {
                          # Read in my spike data - all of it  - I
                          print(paste("Working on file: ",filenameI, sep=" "))
                          # Calculate the number of lines
                          wcCommand=paste("wc -l", filenameI, sep=" ")
                          OnlyLinesI=paste(wcCommand, "| cut -d ' ' -f 1", sep=" ")
                          print(OnlyLinesI)
                          TotalLinesI=as.numeric(system(OnlyLinesI, intern=TRUE))
                          print(paste("Total lines in I input file: ", TotalLinesI))
                          # Import into R
                          spikeDataI <- as.matrix(read.table(filenameI, nrows = TotalLinesI, colClasses = "numeric"))
                          print("Read I spike data file.")
                          colnames(spikeDataI) <- c("Time", "NeuronsI")
                          print("Colnames of spikeDataI are:")
                          colnames (spikeDataI)
                          print("Dimensions of spikeDataI are:")
                          dim(spikeDataI)
                          return (spikeDataI)
                      }
                      )

returnvalues <- mclapply(readFunctions, function(f) f(), mc.cores = length(readFunctions))

#cl <- makeCluster(16)
#registerDoParallel(cl)

#foreach (i = 1:ncol(timeToSnapshot)) %dopar% {
    # Rudimentary check to see if it's a number, otherwise skip
mclapply(1:length(timeToSnapshot), function(x) mainworkfunction(timeToSnapshot[[x]], returnvalues[1,], returnvalues[2,]), mc.cores = length(timeToSnapshot))

    #    mainworkfunction(as.numeric(timeToSnapshot[,i]), spikeDataE, spikeDataI, outputFile)
#}

print(paste("Time taken by R is: ", Sys.time() - strt))
#stopCluster(cl)

