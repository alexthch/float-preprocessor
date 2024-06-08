#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#ifdef _WIN32
#define C_WIN_CPP
#include "c-win.cpp"
void munmap_file(void* addr);
#else
#define C_UNIX_H
#include "c-unx.c"
static void munmap_file(void* ptr, size_t length);
#endif

#include "fast_float.h"
#include "tabulate.hpp"

#include <fstream>
#include <filesystem>
#include <string>
#include <thread>
#include <vector>
#include <string_view>
#include <mutex>
#include <bitset>
#include <limits>
#include <cmath>
#include <iomanip>
#include <map>

using namespace tabulate;
using namespace fast_float;

using Row_t = Table::Row_t;

typedef std::string_view strvw;
typedef std::pair<size_t, size_t> coords;

struct multResult {

	std::vector<size_t> tralingSymbols;
	float mse;
	float maxRelativeDeviation;


	multResult() {
		this->tralingSymbols.resize(33);
		this->mse = 0;
		this->maxRelativeDeviation = 0;
	}

};


void* mmap_file(const char* filename, size_t* length);
void testFile();

struct threadPool {
	size_t threads;
	std::vector<std::thread> threadList;

	threadPool() {
		this->threads = std::thread::hardware_concurrency() - 1;
	}
	threadPool(size_t threads) : threads(threads) {
	};

	~threadPool() {
		
	}

	void start() {
		for (size_t i = 0; i < threads; ++i) {
			threadList.push_back(std::thread());
		}
	}

};

struct metric {

	size_t duration;
	std::string name;
	size_t size;
	size_t throughput;
	std::string unit;

	metric(std::string name, size_t duration, size_t size, std::string unit) : duration(duration), name(name), size(size), unit(unit) {
		this->throughput = size / this->duration;
		this->duration /= 1000;
	}

};

struct mmfile {
	void* map;
	char* charMap;
	size_t length;
	std::string filename;
	mmfile(const char* filename) : filename(filename) {
		map = mmap_file(filename, &length);
		charMap = static_cast<char*>(map);
	}
#ifdef _WIN32
	~mmfile() {
		munmap_file(map);
	}
#else
	~mmfile() {
		munmap_file(map, length);
	}
#endif
};

class Dataset {

public:

	mmfile file;
	std::string filename;
	threadPool* trPool;
	
	//HandlePrintingVariables
	bool printLogs = false;
	bool printAnalysisResults = false;
	bool exportResults = false;
	std::string exportFilename = "free.lunch";
	std::vector<metric> metrics;

	//Fast guessing params
	size_t bytesToCheck = 4096*4;
	size_t datasetSizeGuess;
	float guessScaling = 1;


	//Actual dimensions of dataset
	size_t amountOfColumns;
	size_t actualSize = 0;
	
	//Parsing params
	char lineBreak = '\n';
	char delimiter = ',';
	
	std::vector<size_t> timers;

	//Containers for the data
	std::vector<std::vector<float>> floatResults;
	float maxInDataset = 0;
	float minInDataset = 0;
	std::vector<std::string> Headers;

	//weird variable for thread sync safety
	std::mutex mtx;

	/*
	* Decision params
	*/
	size_t finalM;
	size_t finalP;
	float finalPoFive;
	size_t trailingSymbolsThreshold = 12;


	/*
	* Analysis params 
	*/
	size_t defaultTestSizePercent = 5;
	size_t howManyToTest = 0;
	//Addition
	float error = 0;
	float bias = 0;
	
	//Po5
	std::vector<size_t> Trailing5;
	std::vector<size_t> Trailing25;
	std::vector<size_t> Trailing125;

	

	//mult
	std::vector<size_t> MValues = { 3,5,7,9,11};
	std::vector<size_t> PValue = {10,12,14,15,16,17,18,20};

	std::map<uint32_t, uint32_t> floatPatternMap = {
		{3,  0b01010101010101010101010101010101},
		{5,  0b11001100110011001100110011001100},
		{7,  0b00100100100100100100100100100100},
		{9,  0b00011100011100011100011100011100},
		{11, 0b00010111010001011101000101110100},
		{13, 0b00010011101100010011101100010011},
		{15, 0b00010001000100010001000100010001},
		{17, 0b00001111000011110000111100001111}, // this pattern produces mostly trailing 1s
		{19, 0b00001101011110010100001101011110}, // this pattern produces mostly trailing 1s
		{21, 0b00001100001100001100001100001100},
		{23, 0b00001011001000010110010000101100},
		{25, 0b00001010001111010111000010100011}, // this pattern produces mostly trailing 1s
		{27, 0b00001001011110110100001001011110}, // this pattern produces mostly trailing 1s
		{29, 0b00001000110100111101110010110000},
		{31, 0b00001000010000100001000010000100}
	};

