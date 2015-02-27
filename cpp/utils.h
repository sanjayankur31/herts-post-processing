/*
 * =====================================================================================
 *
 *       Filename:  utils.h
 *
 *    Description:  Functions and other things I use
 *
 *        Version:  1.0
 *        Created:  25/02/15 17:24:52
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ankur Sinha (FranciscoD), ankursinha@fedoraproject.org
 *   Organization:
 *
 * =====================================================================================
 */

#ifndef  utils_INC
#define  utils_INC

#include <iomanip>
#include <exception>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <iterator>
#include <thread>
#include <boost/program_options.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <iostream>
#include <fstream>
#include <ctime>
#include <cmath>
#include "gnuplot-iostream/gnuplot-iostream.h"


/*-----------------------------------------------------------------------------
 *  DECLARATIONS
 *-----------------------------------------------------------------------------*/

namespace po = boost::program_options;

/*  Make sure this matches in auryn, or you break down */
struct spikeEvent_type
{
    double time;
    unsigned int neuronID;
};

struct SNR
{

    double SNR;
    double mean;
    double std;
};

struct param
{
    /*  File that contains the plotting times */
    std::string plot_times_file;

    std::string config_file;
    /*  Number of excitatory neurons */
    unsigned int NE;
    unsigned int graph_widthE;
    /*  Number of inhibitory neurons */
    unsigned int NI;
    unsigned int graph_widthI;

    /*  Times of each stage initialised to 0 */
    std::vector<unsigned int> stage_times;
    /*  Additional plot times */
    std::vector<double> plot_times;
    /*  number of patterns to be used in this simulation */
    unsigned int  num_pats;
    /*  number of columns in the per pattern graphs */
    unsigned int graph_pattern_cols;

    /*  output prefix */
    std::string output_file;

    std::string patternfile_prefix;
    std::string recallfile_prefix;

    unsigned int mpi_ranks;

};

struct param parameters;

struct what_to_plot
{
    bool master;
    bool pattern_graphs;
    bool snr_graphs;
};

/*-----------------------------------------------------------------------------
 *  FUNCTIONS
 *-----------------------------------------------------------------------------*/

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  LoadPatternsAndRecalls
 *  Description:  Loads patterns and recalls from files into given vectors
 *
 *  Returns: void
 *  Arguments:
 *      - patterns: reference to 2D vector that will hold my patterns
 *      - recalls: reference to 2D vector that will hold my recalls
 * =====================================================================================
 */
    void
LoadPatternsAndRecalls ( std::vector<std::vector<unsigned int> > &patterns, std::vector<std::vector<unsigned int> >&recalls)
{
#if  DEBUG_ENABLED
    clock_t clock_start, clock_end;
    clock_start  = clock();
#endif     /* -----  DEBUG_ENABLED  ----- */
    std::ostringstream converter;
    unsigned int neuronID;
    std::ifstream ifs;

    for (unsigned int i = 0; i < parameters.num_pats; ++i)
    {
        converter.str("");
        converter.clear();
        converter << parameters.patternfile_prefix <<  std::setw(8)  << std::setfill('0') << std::to_string(i+1) << ".pat";
        std::string pattern = converter.str();
        patterns.emplace_back(std::vector<unsigned int> ());

        try
        {
            ifs.open(pattern);

            while(ifs >> neuronID)
                patterns[i].emplace_back(neuronID);

            ifs.close();
            std::sort(patterns[i].begin(), patterns[i].end());
#if  DEBUG_ENABLED
            std::cout << "Items loaded and sorted in pattern number " << i << " is " << patterns[i].size() << "\n";
#endif     /* -----  DEBUG_ENABLED  ----- */
        }
        catch (std::ios_base::failure &e)
        {
            std::cerr << e.what() << "\n";
        }

        converter.str("");
        converter.clear();
        converter << parameters.recallfile_prefix <<  std::setw(8)  << std::setfill('0') << std::to_string(i+1) << ".pat";
        std::string recall = converter.str();
        recalls.emplace_back(std::vector<unsigned int> ());

        try
        {
            ifs.open(recall);

            while(ifs >> neuronID)
                recalls[i].emplace_back(neuronID);

            ifs.close();
            std::sort(recalls[i].begin(), recalls[i].end());
#if  DEBUG_ENABLED
            std::cout << "Items loaded and sorted in recall number " << i << " is " << recalls[i].size() << "\n";
#endif     /* -----  DEBUG_ENABLED  ----- */
        }
        catch (std::ios_base::failure &e)
        {
            std::cerr << e.what() << "\n";
        }
    }
#if  DEBUG_ENABLED
    clock_end = clock();
    std::cout << patterns.size() << " patterns and " << recalls.size() << " recalls read in " << (clock_end - clock_start)/CLOCKS_PER_SEC << " seconds.\n";

#endif     /* -----  DEBUG_ENABLED  ----- */
}		/* -----  end of function LoadPatternsAndRecalls  ----- */

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  CalculateTimeToPlotList
 *  Description:  If the file isn't found, this calculates the times using a
 *  simple logic
 *
 *  Returns: a vector with the times that were read from the file
 *  Arguments:
 *      - NONE
 * =====================================================================================
 */
