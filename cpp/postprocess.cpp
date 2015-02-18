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

//#define DEBUG 0

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

} parameters;

long
GetFileSize(std::string filename)
{
    FILE *p_file = NULL;
    p_file = fopen(filename.c_str(),"rb");
    fseek(p_file,0,SEEK_END);
    int size = ftell(p_file);
    fclose(p_file);
    return size;
}
/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  calculateTimeToPlotList
 *  Description:  returns the sorted values
 * =====================================================================================
 */
std::vector<double>
calculateTimeToPlotList (unsigned int num_pats, std::vector <unsigned int> stage_times)
{
    /*  Intervals after the recall stimulus that I want to graph at */
    float graphing_intervals[2] = {0., 1. };
    std::vector<double> graphing_times;
    double times = 0;
    std::ofstream ofs("00-Recall-times.txt", std::ofstream::out);
    ofs << "Intervals: ";
    for (int j = 0; j < (int)(sizeof(graphing_intervals)/sizeof(float)); j += 1 ) {
        ofs << graphing_intervals[j] << "\t";
    }
    ofs << "\n";

    /*  Initial stimulus and stabilisation */
    times = stage_times[0] + stage_times[1];

    for (unsigned int i = 1; i <= num_pats; ++i ) {
        /*  pattern strengthening and pattern stabilisation */
        times += (stage_times[2] + stage_times[3]);

        for (unsigned int k = 1; k <= i; ++k)
        {
            /*  Recall stimulus given */
            times += stage_times[4];
            ofs << times << "\t";

            /*  calculate my plotting intervals */
            for (int j = 0; j < (int)(sizeof(graphing_intervals)/sizeof(float)); j += 1 ) {
                graphing_times.push_back(times+graphing_intervals[j]);
            }
            /*  Recall check time added */
            times += stage_times[5];
        }
    }
    ofs << "\n";
    ofs.close();
    std::sort(graphing_times.begin(), graphing_times.end());
    return graphing_times;
}		/* -----  end of function calculateTimeToPlotList  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  getSNR
 *  Description:  
 * =====================================================================================
 */
    struct SNR
getSNR (std::vector<unsigned int> patternRates, std::vector<unsigned int> noiseRates )
{
    struct SNR snr;
    unsigned int sum_patterns = std::accumulate(patternRates.begin(), patternRates.end(),0.0);
    snr.mean = sum_patterns/patternRates.size();

    double sq_diff = 0.;
    std::for_each(patternRates.begin(), patternRates.end(), [&] (const double d) {
            sq_diff += pow((d - snr.mean),2);
            });

    snr.std = sqrt(sq_diff/(patternRates.size() -1));

    unsigned int sum_noises = std::accumulate(noiseRates.begin(), noiseRates.end(),0.0);
    double mean_noises = sum_noises/noiseRates.size();

    sq_diff = 0.;
    std::for_each(noiseRates.begin(), noiseRates.end(), [&] (const double d) {
            sq_diff += pow((d - mean_noises),2);
            });

    double std_noises = sqrt(sq_diff/(noiseRates.size() -1));

    snr.SNR = ((2.*pow((snr.mean - mean_noises),2))/(pow(snr.std,2) + pow(std_noises,2)));

    return snr;
}		/* -----  end of function getSNR  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  binary_upper_bound
 *  Description:  Last occurence of a key using binary search
 * =====================================================================================
 */
    char *