	std::map<coords, multResult> multResults;

	
	Dataset(const std::string& filename, threadPool* threads) : file(filename.c_str()), filename(filename), trPool(threads) {
		
	}

	~Dataset() {
		

		if (printLogs) {

			Table logTable;
			logTable.add_row({ "Name", "Duration", "Size" , "Throughput"});
			logTable.format().column_separator("");
			logTable.column(0).format().width(30);
			logTable.column(1).format().width(20);
			logTable.column(2).format().width(20);
			logTable.column(3).format().width(30);
			
			for (auto m : metrics) {
				logTable.add_row({ m.name, std::to_string(m.duration), std::to_string(m.size), std::to_string(m.throughput)+m.unit});
			}
			

			std::cout << logTable << "\n" << std::endl;
			std::cout << "Amount of floats: " << this->actualSize << " | Columns: " << this->amountOfColumns << "Amount of rows: "<< this->actualSize/this->amountOfColumns << std::endl;
		}

		if (printAnalysisResults) {

			Table po5Table;
			Row_t header{ "Power/Trailing" };
			for (int i = 1; i < 33; ++i) {
				header.push_back(std::to_string(i));
			}
			po5Table.add_row(header);
			po5Table.format().column_separator("");
			
			Row_t row5{"1"};
			for (int i = 1; i < 33; ++i) {
				row5.push_back(std::to_string(this->Trailing5[i]));
			}
			po5Table.add_row(row5);

			Row_t row25{"2"};
			for (int i = 1; i < 33; ++i) {
				row25.push_back(std::to_string(this->Trailing25[i]));
			}
			po5Table.add_row(row25);

			Row_t row125{ "3" };
			for (int i = 1; i < 33; ++i) {
				row125.push_back(std::to_string(this->Trailing125[i]));
			}
			po5Table.add_row(row125);

			std::cout << po5Table << "\n" << std::endl;

			Table addTable;
			addTable.format().column_separator("");
			addTable.add_row({ "Param", "Value" });

			std::stringstream ss;
			ss << std::scientific << this->error;
			std::string errorString = ss.str();
			addTable.add_row({ "Error", errorString });
			addTable.add_row({ "Bias", std::to_string(this->bias) });
			addTable.add_row({ "Max", std::to_string(this->maxInDataset) });
			addTable.add_row({ "Min", std::to_string(this->minInDataset) });
			
			std::cout << addTable << "\n" << std::endl;
			
			
			Table multTable;
			Row_t headerMult{ "M","P","MSE","Max % Dev" };
			for (int i = 1; i < 33; ++i) {
				headerMult.push_back(std::to_string(i));
			}
			//multTable.add_row(headerMult);
			multTable.format().column_separator("");
			size_t lastM = 0;

			for (auto res : this->multResults) {

				std::stringstream ss;
				ss << std::scientific << res.second.mse;
				std::string mseString = ss.str();

				if(lastM != res.first.first) {
					multTable.add_row(headerMult);
					lastM = res.first.first;
				}

				Row_t rowMult{ std::to_string(res.first.first), std::to_string(res.first.second), mseString, std::to_string(100*res.second.maxRelativeDeviation) };
				for(int i = 1; i < 33; ++i){
					rowMult.push_back(std::to_string(res.second.tralingSymbols[i]));
				}
				multTable.add_row(rowMult);
			}
			std::cout << multTable << std::endl;

		}

		if (exportResults) {

			exportResultsToFile();

		}
		
	}
	
	void calculateBiasForAddition() {

		size_t difference = this->maxInDataset - this->minInDataset;
		size_t log2 = std::log2(difference);
		this->bias = std::pow(2, log2+1) - this->minInDataset;

	}

	void runAddition() {

		size_t howManyPerThread = (this->actualSize / (100 / this->defaultTestSizePercent)) / this->trPool->threads;
		this->howManyToTest = howManyPerThread * this->trPool->threads;

		float meanFactor = 1.0f / (howManyPerThread * this->trPool->threads);

		calculateBiasForAddition();

		for (size_t i = 0; i < trPool->threads; ++i) {
			trPool->threadList[i] = std::thread(&Dataset::analyzeAddition, this, i, howManyPerThread, this->bias, meanFactor);
		};

		for (size_t i = 0; i < trPool->threads; ++i) {
			trPool->threadList[i].join();
		}

	}

