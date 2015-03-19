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
#include <boost/algorithm/string/replace.hpp>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <ctime>
#include <cmath>
#include "gnuplot-iostream/gnuplot-iostream.h"
#include <mutex>


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

    bool print_snr;
};

struct param parameters;

struct what_to_plot
{
    what_to_plot (): master(false), pattern_graphs(false), snr_graphs (false), SNR_from_file (false), cumVSover(false), multiSNR(false), multiMeans(false), multiSD(false), snrVSwPats(false), meanVSwPats(false), sdVSwPats(false), formatPNG(false), processRas(false) {}
    bool master;
    bool pattern_graphs;
    bool snr_graphs;
    bool SNR_from_file;
    bool cumVSover;
    std::vector <double> wPats;
    bool multiSNR;
    bool multiMeans;
    bool multiSD;
    bool snrVSwPats;
    bool meanVSwPats;
    bool sdVSwPats;
    bool formatPNG;
    bool processRas;
};

struct what_to_plot plot_this;

std::mutex snr_data_mutex;

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
    timefilestream.open(parameters.plot_times_file);
    std::cerr << "Recall times file is: " << parameters.plot_times_file << "\n";
    if (timefilestream.is_open())
    {
        while (timefilestream >> temp)
        {
            graphing_times.push_back(temp);
        }
    }
    else
    {
        std::cerr << "File has not been opened!";
        std::cerr << "Something went wrong reading from file. Autogenerating" << "\n";
        graphing_times = CalculateTimeToPlotList();
    }

    std::cout << "Values read - " << graphing_times.size() << "\n";
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
 *         Name:  PlotHistogram
 *  Description:  
 * =====================================================================================
 */
    void
PlotHistogram (std::vector<unsigned int> values, std::string outputFileName, double chunk_time, std::string colour, std::string legendLabel)
{
#if  0     /* ----- #if 0 : If0Label_1 ----- */
    std::vector<std::pair<unsigned int, unsigned int> > plot_hack;

    sort(values.begin(), values.end());

    for (std::vector<unsigned int>::iterator it = values.begin(); it != values.end(); it++)
        plot_hack.emplace_back(std::pair<unsigned int, unsigned int>(*it, *it));


    double binwidth = (values.back() - values.front())/values.size();
    Gnuplot gp;
    gp << "set style fill solid 0.5; set tics out nomirror; \n"; 
/*     gp << "set xrange [" << values.front() << ":" << values.back() << "];\n";
 */
    gp << "set xrange [0:];\n";
    gp << "set yrange [0:]; set xlabel \"firing rate\"; set ylabel \"number of neurons\"; \n";
    gp << "binwidth=" << binwidth << "; bin(x,width)=width*floor(x/width)+width/2.0; \n";
    gp << "set term png font \"/usr/share/fonts/dejavu/DejaVuSans.ttf,20\" size 1440,1440; \n";
    gp << "set output \"" << outputFileName << "\n";
    gp << "set title \"Histogram of firing rates at time " << chunk_time << "\"; \n";
    gp << "plot '-' using (bin($1,binwidth)):(1.0) smooth freq with boxes lc rgb \"" << colour << "\" t \"" << legendLabel << "\"; \n";
    gp.send1d(plot_hack);

    std::cout << outputFileName << "plotted." << "\n";
#endif     /* ----- #if 0 : If0Label_1 ----- */


}		/* -----  end of function PlotHistogram  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  GetSNR
 *  Description:  Get the SNR of one pattern
 * =====================================================================================
 */
    struct SNR
