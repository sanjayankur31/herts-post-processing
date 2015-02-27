/*
 * =====================================================================================
 *
 *       Filename:  postprocess.cpp
 *
 *    Description:  my postprocessing c++ file
 *
 *        Version:  1.0
 *        Created:  25/02/15 17:06:53
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ankur Sinha (FranciscoD), ankursinha@fedoraproject.org
 *   Organization:  
 *
 * =====================================================================================
 */

#include "utils.h"

/*  Holds my parameters both from the configuration file and the command line
 *  Since this is global, I'm not passing parameters as an argument to any
 *  function.
 *
 *  Extern since it's defined in the header file.
 */
extern struct param parameters;

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:  Main method with post processing logic
 * =====================================================================================
 */
    int
main ( int ac, char *av[] )
{
    clock_t global_clock_start, global_clock_end;
    std::vector<std::vector<unsigned int> > patterns;
    std::vector<std::vector<unsigned int> > recalls;
    std::vector<double> graphing_times;
    unsigned int threads_max = 0;
    struct what_to_plot plot_this;
    unsigned int task_counter = 0;
    std::vector<std::thread> threadlist;
    std::thread master_graph_thread;

    global_clock_start = clock();


    /*-----------------------------------------------------------------------------
     *  COMMAND LINE AND CONFIGURATION FILE PROCESSING
     *-----------------------------------------------------------------------------*/
    try 
    {
        po::options_description cli("Command line options");
        cli.add_options()
            ("help,h", "produce help message")
            ("out,o", po::value<std::string>(&(parameters.output_file)), "output filename")
            ("config,c", po::value<std::string>(&(parameters.config_file))->default_value("simulation_config.cfg"),"configuration file to be used to find parameter values for this simulation run")
            ("all,a","Plot all plots - this may take a while?")
            ("master,m","Plot master time plot - this may take a while?")
            ("pattern,p","Plot pattern plots?")
            ("snr,s","Plot SNR plots?")
            ;


        po::options_description params("Parameters");
        params.add_options()
            ("patternfile_prefix", po::value<std::string>(&(parameters.patternfile_prefix)), "Pattern files prefix")
            ("recallfile_prefix", po::value<std::string>(&(parameters.recallfile_prefix)), "Recall files prefix")
            ("num_pats", po::value<unsigned int>(&(parameters.num_pats)), "number of pattern file(s) to pick from pattern set")
            ("graph_pattern_cols", po::value<unsigned int>(&(parameters.graph_pattern_cols)), "Columns in the per pattern matrices - multiple of 10 please")
            ("NE", po::value<unsigned int>(&(parameters.NE)), "Number of excitatory neurons")
            ("graph_widthE", po::value<unsigned int>(&(parameters.graph_widthE)), "Excitatory columns in the final matrix")
            ("NI", po::value<unsigned int>(&(parameters.NI)), "Number of inhibitory neurons")
            ("graph_widthI", po::value<unsigned int>(&(parameters.graph_widthI)), "Inhibitory columns in the final matrix")
            ("stage_times", po::value<std::vector<unsigned int> >(&(parameters.stage_times))-> multitoken(), "comma separated list of times for each stage in order")
            ("plot_times", po::value<std::vector<double> >(&(parameters.plot_times))-> multitoken(), "comma separated list of additional plot times")
            ("mpi_ranks", po::value<unsigned int>(&(parameters.mpi_ranks)), "Number of MPI ranks being used")
            ;

        po::options_description visible("Allowed options");
        visible.add(cli).add(params);

        po::variables_map vm;
        po::store(po::basic_command_line_parser<char>(ac, av).options(cli).allow_unregistered().run(), vm);
        po::notify(vm);

        if (vm.count("help")) 
        {
            std::cout << visible << "\n";
            return 0;
        }
        if (vm.count("all")) 
        {
            plot_this.master = true;
            plot_this.pattern_graphs = true;
            plot_this.snr_graphs = true;
        }
        else 
        {
            if (vm.count("master"))
            {
                plot_this.master = true;
            }
            if (vm.count("pattern"))
            {
                plot_this.pattern_graphs = true;
            }
            if (vm.count("snr"))
            {
                plot_this.snr_graphs = true;
            }
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
    catch(std::exception &e) 
    {
        std::cerr << "error: " << e.what() << "\n";
        return 1;
    }
    catch(...) 
    {
        std::cerr << "Exception of unknown type!\n";
    }


    /*-----------------------------------------------------------------------------
     *  MAIN LOGIC BEGINS HERE
     *-----------------------------------------------------------------------------*/
    /*  At the most, use 20 threads, other wise WAIT */
    threads_max = (parameters.mpi_ranks <= 20) ? parameters.mpi_ranks : 20;
    std::cout << "Maximum number of threads in use: " << threads_max << "\n";

    /* Let this run independently. It takes quite a while */
    if (plot_this.master)
    {
        master_graph_thread = std::thread (PlotMasterGraph);
    }

    /*  Get plot times */
    graphing_times = ReadTimeToPlotListFromFile();
    if(graphing_times.size() == 0)
    {
        std::cerr << "Graphing times were not generated. Exiting program." << "\n";
        return -1;
    }

    /*  Load patterns and recalls */
    LoadPatternsAndRecalls(std::ref(patterns), std::ref(recalls));
    global_clock_end = clock();
    std::cout << "Total time taken: " << (global_clock_end - global_clock_start)/CLOCKS_PER_SEC << "\n";
    return 0;
}				/* ----------  end of function main  ---------- */
