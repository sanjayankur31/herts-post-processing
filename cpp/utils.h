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
 *         Name:  MasterFunction
 *  Description:
 * =====================================================================================
 */
    void
MasterFunction (std::vector<boost::iostreams::mapped_file_source> &spikes_E, std::vector<boost::iostreams::mapped_file_source> &spikes_I, std::vector<std::vector<unsigned int> >&patterns, std::vector<std::vector <unsigned int> >&recalls, double chunk_time)
{
    return;
}		/* -----  end of function MasterFunction  ----- */
#endif   /* ----- #ifndef utils_INC  ----- */