GetSNR (std::vector<unsigned int> patternRates, std::vector<unsigned int> noiseRates )
{
    struct SNR snr;
    unsigned int sum_patterns = std::accumulate(patternRates.begin(), patternRates.end(),0.0);
    snr.mean = sum_patterns/patternRates.size();

    double sq_diff = 0.;
    std::for_each(patternRates.begin(), patternRates.end(), [&] (const double d) {
            sq_diff += pow((d - snr.mean),2);
            });

    snr.std = sqrt(sq_diff/(patternRates.size() -1));

    unsigned int sum_noises = std::accumulate(noiseRates.begin(), noiseRates.end(),0.0);
    double mean_noises = sum_noises/noiseRates.size();

    sq_diff = 0.;
    std::for_each(noiseRates.begin(), noiseRates.end(), [&] (const double d) {
            sq_diff += pow((d - mean_noises),2);
            });

    double std_noises = sqrt(sq_diff/(noiseRates.size() -1));

    snr.SNR = ((2.*pow((snr.mean - mean_noises),2))/(pow(snr.std,2) + pow(std_noises,2)));

    return snr;
}		/* -----  end of function GetSNR  ----- */



/*
 * ===  FUNCTION  ======================================================================
 *         Name:  MasterFunction
 *  Description:
 * =====================================================================================
 */
    void
