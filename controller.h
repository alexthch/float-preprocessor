#ifndef CONTROLLER_H
#define CONTROLLER_H


#include "cmdparser.hpp"
#include "functions.h"

#include <thread>
#include <string>
#include <chrono>
#include <sstream>

class Timer
{
public:
    
    Timer(size_t* durationPtr)
    {
        this->m_DurationPointer = durationPtr;
        m_StartTimepoint = std::chrono::high_resolution_clock::now();
    }
    ~Timer() {
        Stop();
    }
    void Stop() {
        auto endTimepoint = std::chrono::high_resolution_clock::now();

        auto start = std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimepoint).time_since_epoch().count();
        auto end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().count();

        *m_DurationPointer = end - start;
    }
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTimepoint;
    size_t* m_DurationPointer;
};

static size_t amountOfThreads = std::thread::hardware_concurrency() - 1;

void configureParser(cli::Parser& parser) {

	parser.set_required<std::string>("f", "file", "filename", "filename to preprocess");
	parser.set_optional<std::string>("o", "output", "filename_output.csv", "output filename");

    parser.set_optional<float>("g", "guess", 1.0f, "Sets a scalar to force the program to allocate more memory for the dataset");
    parser.set_optional<size_t>("s", "sizet", 10, "How many % of dataset is analyzed for training");
	parser.set_optional<size_t>("t", "threads", amountOfThreads, "Force to use amount of threads. Otherwise it will detect amount of logical cores.");
    parser.set_optional<size_t>("b", "bytes", 4096, "The amount of bytes scanned in the beginning, use large values if many columns");
	parser.set_optional<bool>("x", "xprint", false, "Prints all analysis results to the console");
	parser.set_optional<std::string>("y", "yprint", "free.lunch", "Prints all logs to a file.");
	parser.set_optional<bool>("z", "zprint", false, "Prints all logs to the console");

	//force single scheme
	parser.set_optional<bool>("m", "multi", false, "Forces multiplication scheme on the dataset");
	parser.set_optional<bool>("a", "add", false, "Forces addition scheme on the dataset");
	parser.set_optional<bool>("p", "pow", false, "Forces power scheme on the dataset");

    parser.set_optional<std::string>("w", "wparam", "3,12", "Sets the parameters for the multiplication scheme. Format M,P");

}

void handlePrinting(const cli::Parser& parser, Dataset *dataset) {

    if (parser.get<bool>("x")) {
        
        dataset->printAnalysisResults = true;

    }
    if (parser.get<bool>("z")) {
        
        dataset->printLogs = true;

    }
    if (parser.get<std::string>("y") != "free.lunch") {
        
        dataset->exportResults = true;
        dataset->exportFilename = parser.get<std::string>("y");

    }

}

void handleScheme(const cli::Parser& parser, Dataset * dataset) {
    if (parser.get<bool>("m")) {
        
        std::istringstream iss(parser.get<std::string>("w"));

        size_t m, p;
        char c;
        iss >> m >> c >> p;

        if (m % 2 == 1 && p < 32) {
            dataset->finalM = m;
            dataset->finalP = p;
        }
        else {
            dataset->finalM = 3;
            dataset->finalP = 12;
        }

        size_t duration = 0;
		{
			Timer timer(&duration);
			
            dataset->masterPerformMultiplication();

        }
        dataset->metrics.push_back(metric("Multiplication performace", duration, dataset->actualSize*4, "MB/s"));

        dataset->exportPreprocessedFile();

    }
    else if (parser.get<bool>("a")) {
        
        size_t duration = 0;
        {
            Timer timer(&duration);
            dataset->runAddition();
        }
        dataset->metrics.push_back(metric("Analysis of addition", duration, dataset->howManyToTest*4, "MB/s"));
        duration = 0;
        {
            Timer timer(&duration);
            dataset->masterPerformAddition();
        }
        dataset->metrics.push_back(metric("Addition performance", duration, dataset->actualSize*4, "MB/s"));
        dataset->exportPreprocessedFile();
    }
    else if (parser.get<bool>("p")) {
        
        dataset->finalPoFive = 25.0f;

        size_t duration = 0;
        {
            Timer timer(&duration);

            dataset->masterPerformPowersOfFive();
        }
        dataset->metrics.push_back(metric("Powers of five performance", duration, dataset->actualSize*4, "MB/s"));
        dataset->exportPreprocessedFile();
    }
    else {
        size_t duration { 0 };

        {
            Timer timer(&duration);
            dataset->runAddition();
        }

        dataset->metrics.push_back(metric("Analysis of addition", duration, dataset->howManyToTest*4, "MB/s"));
        duration = 0;
        {
            Timer timer(&duration);
            dataset->runMultiplication();
        }

        size_t multSize = static_cast<size_t>(dataset->MValues.size()) * static_cast<size_t>(dataset->PValue.size()) * 4 * dataset->howManyToTest;

        dataset->metrics.push_back(metric("Analysis of multiplication", duration, multSize, "MB/s"));
        duration = 0;
        {
            Timer tiomer(&duration);
            dataset->runPowersOfFive();
        }
        dataset->metrics.push_back(metric("Analysis of Powers of five", duration, dataset->howManyToTest*12, "MB/s"));
        
    }
}

void runParser(const cli::Parser& parser) {
        
    size_t consoleAmountOfThreads = parser.get<size_t>("t");
    if (consoleAmountOfThreads != amountOfThreads) {

        amountOfThreads = consoleAmountOfThreads;

    }

    threadPool threadpool(amountOfThreads);
    threadpool.start();

    Dataset dataset(parser.get<std::string>("f"), &threadpool);
    handlePrinting(parser, &dataset);
    dataset.guessScaling = parser.get<float>("g");
    dataset.defaultTestSizePercent = parser.get<size_t>("s");
    size_t duration { 0 };
    dataset.bytesToCheck = parser.get<size_t>("b");
    {
        Timer timer(&duration);
        dataset.parseHeaders();
        dataset.guessDatasetSize();
        dataset.startCastingProcess();

    }
    dataset.metrics.push_back(metric("Load and casting", duration, dataset.file.length, "MB/s"));
    {
        handleScheme(parser, &dataset);
    }

    

}


#endif // !CONTROLLER_H