	void analyzeAddition(size_t threadIdx, size_t length, float bias, float meanFactor) {

		std::vector<float> *threadSubset = &this->floatResults[threadIdx];

		float mse = 0;

		for (size_t i = 0; i < length; ++i) {

			float value = (*threadSubset)[i] + bias;
			mse += meanFactor * std::pow(value - bias - (*threadSubset)[i], 2);

			
		}
		std::lock_guard<std::mutex> lock(mtx);
		this->error += mse;
		
	}

	void runMultiplication() {
		
		for (auto m : this->MValues) {

			for (auto p : this->PValue) {

				multResults[coords(m, p)] = multResult();

			}

		}
		
		size_t howManyPerThread = (this->actualSize / (100 / this->defaultTestSizePercent)) / this->trPool->threads;
		this->howManyToTest = howManyPerThread * this->trPool->threads;
		float meanFactor = 1.0f / (howManyPerThread*this->trPool->threads);

		for (size_t i = 0; i < trPool->threads; ++i) {
			trPool->threadList[i] = std::thread(&Dataset::analyzeMultiplication, this, i, howManyPerThread, this->MValues, this->PValue, this->floatPatternMap, meanFactor);
		};

		for (size_t i = 0; i < trPool->threads; ++i) {
			trPool->threadList[i].join();
		}
	}

	void analyzeMultiplication(size_t threadIdx, size_t length, std::vector<size_t> MVals, std::vector<size_t> PVals, std::map<uint32_t, uint32_t> patterns, float meanFactor) {

		std::vector<float> *threadSubset = &this->floatResults[threadIdx];

		for (auto m : MVals) {

			for (auto p : PVals) {

				multResult localThreadResult;

				uint32_t patternPrep = 0xFFFFFFFF << p;

				uint32_t patternToEnforce = patterns[m] >> (32 - p);

				for (size_t i = 0; i < length; ++i) {

					if((*threadSubset)[i] != 0) {

						float value = (*threadSubset)[i];

						uint32_t* ptrUint = reinterpret_cast<uint32_t*>(&value);

						*ptrUint = (*ptrUint & patternPrep) | patternToEnforce;

						value *= m;

						this->countTrailingSymbols(&value, &localThreadResult.tralingSymbols);
						
						float deviation = value / m - (*threadSubset)[i];
						localThreadResult.mse += meanFactor * std::pow(deviation, 2);

						deviation = std::abs(deviation / (*threadSubset)[i]);

						if (localThreadResult.maxRelativeDeviation < deviation) {
							localThreadResult.maxRelativeDeviation = deviation;
						}

					}
					else {
						++localThreadResult.tralingSymbols[32];
					}
				}

				std::lock_guard<std::mutex> lock(mtx);
				std::vector<size_t> & globalTrailingSymbols = multResults[coords(m, p)].tralingSymbols;
				for (int i = 0; i < 33; ++i) {
					globalTrailingSymbols[i] += localThreadResult.tralingSymbols[i];
				}
				multResults[coords(m, p)].mse += localThreadResult.mse;
				if (multResults[coords(m, p)].maxRelativeDeviation < localThreadResult.maxRelativeDeviation) {
					multResults[coords(m, p)].maxRelativeDeviation = localThreadResult.maxRelativeDeviation;
				}
			}

		}
		

	}
	void parseHeaders() {
		char* last = file.charMap;
		char * start = file.charMap;
		std::vector<std::string> headers;
		while (*start != this->lineBreak) {

			if (*start == this->delimiter) {

				std::string header(last, start - last);
				headers.push_back(header);
				last = start + 1;

			}

			++start;

		}

		std::string header(last, start -1 - last);
		headers.push_back(header);
		this->file.charMap = start + 1;
		this->Headers = std::move(headers);
	}