std::vector<double>
CalculateTimeToPlotList ()
{
    std::vector<double> graphing_times;
    double times = 0;

    /*  Initial stimulus and stabilisation */
    times = parameters.stage_times[0] + parameters.stage_times[1];

    for (unsigned int i = 1; i <= parameters.num_pats; ++i ) {
        /*  pattern strengthening and pattern stabilisation */
        times += (parameters.stage_times[2] + parameters.stage_times[3]);

        for (unsigned int k = 1; k <= i; ++k)
        {
            /*  Recall stimulus given */
            times += parameters.stage_times[4];
            graphing_times.push_back(times);

            /*  Recall check time added */
            times += parameters.stage_times[5];
        }
    }
    return graphing_times;
}		/* -----  end of function CalculateTimeToPlotList  ----- */

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  CalculateTimeToPlotList
 *  Description:  A different implementation where I just read them from a file
 *
 *  Returns: a vector with the times that were read from the file
 *  Arguments:
 *      - NONE
 * =====================================================================================
 */
std::vector<double>
ReadTimeToPlotListFromFile ()
{
    std::vector<double> graphing_times;
    double temp;
    std::ifstream timefilestream;
    try {
        timefilestream.open(parameters.plot_times_file);
        while (timefilestream >> temp)
        {
            graphing_times.push_back(temp);
        }

    }
    catch (std::ios_base::failure &e)
    {
        std::cerr << e.what() << "\n";
        std::cerr << "Something went wrong reading from file. Autogenerating" << "\n";
        graphing_times = CalculateTimeToPlotList();
    }

    std::sort(graphing_times.begin(), graphing_times.end());
    return graphing_times;
}		/* -----  end of function CalculateTimeToPlotList  ----- */


/*
 * ===  FUNCTION  ======================================================================
 *         Name:  PlotMasterGraph
 *  Description:  TODO - place holder for the time being
 * =====================================================================================
 */
    void
PlotMasterGraph ( )
{
    clock_t clock_start, clock_end;
    clock_start = clock();

    clock_end = clock();
    std::cout << "Total time taken to plot master plot: " << (clock_end - clock_start)/CLOCKS_PER_SEC << "\n";
    return;
}		/* -----  end of function PlotMasterGraph  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  BinaryUpperBound
 *  Description:  Last occurence of a key using binary search
 * =====================================================================================
 */
    char *