binary_upper_bound (double timeToCompare, boost::iostreams::mapped_file_source &openMapSource )
{
    char *spikesStart = NULL;
    unsigned long int numStart = 0;
    unsigned long int numEnd = 0;
    char *currentSpike = NULL;
    unsigned long int numCurrent = 0;
    unsigned long int numdiff = 0;
    unsigned long int step = 0;
    unsigned long int sizeofstruct = sizeof(struct spikeEvent_type);
    struct spikeEvent_type *currentRecord = NULL;


    /*  start of last record */
    spikesStart =  (char *)openMapSource.data();
    numStart = 0;
    /*  end of last record */
    numEnd = (openMapSource.size()/sizeofstruct -1);

    /*  Number of structs */

    numdiff = numEnd - numStart;
#ifdef DEBUG
    std::cout << "Finding last of " << timeToCompare << "\n";
    unsigned long int sizediff = 0;
    char *spikesEnd = NULL;
    spikesEnd =  (spikesStart + openMapSource.size() - sizeofstruct);
    sizediff = spikesEnd - spikesStart;
    std::cout << "Struct size is: " << sizeofstruct << "\n";
    std::cout << "Char size is: " << sizeof(char)  << "\n";
    std::cout << "size of int is: " << sizeof(int)  << "\n";
    std::cout << "Number of records in this file: " << (openMapSource.size() - sizeofstruct)/sizeofstruct << "\n";
    std::cout << "Number of records in this file: " << (spikesEnd - spikesStart)/sizeofstruct << "\n";
    printf("With printf subtraction %zu\n",(spikesEnd - spikesStart));
    std::cout << "Proper subtraction : " << (spikesEnd - spikesStart) << "\n";
    std::cout << "sizediff : " << sizediff << "\n";
    printf("With printf sizediff %zu\n",sizediff);
    std::cout << "multiplier " << (spikesEnd - spikesStart)/sizediff << "\n";
    std::cout << "Number of struct records in this file: " << numdiff << "\n";
#endif

    while( numdiff > 0)
    {
        numCurrent = numStart;
        step = (numdiff/2);

        numCurrent += step;
        currentSpike = spikesStart + numCurrent * sizeofstruct;
        currentRecord = (struct spikeEvent_type *)currentSpike;
#ifdef DEBUG
        std::cout << "Current record is: " << currentRecord->time << "\t" << currentRecord->neuronID << " at line" << numCurrent << "\n";
#endif

        if (!(timeToCompare < currentRecord->time))
        {
            numStart = ++numCurrent;
            numdiff -= step + 1;
        }
        else
            numdiff = step;
    }

    currentSpike = spikesStart + (numStart * sizeofstruct);
    currentRecord = (struct spikeEvent_type *)currentSpike;
#ifdef DEBUG
    std::cout << "Returning: " << currentRecord->time << "\t" << currentRecord->neuronID << "\n";
#endif
    return currentSpike;
}		/* -----  end of function binary_upper_bound  ----- */


/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  binary_lower_bound
 *  Description:  First occurence of a key using binary search
 * =====================================================================================
 */
    char *
binary_lower_bound (double timeToCompare, boost::iostreams::mapped_file_source &openMapSource )
{
    char *spikesStart = NULL;
    unsigned long int numStart = 0;
    unsigned long int numEnd = 0;
    char *currentSpike = NULL;
    unsigned long int numCurrent = 0;
    unsigned long int numdiff = 0;
    unsigned long int step = 0;
    unsigned long int sizeofstruct = sizeof(struct spikeEvent_type);
    struct spikeEvent_type *currentRecord = NULL;


    /*  start of last record */
    spikesStart =  (char *)openMapSource.data();
    numStart = 0;
    /*  end of last record */
    numEnd = (openMapSource.size()/sizeofstruct -1);

    /*  Number of structs */
    numdiff = numEnd - numStart;

#ifdef DEBUG
    std::cout << "Finding first of " << timeToCompare << "\n";
    unsigned long int sizediff = 0;
    char *spikesEnd = NULL;
    spikesEnd =  (spikesStart + openMapSource.size() - sizeofstruct);
    sizediff = spikesEnd - spikesStart;
    std::cout << "Struct size is: " << sizeofstruct << "\n";
    std::cout << "Char size is: " << sizeof(char)  << "\n";
    std::cout << "size of int is: " << sizeof(int)  << "\n";
    std::cout << "Number of records in this file: " << (openMapSource.size() - sizeofstruct)/sizeofstruct << "\n";
    std::cout << "Number of records in this file: " << (spikesEnd - spikesStart)/sizeofstruct << "\n";
    printf("With printf subtraction %zu\n",(spikesEnd - spikesStart));
    std::cout << "Proper subtraction : " << (spikesEnd - spikesStart) << "\n";
    std::cout << "sizediff : " << sizediff << "\n";
    printf("With printf sizediff %zu\n",sizediff);
    std::cout << "multiplier " << (spikesEnd - spikesStart)/sizediff << "\n";
    std::cout << "Number of struct records in this file: " << numdiff << "\n";
#endif

    while( numdiff > 0)
    {
        numCurrent = numStart;
        step = (numdiff/2);

        numCurrent += step;
        currentSpike = spikesStart + numCurrent * sizeofstruct;
        currentRecord = (struct spikeEvent_type *)currentSpike;
#ifdef DEBUG
        std::cout << "Current record is: " << currentRecord->time << "\t" << currentRecord->neuronID << " at line" << numCurrent << "\n";
#endif

        if (currentRecord->time < timeToCompare)
        {
            numStart = ++numCurrent;
            numdiff -= step + 1;
        }
        else
            numdiff = step;
    }

    currentSpike = spikesStart + (numStart * sizeofstruct);
    currentRecord = (struct spikeEvent_type *)currentSpike;
#ifdef DEBUG
    std::cout << "Returning: " << currentRecord->time << "\t" << currentRecord->neuronID << "\n";
#endif
    return currentSpike;
}		/* -----  end of function binary_lower_bound  ----- */