	void guessDatasetSize(){
		size_t nextPosition = 0;
		strvw chunk(file.charMap, this->bytesToCheck);
		size_t lineCount = 0;

		while ((nextPosition = chunk.find(this->lineBreak, nextPosition)) != strvw::npos) {

			++nextPosition;
			++lineCount;

		}

		nextPosition = 0;
		strvw firstLineEnd = chunk.substr(0, chunk.find(this->lineBreak, nextPosition));
		size_t columns = 1;
		while ((nextPosition = firstLineEnd.find(this->delimiter, nextPosition)) != strvw::npos) {
			++nextPosition;
			++columns;
		}

		size_t bytesUsedOnDel = (columns - 1) * lineCount;
		size_t bytesUsedOnFloats = chunk.find_last_of(this->lineBreak) - bytesUsedOnDel - lineCount;

		size_t bytesPerFloat = bytesUsedOnFloats / (columns * lineCount);
		this->datasetSizeGuess = guessScaling*this->file.length / bytesPerFloat;
		amountOfColumns = columns;
	}

	/* Used after parsing headers */
	void guessDatasetSize(char* afterHeaders) {
		size_t nextPosition = 0;
		strvw chunk(afterHeaders, this->bytesToCheck);
		size_t lineCount = 0;

		while ((nextPosition = chunk.find(this->lineBreak, nextPosition)) != strvw::npos) {

			++nextPosition;
			++lineCount;

		}

		nextPosition = 0;
		strvw firstLineEnd = chunk.substr(0, chunk.find(this->lineBreak, nextPosition));
		size_t columns = 1;
		while ((nextPosition = firstLineEnd.find(this->delimiter, nextPosition)) != strvw::npos) {
			++nextPosition;
			++columns;
		}

		size_t bytesUsedOnDel = (columns - 1) * lineCount;
		size_t bytesUsedOnFloats = chunk.find_last_of(this->lineBreak) - bytesUsedOnDel - lineCount;

		size_t bytesPerFloat = bytesUsedOnFloats / (columns * lineCount);
		this->datasetSizeGuess = guessScaling * this->file.length / bytesPerFloat;
		amountOfColumns = columns;
	}

	void castFloats(const char* start, const char* end, size_t threadIdx) {
		std::vector<float> floatResult(this->datasetSizeGuess/this->trPool->threads);
		
		float max = std::numeric_limits<float>::min();
		float min = std::numeric_limits<float>::max();
		
		size_t counter = 0;

		from_chars_result result;
		result.ptr = start-1;

		while (++result.ptr < end){

			result = from_chars(result.ptr, start + (start - end), floatResult[counter]);
			if(result.ec == std::errc::invalid_argument){
				
			}
			else {

				if (floatResult[counter] > max) {
					max = floatResult[counter];
				}
				if (floatResult[counter] < min) {
					min = floatResult[counter];
				}

				++counter;
			}
		}
		
		floatResult.resize(counter);
		this->floatResults[threadIdx] = std::move(floatResult);
		std::lock_guard<std::mutex> lock(mtx);
		actualSize += counter;

		if (max > maxInDataset) {
			maxInDataset = max;
		}
		if (min < minInDataset) {
			minInDataset = min;
		}

	}

	void startCastingProcess() {

		size_t chunkSize = file.length / trPool->threads;
		
		const char* start = file.charMap;


		std::vector<const char*> startingPoints(trPool->threads);
		std::vector<const char*> endingPoints(trPool->threads);
		
		for (size_t i = 0; i < trPool->threads; ++i) {
			
			strvw chunk(start, chunkSize);
			size_t position = chunk.find_last_of(this->lineBreak);
			startingPoints[i] = start;
			endingPoints[i] = start + position;
			start += ++position;
		}

		endingPoints[trPool->threads -1] = file.charMap + file.length;

		this->floatResults.resize(trPool->threads);

		for (int i = 0; i < trPool->threads; ++i) {
			
			trPool->threadList[i] = std::thread(&Dataset::castFloats, this, startingPoints[i], endingPoints[i], i);

		}

		for (int i = 0; i < trPool->threads; ++i) {
			trPool->threadList[i].join();
		}

	}

	void runPowersOfFive() {

		Trailing5.resize(33);
		Trailing25.resize(33);
		Trailing125.resize(33);

		size_t howmany = (this->actualSize / (100/defaultTestSizePercent))/trPool->threads;
		this->howManyToTest = howmany * trPool->threads;

		for (int i = 0; i < trPool->threads; ++i) {
			trPool->threadList[i] = std::thread(&Dataset::analyzePowersOfFive, this, howmany, i);
		}
		
		for (int i = 0; i < trPool->threads; ++i) {
			trPool->threadList[i].join();
		}
		
	}