BinaryUpperBound (double timeToCompare, boost::iostreams::mapped_file_source &openMapSource )
{
    char *spikesStart = NULL;
    unsigned long int numStart = 0;
    unsigned long int numEnd = 0;
    char *currentSpike = NULL;
    unsigned long int numCurrent = 0;
    unsigned long int numdiff = 0;
    unsigned long int step = 0;
    unsigned long int sizeofstruct = sizeof(struct spikeEvent_type);
    struct spikeEvent_type *currentRecord = NULL;


    /*  start of last record */
    spikesStart =  (char *)openMapSource.data();
    numStart = 0;
    /*  end of last record */
    numEnd = (openMapSource.size()/sizeofstruct -1);

    /*  Number of structs */

    numdiff = numEnd - numStart;
#ifdef DEBUG
    std::cout << "Finding last of " << timeToCompare << "\n";
    unsigned long int sizediff = 0;
    char *spikesEnd = NULL;
    spikesEnd =  (spikesStart + openMapSource.size() - sizeofstruct);
    sizediff = spikesEnd - spikesStart;
    std::cout << "Struct size is: " << sizeofstruct << "\n";
    std::cout << "Char size is: " << sizeof(char)  << "\n";
    std::cout << "size of int is: " << sizeof(int)  << "\n";
    std::cout << "Number of records in this file: " << (openMapSource.size() - sizeofstruct)/sizeofstruct << "\n";
    std::cout << "Number of records in this file: " << (spikesEnd - spikesStart)/sizeofstruct << "\n";
    printf("With printf subtraction %zu\n",(spikesEnd - spikesStart));
    std::cout << "Proper subtraction : " << (spikesEnd - spikesStart) << "\n";
    std::cout << "sizediff : " << sizediff << "\n";
    printf("With printf sizediff %zu\n",sizediff);
    std::cout << "multiplier " << (spikesEnd - spikesStart)/sizediff << "\n";
    std::cout << "Number of struct records in this file: " << numdiff << "\n";
#endif

    while( numdiff > 0)
    {
        numCurrent = numStart;
        step = (numdiff/2);

        numCurrent += step;
        currentSpike = spikesStart + numCurrent * sizeofstruct;
        currentRecord = (struct spikeEvent_type *)currentSpike;
#ifdef DEBUG
        std::cout << "Current record is: " << currentRecord->time << "\t" << currentRecord->neuronID << " at line" << numCurrent << "\n";
#endif

        if (!(timeToCompare < currentRecord->time))
        {
            numStart = ++numCurrent;
            numdiff -= step + 1;
        }
        else
            numdiff = step;
    }

    currentSpike = spikesStart + (numStart * sizeofstruct);
    currentRecord = (struct spikeEvent_type *)currentSpike;
#ifdef DEBUG
    std::cout << "Returning: " << currentRecord->time << "\t" << currentRecord->neuronID << "\n";
#endif
    return currentSpike;
}		/* -----  end of function BinaryUpperBound  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  BinaryLowerBound
 *  Description:  First occurence of a key using binary search
 * =====================================================================================
 */
    char *
BinaryLowerBound (double timeToCompare, boost::iostreams::mapped_file_source &openMapSource )
{
    char *spikesStart = NULL;
    unsigned long int numStart = 0;
    unsigned long int numEnd = 0;
    char *currentSpike = NULL;
    unsigned long int numCurrent = 0;
    unsigned long int numdiff = 0;
    unsigned long int step = 0;
    unsigned long int sizeofstruct = sizeof(struct spikeEvent_type);
    struct spikeEvent_type *currentRecord = NULL;


    /*  start of last record */
    spikesStart =  (char *)openMapSource.data();
    numStart = 0;
    /*  end of last record */
    numEnd = (openMapSource.size()/sizeofstruct -1);

    /*  Number of structs */
    numdiff = numEnd - numStart;

#ifdef DEBUG
    std::cout << "Finding first of " << timeToCompare << "\n";
    unsigned long int sizediff = 0;
    char *spikesEnd = NULL;
    spikesEnd =  (spikesStart + openMapSource.size() - sizeofstruct);
    sizediff = spikesEnd - spikesStart;
    std::cout << "Struct size is: " << sizeofstruct << "\n";
    std::cout << "Char size is: " << sizeof(char)  << "\n";
    std::cout << "size of int is: " << sizeof(int)  << "\n";
    std::cout << "Number of records in this file: " << (openMapSource.size() - sizeofstruct)/sizeofstruct << "\n";
    std::cout << "Number of records in this file: " << (spikesEnd - spikesStart)/sizeofstruct << "\n";
    printf("With printf subtraction %zu\n",(spikesEnd - spikesStart));
    std::cout << "Proper subtraction : " << (spikesEnd - spikesStart) << "\n";
    std::cout << "sizediff : " << sizediff << "\n";
    printf("With printf sizediff %zu\n",sizediff);
    std::cout << "multiplier " << (spikesEnd - spikesStart)/sizediff << "\n";
    std::cout << "Number of struct records in this file: " << numdiff << "\n";
#endif

    while( numdiff > 0)
    {
        numCurrent = numStart;
        step = (numdiff/2);

        numCurrent += step;
        currentSpike = spikesStart + numCurrent * sizeofstruct;
        currentRecord = (struct spikeEvent_type *)currentSpike;
#ifdef DEBUG
        std::cout << "Current record is: " << currentRecord->time << "\t" << currentRecord->neuronID << " at line" << numCurrent << "\n";
#endif

        if (currentRecord->time < timeToCompare)
        {
            numStart = ++numCurrent;
            numdiff -= step + 1;
        }
        else
            numdiff = step;
    }

    currentSpike = spikesStart + (numStart * sizeofstruct);
    currentRecord = (struct spikeEvent_type *)currentSpike;
#ifdef DEBUG
    std::cout << "Returning: " << currentRecord->time << "\t" << currentRecord->neuronID << "\n";
#endif
    return currentSpike;
}		/* -----  end of function BinaryLowerBound  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  ExtractChunk
 *  Description:  Extract chunks between the two buffer pointers
 * =====================================================================================
 */