MasterFunction (std::vector<boost::iostreams::mapped_file_source> &spikes_E, std::vector<boost::iostreams::mapped_file_source> &spikes_I, std::vector<std::vector<unsigned int> >&patterns, std::vector<std::vector <unsigned int> >&recalls, double chunk_time, unsigned int patternRecalled, std::multimap <double, struct SNR> &snr_data)
{
    std::vector <unsigned int>neuronsE;
    std::vector <unsigned int>neuronsI;
    std::vector <unsigned int>pattern_neurons_rate;
    std::vector <unsigned int>noise_neurons_rate;
    std::vector <unsigned int>recall_neurons_rate;
    std::vector <unsigned int>neuronsE_rate;
    std::vector <unsigned int>neuronsI_rate;
    std::ostringstream converter;
    struct SNR snr_at_chunk_time;

    std::cout << "Chunk time received: " << chunk_time << "\n";

#ifdef DEBUG
    std::cout << "[MasterFunction] Thread " << std::this_thread::get_id() << " active." << "\n";
#endif
    /*  Fill up my vectors with neurons that fired in this period */
    for (unsigned int i = 0; i < parameters.mpi_ranks; ++i)
    {
        /*  Excitatory */
        char * chunk_start = BinaryLowerBound(chunk_time - 1., std::ref(spikes_E[i]));
        char * chunk_end = BinaryUpperBound(chunk_time, std::ref(spikes_E[i]));
        std::vector<unsigned int> temp = ExtractChunk(chunk_start, chunk_end);
        neuronsE.reserve(neuronsE.size() + std::distance (temp.begin(), temp.end()));
        neuronsE.insert(neuronsE.end(), temp.begin(), temp.end()); 
        if (neuronsE.size() == 0)
        {
            std::cout << chunk_time << " not found in E file "  << i << "!\n";
            return;
        }

        /*  Inhibitory */
        chunk_start = BinaryLowerBound(chunk_time - 1., std::ref(spikes_I[i]));
        chunk_end = BinaryUpperBound(chunk_time, std::ref(spikes_I[i]));
        temp = ExtractChunk(chunk_start, chunk_end);
        neuronsI.reserve(neuronsI.size() + std::distance (temp.begin(), temp.end()));
        neuronsI.insert(neuronsI.end(), temp.begin(), temp.end()); 
        if (neuronsI.size() == 0)
        {
            std::cout << chunk_time << " not found in I file "  << i << "!\n";
            return;
        }
    }
    /*  Sort - makes next operations more efficient, or I think it does */
    std::sort(neuronsE.begin(), neuronsE.end());
    std::sort(neuronsI.begin(), neuronsI.end());

    /*  I can make this part a function, but that will imply that I'll have to
     *  do multiple passes over the neuron list which is less efficient
     *  than what I'm doing now. 
     *
     *  For E neurons, for example, I'll first find the rate, then place them
     *  in categories. At the moment, I'm doing all of this in one single pass
     *  I see no reason to write a function if that'll just make it less
     *  efficient.
    */
    /*  Get frequencies of inhibitory neurons */
    std::vector<unsigned int>::iterator search_begin = neuronsI.begin();
    for(unsigned int i = 1; i <= parameters.NI; ++i)
    {
        int rate = 0;
        rate = (std::upper_bound(search_begin, neuronsI.end(), i) != neuronsI.end()) ?  (std::upper_bound(search_begin, neuronsI.end(), i) - search_begin) : 0;

        search_begin = std::upper_bound(search_begin, neuronsI.end(), i);
        neuronsI_rate.emplace_back(rate);
    }
    /*  We have the inhibitory firing rate! */

    /* Get frequencies of excitatory neurons and classify them into their
     * groups 
    */
    search_begin = neuronsE.begin();
    for(unsigned int i = 1; i <= parameters.NE; ++i)
    {
        int rate = 0;
        rate = (std::upper_bound(search_begin, neuronsE.end(), i) != neuronsE.end()) ?  (std::upper_bound(search_begin, neuronsE.end(), i) - search_begin) : 0;
        search_begin = std::upper_bound(search_begin, neuronsE.end(), i);

        /*  It's part of the pattern */
        if(pattern_neurons_rate.size() != patterns[patternRecalled].size() && std::binary_search(patterns[patternRecalled].begin(), patterns[patternRecalled].end(), i))
        {
            pattern_neurons_rate.emplace_back(rate);
        }
        /*  It's not in the pattern and therefore a noise neuron */
        else
        {
            noise_neurons_rate.emplace_back(rate);
        }
        /* It's a recall neuron - not using this at the moment */
        if(recall_neurons_rate.size() != recalls[patternRecalled].size() && std::binary_search(recalls[patternRecalled].begin(), recalls[patternRecalled].end(), i))
        {
            recall_neurons_rate.emplace_back(rate);
        }
        /*  All E neurons */
        neuronsE_rate.emplace_back(rate);
    }

    converter.str("");
    converter.clear();
    converter << parameters.output_file << "-" << chunk_time << ".e.histogram.png"; 
    PlotHistogram(neuronsE_rate, converter.str(), chunk_time, "blue", "Excitatory");
    converter.str("");
    converter.clear();
    converter << parameters.output_file << "-" << chunk_time << ".i.histogram.png"; 
    PlotHistogram(neuronsI_rate, converter.str(), chunk_time, "red", "Inhibitory");

    if (plot_this.pattern_graphs)
    {
        converter.str("");
        converter.clear();
        converter << parameters.output_file << "-" << chunk_time << ".pattern." << patternRecalled << ".histogram.png";
        PlotHistogram(pattern_neurons_rate, converter.str(), chunk_time, "green" , "Pattern");
        converter.str("");
        converter.clear();
        converter << parameters.output_file << "-" << chunk_time << ".noise." << patternRecalled << ".histogram.png"; 
        PlotHistogram(noise_neurons_rate, converter.str(), chunk_time, "black", "Noise");
    }


    snr_at_chunk_time = GetSNR(pattern_neurons_rate, noise_neurons_rate);
    std::cout << "SNR at time " << chunk_time << " is " << snr_at_chunk_time.SNR << "\n";
    std::lock_guard<std::mutex> guard(snr_data_mutex);
    snr_data.insert(std::pair<double, struct SNR>(chunk_time, snr_at_chunk_time));
    std::cout << "SNR complete size " << " is " << snr_data.size() << "\n";


    return;
}		/* -----  end of function MasterFunction  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  PlotSNRGraphs
 *  Description:  
 * =====================================================================================
 */
    void