	void analyzePowersOfFive(size_t howMany, int threadId) {
		
		std::vector<size_t> trailingSymbols5(33);
		std::vector<size_t> trailingSymbols25(33);
		std::vector<size_t> trailingSymbols125(33);

		std::vector<float> * fvec = &this->floatResults[threadId];
		
		for (int i=0; i < howMany; ++i){
			
			float value = 5*(*fvec)[i];	
			countTrailingSymbolsForPo5(&value, &trailingSymbols5);

			value = 25.0f * (*fvec)[i];
			countTrailingSymbolsForPo5(&value, &trailingSymbols25);

			value = 125.0f * (*fvec)[i];
			countTrailingSymbolsForPo5(&value, &trailingSymbols125);

		}
		
		std::lock_guard<std::mutex> lock(mtx);
		for (int i = 0; i < 33; ++i) {
			Trailing5[i] += trailingSymbols5[i];
			Trailing25[i] += trailingSymbols25[i];
			Trailing125[i] += trailingSymbols125[i];
		}
	
	}



	inline void countZeroes(float* value, std::vector<size_t>* whereToStore) {
		uint32_t* floatAsInt = reinterpret_cast<uint32_t*>(value);
		std::bitset<32> bit(*floatAsInt);
		short zeroes = 0;
		while (bit[zeroes] == 0 && zeroes < 32) {
			++zeroes;
		}
		(*whereToStore)[zeroes] += 1;
	}

	inline void countTrailingSymbolsForPo5(float* value, std::vector<size_t>* whereToStore) {
		uint32_t* floatAsInt = reinterpret_cast<uint32_t*>(value);
		std::bitset<32> bit(*floatAsInt);
		if (bit[0] != bit[1]) {
			bit[0].flip();
		}
		short trailingSymbolCounter = 1;
		bool LastSymbol = bit[0];

		while (bit[trailingSymbolCounter] == LastSymbol && trailingSymbolCounter < 32) {
			++trailingSymbolCounter;
		}
		
		(*whereToStore)[trailingSymbolCounter] += 1;
	}

	inline void countTrailingSymbols(float* value, std::vector<size_t>* whereToStore) {
		uint32_t* floatAsInt = reinterpret_cast<uint32_t*>(value);
		std::bitset<32> bit(*floatAsInt);
		short trailingSymbolCounter = 1;
		bool LastSymbol = bit[0];

		while (bit[trailingSymbolCounter] == LastSymbol && trailingSymbolCounter < 32) {
			++trailingSymbolCounter;
		}
		
		(*whereToStore)[trailingSymbolCounter] += 1;
	}

	
	void masterPerformAddition() {

		for(size_t i = 0; i < trPool->threads; ++i){
			trPool->threadList[i] = std::thread(&Dataset::slavePerformAddition, this, this->bias, i);
		}

		for(size_t i=0; i < trPool->threads; ++i){
			trPool->threadList[i].join();
		}

	}

	void slavePerformAddition(float bias, size_t threadIdx) {

		for (auto&f: this->floatResults[threadIdx]) {
			f += bias;
		}

	}

	void masterPerformMultiplication() {

		size_t M = this->finalM;

		for (size_t i = 0; i < trPool->threads; ++i) {
			trPool->threadList[i] = std::thread(&Dataset::slavePerformMultiplication, this, i, M, this->finalP, this->floatPatternMap[M]);
		}

		for (size_t i = 0; i < trPool->threads; ++i) {
			trPool->threadList[i].join();
		}

	}

	

	void slavePerformMultiplication(size_t threadIdx, size_t M, size_t P, uint32_t pattern) {

		float m = static_cast<float>(M);
		
		uint32_t patternPrep = 0xFFFFFFFF << P;

		uint32_t patternToEnforce = pattern >> (32 - P);


		for (auto& f : this->floatResults[threadIdx]) {

			if (f != 0) {

				uint32_t* ptr = reinterpret_cast<uint32_t*>(&f);
				*ptr = (*ptr & patternPrep) | patternToEnforce;
				f *= m;
			}

		}

	}

	void masterPerformPowersOfFive() {

		for (size_t i = 0; i < trPool->threads; ++i) {
			trPool->threadList[i] = std::thread(&Dataset::slavePerformPowersOfFive, this, i, this->finalPoFive);
		}

		for (size_t i = 0; i < trPool->threads; ++i) {
			trPool->threadList[i].join();
		}

	}

