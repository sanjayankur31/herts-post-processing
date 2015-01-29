/*
 * =====================================================================================
 *
 *       Filename:  postprocess.cpp
 *
 *    Description:  main postprocessing file
 *
 *
 *        Version:  1.0
 *        Created:  28/01/15 17:54:36
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ankur Sinha, a.sinha2@herts.ac.uk
 *   Organization:  University of Hertfordshire, UK
 *
 * =====================================================================================
 */

#include <iomanip>
#include <exception>
#include <cstdlib>
#include <string>
#include <iterator>
#include <thread>
#include <boost/program_options.hpp>
#include <iostream>
#include <fstream>
#include <ctime>

namespace po = boost::program_options;

struct param
{
    std::string config_file;
    /*  Number of excitatory neurons */
    unsigned int NE;
    /*  Number of inhibitory neurons */
    unsigned int NI;

    /*  Times of each stage initialised to 0 */
    std::vector<unsigned int> stage_times;
    /*  number of patterns to be used in this simulation */
    unsigned int  num_pats;

    /*  output prefix */
    std::string output_file;

    std::string patternfile_prefix;
    std::string recallfile_prefix;

    std::string excitatory_raster_file;
    std::string inhibitory_raster_file;

} parameters;


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  calculateTimeToPlotList
 *  Description:  
 * =====================================================================================
 */
std::vector<double>
calculateTimeToPlotList (int num_pat, std::vector <unsigned int> stage_times)
{
    /*  Intervals after the recall stimulus that I want to graph at */
    float graphing_intervals[4] = {0.1,0.2,0.5,1};
    std::vector<double> graphing_times;
    double times = 0;
    times = stage_times[0] + stage_times[1];

    for (int i = 0; i < num_pat; i += 1 ) {
        times += (stage_times[2] + stage_times[3]);
        
        for (int j = 0; j < (int)(sizeof(graphing_intervals)/sizeof(float)); j += 1 ) {
            graphing_times.push_back(times+graphing_intervals[j]);
        }
        times += (stage_times[4]);
    }
    return graphing_times;
}		/* -----  end of function calculateTimeToPlotList  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  extractSegments
 *  Description:  Extract the required segment from the main map
 * =====================================================================================
 */
    void
extractSegments (  )
{
    return;
}		/* -----  end of function extractSegments  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:  
 * =====================================================================================
 */
    int
main(int ac, char* av[])
{
    int num_threads = 12;
    int errcode = 0;
    std::vector<double> graphing_times;
    clock_t clock_start, clock_end;
    std::multimap<double, unsigned int> raster_data_E;
    std::multimap<double, unsigned int> raster_data_I;


    try {

        po::options_description cli("Command line options");
        cli.add_options()
            ("help,h", "produce help message")
            ("out", po::value<std::string>(&(parameters.output_file)), "output filename")
            ("config,c", po::value<std::string>(&(parameters.config_file))->default_value("simulation_config.cfg"),"configuration file to be used to find parameter values for this simulation run")
            ("excitatory,e", po::value<std::string>(&(parameters.excitatory_raster_file)),"Excitatory raster file path")
            ("inhibitory,i", po::value<std::string>(&(parameters.inhibitory_raster_file)),"Inhibitory raster file path")
            ;


        po::options_description params("Parameters");
        params.add_options()
            ("patternfile_prefix", po::value<std::string>(&(parameters.patternfile_prefix)), "Pattern files prefix")
            ("recallfile_prefix", po::value<std::string>(&(parameters.recallfile_prefix)), "Recall files prefix")
            ("num_pats", po::value<unsigned int>(&(parameters.num_pats)), "number of pattern file(s) to pick from pattern set")
            ("NE", po::value<unsigned int>(&(parameters.NE)), "Number of excitatory neurons")
            ("NI", po::value<unsigned int>(&(parameters.NI)), "Number of inhibitory neurons")
            ("stage_times", po::value<std::vector<unsigned int> >(&(parameters.stage_times))-> multitoken(), "comma separated list of times for each stage in order")
            ;

        po::options_description visible("Allowed options");
        visible.add(cli).add(params);

        po::variables_map vm;
        po::store(po::basic_command_line_parser<char>(ac, av).options(cli).allow_unregistered().run(), vm);
        po::notify(vm);

        if (vm.count("help")) {
            std::cout << visible << "\n";
            return 0;
        }

        std::ifstream ifs(parameters.config_file.c_str());
        if (!ifs)
        {
            std::cout << "Cannot open configuration file - " << parameters.config_file << "\n";
            return -1;
        }
        else
        {
            po::store(po::parse_config_file(ifs, params, true), vm);
            po::notify(vm);
        }
    }
    catch(std::exception &e) {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    catch(...) {
        std::cerr << "Exception of unknown type!\n";
    }

    graphing_times = calculateTimeToPlotList(parameters.num_pats, parameters.stage_times);

    for (std::vector<double>::const_iterator i = graphing_times.begin(); i != graphing_times.end(); ++i)
        std::cout << *i << ' ';
    std::cout << "\n";

    if (parameters.excitatory_raster_file != "" && parameters.inhibitory_raster_file != "")
    {
        std::ifstream ifs_excitatory(parameters.excitatory_raster_file);
        std::ifstream ifs_inhibitory(parameters.inhibitory_raster_file);
        double timeE; 
        unsigned int neuronID;
        clock_start = clock();
        while( ifs_excitatory >> timeE >> neuronID)
            raster_data_E.insert(std::pair<double, unsigned int>(timeE, neuronID));
        clock_end = clock();
        std::cout << "Time taken to read in the file: " << (clock_end - clock_start)/CLOCKS_PER_SEC << "\n";
        
        clock_start = clock();
        for(std::multimap<double, unsigned int>::iterator it = raster_data_E.lower_bound(graphing_times[0] -1.); it != raster_data_E.upper_bound(graphing_times[0]); ++it)
            std::cout << it->first << "\t" << it->second << "\n";

        clock_end = clock();
        std::cout << "Time taken to find and list one section: " << (clock_end - clock_start)/CLOCKS_PER_SEC << "\n";
    }



}				/* ----------  end of function main  ---------- */