PlotSNRGraphs (std::multimap <double, struct SNR> snr_data)
{
    std::ostringstream plot_command;
    Gnuplot gp;
    std::vector<std::pair<double, double> > points_means;
    unsigned int max_point = 0;

    /* Initial set up */
    if (!plot_this.formatPNG)
    {
        gp << "set output \"SNR.svg\" \n";
        gp << "set term svg font \"/usr/share/fonts/dejavu/DejaVuSans.ttf,20\" size 2880,1440 dynamic enhanced mousing standalone; \n";
    }
    else
    {
        gp << "set output \"SNR.png\" \n";
        gp << "set term png font \"/usr/share/fonts/dejavu/DejaVuSans.ttf,20\" size 2880,1440 ; \n";
    }
    gp << "set title \"SNR vs number of patterns - " << parameters.output_file << "\" \n";


#if  0     /* ----- #if 0 : If0Label_2 ----- */
    std::cout << "All snrs: " << "\n";
    for (std::multimap <double, struct SNR>::iterator it = snr_data.begin(); it != snr_data.end(); it++)
    {
        std::cout << it->first << ":\t" << it->second.SNR << "\n";
    }
#endif     /* ----- #if 0 : If0Label_2 ----- */


    unsigned int time_counter = 0;
    for (unsigned int i = 0; i < parameters.num_pats; i++)
    {
        double mean = 0;
        for(unsigned int j = 0; j <= i; j++)
        {
            std::multimap <double, struct SNR>::iterator it = snr_data.begin();
            std::advance(it, time_counter);
            time_counter++;
            mean += ((it->second).SNR);
            plot_command << "set label \"\" at " << i+1 << "," << (it->second).SNR << " point pointtype " << j +4 << " ;\n";
            if ((it->second).SNR > max_point)
                max_point = ceil((it->second).SNR);
        }
        mean /= (i+1);
        std::cout << i+1 << ":\t" << mean << "\n";
        points_means.emplace_back(std::pair<double, double>(i+1, mean));
    }

    gp << plot_command.str();

    gp << "set xrange[" << 0.5 << ":" << parameters.num_pats + 1 << "]; \n";
    gp << "set yrange[" << 0 << ":" << max_point << "]; \n";
    gp << "set ylabel \"SNR\"; \n";
    gp << "set xtics 1; \n";
    gp << "set ytics 1; \n";
    gp << "set grid; \n";
    gp << "set xlabel \"Number of patterns stored\"; \n";
    gp << "plot '-' with lines title 'mean SNR' \n";
    gp.send1d(points_means);


    return;
}		/* -----  end of function PlotSNRGraphs  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  PrintSNRDataToFile
 *  Description:  
 * =====================================================================================
 */
    void
PrintSNRDataToFile (std::multimap <double, struct SNR> snr_data)
{
    std::cout << "Printing SNR data to file." << "\n";
    unsigned int time_counter = 0;
    std::ofstream snr_stream;
    std::ofstream mean_stream;
    std::ofstream std_stream;
    snr_stream.open("00-SNR-data.txt");
    mean_stream.open("00-Means-data.txt");
    std_stream.open("00-SD-data.txt");

    for (unsigned int i = 0; i < parameters.num_pats; i++)
    {
        struct SNR means;
        means.SNR = 0;
        means.std = 0;
        means.mean = 0;
        for(unsigned int j = 0; j <= i; j++)
        {
            std::multimap <double, struct SNR>::iterator it = snr_data.begin();
            std::advance(it, time_counter);
            time_counter++;

            means.SNR += ((it->second).SNR);
            means.mean += ((it->second).mean);
            means.std += ((it->second).std);

            snr_stream << (it->second).SNR << "\n";
            mean_stream << (it->second).mean << "\n";
            std_stream << (it->second).std << "\n";
        }
        means.SNR /= (i+1);
        means.mean /= (i+1);
        means.std /= (i+1);
        snr_stream << means.SNR << "\n";
        mean_stream << means.mean << "\n";
        std_stream << means.std << "\n";
    }

    snr_stream.close();
    mean_stream.close();
    std_stream.close();
    return ;
}		/* -----  end of function PrintSNRDataToFile  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  GenerateSNRPlotFromFile
 *  Description:  
 * =====================================================================================
 */
    void
