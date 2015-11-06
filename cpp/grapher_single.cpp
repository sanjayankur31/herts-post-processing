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
main ( int argc, char *argv[] )
{
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
            ("singleMeanAndSTD,k","Generate a graph with mean and std from one set of data files 00-{Mean,STD}-data.txt")
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

        if (vm.count("all"))
        {
            plot_this.master = true;
            plot_this.snr_graphs = true;
        }
        else
        {
            if (vm.count("master"))
            {
                plot_this.master = true;
            }
            if (vm.count("pattern"))
            if (vm.count("snr"))
            {
                plot_this.snr_graphs = true;
            }
            if (vm.count("print-snr"))
            {
                parameters.print_snr = true;
            }
            if (vm.count("png"))
            {
                plot_this.formatPNG = true;
            }
            if (vm.count("singleMeanAndSTD"))
            {
                plot_this.singleMeanAndSTD = true;
                plot_this.multiMeanAndSTD = false;
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

    return 0;
}				/* ----------  end of function main  ---------- */
