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
#include <boost/tuple/tuple.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <ctime>
#include <cmath>
#include "gnuplot-iostream/gnuplot-iostream.h"
#include <mutex>
#include "auryn_definitions.h"
#include <unordered_map>
#include <boost/mpi.hpp>


/*-----------------------------------------------------------------------------
 *  DECLARATIONS
 *-----------------------------------------------------------------------------*/

namespace po = boost::program_options;

struct SNR_data
{
    SNR_data(): SNR(0), mean(0), std(0), mean_noise(0), std_noise(0) {}

    double SNR;
    double mean;
    double std;
    double mean_noise;
    double std_noise;
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
    what_to_plot (): master(false), snr_graphs (false), cum_VS_over(false), multiSNR(false), multiMean(false), multiSTD(false), snr_VS_wPats(false), mean_VS_wPats(false), std_VS_wPats(false), formatPNG(false),  singleMeanAndSTD(false), multiMeanAndSTD(false), for_prints(false), for_meetings(false) {}
    bool master;
    bool snr_graphs;
    bool cum_VS_over;
    std::vector <double> wPats;
    bool multiSNR;
    bool multiMean;
    bool multiSTD;
    bool snr_VS_wPats;
    bool mean_VS_wPats;
    bool std_VS_wPats;
    bool formatPNG;
    bool singleMeanAndSTD;
    bool multiMeanAndSTD;
    bool for_prints;
    bool for_meetings;
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

        ifs.open(pattern);
        if (ifs.is_open())
        {
            while(ifs >> neuronID)
                patterns[i].emplace_back(neuronID);

            std::sort(patterns[i].begin(), patterns[i].end());
#if  DEBUG_ENABLED
            std::cout << "Items loaded and sorted in pattern number " << i << " is " << patterns[i].size() << "\n";
#endif     /* -----  DEBUG_ENABLED  ----- */
        }
        ifs.close();

        converter.str("");
        converter.clear();
        converter << parameters.recallfile_prefix <<  std::setw(8)  << std::setfill('0') << std::to_string(i+1) << ".pat";
        std::string recall = converter.str();
        recalls.emplace_back(std::vector<unsigned int> ());

        ifs.open(recall);

