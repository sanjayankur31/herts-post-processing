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
 *         Name:  tardis
 *  Description:  
 * =====================================================================================
 */
    void
tardis (std::multimap<double, unsigned int> &dataE, std::multimap<double, unsigned int> &dataI, std::vector<std::vector<unsigned int> >& patterns, std::vector<std::vector <unsigned int> >& recalls, double timeToFly, struct param parameters)
{
    std::vector <unsigned int>neuronsE;
    std::vector <unsigned int>neuronsI;
    std::vector <std::vector<unsigned int> >pattern_neurons_rate;
    std::vector <std::vector<unsigned int> >noise_neurons_rate;
    std::vector <std::vector<unsigned int> >recall_neurons_rate;
    std::vector <unsigned int>neuronsE_rate;
    std::vector <unsigned int>neuronsI_rate;

    converter.str("");
    converter.clear();
    converter << parameters.patternfile_prefix <<  std::setw(8)  << std::setfill('0') << std::to_string(i) << ".pat";
    /*  Throw into parameter file! */
    int rowWidthE = 80;
    int rowWidthI = 20;

    std::cout << "[TARDIS] Thread " << std::this_thread::get_id() << " working on it, Sir!" << "\n";
    for(std::multimap<double, unsigned int>::iterator it = dataI.lower_bound(timeToFly -1.); it != dataI.upper_bound(timeToFly); ++it)
        neuronsI.emplace_back(it->second);

    for(std::vector<unsigned int>::const_iterator it = neuronsI.cbegin(); it != neuronsI.cend(); ++it)
    {

    }




    for(std::multimap<double, unsigned int>::iterator it = dataE.lower_bound(timeToFly -1.); it != dataE.upper_bound(timeToFly); ++it)
        neuronsE.emplace_back(it->second);
    std::sort(neuronsE.begin(), neuronsE.end());

    std::vector<unsigned int>::iterator search_begin = neuronsE.begin();
    for(unsigned int i = 1; i <= parameters.NE; ++i)
    {
        int rate = 0;
        rate = (std::upper_bound(search_begin, neuronsE.end(), i) != neuronsE.end()) ?  (std::upper_bound(search_begin, neuronsE.end(), i) - search_begin) : 0;
        search_begin = std::upper_bound(search_begin, neuronsE.end(), i);

        for (unsigned int j = 0; j < parameters.num_pats; j++)
        {
            if(pattern_neurons_rate[j].size() != patterns[j].size() && std::binary_search(patterns[j].begin(), patterns[j].end(), i))
            {
                pattern_neurons_rate[j].emplace_back(rate);
            }
            else
            {
                noise_neurons_rate[j].emplace_back(rate);
            }
            if(recall_neurons_rate[j].size() != recalls[j].size() && std::binary_search(recalls[j].begin(), recalls[j].end(), i))
            {
                recall_neurons_rate[j].emplace_back(rate);
            }
        }
        neuronsE_rate.emplace_back(rate);
    }

    for(unsigned int i = 0; i < parameters.NE; ++i)
    {
        std::ofstream neuronsE_stream;
        std::ofstream neuronsI_stream;
        std::ofstream pattern_neurons_stream;
        std::ofstream recall_neurons_stream;

        for (unsigned int j = 0; j < parameters.num_pats; j++)


    }




    std::cout << "[TARDIS] Thread " << std::this_thread::get_id() << " reporting on conclusion, Sir!" << "\n";
}		/* -----  end of function tardis  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  populateMaps
 *  Description:  Funtion that populates my maps - so that I can multithread it
 * =====================================================================================
 */
    int 
populateMaps(std::multimap<double, unsigned int> &map_to_populate, std::string filename)
{
    std::ifstream ifs(filename);
    double timetemp; 
    unsigned int neuronID;

    clock_t clock_start = clock();
    while( ifs >> timetemp >> neuronID)
        map_to_populate.insert(std::pair<double, unsigned int>(timetemp, neuronID));
    clock_t clock_end = clock();
    std::cout << "File " << filename << " read to map in " <<  (clock_end - clock_start)/CLOCKS_PER_SEC << " seconds.\n";
    return 0;

}



