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
extern struct what_to_plot plot_this;

/*  Mutex for my snr_data */
extern std::mutex snr_data_mutex;

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
            ("all,a","Plot all plots - this may take a while?")
            ("master,M","Plot master time plot - this may take a while?")
            ("pattern,p","Plot pattern plots?")
            ("snr,s","Plot SNR plots after processing ras files?")
            ("print-snr,S","Print SNR data after processing ras files?")
            ("generate-snr-plot-from-file,g","Generate metrics from a printed files 00-{SNR,Mean,STD}-data.txt.")
            ("generate-cum-vs-over-snr-plot-from-file,t","Generate cumulative _VS_ clipped snr from two printed files 00-SNR-data-{overwritten,cumulative}.txt.")
            ("generate-snr-vs-wpats-plot-from-file,w","Also generate snr _VS_ wpats plot along with snr-for-multiple-pats - picks wpats from arguments of W")
            ("generate-means-vs-wpats-plot-from-file,m","Also generate mean _VS_ wpats plot along with snr-for-multiple-pats - picks wpats from arguments of W")
            ("generate-std-vs-wpats-plot-from-file,d","Also generate std _VS_ wpats plot along with snr-for-multiple-pats - picks wpats from arguments of W")
            ("pats,W", po::value<std::vector<double> >(&(plot_this.wPats))-> multitoken(), "w_pat values that input files are available for")
            ("multiSNR,r","Multi SNR graph requires -W values to be given")
            ("multiMean,n","Multi Mean graph requires -W values to be given")
            ("multiSTD,D","Multi STD graph requires -W values to be given")
            ("png,P", "Generate graphs in png. Default is svg.")
            ("singleMeanAndSTD,k","Generate a graph with mean and std from one set of data files 00-{Mean,STD}-data.txt")
            ("multiMeanAndSTD,K","Generate a graph with multiple mean and std plots")
            ("for-prints,f","Generate graphs for use in papers - this changes the tics and things so they're more visible when embedded in papers")
            ("for-meetings,e","Generate graphs to show in meetings - this increases the font size and reduces ticks")
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
            if (vm.count("generate-snr-plot-from-file"))
            {
                plot_this.processRas = false;
                plot_this.Metrics_from_file = true;
            }
            if (vm.count("generate-cum-vs-over-snr-plot-from-file"))
            {
                plot_this.cum_VS_over = true;
                plot_this.processRas = false;
            }
            if (vm.count("generate-snr-vs-wpats-plot-from-file"))
            {
                plot_this.processRas = false;
                plot_this.snr_VS_wPats = true;
            }
            if (vm.count("generate-means-vs-wpats-plot-from-file"))
            {
                plot_this.processRas = false;
                plot_this.mean_VS_wPats = true;
            }
            if (vm.count("generate-std-vs-wpats-plot-from-file"))
            {
                plot_this.processRas = false;
                plot_this.std_VS_wPats = true;
            }
            if (vm.count("print-snr"))
            {
                plot_this.processRas = true;
                parameters.print_snr = true;
            }
            if (vm.count("png"))
            {
                plot_this.formatPNG = true;
            }
            if (vm.count("multiSNR"))
            {
                plot_this.processRas = false;
                plot_this.multiSNR = true;
            }
            if (vm.count("multiMean"))
            {
                plot_this.processRas = false;
                plot_this.multiMean = true;
            }
            if (vm.count("multiSTD"))
            {
                plot_this.processRas = false;
                plot_this.multiSTD = true;
            }
            if (vm.count("singleMeanAndSTD"))
            {
                plot_this.processRas = false;
                plot_this.singleMeanAndSTD = true;
                plot_this.multiMeanAndSTD = false;
            }
            if (vm.count("multiMeanAndSTD"))
            {
                plot_this.processRas = false;
                plot_this.singleMeanAndSTD = false;
                plot_this.multiMeanAndSTD = true;
            }
            if (vm.count("for-prints"))
            {
                plot_this.for_prints = true;
                plot_this.formatPNG = false;
            }
            if (vm.count("for-meetings"))
            {
                plot_this.for_meetings = true;
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


    if(plot_this.processRas)
    {
        /*  Main worker loop */
        unsigned int total_snr_lines_to_be_processed = (parameters.num_pats * (parameters.num_pats + 1)/2);
        
        /*  diagnostics printed by rank 0 only */
        if (world.rank() == 0)
            std::cout << "Ranks available and therefore in use: " << parameters.mpi_ranks << "\n";

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
    }
    return 0;


#if  0     /* ----- #if 0 : If0Label_1 ----- */
        /*  Wait for all your threads to finish first! */
        if(plot_this.snr_graphs)
        {
            std::cout << "Size of snr vector is: " << snr_data.size() << "\n";
            PlotSNRGraphs(snr_data);
        }
        if (parameters.print_snr)
        {
            PrintSNRDataToFile(snr_data);
        }

    }

    if(plot_this.Metrics_from_file)
    {
        /*  No post processing, we have a file, we'll plot from it */
        GenerateSNRPlotFromFile("00-SNR-data.txt");
        GenerateMeanPlotFromFile("00-Mean-data.txt");
        GenerateSTDPlotFromFile("00-STD-data.txt");
        GenerateMeanNoisePlotFromFile("00-Mean-noise-data.txt");
        GenerateSTDNoisePlotFromFile("00-STD-noise-data.txt");
    }
    if(plot_this.cum_VS_over)
    {
        std::vector<std::pair<std::string, std::string> > inputs;
        inputs.emplace_back(std::pair<std::string, std::string>("00-SNR-data-cumulative.txt", "cumulative"));
        inputs.emplace_back(std::pair<std::string, std::string>("00-SNR-data-overwritten.txt", "clipped"));
        GenerateMultiSNRPlotFromFile(inputs);
    }
    if(plot_this.singleMeanAndSTD)
    {
        std::vector<boost::tuple<std::string, std::string, std::string> > inputs;
        inputs.emplace_back(boost::tuple<std::string, std::string, std::string> ("00-Mean-data.txt", "00-STD-data.txt", " "));
        GenerateMultiMeanWithSTDFromFiles(inputs, "MeanWithSTD");

        inputs.clear();
        inputs.emplace_back(boost::tuple<std::string, std::string, std::string> ("00-Mean-noise-data.txt", "00-STD-noise-data.txt", " "));
        GenerateMultiMeanWithSTDFromFiles(inputs, "MeanWithSTD-noise");

        inputs.clear();
        inputs.emplace_back(boost::tuple<std::string, std::string, std::string> ("00-Mean-data.txt", "00-STD-data.txt", "p"));
        inputs.emplace_back(boost::tuple<std::string, std::string, std::string> ("00-Mean-noise-data.txt", "00-STD-noise-data.txt", "b"));
        GenerateMultiMeanWithSTDFromFiles(inputs, "MeanWithSTD-both");

    }
    if(plot_this.wPats.size() != 0 && plot_this.multiMeanAndSTD)
    {
        std::vector<boost::tuple<std::string, std::string, std::string> > inputs;
        std::ostringstream converter;
        std::ostringstream converter1;
        std::ostringstream converter2;
        for (std::vector<double>::iterator it = plot_this.wPats.begin(); it != plot_this.wPats.end(); it++)
        {
            converter.clear();
            converter.str("");
            converter2.clear();
            converter2.str("");
            converter1.clear();
            converter1.str("");
            converter << "00-Mean-data-k-w-" << *it << ".txt";
            converter1 << "00-STD-data-k-w-" << *it << ".txt";
            std::cout << converter.str() << ", " << converter1.str();
            if(!plot_this.formatPNG)
            {
                converter2 << "w\\_pat = " << (*it)* 3 << "nS" ;
            }
            else
                converter2 << "w_pat = " << (*it)* 3 << "nS" ;
            inputs.emplace_back(boost::tuple<std::string, std::string, std::string>(converter.str(), converter1.str(), converter2.str()));
        }
        GenerateMultiMeanWithSTDFromFiles(inputs, "MultiMeanWithSTD");
        inputs.clear();

        for (std::vector<double>::iterator it = plot_this.wPats.begin(); it != plot_this.wPats.end(); it++)
        {
            converter.clear();
            converter.str("");
            converter2.clear();
            converter2.str("");
            converter1.clear();
            converter1.str("");
            converter << "00-Mean-noise-data-k-w-" << *it << ".txt";
            converter1 << "00-STD-noise-data-k-w-" << *it << ".txt";
            std::cout << converter.str() << ", " << converter1.str();
            if(!plot_this.formatPNG)
            {
                converter2 << "w\\_pat = " << (*it)* 3 << "nS" ;
            }
            else
                converter2 << "w_pat = " << (*it)* 3 << "nS" ;
            inputs.emplace_back(boost::tuple<std::string, std::string, std::string>(converter.str(), converter1.str(), converter2.str()));
        }
        GenerateMultiMeanWithSTDFromFiles(inputs, "MultiMeanWithSTD-noise");
        inputs.clear();

        for (std::vector<double>::iterator it = plot_this.wPats.begin(); it != plot_this.wPats.end(); it++)
        {
            converter.clear();
            converter.str("");
            converter2.clear();
            converter2.str("");
            converter1.clear();
            converter1.str("");
            converter << "00-Mean-data-k-w-" << *it << ".txt";
            converter1 << "00-STD-data-k-w-" << *it << ".txt";
            std::cout << converter.str() << ", " << converter1.str();
            if(!plot_this.formatPNG)
            {
                converter2 << "p - w\\_pat = " << (*it)* 3 << "nS" ;
            }
            else
                converter2 << "p - w_pat = " << (*it)* 3 << "nS" ;
            inputs.emplace_back(boost::tuple<std::string, std::string, std::string>(converter.str(), converter1.str(), converter2.str()));
        }
        for (std::vector<double>::iterator it = plot_this.wPats.begin(); it != plot_this.wPats.end(); it++)
        {
            converter.clear();
            converter.str("");
            converter2.clear();
            converter2.str("");
            converter1.clear();
            converter1.str("");
            converter << "00-Mean-noise-data-k-w-" << *it << ".txt";
            converter1 << "00-STD-noise-data-k-w-" << *it << ".txt";
            std::cout << converter.str() << ", " << converter1.str();
            if(!plot_this.formatPNG)
            {
                converter2 << "b - w\\_pat = " << (*it)* 3 << "nS" ;
            }
            else
                converter2 << "b - w_pat = " << (*it)* 3 << "nS" ;
            inputs.emplace_back(boost::tuple<std::string, std::string, std::string>(converter.str(), converter1.str(), converter2.str()));
        }
        GenerateMultiMeanWithSTDFromFiles(inputs, "MultiMeanWithSTD-both");

    }
    if(plot_this.wPats.size() != 0 && plot_this.multiMean)
    {
        std::vector<std::pair<std::string, std::string> > inputs;
        std::ostringstream converter;
        std::ostringstream converter1;
        for (std::vector<double>::iterator it = plot_this.wPats.begin(); it != plot_this.wPats.end(); it++)
        {
            converter.clear();
            converter.str("");
            converter1.clear();
            converter1.str("");
            converter << "00-Mean-data-k-w-" << *it << ".txt";
            std::cout << converter.str();
            if(!plot_this.formatPNG)
            {
                converter1 << "w\\_pat = " << (*it)* 3 << "nS" ;
            }
            else
                converter1 << "w_pat = " << (*it)* 3 << "nS" ;
            inputs.emplace_back(std::pair<std::string, std::string>(converter.str(), converter1.str()));
        }
        GenerateMultiMeanPlotFromFile(inputs);
        inputs.clear();

        for (std::vector<double>::iterator it = plot_this.wPats.begin(); it != plot_this.wPats.end(); it++)
        {
            converter.clear();
            converter.str("");
            converter1.clear();
            converter1.str("");
            converter << "00-Mean-noise-data-k-w-" << *it << ".txt";
            std::cout << converter.str();
            if(!plot_this.formatPNG)
            {
                converter1 << "w\\_pat = " << (*it)* 3 << "nS" ;
            }
            else
                converter1 << "w_pat = " << (*it)* 3 << "nS" ;
            inputs.emplace_back(std::pair<std::string, std::string>(converter.str(), converter1.str()));
        }
        GenerateMultiMeanNoisePlotFromFile(inputs);
    }
    if(plot_this.wPats.size() != 0 && plot_this.multiSTD)
    {
        std::vector<std::pair<std::string, std::string> > inputs;
        std::ostringstream converter;
        std::ostringstream converter1;
        for (std::vector<double>::iterator it = plot_this.wPats.begin(); it != plot_this.wPats.end(); it++)
        {
            converter.clear();
            converter.str("");
            converter1.clear();
            converter1.str("");
            converter << "00-STD-data-k-w-" << *it << ".txt";
            std::cout << converter.str();
            if(!plot_this.formatPNG)
            {
                converter1 << "w\\_pat = " << (*it)* 3  << "nS";
            }
            else
                converter1 << "w_pat = " << (*it)* 3 << "nS" ;
            inputs.emplace_back(std::pair<std::string, std::string>(converter.str(), converter1.str()));
        }
        GenerateMultiSTDPlotFromFile(inputs);

        inputs.clear();

        for (std::vector<double>::iterator it = plot_this.wPats.begin(); it != plot_this.wPats.end(); it++)
        {
            converter.clear();
            converter.str("");
            converter1.clear();
            converter1.str("");
            converter << "00-STD-noise-data-k-w-" << *it << ".txt";
            std::cout << converter.str();
            if(!plot_this.formatPNG)
            {
                converter1 << "w\\_pat = " << (*it)* 3  << "nS";
            }
            else
                converter1 << "w_pat = " << (*it)* 3 << "nS" ;
            inputs.emplace_back(std::pair<std::string, std::string>(converter.str(), converter1.str()));
        }
        GenerateMultiSTDNoisePlotFromFile(inputs);
    }
    if(plot_this.wPats.size() != 0 && plot_this.multiSNR)
    {
        std::vector<std::pair<std::string, std::string> > inputs;
        std::ostringstream converter;
        std::ostringstream converter1;
        for (std::vector<double>::iterator it = plot_this.wPats.begin(); it != plot_this.wPats.end(); it++)
        {
            converter.clear();
            converter.str("");
            converter1.clear();
            converter1.str("");
            converter << "00-SNR-data-k-w-" << *it << ".txt";
            std::cout << converter.str();
            if(!plot_this.formatPNG)
            {
                converter1 << "w\\_pat = " << (*it)* 3  << "nS";
            }
            else
                converter1 << "w_pat = " << (*it)* 3 << "nS" ;
            inputs.emplace_back(std::pair<std::string, std::string>(converter.str(), converter1.str()));
        }
        GenerateMultiSNRPlotFromFile(inputs);
    }
    if(plot_this.wPats.size() != 0 && plot_this.std_VS_wPats)
    {
        std::vector<std::pair<std::string, double> > inputs;
        std::ostringstream converter;
        for (std::vector<double>::iterator it = plot_this.wPats.begin(); it != plot_this.wPats.end(); it++)
        {
            converter.clear();
            converter.str("");
            converter << "00-STD-data-k-w-" << *it << ".txt";
            std::cout << converter.str();
            inputs.emplace_back(std::pair<std::string, double>(converter.str(), (*it)*0.3));
        }
        GenerateSTD_VS_WPatFromFile(inputs);

        inputs.clear();
        for (std::vector<double>::iterator it = plot_this.wPats.begin(); it != plot_this.wPats.end(); it++)
        {
            converter.clear();
            converter.str("");
            converter << "00-STD-noise-data-k-w-" << *it << ".txt";
            std::cout << converter.str();
            inputs.emplace_back(std::pair<std::string, double>(converter.str(), (*it)*0.3));
        }
        GenerateSTDNoise_VS_WPatFromFile(inputs);
    }
    if(plot_this.wPats.size() != 0 && plot_this.mean_VS_wPats)
    {
        std::vector<std::pair<std::string, double> > inputs;
        std::ostringstream converter;
        for (std::vector<double>::iterator it = plot_this.wPats.begin(); it != plot_this.wPats.end(); it++)
        {
            converter.clear();
            converter.str("");
            converter << "00-Mean-data-k-w-" << *it << ".txt";
            std::cout << converter.str();
            inputs.emplace_back(std::pair<std::string, double>(converter.str(), (*it)*0.3));
        }

        GenerateMean_VS_WPatFromFile(inputs);

        inputs.clear();
        for (std::vector<double>::iterator it = plot_this.wPats.begin(); it != plot_this.wPats.end(); it++)
        {
            converter.clear();
            converter.str("");
            converter << "00-Mean-noise-data-k-w-" << *it << ".txt";
            std::cout << converter.str();
            inputs.emplace_back(std::pair<std::string, double>(converter.str(), (*it)*0.3));
        }
        GenerateMeanNoise_VS_WPatFromFile(inputs);
    }
    if(plot_this.wPats.size() != 0 && plot_this.snr_VS_wPats)
    {
        std::vector<std::pair<std::string, double> > inputs;
        std::ostringstream converter;
        for (std::vector<double>::iterator it = plot_this.wPats.begin(); it != plot_this.wPats.end(); it++)
        {
            converter.clear();
            converter.str("");
            converter << "00-SNR-data-k-w-" << *it << ".txt";
            std::cout << converter.str();
            inputs.emplace_back(std::pair<std::string, double>(converter.str(), (*it)*0.3));
        }
        GenerateSNR_VS_WPatFromFile(inputs);
    }
    global_clock_end = clock();

    /* Generate master graph at the end. This will basically make system calls
     * and things or just call a bash script for me
     */
    if (plot_this.master)
    {
        PlotMasterGraph();
    }
    std::cout << "Total time taken: " << (global_clock_end - global_clock_start)/CLOCKS_PER_SEC << "\n";
    return 0;
#endif     /* ----- #if 0 : If0Label_1 ----- */

}				/* ----------  end of function main  ---------- */
