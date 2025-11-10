#include "Scanner.h"

std::string readFromInput(std::istream &source) {
    return std::string(std::istreambuf_iterator<char>(source), std::istreambuf_iterator<char>());
}

int main(int argc, char *argv[]) {
    std::string input = readFromInput(std::cin);
    Scanner scanner(input);
    scanner.scanTokens();
    scanner.output();
}
