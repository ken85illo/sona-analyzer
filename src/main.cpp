#include "Scanner.h"
#include <fstream>
#include <iostream>

std::string readFromFile(std::ifstream &source) {
    return std::string(std::istreambuf_iterator<char>(source), std::istreambuf_iterator<char>());
}

int main(int argc, char *argv[]) {
    if (argc <= 0) {
        std::cout << "Please provide a source file!\n";
        std::cout << "Usage: sona_lexical_analyzer <source_file_path>\n";
        return -1;
    }

    std::ifstream source(argv[1]);

    if (!source.is_open()) {
        std::cout << "Source File doesn't exist!\n";
        return -1;
    }

    std::string input = readFromFile(source);
    Scanner scanner(input);
    scanner.scanTokens();
    scanner.outputToFile("tokens.csv");
}
