#include "AstPrinter.h"
#include "Parser.h"
#include "Scanner.h"
#include "TokenType.h"

std::string readFromInput(std::istream &source) {
    return std::string(std::istreambuf_iterator<char>(source), std::istreambuf_iterator<char>());
}

int main(int argc, char *argv[]) {
    std::string input = readFromInput(std::cin);
    Scanner scanner(input);

    const TokenVec &tokens = scanner.scanTokens();
    Parser parser(tokens);
    auto parseOut = parser.parse();

    // if (hadError()) {
    //     exit(-1);
    // }

    ordered_json out;
    out["lexical"] = scanner.output();
    out["syntactical"] = (!parseOut) ? ordered_json() : parseOut->toJson();
    out["errors"] = errorJson();

    std::cout << out.dump(1); // remove yung padding para mas mabilis yung parsing
}