/* 
 * ===  FUNCTION  ======================================================================
 *         Name:  tardis
 *  Description:  
 * =====================================================================================
 */
    void
tardis (std::vector<boost::iostreams::mapped_file_source> &dataMapsE, std::vector<boost::iostreams::mapped_file_source> &dataMapsI, std::vector<std::vector<unsigned int> >& patterns, std::vector<std::vector <unsigned int> >& recalls, double timeToFly, struct param parameters)
{
    std::vector <unsigned int>neuronsE;
    std::vector <unsigned int>neuronsI;
    std::vector <std::vector<unsigned int> >pattern_neurons_rate;
    std::vector <std::vector<unsigned int> >noise_neurons_rate;
    std::vector <std::vector<unsigned int> >recall_neurons_rate;
    std::vector <unsigned int>neuronsE_rate;
    std::vector <unsigned int>neuronsI_rate;
    std::ostringstream converter;

    /*  Initialise the 2D vectors */
    for (unsigned int i = 0; i < parameters.num_pats; ++i)
    {
        pattern_neurons_rate.emplace_back(std::vector<unsigned int>());
        noise_neurons_rate.emplace_back(std::vector<unsigned int>());
        recall_neurons_rate.emplace_back(std::vector<unsigned int>());
    }

#ifdef DEBUG
    std::cout << "[TARDIS] Thread " << std::this_thread::get_id() << " working on it, Sir!" << "\n";
#endif
    /*  Fill up my vectors with neurons that fired in this period */
    for (unsigned int i = 0; i < parameters.mpi_ranks; ++i)
    {
        /*  Excitatory */
        char * chunk_start = binary_lower_bound(timeToFly - 1., std::ref(dataMapsE[i]));
        char * chunk_end = binary_upper_bound(timeToFly, std::ref(dataMapsE[i]));
        char * chunkit = NULL;
        if (chunk_end - chunk_start > 0)
        {
            chunkit = chunk_start;
            while (chunkit <= chunk_end)
            {
                struct spikeEvent_type *buffer;
                buffer = (struct spikeEvent_type *)chunkit;
                neuronsE.emplace_back(buffer->neuronID);
                chunkit += sizeof(struct spikeEvent_type);

            }
        }
        else
        {
            std::cout << timeToFly << " not found in E file "  << i << "!\n";
            return;
        }

        /*  Inhibitory */
        chunk_start = binary_lower_bound(timeToFly - 1., std::ref(dataMapsI[i]));
        chunk_end = binary_upper_bound(timeToFly, std::ref(dataMapsI[i]));
        if (chunk_end - chunk_start > 0)
        {
            chunkit = chunk_start;
            while (chunkit <= chunk_end)
            {
                struct spikeEvent_type *buffer;
                buffer = (struct spikeEvent_type *)chunkit;
                neuronsI.emplace_back(buffer->neuronID);
                chunkit += sizeof(struct spikeEvent_type);
            }
        }
        else
        {
            std::cout << timeToFly << " not found in I file "  << i << "!\n";
            return;
        }

    }
    /*  Sort - makes next operations more efficient, or I think it does */
    std::sort(neuronsE.begin(), neuronsE.end());
    std::sort(neuronsI.begin(), neuronsI.end());

    /*  Get frequencies of inhibitory neurons */
    std::vector<unsigned int>::iterator search_begin = neuronsI.begin();
    for(unsigned int i = 1; i <= parameters.NI; ++i)
    {
        int rate = 0;
        rate = (std::upper_bound(search_begin, neuronsI.end(), i) != neuronsI.end()) ?  (std::upper_bound(search_begin, neuronsI.end(), i) - search_begin) : 0;

        search_begin = std::upper_bound(search_begin, neuronsI.end(), i);
        neuronsI_rate.emplace_back(rate);
    }
    /*  We have the inhibitory firing rate! */



    /* Get frequencies of excitatory neurons */
    search_begin = neuronsE.begin();
    for(unsigned int i = 1; i <= parameters.NE; ++i)
    {
        int rate = 0;
        rate = (std::upper_bound(search_begin, neuronsE.end(), i) != neuronsE.end()) ?  (std::upper_bound(search_begin, neuronsE.end(), i) - search_begin) : 0;
        search_begin = std::upper_bound(search_begin, neuronsE.end(), i);

        for (unsigned int j = 0; j < parameters.num_pats; j++)
        {
            /*  It's part of the pattern */
            if(pattern_neurons_rate[j].size() != patterns[j].size() && std::binary_search(patterns[j].begin(), patterns[j].end(), i))
            {
                pattern_neurons_rate[j].emplace_back(rate);
            }
            /*  It's not in the pattern and therefore a noise neuron */
            else
            {
                noise_neurons_rate[j].emplace_back(rate);
            }
            /* It's a recall neuron - not using this at the moment */
            if(recall_neurons_rate[j].size() != recalls[j].size() && std::binary_search(recalls[j].begin(), recalls[j].end(), i))
            {
                recall_neurons_rate[j].emplace_back(rate);
            }
        }
        /*  All E neurons */
        neuronsE_rate.emplace_back(rate);
    }

    /*  We have excitatory firing rate of all, pattern and recall neurons */
    /*  Multi line files for histograms and min max ranges */
    /*  Excitatory multiline */
    converter.str("");
    converter.clear();
    converter << parameters.output_file << "-" << std::to_string(timeToFly) << ".e.rate-multiline";
    std::ofstream neuronsE_multiline(converter.str());
    for (std::vector<unsigned int>::iterator it = neuronsE_rate.begin(); it != neuronsE_rate.end(); ++it)
        neuronsE_multiline << *it << "\n";
    neuronsE_multiline.close();

    /*  Inhibitory multiline */
    converter.str("");
    converter.clear();
    converter << parameters.output_file << "-" << std::to_string(timeToFly) << ".i.rate-multiline";
    std::ofstream neuronsI_multiline(converter.str());
    for (std::vector<unsigned int>::iterator it = neuronsI_rate.begin(); it != neuronsI_rate.end(); ++it)
        neuronsI_multiline << *it << "\n";
    neuronsI_multiline.close();

    for(unsigned int i = 0; i < parameters.num_pats; ++i)
    {
        /*  Multiline for pattern neurons */
        converter.str("");
        converter.clear();
        converter << parameters.output_file << "-" << std::to_string(timeToFly) << "-" << std::setw(8)  << std::setfill('0') << std::to_string(i+1) << ".pattern.rate.multiline";
        std::ofstream patternNeurons_multiline(converter.str());
        for (std::vector<unsigned int>::iterator it = pattern_neurons_rate[i].begin(); it != pattern_neurons_rate[i].end(); ++it)
            patternNeurons_multiline << *it << "\n";
        patternNeurons_multiline.close();

        /*  Multiline for noise neurons */
        converter.str("");
        converter.clear();
        converter << parameters.output_file << "-" << std::to_string(timeToFly) << "-" << std::setw(8)  << std::setfill('0') << std::to_string(i+1) << ".noise.rate.multiline";
        std::ofstream noiseNeurons_multiline(converter.str());
        for (std::vector<unsigned int>::iterator it = recall_neurons_rate[i].begin(); it != recall_neurons_rate[i].end(); ++it)
            noiseNeurons_multiline << *it << "\n";
        noiseNeurons_multiline.close();
    }

    /*  Combined matrix */
    converter.str("");
    converter.clear();
    converter << parameters.output_file << "-" << std::to_string(timeToFly) << ".combined.matrix";
    std::ofstream neurons_matrix_stream (converter.str());
    std::vector<unsigned int>::iterator neuronsE_iterator = neuronsE_rate.begin();
    std::vector<unsigned int>::iterator neuronsI_iterator = neuronsI_rate.begin();
    for (unsigned int i = 0; i < (parameters.NE + parameters.NI); ++i)
    {
        if((i % (parameters.graph_widthI + parameters.graph_widthE)) < parameters.graph_widthE)
        {
            /*  print neuronE to neuron matrix stream */
            neurons_matrix_stream << *neuronsE_iterator;
            neuronsE_iterator++;
        }
        else
        {
            /*  print neuronI to neuron matrix stream */
            neurons_matrix_stream << *neuronsI_iterator;
            neuronsI_iterator++;
        }

        /*  Tab or newline? */
        if ((i % (parameters.graph_widthI + parameters.graph_widthE)) != ((parameters.graph_widthE + parameters.graph_widthI -1)))
        {
            /*  print \n to stream, a new row starts */
            neurons_matrix_stream << "\t";
        }
        else 
        {
            neurons_matrix_stream << "\n";
            /*  print tab to stream, same row continues */
        }
    }
    neurons_matrix_stream.close();

    struct SNR snr;
    for (unsigned int j = 0; j < parameters.num_pats; j++)
    {
        /*  SNR matrix */
        converter.str("");
        converter.clear();
        converter << parameters.output_file << "-" << std::to_string(timeToFly) << "-" << std::setw(8)  << std::setfill('0') << std::to_string(j+1) << ".pattern-signal-noise-ratio.matrix";
        std::ofstream pattern_snr (converter.str());
        snr = getSNR(pattern_neurons_rate[j], noise_neurons_rate[j]);
        pattern_snr << timeToFly << "\t" << snr.SNR << "\t" << snr.mean << "\t" << snr.std << "\n";
        pattern_snr.close();

        /*  Pattern matrix */
        converter.str("");
        converter.clear();
        converter << parameters.output_file << "-" << std::to_string(timeToFly) << "-" << std::setw(8)  << std::setfill('0') << std::to_string(j+1) << ".pattern.matrix";

        /* I could pad the matrix if it isn't rectangular in shape. I
         * won't, though. 
         */ 
/*         for (unsigned int i = pattern_neurons_rate[j].size(); (i % 10) != 0; ++i)
 *             pattern_neurons_rate[j].emplace_back(0);
 */

        std::ofstream pattern_stream (converter.str());
        for (unsigned int i = 0; i != pattern_neurons_rate[j].size(); ++i)
        {
            pattern_stream << pattern_neurons_rate[j][i];
            if(((i+1)%parameters.graph_pattern_cols) == 0)
                pattern_stream << "\n";
            else 
                pattern_stream << "\t";
        }
        pattern_stream.close();
    }

    /*  Not printing out recall neurons for the time being */

#ifdef DEBUG
    std::cout << "[TARDIS] Thread " << std::this_thread::get_id() << " reporting on conclusion, Sir!" << "\n";
#endif
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

    for (unsigned int i = 0; i < parameters.num_pats; ++i)
    {
        converter.str("");
        converter.clear();
        converter << parameters.patternfile_prefix <<  std::setw(8)  << std::setfill('0') << std::to_string(i+1) << ".pat";
        std::string pattern = converter.str();
        ifs.open(pattern);
        patterns.emplace_back(std::vector<unsigned int> ());
        while(ifs >> neuronID)
            patterns[i].emplace_back(neuronID);
        ifs.close();
        std::sort(patterns[i].begin(), patterns[i].end());
#ifdef DEBUG
        std::cout << "Items loaded and sorted in pattern number " << i << " is " << patterns[i].size() << "\n";
#endif

        converter.str("");
        converter.clear();
        converter << parameters.recallfile_prefix <<  std::setw(8)  << std::setfill('0') << std::to_string(i+1) << ".pat";
        std::string recall = converter.str();
        recalls.emplace_back(std::vector<unsigned int> ());
        ifs.open(recall);
        while(ifs >> neuronID)
            recalls[i].emplace_back(neuronID);
        ifs.close();
        std::sort(recalls[i].begin(), recalls[i].end());
#ifdef DEBUG
        std::cout << "Items loaded and sorted in recall number " << i << " is " << recalls[i].size() << "\n";
#endif
    }
    clock_t clock_end = clock();
    std::cout << patterns.size() << " patterns and " << recalls.size() << " recalls read in " << (clock_end - clock_start)/CLOCKS_PER_SEC << " seconds.\n";

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
    clock_t clock_start, clock_end, global_clock_start, global_clock_end;

    std::vector<boost::iostreams::mapped_file_source> raster_data_source_E;
    std::vector<boost::iostreams::mapped_file_source> raster_data_source_I;
    std::vector<std::thread> timeLords;
    int doctors_max; 
    std::vector<std::vector<unsigned int> > patterns;
    std::vector<std::vector<unsigned int> > recalls;
    std::ostringstream converter;

    global_clock_start = clock();
    try {

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


    /*  At the most, use 20 threads, other wise WAIT */
    doctors_max = (parameters.mpi_ranks <= 20) ? parameters.mpi_ranks : 20;
    std::cout << "Maximum number of threads: " << doctors_max << "\n";

    /*  Calculate the times when I need to generate my graphs */
    /*  Remove duplicates */
    graphing_times = calculateTimeToPlotList(parameters.num_pats, parameters.stage_times);
    graphing_times.insert(graphing_times.end(), parameters.plot_times.begin(), parameters.plot_times.end());
    std::sort(graphing_times.begin(), graphing_times.end());
    graphing_times.erase(std::unique (graphing_times.begin(), graphing_times.end()), graphing_times.end());
#ifdef DEBUG
    for (std::vector<double>::iterator it =  graphing_times.begin(); it != graphing_times.end(); ++it)
        std::cout << *it << "\t";
    std::cout << "\n";
#endif
    std::cout << "Getting files for " << graphing_times.size() << " snapshots." << "\n";


    /*  Get my patterns and recalls into vectors - these files are relatively
     *  minuscule */
    getPatternsAndRecalls(std::ref(patterns), std::ref(recalls), parameters);

    /*  Open my memory mapped files - no need to read the entire data into
     *  memory */
    clock_start = clock();
    for (unsigned int i = 0; i < parameters.mpi_ranks; ++i)
    {
        converter.str("");
        converter.clear();
        converter << parameters.output_file << "." << i << "_e.ras";
#ifdef DEBUG
        std::cout << "Loading e file: " << converter.str() << "\n";
#endif
        raster_data_source_E.emplace_back(boost::iostreams::mapped_file_source());
        raster_data_source_E[i].open(converter.str());

        converter.str("");
        converter.clear();
        converter << parameters.output_file << "." << i << "_i.ras";
#ifdef DEBUG
        std::cout << "Loading i file: " << converter.str() << "\n";
#endif
        raster_data_source_I.emplace_back(boost::iostreams::mapped_file_source());
        raster_data_source_I[i].open(converter.str());
    }
    clock_end = clock();
    std::cout << raster_data_source_E.size() <<  " E and " << raster_data_source_I.size() << " I files mapped in " << (clock_end - clock_start)/CLOCKS_PER_SEC << " seconds.\n";


    clock_start = clock();
    int task_counter = 0;
    for(std::vector<double>::const_iterator i = graphing_times.begin(); i != graphing_times.end(); ++i)
    {
        std::vector<std::vector<unsigned int> > extracted_data_temp;
        /*  Only start a new thread if less than thread_max threads are running */
        if (task_counter < doctors_max)
        {
            timeLords.emplace_back(std::thread (tardis, std::ref(raster_data_source_E), std::ref(raster_data_source_I), std::ref(patterns), std::ref(recalls), *i, parameters));
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
    global_clock_end = clock();
    std::cout << "Total time taken: " << (global_clock_end - global_clock_start)/CLOCKS_PER_SEC << "\n";

    return 0;
}				/* ----------  end of function main  ---------- */