	void slavePerformPowersOfFive(size_t threadIdx, float multiplier) {

		for(auto & f: this->floatResults[threadIdx]){
			f *= multiplier;

			uint32_t* floatAsInt = reinterpret_cast<uint32_t*>(&f);
			std::bitset<32> bit(*floatAsInt);
			if (bit[0] != bit[1]) {
				bit[0].flip();
			}


		}

	}
	void exportPreprocessedFile() {
		std::string fileName = "preprocessed_output.csv";
		std::ofstream file(fileName);

		if (!file.is_open()) {
			std::cerr << "Unable to open file" << std::endl;
			return;
		}
		std::cout << "\nExporting preprocessed file to " << fileName << "\n - Progress - " << std::endl;
		// Construct header row
		std::string headerRow;
		for (int i = 0; i < this->amountOfColumns - 1; ++i) {
			headerRow += this->Headers[i] + ",";
		}
		headerRow += this->Headers[this->amountOfColumns - 1];
		file << headerRow << "\n";

		float vectors = static_cast<float>(this->floatResults.size());
		
		float progressCounter = 0.0f;
		
		printProgressBar(0.0f);
		//main loop
		for (const auto vec : this->floatResults) {
			
			std::ostringstream row;

			int counter = 1;

			for (const auto f : vec) {

				if (counter == this->amountOfColumns) {
					
					row << std::setprecision(32) << f << "\n";
					file << row.str();

					counter = 1;
					row.str("");
					
					
				}
				else {
					row << std::setprecision(32) << f << ",";
					++counter;
				}
				
			}
			progressCounter += 1.0f;
			float progress = progressCounter / vectors;
			
			printProgressBar(progress);

		}

		file.close();
	}
	void printProgressBar(float progress) {
		int barWidth = 70;
		std::cout << "[";
		int pos = barWidth * progress;
		for (int i = 0; i < barWidth; ++i) {
			if (i < pos) std::cout << "=";
			else if (i == pos) std::cout << ">";
			else std::cout << " ";
		}
		std::cout << "] " << int(progress * 100.0) << " %\r";
		std::cout.flush();
	}
	void exportResultsToFile() {

		
		std::string reportDir = "reports/" + this->filename;
		if (this->exportFilename != "free.lunch") {
			reportDir = "reports/" + this->exportFilename;
		}
		namespace fs = std::filesystem;
		fs::path dirPath = "reports";

		if (!fs::exists(dirPath)) {
			fs::create_directory(dirPath);
		}
		dirPath = reportDir;
		if (!fs::exists(dirPath)) {
			fs::create_directory(dirPath);
		}

		std::string addFilename = reportDir + "/addition.csv";
		std::string multFilename = reportDir + "/multiplication.csv";
		std::string po5Filename = reportDir + "/powersOfFive.csv";

		
		std::ofstream addFile(addFilename);

		addFile << "Param,Value\n";
		addFile << "Error," << this->error << "\n";
		addFile << "Bias," << this->bias << "\n";
		addFile << "Max," << this->maxInDataset << "\n";
		addFile << "Min," << this->minInDataset;
		addFile.close();

		std::ofstream multFile(multFilename);

		std::string header = "M,P,MSE,Max%dev";
		for (int i = 1; i < 33; ++i) {
			header += "," + std::to_string(i);
		}
		multFile << header << "\n";
		
		for (auto mu : this->multResults) {

			multFile << mu.first.first << "," << mu.first.second << "," << std::scientific << mu.second.mse << "," << mu.second.maxRelativeDeviation;

			std::string row = "";
			for (int i = 1; i < 33; ++i) {
				row += "," + std::to_string(mu.second.tralingSymbols[i]);
			}
			multFile << row << "\n";
		}

		multFile.close();

		std::ofstream po5File(po5Filename);

		header = "Power/Trailing";
		for (int i = 1; i < 33; ++i) {
			header += "," + std::to_string(i);
		}
		po5File << header << "\n";

		po5File << "1";
		for (int i = 1; i < 33; ++i) {
			po5File << "," << this->Trailing5[i];
		}
		po5File << "\n2";
		
		for (int i = 1; i < 33; ++i) {
			po5File << "," << this->Trailing25[i];
		}
		po5File << "\n3";
		for (int i = 1; i < 33; ++i) {
			po5File << "," << this->Trailing125[i];
		}

		po5File.close();

	}
	
};

#endif // !FUNCTIONS_H