GenerateSNRPlotFromFile (std::string dataFile, std::string addendum = "", unsigned int lc = 1 )
{
    Gnuplot gp;
    std::ostringstream plot_command;
    std::vector<std::pair<double, double> > points_means;
    unsigned int max_point = 0;

    std::ifstream file_stream;
    file_stream.open(dataFile.c_str() , std::ifstream::in);
    unsigned int numlines = std::count(std::istreambuf_iterator<char>(file_stream),std::istreambuf_iterator<char>(), '\n');
    file_stream.close();

    if( (parameters.num_pats * (parameters.num_pats + 3)/2) != numlines)
    {
        std::cerr << "initial check failed. numlines = " << numlines << " numpats = " << parameters.num_pats << "\n";
    }


    file_stream.open(dataFile.c_str(), std::ifstream::in);

    /* Initial set up */
    if (!plot_this.formatPNG)
    {
        gp << "set output \"SNR.svg\" \n";
        gp << "set term svg font \"/usr/share/fonts/dejavu/DejaVuSans.ttf,20\" size 2880,1440 dynamic enhanced mousing standalone; \n";
    }
    else
    {
        gp << "set output \"SNR.png\" \n";
        gp << "set term png font \"/usr/share/fonts/dejavu/DejaVuSans.ttf,20\" size 2880,1440 ; \n";
    }
    gp << "set title \"SNR vs number of patterns - " << parameters.output_file << "\" \n";


    for (unsigned int i = 0; i < parameters.num_pats; i++)
    {
        double mean_snr = 0.;
        for(unsigned int j = 0; j <= i; j++)
        {
            double snr_value = 0.;
            file_stream >> snr_value;
            plot_command << "set label \"\" at " << i+1 << "," << snr_value << " point pointtype " << j+1 << " lc " << lc << " ;\n";
            if (snr_value > max_point)
                max_point = ceil(snr_value);
        }
        file_stream >> mean_snr;
        std::cout << i+1 << ":\t" << mean_snr << "\n";
        points_means.emplace_back(std::pair<double, double>(i+1, mean_snr));
    }

    file_stream.close();

    gp << plot_command.str();

    gp << "set xrange[" << 0.5 << ":" << parameters.num_pats + 1 << "]; \n";
    gp << "set yrange[" << 0 << ":" << max_point << "]; \n";
    gp << "set ylabel \"SNR\"; \n";
    gp << "set xtics 1; \n";
    gp << "set ytics 1; \n";
    gp << "set grid; \n";
    gp << "set xlabel \"Number of patterns stored\"; \n";
    gp << "plot '-' with lines title 'mean SNR - " << addendum.c_str() << "' lc " << lc << "; \n";
    gp.send1d(points_means);

    return;
}		/* -----  end of function GenerateSNRPlotFromFile  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  GenerateMultiMetricPlotFromFile
 *  Description:  
 * =====================================================================================
 */
    void