        if (ifs.is_open())
        {
            while(ifs >> neuronID)
                recalls[i].emplace_back(neuronID);

            std::sort(recalls[i].begin(), recalls[i].end());
#if  DEBUG_ENABLED
            std::cout << "Items loaded and sorted in recall number " << i << " is " << recalls[i].size() << "\n";
#endif     /* -----  DEBUG_ENABLED  ----- */
        }
        ifs.close();
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
std::vector<AurynTime>
CalculateTimeToPlotList ()
{
    std::vector<AurynTime> graphing_times;
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
 *         Name:  ReadTimeToPlotListFromFile
 *  Description:  A different implementation where I just read them from a file
 *
 *  Returns: a vector with the times that were read from the file
 *  Arguments:
 *      - NONE
 * =====================================================================================
 */
std::vector<AurynTime>
ReadTimeToPlotListFromFile ()
{
    std::vector<AurynTime> graphing_times;
    double temp;
    std::ifstream timefilestream;
    timefilestream.open(parameters.plot_times_file, std::ifstream::in);
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
}		/* -----  end of function ReadTimeToPlotListFromFile  ----- */


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
    /*  Needs to be 1 because we now have an extra header data struct right at
     *  the beginning */
    unsigned long int numStart = 1;
    unsigned long int numEnd = 0;
    char *currentSpike = NULL;
    unsigned long int numCurrent = 0;
    unsigned long int numdiff = 0;
    unsigned long int step = 0;
    unsigned long int sizeofstruct = sizeof(struct SpikeEvent_type);
    struct SpikeEvent_type *currentRecord = NULL;


    /*  start of last record */
    spikesStart =  (char *)openMapSource.data();
    numStart = 1;
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
        currentRecord = (struct SpikeEvent_type *)currentSpike;
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
    currentRecord = (struct SpikeEvent_type *)currentSpike;
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
    /*  Needs to be 1 because we now have an extra header data struct right at
     *  the beginning */
    unsigned long int numStart = 1;
    unsigned long int numEnd = 0;
    char *currentSpike = NULL;
    unsigned long int numCurrent = 0;
    unsigned long int numdiff = 0;
    unsigned long int step = 0;
    unsigned long int sizeofstruct = sizeof(struct SpikeEvent_type);
    struct SpikeEvent_type *currentRecord = NULL;


    /*  start of last record */
    spikesStart =  (char *)openMapSource.data();
    numStart = 1;
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
        currentRecord = (struct SpikeEvent_type *)currentSpike;
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
    currentRecord = (struct SpikeEvent_type *)currentSpike;
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
            struct SpikeEvent_type *buffer;
            buffer = (struct SpikeEvent_type *)chunkit;
            neurons.emplace_back(buffer->neuronID);
            chunkit += sizeof(struct SpikeEvent_type);
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
PlotHistogram (std::vector<unsigned int> values, std::string outputFileName, AurynTime chunk_time, std::string colour, std::string legendLabel, std::string xlabel, std::string ylabel, float normalise = 1.0)
{
    sort(values.begin(), values.end());

    double binwidth = (values.back() - values.front());
    binwidth /= values.size();

    Gnuplot gp;
    gp << "set style fill solid 0.5; set tics out nomirror; \n"; 
    gp << "set xrange [0:]; \n";
    gp << "set xlabel \"" << xlabel << "\"; set ylabel \"" << ylabel << "\"; \n";
    gp << "binwidth=" << binwidth << "; bin(x,width)=width*floor(x/width)+width/2.0; \n";
    gp << "set term png font \"/usr/share/fonts/dejavu/DejaVuSans.ttf,20\" size 1440,1440; \n";
    gp << "set output \"" << outputFileName << "\"; \n";
    gp << "set title \"Histogram at time " << chunk_time << "\"; \n";
    gp << "plot '-'  using (bin($1,binwidth)):(" << normalise << ") smooth freq with boxes lc rgb \"" << colour << "\" t \"" << legendLabel << "\" ; \n";
    gp.send1d(boost::make_tuple(values, values));

    std::cout << outputFileName << " plotted." << "\n";

}		/* -----  end of function PlotHistogram  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  PlotDualHistogram
 *  Description:  Two histograms together, like E and I, noise and background
 *  and so on
 * =====================================================================================
 */
    void
PlotDualHistogram (std::vector<unsigned int> values, std::vector<unsigned int> values1, std::string outputFileName, AurynTime chunk_time, std::string colour1, std::string legendLabel1, std::string colour2, std::string legendLabel2, std::string xlabel, std::string ylabel, float normalise1 = 1.0, float normalise2 = 1.0)
{
    sort(values.begin(), values.end());
    sort(values1.begin(), values1.end());

    double binwidth = (values.back() - values.front());
    double binwidth1 = (values1.back() - values1.front());

    binwidth /= values.size();
    binwidth1 /= values1.size();

    double graph_binwidth = (binwidth < binwidth1)? binwidth : binwidth1;

    Gnuplot gp;
    gp << "set style fill solid 0.5; set tics out nomirror; \n"; 
    gp << "set xrange [0:]; \n";
    gp << "set xlabel \"" << xlabel << "\"; set ylabel \"" << ylabel << "\"; \n";
    gp << "binwidth=" << graph_binwidth << "; bin(x,width)=width*floor(x/width)+width/2.0; \n";
    gp << "set term png font \"/usr/share/fonts/dejavu/DejaVuSans.ttf,20\" size 1440,1440; \n";
    gp << "set output \"" << outputFileName << "\"; \n";
    gp << "set title \"Histogram at time " << chunk_time << "\"; \n";
    gp << "plot '-'  using (bin($1,binwidth)):(" << normalise1 << ") smooth freq with boxes lc rgb \"" << colour1 << "\" t \"" << legendLabel1 << "\" ,";
    gp << "'-'  using (bin($1,binwidth)):(" << normalise2 << ") smooth freq with boxes lc rgb \"" << colour2 << "\" t \"" << legendLabel2 << "\" ; \n";
    gp.send1d(boost::make_tuple(values, values));
    gp.send1d(boost::make_tuple(values1, values1));

    std::cout << outputFileName << " plotted." << "\n";

}		/* -----  end of function PlotDualHistogram  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  PlotConnectionsAndRates
 *  Description:  
 * =====================================================================================
 */
    void
PlotConnectionsAndRates ( std::vector<unsigned int>rates, std::vector<unsigned int>connections_e, std::vector<unsigned int> connections_i, std::string outputFilePrefix, unsigned int chunk_time )
{
    std::vector<unsigned int>neuron_numbers;
    std::vector<int>negative_connections_i;

    for (std::vector<unsigned int>::iterator it = connections_i.begin(); it!= connections_i.end(); it++)
    {
        negative_connections_i.emplace_back((*it) * -1);
    }

    for(int i = 1; i < 801; i++)
        neuron_numbers.emplace_back(i);

    Gnuplot gp;
    gp << "set term png font \"/usr/share/fonts/dejavu/DejaVuSans.ttf,20\" size 1440,1440; \n";
    gp << "set xrange [0:800]; \n";
    gp << "set xlabel \"pattern neurons\"; set ylabel \"Connections\"; \n";
    gp << "set output \"" << outputFilePrefix << ".connections.png\"; \n";
    gp << "set title \"Pattern neurons - connections - " << chunk_time << "\"; \n";
    /* 
    gp << "plot '-' lw 3 with lines title 'Connections from E',";
    gp << "'-' lw 3 with lines title 'Connections from I'; \n";
    */
    gp << "set style data histogram; set style histogram cluster; \n";
    gp << "plot '-' using 2 title 'Connections from E', '-' using 2 title 'Connections from I' ; \n";
    gp.send1d(boost::make_tuple(neuron_numbers, connections_e));
    gp.send1d(boost::make_tuple(neuron_numbers, negative_connections_i));

    Gnuplot gp1;
    gp1 << "set term png font \"/usr/share/fonts/dejavu/DejaVuSans.ttf,20\" size 1440,1440; \n";
    gp1 << "set xrange [0:]; \n";
    gp1 << "set xlabel \"neurons\"; set ylabel \"Firing rate\"; \n";
    gp1 << "set output \"" << outputFilePrefix << ".rates.png\"; \n";
    gp1 << "set title \"Pattern neurons - rates - " << chunk_time << "\"; \n";
    gp1 << "plot '-' using 1:2 lw 3 with points; \n";
    gp1.send1d(boost::make_tuple(neuron_numbers, rates));


    Gnuplot gp2;
    gp2 << "set term png font \"/usr/share/fonts/dejavu/DejaVuSans.ttf,20\" size 1440,1440; \n";
    gp2 << "set xrange [0:]; \n";
    gp2 << "set xlabel \"connections from E\"; set ylabel \"Firing rate\"; \n";
    gp2 << "set output \"" << outputFilePrefix << ".rates-e.png\"; \n";
    gp2 << "set title \"Pattern neurons - rates vs E connections - " << chunk_time << "\"; \n";
    gp2 << "plot '-' using 1:2 lw 3 with points; \n";
    gp2.send1d(boost::make_tuple(connections_e, rates));


    Gnuplot gp3;
    gp3 << "set term png font \"/usr/share/fonts/dejavu/DejaVuSans.ttf,20\" size 1440,1440; \n";
    gp3 << "set xrange [0:]; \n";
    gp3 << "set xlabel \"connections from I\"; set ylabel \"Firing rate\"; \n";
    gp3 << "set output \"" << outputFilePrefix << ".rates-i.png\"; \n";
    gp3 << "set title \"Pattern neurons - I connections vs rates" << chunk_time << "\"; \n";
    gp3 << "plot '-' using 1:2 lw 3 with points; \n";
    gp3.send1d(boost::make_tuple(connections_i, rates));

    std::cout << "Sizes are: " << rates.size() << ", " << connections_i.size() << ", " << connections_e.size() << std::endl;

    std::vector<boost::tuple<unsigned int, unsigned int, unsigned int> > data(800);
    for (int i = 0; i < rates.size(); i++)
    {
        boost::tuple<unsigned int, unsigned int, unsigned int> something(connections_i[i], connections_e[i], rates[i]);
        data.emplace_back(something);
    }

    Gnuplot gp4;
    gp4 << "set term png font \"/usr/share/fonts/dejavu/DejaVuSans.ttf,20\" size 1440,1440; \n";
    gp4 << "set xlabel \"connections from I\"; set ylabel \"Connections from E\"; \n";
    gp4 << "set output \"" << outputFilePrefix << ".splot.png\"; \n";
    gp4 << "set title \"Pattern neurons - connections vs firing rate " << chunk_time << "\"; \n";
    gp4 << "splot '-' with errorlines title 'Firing Rate'; \n";
    gp4.send1d(data);

    std::vector<unsigned int>conn_sum;
    for (int i = 0; i < rates.size(); i++)
    {
        conn_sum.emplace_back(connections_e[i] + connections_i[i]);
    }
    PlotHistogram(conn_sum, outputFilePrefix + ".totalConnections.histogram.png", chunk_time, "blue", "Total incoming connections", "number of total incoming connections", "ratio of total pattern neurons", 1./800.);

    std::cout << outputFilePrefix << " files plotted" << std::endl;

}		/* -----  end of function PlotConnectionsAndRates  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  GetSNR
 *  Description:  Get the SNR of one pattern
 * =====================================================================================
 */
    struct SNR_data
GetSNR (std::vector<unsigned int> patternRates, std::vector<unsigned int> noiseRates )
{
    struct SNR_data snr;
    unsigned int sum_patterns = std::accumulate(patternRates.begin(), patternRates.end(),0.0);
    snr.mean = sum_patterns/patternRates.size();

    double sq_diff = 0.;
    std::for_each(patternRates.begin(), patternRates.end(), [&] (const double d) {
            sq_diff += pow((d - snr.mean),2);
            });

    snr.std = sqrt(sq_diff/(patternRates.size() -1));

    unsigned int sum_noises = std::accumulate(noiseRates.begin(), noiseRates.end(),0.0);
    snr.mean_noise = sum_noises/noiseRates.size();

    sq_diff = 0.;
    std::for_each(noiseRates.begin(), noiseRates.end(), [&] (const double d) {
            sq_diff += pow((d - snr.mean_noise),2);
            });

    snr.std_noise  = sqrt(sq_diff/(noiseRates.size() -1));

    snr.SNR = ((2.*pow((snr.mean - snr.mean_noise),2))/(pow(snr.std,2) + pow(snr.std_noise,2)));

    return snr;
}		/* -----  end of function GetSNR  ----- */



/*
 * ===  FUNCTION  ======================================================================
 *         Name:  MasterFunction
 *  Description:
 * =====================================================================================
 */
    void
MasterFunction (std::vector<boost::iostreams::mapped_file_source> &spikes_E, std::vector<boost::iostreams::mapped_file_source> &spikes_I, std::vector<std::vector<unsigned int> >&patterns, std::vector<std::vector <unsigned int> >&recalls, AurynTime chunk_time, unsigned int patternRecalled, std::unordered_map<unsigned int, unsigned int> con_ee_count, std::unordered_map<unsigned int, unsigned int> con_ie_count, boost::mpi::communicator world)
{
    std::vector <unsigned int>neuronsE;
    std::vector <unsigned int>neuronsI;
    std::vector <unsigned int>pattern_neurons_rate;
    std::vector <unsigned int>pattern_neurons_connections_i;
    std::vector <unsigned int>pattern_neurons_connections_e;
    std::vector <unsigned int>noise_neurons_rate;
    std::vector <unsigned int>recall_neurons_rate;
    std::vector <unsigned int>neuronsE_rate;
    std::vector <unsigned int>neuronsI_rate;
    std::ostringstream converter;
    struct SNR_data snr_at_chunk_time;

    std::cout << "Chunk time received: " << chunk_time << "\n";

#ifdef DEBUG
    std::cout << "[MasterFunction] Thread " << std::this_thread::get_id() << " active." << "\n";
#endif
    /*  Fill up my vectors with neurons that fired in this period */
    for (unsigned int i = 0; i < parameters.mpi_ranks; ++i)
    {
        /*  Excitatory */
        char * chunk_start = BinaryLowerBound(chunk_time - (1.0/dt), std::ref(spikes_E[i]));
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
        chunk_start = BinaryLowerBound(chunk_time - (1.0/dt), std::ref(spikes_I[i]));
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

    /*  Print inhibitory neurons to a file */
    converter.str("");
    converter.clear();
    converter << "00-firing-rate.i." << chunk_time*dt << "." << world.rank() << ".txt";
    std::ofstream firing_rates_i;
    firing_rates_i.open(converter.str(), std::ofstream::out | std::ofstream::trunc);
    if (firing_rates_i.is_open())
    {
        unsigned int i = 1;
        for (std::vector <unsigned int>::iterator it = neuronsI.begin(); it != neuronsI.end(); it++)
        {
            firing_rates_i << i++ << "\t" << *it << std::endl;
        }
    }
    firing_rates_i.close();

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
            pattern_neurons_connections_e.emplace_back(con_ee_count[i]);
            pattern_neurons_connections_i.emplace_back(con_ie_count[i]);
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
        /*  All other E neurons */
        neuronsE_rate.emplace_back(rate);
    }

    /*  Print excitatory neurons to a file */
    converter.str("");
    converter.clear();
    converter << "00-firing-rate.e." << chunk_time*dt << ".txt";
    std::ofstream firing_rates_e;
    firing_rates_e.open(converter.str(), std::ofstream::out | std::ofstream::trunc);
    if (firing_rates_e.is_open())
    {
        unsigned int i = 1;
        for (std::vector <unsigned int>::iterator it = neuronsE.begin(); it != neuronsE.end(); it++)
        {
            firing_rates_e << i++ << "\t" << *it << std::endl;
        }
    }
    firing_rates_e.close();

    /*  Print pattern and noise files to a file */
    std::ofstream firing_rates_e_pattern, connections_e, connections_i;
    converter.str("");
    converter.clear();
    converter << "00-firing-rate.e.pattern." << chunk_time*dt << ".txt";
    firing_rates_e_pattern.open(converter.str(), std::ofstream::out| std::ofstream::trunc);
    converter.str("");
    converter.clear();
    converter << "00-firing-rate.e.pattern.connections_e." << chunk_time*dt <<  ".txt";
    connections_e.open(converter.str(), std::ofstream::out | std::ofstream::trunc);
    converter.str("");
    converter.clear();
    converter << "00-firing-rate.e.pattern.connections_i." << chunk_time*dt <<  ".txt";
    connections_i.open(converter.str(), std::ofstream::out| std::ofstream::trunc);
    if (firing_rates_e_pattern.is_open() && connections_e.is_open() && connections_i.is_open())
    {
        for (unsigned int i = 0;  i <= pattern_neurons_rate.size(); i++)
        {
            firing_rates_e_pattern << i << "\t" << pattern_neurons_rate[i] << std::endl;
            connections_e << i << "\t" << pattern_neurons_connections_e[i] << std::endl;
            connections_i << i << "\t" << pattern_neurons_connections_i[i] << std::endl;
        }
    }
    else
    {
        std::cerr << "Couldn't open some file for writing. Skipping. Run again." << std::endl;
    }
    firing_rates_e_pattern.close();
    connections_e.close();
    connections_i.close();

    /*  Plot histogram for E and I neurons */
    converter.str("");
    converter.clear();
    converter << parameters.output_file << "-" << chunk_time*dt << ".e.i.histogram.png"; 
    PlotDualHistogram(neuronsE_rate, neuronsI_rate, converter.str(), chunk_time*dt, "blue", "Excitatory", "red", "Inhibitory", "firing rate", "ratio of neurons", 1./8000., 1./2000.);

    /*  Plot graphs for pattern and noise */
    converter.str("");
    converter.clear();
    converter << parameters.output_file << "-" << chunk_time*dt << ".pattern.noise." << patternRecalled << ".histogram.png";
    PlotDualHistogram(pattern_neurons_rate, noise_neurons_rate, converter.str(), chunk_time*dt, "green" , "Pattern", "black", "Noise", "firing rate", "ratio of neurons" , 1./800., 1./7200. );

    converter.str("");
    converter.clear();
    converter << parameters.output_file << "-" << chunk_time*dt << ".con-rates." << patternRecalled << ".histogram.png";
    PlotDualHistogram(pattern_neurons_connections_e, pattern_neurons_connections_i, converter.str(), chunk_time*dt, "green" , "From E", "black", "From I", "Number of incoming connections", "ratio of pattern neurons" , 1./800., 1./800. );


    /*  Plot graphs for connections and rates */
    converter.str("");
    converter.clear();
    converter << parameters.output_file << "-" << chunk_time*dt << ".con-rates." << patternRecalled; 
    PlotConnectionsAndRates(pattern_neurons_rate, pattern_neurons_connections_e, pattern_neurons_connections_i, converter.str(), chunk_time*dt);

    /*  Print collected data to files */
    snr_at_chunk_time = GetSNR(pattern_neurons_rate, noise_neurons_rate);

    std::ofstream snr_file, mean_file, std_file, mean_noise_file, std_noise_file;
    converter.str("");
    converter.clear();
    converter << "00-SNR-data." << world.rank() << ".txt";
    snr_file.open(converter.str(), std::ofstream::out | std::ofstream::trunc);
    snr_file << (chunk_time*dt) << "\t" << snr_at_chunk_time.SNR << std::endl;
    snr_file.close();

    converter.str("");
    converter.clear();
    converter << "00-Mean-data." << world.rank() << ".txt";
    mean_file.open(converter.str(), std::ofstream::out | std::ofstream::trunc);
    mean_file << (chunk_time*dt) << "\t" << snr_at_chunk_time.mean << std::endl;
    mean_file.close();

    converter.str("");
    converter.clear();
    converter << "00-STD-data." << world.rank() << ".txt";
    std_file.open(converter.str(), std::ofstream::out | std::ofstream::trunc);
    std_file << (chunk_time*dt) << "\t" << snr_at_chunk_time.std << std::endl;
    std_file.close();

    converter.str("");
    converter.clear();
    converter << "00-Mean-noise-data." << world.rank() << ".txt";
    mean_noise_file.open(converter.str(), std::ofstream::out | std::ofstream::trunc);
    mean_noise_file << (chunk_time*dt) << "\t" << snr_at_chunk_time.mean_noise << std::endl;
    mean_noise_file.close();
;
    converter.str("");
    converter.clear();
    converter << "00-STD-noise-data." << world.rank() << ".txt";
    std_noise_file.open(converter.str(), std::ofstream::out | std::ofstream::trunc);
    std_noise_file << (chunk_time*dt) << "\t" << snr_at_chunk_time.std_noise << std::endl;
    std_noise_file.close();

    std::cout << "Rank finished printing SNR files." << std::endl;
    return;
}		/* -----  end of function MasterFunction  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  PlotSNRGraphs
 *  Description:  
 * =====================================================================================
 */
    void
PlotSNRGraphs (std::multimap <double, struct SNR_data> snr_data)
{
    std::ostringstream plot_command;
    Gnuplot gp;
    std::vector<std::pair<double, double> > points_means;
    unsigned int max_point = 0;

    /* Initial set up */
    if (!plot_this.formatPNG)
    {
        gp << "set output \"SNR.svg\" \n";
        if(plot_this.for_meetings)
        {
            gp << "set term svg font \"/usr/share/fonts/dejavu/DejaVuSans.ttf,30\" size 2880,1440 dynamic enhanced mousing standalone; \n";
        }
        else
        {
            gp << "set term svg font \"/usr/share/fonts/dejavu/DejaVuSans.ttf,20\" size 2880,1440 dynamic enhanced mousing standalone; \n";
        }
    }
    else
    {
        gp << "set output \"SNR.png\" \n";
        if(plot_this.for_meetings)
        {
            gp << "set term png font \"/usr/share/fonts/dejavu/DejaVuSans.ttf,30\" size 2880,1440 ; \n";
        }
        else
        {
            gp << "set term png font \"/usr/share/fonts/dejavu/DejaVuSans.ttf,20\" size 2880,1440 ; \n";

        }
    }
    gp << "set title \"SNR  vs  number of patterns - " << parameters.output_file << "\" \n";


#if  0     /* ----- #if 0 : If0Label_2 ----- */
    std::cout << "All snrs: " << "\n";
    for (std::multimap <double, struct SNR_data>::iterator it = snr_data.begin(); it != snr_data.end(); it++)
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
            std::multimap <double, struct SNR_data>::iterator it = snr_data.begin();
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
    gp << "set xlabel \"Number of patterns stored\"; \n";
    gp << "set ylabel \"SNR\"; \n";
    if(!plot_this.for_meetings)
    {
        gp << "set xtics 1; \n";
        gp << "set ytics 1; \n";
        gp << "set grid; \n";
        gp << "plot '-' with lines title 'mean SNR' \n";
    }
    else
    {
        gp << "plot '-' lw 3 with lines title 'mean SNR' \n";
    }
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
PrintSNRDataToFile (std::multimap <double, struct SNR_data> snr_data)
{
    std::cout << "Printing SNR data to file." << "\n";
    unsigned int time_counter = 0;
    std::ofstream snr_stream;
    std::ofstream mean_stream;
    std::ofstream std_stream;
    std::ofstream mean_noise_stream;
    std::ofstream std_noise_stream;
    snr_stream.open("00-SNR-data.txt");
    mean_stream.open("00-Mean-data.txt");
    std_stream.open("00-STD-data.txt");
    mean_noise_stream.open("00-Mean-noise-data.txt");
    std_noise_stream.open("00-STD-noise-data.txt");

    for (unsigned int i = 0; i < parameters.num_pats; i++)
    {
        struct SNR_data means;
        for(unsigned int j = 0; j <= i; j++)
        {
            std::multimap <double, struct SNR_data>::iterator it = snr_data.begin();
            std::advance(it, time_counter);
            time_counter++;

            means.SNR += ((it->second).SNR);
            means.mean += ((it->second).mean);
            means.std += ((it->second).std);
            means.mean_noise += ((it->second).mean_noise);
            means.std_noise += ((it->second).std_noise);

            snr_stream << (it->second).SNR << "\n";
            mean_stream << (it->second).mean << "\n";
            std_stream << (it->second).std << "\n";
            mean_noise_stream << (it->second).mean_noise << "\n";
            std_noise_stream << (it->second).std_noise << "\n";
        }
        means.SNR /= (i+1);
        means.mean /= (i+1);
        means.std /= (i+1);
        means.mean_noise /= (i+1);
        means.std_noise /= (i+1);
        snr_stream << means.SNR << "\n";
        mean_stream << means.mean << "\n";
        std_stream << means.std << "\n";
        mean_noise_stream << means.mean_noise << "\n";
        std_noise_stream << means.std_noise << "\n";
    }

    snr_stream.close();
    mean_stream.close();
    std_stream.close();
    mean_noise_stream.close();
    std_noise_stream.close();
    return ;
}		/* -----  end of function PrintSNRDataToFile  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  GenerateMetricPlotFromFile
 *  Description:  
 * =====================================================================================
 */
    void
GenerateMetricPlotFromFile(std::string dataFile, std::string metric, std::string addendum = "", unsigned int lc = 1 )
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
    if (plot_this.formatPNG)
    {
        gp << "set output \"" << metric << ".png\" \n";
        if (plot_this.for_meetings)
        {
            gp << "set term png font \"/usr/share/fonts/dejavu/DejaVuSans.ttf,30\" size 2880,1440 ; \n";
            gp << "set nogrid; \n";
        }
        else
        {
            gp << "set term png font \"/usr/share/fonts/dejavu/DejaVuSans.ttf,20\" size 2880,1440 ; \n";
            gp << "set grid; \n";
        }
    }
    else if(plot_this.for_prints)
    {
        gp << "set term epslatex color colortext; \n";
        gp << "set output \"" << metric << ".tex\" \n";
        gp << "set border 3 \n";
    }
    else
    {
        gp << "set output \"" << metric << ".svg\" \n";
        if (plot_this.for_meetings)
        {
            gp << "set term svg font \"/usr/share/fonts/dejavu/DejaVuSans.ttf,30\" size 2880,1440 dynamic enhanced mousing standalone; \n";
            gp << "set nogrid; \n";
        }
        else
        {
            gp << "set term svg font \"/usr/share/fonts/dejavu/DejaVuSans.ttf,20\" size 2880,1440 dynamic enhanced mousing standalone; \n";
            gp << "set grid; \n";
        }
    }
    gp << "set title \"" << metric << "  vs  number of patterns - " << parameters.output_file << "\" \n";


    for (unsigned int i = 0; i < parameters.num_pats; i++)
    {
        double mean_value = 0.;
        for(unsigned int j = 0; j <= i; j++)
        {
            double value = 0.;
            file_stream >> value;
            plot_command << "set label \"\" at " << i+1 << "," << value << " point pointtype " << j+1 << " lc " << lc << " ;\n";
            if (value > max_point)
                max_point = ceil(value);
        }
        file_stream >> mean_value;
        std::cout << i+1 << ":\t" << mean_value << "\n";
        points_means.emplace_back(std::pair<double, double>(i+1, mean_value));
    }

    file_stream.close();

    gp << plot_command.str();

    gp << "set xrange[" << 0 << ":" << parameters.num_pats + 1 << "]; \n";
    gp << "set yrange[:" << max_point << "]; \n";
    gp << "set ylabel \"" << metric << "\"; \n";
    if(plot_this.for_prints || plot_this.for_meetings)
    {
        int xvar = parameters.num_pats/4;
        gp << "set xtics nomirror " << xvar << "; \n";
        int yvar = max_point/2;
        gp << "set ytics nomirror " << yvar << "; \n";
    }
    else{
        gp << "set xtics 1; \n";
        gp << "set ytics nomirror autofreq; \n";
    }

    gp << "set xlabel \"Number of patterns stored\"; \n";
    if (plot_this.for_meetings)
    {
        gp << "plot '-' with lines title 'mean " << metric << " - " << addendum.c_str() << "' lc " << lc << " lw 3" << "; \n";
    }
    else
    {
        gp << "plot '-' with lines title 'mean " << metric << " - " << addendum.c_str() << "' lc " << lc << "; \n";
    }
    gp.send1d(points_means);

    return;
}		/* -----  end of function GenerateMetricPlotFromFile  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  GenerateSNRPlotFromFile
 *  Description:  
 * =====================================================================================
 */
    void
GenerateSNRPlotFromFile (std::string dataFile, std::string addendum = "", unsigned int lc = 1 )
{
    GenerateMetricPlotFromFile(dataFile, "SNR", addendum, lc);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  GenerateMeanPlotFromFile
 *  Description:  
 * =====================================================================================
 */
    void
GenerateMeanPlotFromFile (std::string dataFile, std::string addendum = "", unsigned int lc = 1 )
{
    GenerateMetricPlotFromFile(dataFile, "Mean", addendum, lc);
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  GenerateMeanNoisePlotFromFile
 *  Description:  
 * =====================================================================================
 */
    void
GenerateMeanNoisePlotFromFile (std::string dataFile, std::string addendum = "", unsigned int lc = 1 )
{
    GenerateMetricPlotFromFile(dataFile, "MeanNoise", addendum, lc);
}
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  GenerateSTDPlotFromFile
 *  Description:  
 * =====================================================================================
 */
    void
GenerateSTDPlotFromFile (std::string dataFile, std::string addendum = "", unsigned int lc = 1 )
{
    GenerateMetricPlotFromFile(dataFile, "STD", addendum, lc);
}
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  GenerateSTDNoisePlotFromFile
 *  Description:  
 * =====================================================================================
 */
    void
GenerateSTDNoisePlotFromFile (std::string dataFile, std::string addendum = "", unsigned int lc = 1 )
{
    GenerateMetricPlotFromFile(dataFile, "STDNoise", addendum, lc);
}
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
    if (plot_this.formatPNG)
    {
        gp << "set output \"" << title << "-multi.png\" \n";
        if (plot_this.for_meetings)
        {
            gp << "set term png font \"/usr/share/fonts/dejavu/DejaVuSans.ttf,30\" size 2880,1440 ; \n";
            gp << "set nogrid; \n";
        }
        else
        {
            gp << "set term png font \"/usr/share/fonts/dejavu/DejaVuSans.ttf,20\" size 2880,1440 ; \n";
            gp << "set grid; \n";
        }
    }
    else if(plot_this.for_prints)
    {
        gp << "set term epslatex color colortext; \n";
        gp << "set output \"" << title << "-multi-prints.tex\" \n";
        gp << "set border 3 \n";
    }
    else
    {
        gp << "set output \"" << title << "-multi.svg\" \n";
        if (plot_this.for_meetings)
        {
            gp << "set term svg font \"/usr/share/fonts/dejavu/DejaVuSans.ttf,30\" size 2880,1440 dynamic enhanced mousing standalone; \n";
            gp << "set nogrid; \n";
        }
        else
        {
            gp << "set term svg font \"/usr/share/fonts/dejavu/DejaVuSans.ttf,20\" size 2880,1440 dynamic enhanced mousing standalone; \n";
            gp << "set grid; \n";
        }
    }
    gp << "set title \"" << title << "  vs  number of patterns - " << parameters.output_file << "\" \n";
    gp << "set ylabel \"" << title << "\"; \n";
    gp << "set xlabel \"Number of patterns stored\"; \n";
    line_command << "plot ";

    for (std::vector<std::pair<std::string, std::string> >::iterator it = inputs.begin(); it != inputs.end(); it++)
    {
        lc++;
        std::ifstream file_stream;
        std::string data_file(it->first);
        file_stream.open(data_file.c_str() , std::ifstream::in);
        unsigned int numlines = std::count(std::istreambuf_iterator<char>(file_stream),std::istreambuf_iterator<char>(), '\n');
        file_stream.close();
        std::string addendum(it->second);
        std::vector<std::pair<double, double> > points_means;

        if( (parameters.num_pats * (parameters.num_pats + 3)/2) != numlines)
        {
            std::cerr << "initial check failed. numlines = " << numlines << " numpats = " << parameters.num_pats << "\n";
        }

        file_stream.open(data_file.c_str(), std::ifstream::in);

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
        if(plot_this.for_meetings)
        {
            line_command << "'-' with lines title 'mean " << title << " - " << addendum.c_str() << "' lc " << lc << " lw 3, ";
        }
        else
        {
            line_command << "'-' with lines title 'mean " << title << " - " << addendum.c_str() << "' lc " << lc << " lw 2, ";
        }
        file_stream.close();
    }


    gp << "set xrange[" << 0.5 << ":" << parameters.num_pats + 1 << "]; \n";
    gp << "set yrange[:" << max_point << "]; \n";
    if(plot_this.for_prints || plot_this.for_meetings)
    {
        int xvar = parameters.num_pats/4;
        gp << "set xtics nomirror " << xvar << "; \n";
        int yvar = max_point/2;
        gp << "set ytics nomirror " << yvar << "; \n";
    }
    else{
        gp << "set xtics 1; \n";
        gp << "set ytics nomirror autofreq; \n";
    }

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
GenerateMultiMeanPlotFromFile (std::vector<std::pair<std::string, std::string> > inputs)
{
    GenerateMultiMetricPlotFromFile(inputs, "Mean");
}
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  GenerateMultiMeanNoisePlotFromFile
 *  Description:  
 * =====================================================================================
 */
    void
GenerateMultiMeanNoisePlotFromFile (std::vector<std::pair<std::string, std::string> > inputs)
{
    GenerateMultiMetricPlotFromFile(inputs, "MeanNoise");
}
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  GenerateMultiSTDPlotFromFile
 *  Description:  
 * =====================================================================================
 */
    void
GenerateMultiSTDPlotFromFile (std::vector<std::pair<std::string, std::string> > inputs)
{
    GenerateMultiMetricPlotFromFile(inputs, "STD");
}

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  GenerateMultiSTDNoisePlotFromFile
 *  Description:  
 * =====================================================================================
 */
    void
GenerateMultiSTDNoisePlotFromFile (std::vector<std::pair<std::string, std::string> > inputs)
{
    GenerateMultiMetricPlotFromFile(inputs, "STDNoise");
}
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  GenerateMetric_VS_WPatFromFile
 *  Description:  
 * =====================================================================================
 */
    void
GenerateMetric_VS_WPatFromFile(std::vector<std::pair<std::string, double> > inputs, std::string title)
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
        gp << "set title \"" << title << "  vs   w\\\\_pat - " << parameters.output_file << "\" \n";
        gp << "set xlabel \"w\\\\_pat\"; \n";
    }
    else
    {
        gp << "set output \"" << title << "-w_pat.png\" \n";
        gp << "set term png font \"/usr/share/fonts/dejavu/DejaVuSans.ttf,20\" size 2880,1440 ; \n";
        gp << "set title \"" << title << "  vs  w_pat - " << parameters.output_file << "\" \n";
        gp << "set xlabel \"w_pat\"; \n";
    }
    gp << "set ylabel \"" << title << "\"; \n";
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
}		/* -----  end of function GenerateMetric_VS_WPatFromFile  ----- */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  GenerateSNR_VS_WPatFromFile
 *  Description:  
 * =====================================================================================
 */
    void
GenerateSNR_VS_WPatFromFile ( std::vector<std::pair<std::string, double> > inputs )
{
    GenerateMetric_VS_WPatFromFile(inputs, "SNR");
}		/* -----  end of function GenerateSNR_VS_WPatFromFile  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  GenerateMean_VS_WPatFromFile
 *  Description:  
 * =====================================================================================
 */
    void
GenerateMean_VS_WPatFromFile ( std::vector<std::pair<std::string, double> > inputs )
{
    GenerateMetric_VS_WPatFromFile(inputs, "Mean");
}		/* -----  end of function GenerateMean_VS_WPatFromFile  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  GenerateMean_VS_WPatFromFile
 *  Description:  
 * =====================================================================================
 */
    void
GenerateMeanNoise_VS_WPatFromFile ( std::vector<std::pair<std::string, double> > inputs )
{
    GenerateMetric_VS_WPatFromFile(inputs, "Mean");
}		/* -----  end of function GenerateMean_VS_WPatFromFile  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  GenerateSTD_VS_WPatFromFile
 *  Description:  
 * =====================================================================================
 */
    void
GenerateSTD_VS_WPatFromFile ( std::vector<std::pair<std::string, double> > inputs )
{
    GenerateMetric_VS_WPatFromFile(inputs, "STD");
}		/* -----  end of function GenerateSTD_VS_WPatFromFile  ----- */
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  GenerateSTDNoise_VS_WPatFromFile
 *  Description:  
 * =====================================================================================
 */
    void
GenerateSTDNoise_VS_WPatFromFile ( std::vector<std::pair<std::string, double> > inputs )
{
    GenerateMetric_VS_WPatFromFile(inputs, "STDNoise");
}		/* -----  end of function GenerateSTDNoise_VS_WPatFromFile  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  GenerateMultiMeanWithSTDFromFiles
 *  Description:  
 * =====================================================================================
 */
    void
GenerateMultiMeanWithSTDFromFiles (std::vector<boost::tuple<std::string, std::string, std::string> > inputs, std::string title )
{
    Gnuplot gp;
    std::ostringstream line_command;
    std::vector<std::vector<boost::tuple<double, double, double> > > all_points_means;
    unsigned int max_point = 0;
    unsigned int lc = 0;

    /* Initial set up */
    if (plot_this.formatPNG)
    {
        gp << "set output \"" << title;
        if(inputs.size() > 1)
           gp << "-multi.png\" \n";
        else
            gp << ".png\" \n";
        gp << "set term png font \"/usr/share/fonts/dejavu/DejaVuSans.ttf,20\" size 2880,1440 ; \n";
    }
    else if (plot_this.for_prints)
    {
        gp << "set term epslatex color colortext; \n";
        gp << "set output \"" << title;
        if(inputs.size() > 1)
           gp << "-multi.tex\" \n";
        else
            gp << ".tex\" \n";
        gp << "set border 3 \n";
    }
    else
    {
        gp << "set output \"" << title;
        if(inputs.size() > 1)
           gp << "-multi.svg\" \n";
        else
            gp << ".svg\" \n";
        gp << "set term svg font \"/usr/share/fonts/dejavu/DejaVuSans.ttf,20\" size 2880,1440 dynamic enhanced mousing standalone; \n";
    }
    gp << "set title \"" << title << "  vs  number of patterns - " << parameters.output_file << "\" \n";
    gp << "set ylabel \"" << title << "\"; \n";

    gp << "set xlabel \"Number of patterns stored\"; \n";
    line_command << "plot ";

    for (std::vector<boost::tuple<std::string, std::string, std::string> >::iterator it = inputs.begin(); it != inputs.end(); it++)
    {
        lc++;
        std::ifstream file_stream_mean;
        std::ifstream file_stream_std;
        std::string data_file_mean(it->get<0>());
        std::string data_file_std(it->get<1>());
        file_stream_mean.open(data_file_mean.c_str() , std::ifstream::in);
        file_stream_std.open(data_file_std.c_str() , std::ifstream::in);

        unsigned int numlines_mean = std::count(std::istreambuf_iterator<char>(file_stream_mean),std::istreambuf_iterator<char>(), '\n');
        unsigned int numlines_std = std::count(std::istreambuf_iterator<char>(file_stream_std),std::istreambuf_iterator<char>(), '\n');

        file_stream_mean.close();
        file_stream_std.close();
        std::string addendum(it->get<2>());
        std::vector<boost::tuple<double, double, double> > points_means;

        if( (parameters.num_pats * (parameters.num_pats + 3)/2) != numlines_mean || numlines_mean != numlines_std)
        {
            std::cerr << "initial check failed. numlines = " << numlines_mean << ", " << numlines_std << " numpats = " << parameters.num_pats << "\n";
        }

        file_stream_mean.open(data_file_mean.c_str(), std::ifstream::in);
        file_stream_std.open(data_file_std.c_str() , std::ifstream::in);

        for (unsigned int i = 0; i < parameters.num_pats; i++)
        {
            double mean_snr = 0.;
            double std_snr = 0.;
            for(unsigned int j = 0; j <= i; j++)
            {
                /*  Read but ignore */
                double snr_value = 0.;
                file_stream_mean >> snr_value;
                file_stream_std >> snr_value;
            }
            file_stream_mean >> mean_snr;
            file_stream_std >> std_snr;
            std_snr /= 2.0;
            std::cout << i+1 << ":\t" << mean_snr << ", " << std_snr<< "\n";
            if (mean_snr > max_point)
                max_point = ceil(mean_snr);
            points_means.emplace_back(boost::tuple<double, double, double>(i+1, mean_snr, std_snr));
        }
        all_points_means.emplace_back(points_means);
        points_means.clear();
        if (plot_this.for_meetings)
        {
            line_command << "'-' using 1:2:3 with yerrorbars title 'std" <<  " - " << addendum.c_str() << "' lc " << lc << " lw 3, ";
            line_command << "'-' using 1:2:3 with lines title 'mean" << " - " << addendum.c_str() << "' lc " << lc << " lw 3, ";
        }
        else
        {
            line_command << "'-' using 1:2:3 with yerrorbars title 'std" <<  " - " << addendum.c_str() << "' lc " << lc << " lw 2, ";
            line_command << "'-' using 1:2:3 with lines title 'mean" << " - " << addendum.c_str() << "' lc " << lc << " lw 2, ";
        }
        file_stream_mean.close();
    }

    if(plot_this.for_prints || plot_this.for_meetings)
    {
        int xvar = parameters.num_pats/4;
        gp << "set xtics nomirror " << xvar << "; \n";
        int yvar = max_point/2;
        gp << "set ytics nomirror " << yvar << "; \n";
    }
    else
    {
        gp << "set xtics 1; \n";
        gp << "set grid; \n";
        gp << "set ytics nomirror autofreq; \n";
    }

    gp << "set xrange[" << 0.5 << ":" << parameters.num_pats + 1 << "]; \n";
    gp << line_command.str();
    gp << " ;\n";

    for (std::vector<std::vector<boost::tuple<double,double, double> > >::iterator it = all_points_means.begin(); it != all_points_means.end(); it++)
    {
        gp.send1d(*it);
        gp.send1d(*it);
    }

    return ;
}		/* -----  end of function GenerateMultiMeanWithSTDFromFiles  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  GetConnectionCounts
 *  Description:  Gets the number of connections received by post synaptic
 *  neurons
 * =====================================================================================
 */
std::unordered_map<unsigned int, unsigned int>
CountIncomingConnections (std::vector<unsigned int> post_neurons_list)
{
    std::unordered_map<unsigned int, unsigned int>neuron_count_map;
    for(std::vector<unsigned int>::iterator it = post_neurons_list.begin(); it != post_neurons_list.end(); it++)
    {
        std::unordered_map<unsigned int, unsigned int>::iterator it1(neuron_count_map.find(*it));
        if(it1 != neuron_count_map.end())
        {
            it1->second++;
        }
        else
        {
            neuron_count_map[*it] = 1;
        }
    }
    return neuron_count_map;
}		/* -----  end of function GetConnectionCounts  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  GetIncidentConnectionNumbers
 *  Description:  
 * =====================================================================================
 */
int
GetIncidentConnectionNumbers(std::unordered_map<unsigned int, unsigned int> &con_ee, std::unordered_map<unsigned int, unsigned int> &con_ie)
{
    unsigned int neuronID;
    std::ifstream ifs;
    std::vector<unsigned int>post_neurons_e;
    std::vector<unsigned int>post_neurons_i;
    std::string meh;

    ifs.open("00-Con_ee.txt", std::ifstream::in);
    if (ifs.is_open())
    {
        /*  get rid of totals - don't need them */
        ifs >> neuronID;
        ifs >> neuronID;
        ifs >> neuronID;
        while(ifs >> neuronID)
        {
            /*  second column */
            ifs >> neuronID;
            post_neurons_e.emplace_back(neuronID);
            /*  rest of the line - the synaptic weight */
            std::getline(ifs, meh);
        }
        ifs.close();
    }
    else 
    {
        std::cout << "ERROR: couldn't open EE connection file. Aborting" << std::endl;
        return -10;
    }

    con_ee = CountIncomingConnections(post_neurons_e);


    ifs.open("00-Con_ie.txt", std::ifstream::in);
    if (ifs.is_open())
    {
        /*  get rid of totals - don't need them */
        ifs >> neuronID;
        ifs >> neuronID;
        ifs >> neuronID;
        while(ifs >> neuronID)
        {
            /*  second column */
            ifs >> neuronID;
            post_neurons_i.emplace_back(neuronID);
            /*  rest of the line - the synaptic weight */
            std::getline(ifs, meh);
        }
        ifs.close();
    }
    else 
    {
        std::cout << "ERROR: couldn't open IE connection file. Aborting" << std::endl;
        return -10;
    }

    con_ie = CountIncomingConnections(post_neurons_i);
    std::cout << "Number of post neurons for I neurons is: " << post_neurons_i.size() << " and uniq is: " << con_ie.size() << std::endl;
    std::cout << "Number of post neurons for E neurons is: " << post_neurons_e.size() << " and uniq is: " << con_ee.size() << std::endl;
    return 0;
}


#endif   /* ----- #ifndef utils_INC  ----- */
