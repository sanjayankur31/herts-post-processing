/*
 * =====================================================================================
 *
 *       Filename:  postprocess_multisim.cpp
 *
 *    Description:  my postprocessing C++ file for multi simulation runs
 *
 *        Version:  1.0
 *        Created:  05/11/15 15:02:03
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
            ("multiSNR,r","Multi SNR graph requires -W values to be given")
            ("multiMean,n","Multi Mean graph requires -W values to be given")
            ("multiSTD,D","Multi STD graph requires -W values to be given")
            ("multiMeanAndSTD,K","Generate a graph with multiple mean and std plots")
            ("png,P", "Generate graphs in png. Default is svg.")
            ("generate-snr-vs-wpats-plot-from-file,w","Also generate snr _VS_ wpats plot along with snr-for-multiple-pats - picks wpats from arguments of W")
            ("generate-means-vs-wpats-plot-from-file,m","Also generate mean _VS_ wpats plot along with snr-for-multiple-pats - picks wpats from arguments of W")
            ("generate-std-vs-wpats-plot-from-file,d","Also generate std _VS_ wpats plot along with snr-for-multiple-pats - picks wpats from arguments of W")
            ("pats,W", po::value<std::vector<double> >(&(plot_this.wPats))-> multitoken(), "w_pat values that input files are available for")

        po::options_description params("Parameters");
        params.add_options()
            ("num_pats", po::value<unsigned int>(&(parameters.num_pats)), "number of pattern file(s) to pick from pattern set")
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
            plot_this.snr_graphs = true;
        }
        else
        {
            if (vm.count("generate-snr-vs-wpats-plot-from-file"))
            {
                plot_this.snr_VS_wPats = true;
            }
            if (vm.count("generate-snr-vs-wpats-plot-from-file"))
            {
                plot_this.snr_VS_wPats = true;
            }
            if (vm.count("generate-means-vs-wpats-plot-from-file"))
            {
                plot_this.mean_VS_wPats = true;
            }
            if (vm.count("generate-std-vs-wpats-plot-from-file"))
            {
                plot_this.std_VS_wPats = true;
            }
            if (vm.count("multiSNR"))
            {
                plot_this.multiSNR = true;
            }
            if (vm.count("multiMean"))
            {
                plot_this.multiMean = true;
            }
            if (vm.count("multiSTD"))
            {
                plot_this.multiSTD = true;
            }
            if (vm.count("multiMeanAndSTD"))
            {
                plot_this.singleMeanAndSTD = false;
                plot_this.multiMeanAndSTD = true;
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
}