GenerateMultiMetricPlotFromFile (std::vector<std::pair<std::string, std::string> > inputs, std::string title)
{
    Gnuplot gp;
    std::ostringstream plot_command;
    std::ostringstream line_command;
    std::vector<std::vector<std::pair<double, double> > > all_points_means;
    unsigned int max_point = 0;
    unsigned int lc = 0;

    /* Initial set up */
    if (!plot_this.formatPNG)
    {
        gp << "set output \"" << title << "-multi.svg\" \n";
        gp << "set term svg font \"/usr/share/fonts/dejavu/DejaVuSans.ttf,20\" size 2880,1440 dynamic enhanced mousing standalone; \n";
    }
    else
    {
        gp << "set output \"" << title << "-multi.png\" \n";
        gp << "set term png font \"/usr/share/fonts/dejavu/DejaVuSans.ttf,20\" size 2880,1440 ; \n";
    }
    gp << "set title \"" << title << " vs number of patterns - " << parameters.output_file << "\" \n";
    gp << "set ylabel \"" << title << "\"; \n";
    gp << "set xtics 1; \n";
    gp << "set ytics 1; \n";
    gp << "set grid; \n";
    gp << "set xlabel \"Number of patterns stored\"; \n";
    line_command << "plot ";

    for (std::vector<std::pair<std::string, std::string> >::iterator it = inputs.begin(); it != inputs.end(); it++)
    {
        lc++;
        std::ifstream file_stream;
        std::string dataFile(it->first);
        file_stream.open(dataFile.c_str() , std::ifstream::in);
        unsigned int numlines = std::count(std::istreambuf_iterator<char>(file_stream),std::istreambuf_iterator<char>(), '\n');
        file_stream.close();
        std::string addendum(it->second);
        std::vector<std::pair<double, double> > points_means;

        if( (parameters.num_pats * (parameters.num_pats + 3)/2) != numlines)
        {
            std::cerr << "initial check failed. numlines = " << numlines << " numpats = " << parameters.num_pats << "\n";
        }

        file_stream.open(dataFile.c_str(), std::ifstream::in);

        for (unsigned int i = 0; i < parameters.num_pats; i++)
        {
            double mean_snr = 0.;
            for(unsigned int j = 0; j <= i; j++)
            {
                double snr_value = 0.;
                file_stream >> snr_value;
                plot_command << "set label \"\" at " << i+1 << "," << snr_value << " point pointtype " << j+1 << " lc " << lc << " ;\n";
                if (snr_value > max_point)
                    max_point = ceil(snr_value);
            }
            file_stream >> mean_snr;
            std::cout << i+1 << ":\t" << mean_snr << "\n";
            points_means.emplace_back(std::pair<double, double>(i+1, mean_snr));
        }
        all_points_means.emplace_back(points_means);
        points_means.clear();
        line_command << "'-' with lines title 'mean " << title << " - " << addendum.c_str() << "' lc " << lc << " lw 2, ";
        file_stream.close();
    }


    gp << "set xrange[" << 0.5 << ":" << parameters.num_pats + 1 << "]; \n";
    gp << "set yrange[" << 0 << ":" << max_point << "]; \n";
    gp << plot_command.str();
    gp << line_command.str();
    gp << " ;\n";

    for (std::vector<std::vector<std::pair<double,double> > >::iterator it = all_points_means.begin(); it != all_points_means.end(); it++)
        gp.send1d(*it);

    return;
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  GenerateMultiSNRPlotFromFile
 *  Description:  
 * =====================================================================================
 */
    void
GenerateMultiSNRPlotFromFile (std::vector<std::pair<std::string, std::string> > inputs)
{
    GenerateMultiMetricPlotFromFile(inputs, "SNR");
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  GenerateMultiMeanPlotFromFile
 *  Description:  
 * =====================================================================================
 */
    void
GenerateMultiMeansPlotFromFile (std::vector<std::pair<std::string, std::string> > inputs)
{
    GenerateMultiMetricPlotFromFile(inputs, "Mean");
}
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  GenerateMultiSDPlotFromFile
 *  Description:  
 * =====================================================================================
 */
    void
GenerateMultiSDPlotFromFile (std::vector<std::pair<std::string, std::string> > inputs)
{
    GenerateMultiMetricPlotFromFile(inputs, "SD");
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  GenerateMetricVsWPatFromFile
 *  Description:  
 * =====================================================================================
 */
    void
GenerateMetricVsWPatFromFile(std::vector<std::pair<std::string, double> > inputs, std::string title)
{
    Gnuplot gp;
    std::ostringstream line_command;
    std::ostringstream converter;
    unsigned int lc = 0;
    std::vector<std::pair<double,double> > max_means;
    std::vector<std::pair<double,double> > mean_means;
    std::vector<std::pair<double,double> > min_means;

    /* Initial set up */
    if (!plot_this.formatPNG)
    {
        gp << "set output \"" << title << "-w_pat.svg\" \n";
        gp << "set term svg font \"/usr/share/fonts/dejavu/DejaVuSans.ttf,20\" size 2880,1440 dynamic enhanced mousing standalone; \n";
        gp << "set title \"" << title << " vs  w\\\\_pat - " << parameters.output_file << "\" \n";
        gp << "set xlabel \"w\\\\_pat\"; \n";
    }
    else
    {
        gp << "set output \"" << title << "-w_pat.png\" \n";
        gp << "set term png font \"/usr/share/fonts/dejavu/DejaVuSans.ttf,20\" size 2880,1440 ; \n";
        gp << "set title \"" << title << " vs w_pat - " << parameters.output_file << "\" \n";
        gp << "set xlabel \"w_pat\"; \n";
    }
    gp << "set ylabel \"" << title << "\"; \n";
    gp << "set yrange [0:] \n";
    gp << "set grid; \n";

    for (std::vector<std::pair<std::string, double> >::iterator it = inputs.begin(); it != inputs.end(); it++)
    {
        lc++;
        std::ifstream file_stream;
        std::string dataFile(it->first);
        file_stream.open(dataFile.c_str() , std::ifstream::in);
        unsigned int numlines = std::count(std::istreambuf_iterator<char>(file_stream),std::istreambuf_iterator<char>(), '\n');
        file_stream.close();
        double w_pat = it->second;
        std::vector<double> means;

        if( (parameters.num_pats * (parameters.num_pats + 3)/2) != numlines)
        {
            std::cerr << "initial check failed. numlines = " << numlines << " numpats = " << parameters.num_pats << "\n";
        }

        file_stream.open(dataFile.c_str(), std::ifstream::in);

        for (unsigned int i = 0; i < parameters.num_pats; i++)
        {
            double mean_snr = 0.;
            for(unsigned int j = 0; j <= i; j++)
            {
                /*  I don't need the individual values here */
                double snr_value = 0.;
                file_stream >> snr_value;
            }
            file_stream >> mean_snr;
            std::cout << i+1 << ":\t" << mean_snr << "\n";
            means.emplace_back(mean_snr);
        }
        double max_mean = *(std::max_element(means.begin(), means.end()));
        double min_mean = *(std::min_element(means.begin(), means.end()));
        double sum_mean = std::accumulate(means.begin(), means.end(),0.0);
        double mean_mean = sum_mean/means.size();

        max_means.emplace_back(std::pair<double, double>(w_pat, max_mean));
        min_means.emplace_back(std::pair<double, double>(w_pat, min_mean));
        mean_means.emplace_back(std::pair<double, double>(w_pat, mean_mean));
        means.clear();

        file_stream.close();
    }

    converter << "set xtics ( ";
    unsigned int counter = 1;
    for (std::vector<std::pair<double,double> >::iterator it = max_means.begin(); it != max_means.end(); it++)
    {

        converter << it->first ;

        if (counter != max_means.size())
            converter << ", ";

        counter++;
    }
    converter << "); \n";

    gp << converter.str();

    line_command << "plot '-' with linespoints title 'max " << title << " means'  ls 1 lc 1 lw 2, ";
    line_command << "'-' with linespoints title 'mean " << title << " means' ls 2 lc 2 lw 2, ";
    line_command << "'-' with linespoints title 'min " << title << " means' ls 3 lc 3 lw 2; \n";

/*     gp << "set xrange[" << 0.5 << ":" << parameters.num_pats + 1 << "]; \n";
 */
    gp << line_command.str();

    gp.send1d(max_means);
    gp.send1d(mean_means);
    gp.send1d(min_means);

    return;
}		/* -----  end of function GenerateMetricVsWPatFromFile  ----- */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  GenerateSNRvsWPatFromFile
 *  Description:  
 * =====================================================================================
 */
    void
GenerateSNRvsWPatFromFile ( std::vector<std::pair<std::string, double> > inputs )
{
    GenerateMetricVsWPatFromFile(inputs, "SNR");
}		/* -----  end of function GenerateSNRvsWPatFromFile  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  GenerateMeanvsWPatFromFile
 *  Description:  
 * =====================================================================================
 */
    void
GenerateMeansvsWPatFromFile ( std::vector<std::pair<std::string, double> > inputs )
{
    GenerateMetricVsWPatFromFile(inputs, "Mean");
}		/* -----  end of function GenerateMeanvsWPatFromFile  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  GenerateSDvsWPatFromFile
 *  Description:  
 * =====================================================================================
 */
    void
GenerateSDvsWPatFromFile ( std::vector<std::pair<std::string, double> > inputs )
{
    GenerateMetricVsWPatFromFile(inputs, "SD");
}		/* -----  end of function GenerateSDvsWPatFromFile  ----- */

#endif   /* ----- #ifndef utils_INC  ----- */
