/*
 * =====================================================================================
 *
 *       Filename:  postprocess_single.cpp
 *
 *    Description:  my postprocessing c++ file for single simulations
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
extern struct what_to_plot plot_this;


/*
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:  Main method with post processing logic
 * =====================================================================================
 */
    int
main ( int ac, char *av[] )
{
    boost::mpi::environment env;
    boost::mpi::communicator world;

    clock_t global_clock_start, global_clock_end;
    clock_t clock_start, clock_end;
    std::vector<std::vector<unsigned int> > patterns;
    std::vector<std::vector<unsigned int> > recalls;
    std::vector<AurynTime> graphing_times;
    std::multimap <double, struct SNR_data> snr_data;
    std::vector<boost::iostreams::mapped_file_source> spikes_E;
    std::vector<boost::iostreams::mapped_file_source> spikes_I;
    std::ostringstream converter;
    std::unordered_map <unsigned int, unsigned int> con_ee_count, con_ie_count;

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
            ;


        po::options_description params("Parameters");
        params.add_options()
            ("patternfile_prefix", po::value<std::string>(&(parameters.patternfile_prefix)), "Pattern files prefix")
            ("recallfile_prefix", po::value<std::string>(&(parameters.recallfile_prefix)), "Recall files prefix")
            ("plot_times_file", po::value<std::string>(&(parameters.plot_times_file)), "Plot times file")
            ("num_pats", po::value<unsigned int>(&(parameters.num_pats)), "number of pattern file(s) to pick from pattern set")
            ("graph_pattern_cols", po::value<unsigned int>(&(parameters.graph_pattern_cols)), "Columns in the per pattern matrices - multiple of 10 please")
            ("NE", po::value<unsigned int>(&(parameters.NE)), "Number of excitatory neurons")
            ("graph_widthE", po::value<unsigned int>(&(parameters.graph_widthE)), "Excitatory columns in the final matrix")
            ("NI", po::value<unsigned int>(&(parameters.NI)), "Number of inhibitory neurons")
            ("graph_widthI", po::value<unsigned int>(&(parameters.graph_widthI)), "Inhibitory columns in the final matrix")
            ("stage_times", po::value<std::vector<unsigned int> >(&(parameters.stage_times))-> multitoken(), "comma separated list of times for each stage in order")
            ("plot_times", po::value<std::vector<double> >(&(parameters.plot_times))-> multitoken(), "comma separated list of additional plot times")
            ("mpi_ranks", po::value<unsigned int>(&(parameters.mpi_ranks)), "Number of MPI ranks that were used to run the simulation")
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

    /*  Main worker loop */
    unsigned int total_snr_lines_to_be_processed = (parameters.num_pats * (parameters.num_pats + 1)/2);
    
    /*  diagnostics printed by rank 0 only */
    if (world.rank() == 0)
        std::cout << "Ranks available and therefore in use: " << world.size() << "\n";

    /*-----------------------------------------------------------------------------
     *  BEGIN SETUP - each rank will run this code
     *
     *  This can be optimised. For example, if there are more ranks than
     *  entries in the time file, I can easily limit the number of ranks
     *  doing this. The point is, is it worth it? More often than not, I'll
     *  have more entries than ranks and each rank will in fact need all
     *  this general data.
     *-----------------------------------------------------------------------------*/

    /*  Get plot times */
    graphing_times = ReadTimeToPlotListFromFile();
    if(graphing_times.size() == 0)
    {
        std::cerr << "Graphing times were not generated. Exiting program." << "\n";
        return -1;
    }
    /*  Print them out to check */
    std::cout << "Graphing times are: ";
    for (std::vector<AurynTime>::iterator it = graphing_times.begin(); it != graphing_times.end(); it++)
        std::cout << *it << "\t";
    std::cout << "\n";

    /*  Load patterns and recalls */
    LoadPatternsAndRecalls(std::ref(patterns), std::ref(recalls));

    /* Get the connection list */
    GetIncidentConnectionNumbers(std::ref(con_ee_count), std::ref(con_ie_count));

    /*  Load memory mapped files */
    clock_start = clock();
    for (unsigned int i = 0; i < parameters.mpi_ranks; ++i)
    {
        converter.str("");
        converter.clear();
        converter << parameters.output_file << "." << i << "_e.bras";
        spikes_E.emplace_back(boost::iostreams::mapped_file_source());
        spikes_E[i].open(converter.str());

        converter.str("");
        converter.clear();
        converter << parameters.output_file << "." << i << "_i.bras";
        spikes_I.emplace_back(boost::iostreams::mapped_file_source());
        spikes_I[i].open(converter.str());
    }
    clock_end = clock();
    std::cout << spikes_E.size() <<  " E and " << spikes_I.size() << " I files mapped in " << (clock_end - clock_start)/CLOCKS_PER_SEC << " seconds.\n";

    /*-----------------------------------------------------------------------------
     *  END SETUP
     *-----------------------------------------------------------------------------*/

    for (unsigned int i = 0; i <= total_snr_lines_to_be_processed/world.size(); i++)
    {
        /*  The time I want each rank to graph - the index from the list in
         *  fact */
        unsigned int graphing_time = i * world.size() + world.rank();
        unsigned int counter = 0;

        /*  otherwise we don't want ranks kicking off and running into
         *  segfaults */
        if(graphing_time < total_snr_lines_to_be_processed)
        {
            /*  Calculate the pattern that is recalled at this time. I
             *  wonder if there is an easier way */
            for(unsigned int k = 0; k < parameters.num_pats; k++)
            {
                for (unsigned int j = 0; j <=k; j++)
                {
                    if (counter == graphing_time)
                    {
                        std::cout << "Rank " << world.rank() << " is running on line " << i + world.rank() << std::endl;
                        MasterFunction(std::ref(spikes_E), std::ref(spikes_I), std::ref(patterns), std::ref(recalls), graphing_times[graphing_time] + (1.0/dt),  j, con_ee_count, con_ie_count, world);
                    }
                    counter++;
                }
            }
        }
    }
    /*  Wait for all ranks to finish  */
    world.barrier();

    global_clock_end = clock();
    std::cout << "Total time taken: " << (global_clock_end - global_clock_start)/CLOCKS_PER_SEC << "\n";
    return 0;

}				/* ----------  end of function main  ---------- */
