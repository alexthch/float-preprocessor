#include "main.h"

int main(int argc, char** argv) {
	
	//std::cout << "Float Preprocessor\nversion: " << version << std::endl;

	cli::Parser parser(argc, argv);
	configureParser(parser);
	parser.run_and_exit_if_error();

	runParser(parser);

#ifdef TESTING
	std::atexit([]() {
		MemoryCounter::printCounts();
		});
#endif
	return 0;
}