/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  getPatternsAndRecalls
 *  Description:  
 * =====================================================================================
 */
    void
getPatternsAndRecalls ( std::vector<std::vector<unsigned int> > &patterns, std::vector<std::vector<unsigned int> >&recalls, struct param parameters)
{
    clock_t clock_start = clock();
    std::ostringstream converter;
    unsigned int neuronID;
    std::ifstream ifs;
    for (unsigned int i = 1; i <= parameters.num_pats; ++i)
    {
        converter.str("");
        converter.clear();
        converter << parameters.patternfile_prefix <<  std::setw(8)  << std::setfill('0') << std::to_string(i) << ".pat";
        std::string pattern = converter.str();
        ifs.open(pattern);
        while(ifs >> neuronID)
            patterns[i].emplace_back(neuronID);
        ifs.close();
        std::sort(patterns[i].begin(), patterns[i].end());
        std::cout << "Items loaded and sorted in pattern number " << i << " is " << patterns[i].size() << "\n";

        converter.str("");
        converter.clear();
        converter << parameters.recallfile_prefix <<  std::setw(8)  << std::setfill('0') << std::to_string(i) << ".pat";
        std::string recall = converter.str();
        ifs.open(recall);
        while(ifs >> neuronID)
            recalls[i].emplace_back(neuronID);
        ifs.close();
        std::sort(recalls[i].begin(), recalls[i].end());
        std::cout << "Items loaded and sorted in recall number " << i << " is " << recalls[i].size() << "\n";
    }
    clock_t clock_end = clock();
    std::cout << "Patterns and recalls read in " << (clock_end - clock_start)/CLOCKS_PER_SEC << " seconds.\n";

}		/* -----  end of function getPatternsAndRecalls  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:  
 * =====================================================================================
 */
    int
main(int ac, char* av[])
{
    std::vector<double> graphing_times;
    clock_t clock_start, clock_end;
    std::multimap<double, unsigned int> raster_data_E;
    std::multimap<double, unsigned int> raster_data_I;
    std::vector<std::thread> timeLords;
    int doctors_max = 12;
    std::vector<std::vector<unsigned int> > patterns;
    std::vector<std::vector<unsigned int> > recalls;

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

    if (parameters.excitatory_raster_file != "")
    {
        timeLords.emplace_back(std::thread (populateMaps, std::ref(raster_data_E), parameters.excitatory_raster_file));
    } 
    if (parameters.inhibitory_raster_file != "")
    {
        timeLords.emplace_back(std::thread (populateMaps, std::ref(raster_data_I), parameters.inhibitory_raster_file));
    }

    /*  Use the main thread! */
    getPatternsAndRecalls(std::ref(patterns), std::ref(recalls), parameters);

    for (std::thread &t: timeLords)
    {
        if(t.joinable())
        {
            t.join();
        }
    }
    timeLords.clear();
    /*  I need to wait for files to be read before I do anything else */

    clock_start = clock();
    int task_counter = 0;
    for(std::vector<double>::const_iterator i = graphing_times.begin(); i != graphing_times.end(); ++i)
    {
        /*  Only start a new thread if less than thread_max threads are running */
        if (task_counter < doctors_max)
        {
            timeLords.emplace_back(std::thread (tardis, std::ref(raster_data_E), std::ref(raster_data_I), std::ref(patterns), std::ref(recalls), *i, parameters));
            task_counter++;
        }
        /*  If thread_max threads are running, wait for them to finish before
         *  starting a second round.
         *
         *  Yes, this can be optimised by using a thread pool but I really
         *  don't have the patience to look into ThreadPool or a
         *  boost::thread_group today! 
         */
        else
        {
            for (std::thread &t: timeLords)
            {
                if(t.joinable())
                {
                    t.join();
                    task_counter--;
                }
            }
            timeLords.clear();
        }
    }

    /*  Wait for remaining threads to finish */
    for (std::thread &t: timeLords)
    {
        if(t.joinable())
        {
            t.join();
        }
    }
    timeLords.clear();
    clock_end = clock();
    std::cout << "Time taken for the processing part is: " << (clock_end - clock_start)/CLOCKS_PER_SEC << "\n";
    return 0;
}				/* ----------  end of function main  ---------- */
