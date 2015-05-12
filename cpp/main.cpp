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
    clock_t global_clock_start, global_clock_end;
    clock_t clock_start, clock_end;
    std::vector<std::vector<unsigned int> > patterns;
    std::vector<std::vector<unsigned int> > recalls;
    std::vector<double> graphing_times;
    std::multimap <double, struct SNR_data> snr_data;
    unsigned int threads_max = 0;
    unsigned int task_counter = 0;
    std::vector<std::thread> threadlist;
    std::thread master_graph_thread;
    std::vector<boost::iostreams::mapped_file_source> spikes_E;
    std::vector<boost::iostreams::mapped_file_source> spikes_I;
    std::ostringstream converter;

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
            ("generate-snr-plot-from-file,g","Generate snr from a printed file 00-SNR-data.txt.")
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
                parameters.for_prints = true;
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
    if(plot_this.processRas)
    {
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
        for (std::vector<double>::iterator it = graphing_times.begin(); it != graphing_times.end(); it++)
            std::cout << *it << "\t";
        std::cout << "\n";

        /*  Load patterns and recalls */
        LoadPatternsAndRecalls(std::ref(patterns), std::ref(recalls));

        /*  Load memory mapped files */
        clock_start = clock();
        for (unsigned int i = 0; i < parameters.mpi_ranks; ++i)
        {
            converter.str("");
            converter.clear();
            converter << parameters.output_file << "." << i << "_e.ras";
            spikes_E.emplace_back(boost::iostreams::mapped_file_source());
            spikes_E[i].open(converter.str());

            converter.str("");
            converter.clear();
            converter << parameters.output_file << "." << i << "_i.ras";
            spikes_I.emplace_back(boost::iostreams::mapped_file_source());
            spikes_I[i].open(converter.str());
        }
        clock_end = clock();
        std::cout << spikes_E.size() <<  " E and " << spikes_I.size() << " I files mapped in " << (clock_end - clock_start)/CLOCKS_PER_SEC << " seconds.\n";

        /*  Main worker loop */
        unsigned int time_counter = 0;
        for (unsigned int i = 0; i < parameters.num_pats; i++)
        {
            for(unsigned int j = 0; j <= i; j++)
            {
                /*  If thread_max threads are running, wait for them to finish before
                 *  starting a second round.
                 *
                 *  Yes, this can be optimised by using a thread pool but I really
                 *  don't have the patience to look into ThreadPool or a
                 *  boost::thread_group today!
                 */
                if (!(task_counter < threads_max))
                {
                    for (std::thread &t: threadlist)
                    {
                        if(t.joinable())
                        {
                            t.join();
                            task_counter--;
                        }
                    }
                    threadlist.clear();
                }
                /*  Now we can get back to running new threads */

                threadlist.emplace_back(std::thread (MasterFunction, std::ref(spikes_E), std::ref(spikes_I), std::ref(patterns), std::ref(recalls), graphing_times[time_counter++] + 1,  j, std::ref(snr_data)));
                task_counter++;
            }
        }
        /*  End of work */
        while(task_counter > 0)
        {
            for (std::thread &t: threadlist)
            {
                if(t.joinable())
                {
                    t.join();
                    task_counter--;
                }
            }
        }
        threadlist.clear();

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
    std::cout << "Total time taken: " << (global_clock_end - global_clock_start)/CLOCKS_PER_SEC << "\n";
    return 0;
}				/* ----------  end of function main  ---------- */
