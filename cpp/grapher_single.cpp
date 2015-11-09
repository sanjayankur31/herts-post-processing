/*
 * =====================================================================================
 *
 *       Filename:  grapher_single.cpp
 *
 *    Description:  Generate graphs for a single simulation
 *
 *        Version:  1.0
 *        Created:  06/11/15 13:56:52
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Ankur Sinha (FranciscoD), ankursinha@fedoraproject.org
 *   Organization:  
 *
 * =====================================================================================
 */


#include "utils.h"

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  main
 *  Description:  
 * =====================================================================================
 */
int
main ( int ac, char *av[] )
{
    try
    {
        po::options_description cli("Command line options");
        cli.add_options()
            ("help,h", "produce help message")
            ("out,o", po::value<std::string>(&(parameters.output_file)), "output filename")
            ("config,c", po::value<std::string>(&(parameters.config_file))->default_value("simulation_config.cfg"),"configuration file to be used to find parameter values for this simulation run")
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

        if (vm.count("for-prints"))
        {
            plot_this.for_prints = true;
            plot_this.formatPNG = false;
        }
        if (vm.count("for-meetings"))
        {
            plot_this.for_meetings = true;
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

    /*  Generate all my graphs */
    GenerateSNRPlotFromFile("00-SNR-data.txt");


#if  0     /* ----- #if 0 : If0Label_1 ----- */

    std::vector<std::pair<std::string, std::string> > inputs;
    inputs.emplace_back(std::pair<std::string, std::string>("00-SNR-data-cumulative.txt", "cumulative"));
    inputs.emplace_back(std::pair<std::string, std::string>("00-SNR-data-overwritten.txt", "clipped"));
    GenerateMultiSNRPlotFromFile(inputs);
#endif     /* ----- #if 0 : If0Label_1 ----- */

    std::vector<boost::tuple<std::string, std::string, std::string> > inputs1;
    inputs1.emplace_back(boost::tuple<std::string, std::string, std::string> ("00-Mean-data.txt", "00-STD-data.txt", " "));
    GenerateMultiMeanWithSTDFromFiles(inputs1, "MeanWithSTD");

    inputs1.clear();
    inputs1.emplace_back(boost::tuple<std::string, std::string, std::string> ("00-Mean-noise-data.txt", "00-STD-noise-data.txt", " "));
    GenerateMultiMeanWithSTDFromFiles(inputs1, "MeanWithSTD-noise");


    inputs1.clear();
    inputs1.emplace_back(boost::tuple<std::string, std::string, std::string> ("00-Mean-data.txt", "00-STD-data.txt", "p"));
    inputs1.emplace_back(boost::tuple<std::string, std::string, std::string> ("00-Mean-noise-data.txt", "00-STD-noise-data.txt", "b"));
    GenerateMultiMeanWithSTDFromFiles(inputs1, "MeanWithSTD-both");


    return 0;
}				/* ----------  end of function main  ---------- */