std::vector <unsigned int>
ExtractChunk (char * chunk_start, char * chunk_end )
{
    char * chunkit = NULL;
    std::vector <unsigned int> neurons;
    if (chunk_end - chunk_start > 0)
    {
        chunkit = chunk_start;
        while (chunkit <= chunk_end)
        {
            struct spikeEvent_type *buffer;
            buffer = (struct spikeEvent_type *)chunkit;
            neurons.emplace_back(buffer->neuronID);
            chunkit += sizeof(struct spikeEvent_type);
        }
    }
    return neurons;
}		/* -----  end of function ExtractChunk  ----- */


/*
 * ===  FUNCTION  ======================================================================
 *         Name:  MasterFunction
 *  Description:
 * =====================================================================================
 */
    void
MasterFunction (std::vector<boost::iostreams::mapped_file_source> &spikes_E, std::vector<boost::iostreams::mapped_file_source> &spikes_I, std::vector<std::vector<unsigned int> >&patterns, std::vector<std::vector <unsigned int> >&recalls, double chunk_time)
{
    std::vector <unsigned int>neuronsE;
    std::vector <unsigned int>neuronsI;
    std::vector <std::vector<unsigned int> >pattern_neurons_rate;
    std::vector <std::vector<unsigned int> >noise_neurons_rate;
    std::vector <std::vector<unsigned int> >recall_neurons_rate;
    std::vector <unsigned int>neuronsE_rate;
    std::vector <unsigned int>neuronsI_rate;
    std::ostringstream converter;

    /*  Initialise the 2D vectors */
    for (unsigned int i = 0; i < parameters.num_pats; ++i)
    {
        pattern_neurons_rate.emplace_back(std::vector<unsigned int>());
        noise_neurons_rate.emplace_back(std::vector<unsigned int>());
        recall_neurons_rate.emplace_back(std::vector<unsigned int>());
    }

#ifdef DEBUG
    std::cout << "[MasterFunction] Thread " << std::this_thread::get_id() << " active." << "\n";
#endif
    /*  Fill up my vectors with neurons that fired in this period */
    for (unsigned int i = 0; i < parameters.mpi_ranks; ++i)
    {
        /*  Excitatory */
        char * chunk_start = BinaryLowerBound(chunk_time - 1., std::ref(spikes_E[i]));
        char * chunk_end = BinaryUpperBound(chunk_time, std::ref(spikes_E[i]));
        neuronsE = ExtractChunk(chunk_start, chunk_end);
        if (neuronsE.size() == 0)
        {
            std::cout << chunk_time << " not found in E file "  << i << "!\n";
            return;
        }

        /*  Inhibitory */
        chunk_start = BinaryLowerBound(chunk_time - 1., std::ref(spikes_I[i]));
        chunk_end = BinaryUpperBound(chunk_time, std::ref(spikes_I[i]));
        neuronsI = ExtractChunk(chunk_start, chunk_end);
        if (neuronsI.size() == 0)
        {
            std::cout << chunk_time << " not found in I file "  << i << "!\n";
            return;
        }
    }
    /*  Sort - makes next operations more efficient, or I think it does */
    std::sort(neuronsE.begin(), neuronsE.end());
    std::sort(neuronsI.begin(), neuronsI.end());

    return;
}		/* -----  end of function MasterFunction  ----- */
#endif   /* ----- #ifndef utils_INC  ----- */
