#include "AstPrinter.h"
#include "Parser.h"
#include "Scanner.h"
#include "TokenType.h"

std::string readFromInput(std::istream &source) {
    std::string result = std::string(std::istreambuf_iterator<char>(source), std::istreambuf_iterator<char>());
    return result;
}

int main(int argc, char *argv[]) {
    std::string input = readFromInput(std::cin);

    Scanner scanner(input);

    const TokenVec &tokens = scanner.scanTokens();
    Parser parser(tokens);
    auto parseOut = parser.parse();

    ordered_json out;
    out["lexical"] = scanner.output();
    out["syntactical"] = (!parseOut) ? ordered_json() : parseOut->toJson();
    out["errors"] = errorJson();

    std::cout << out.dump();
}
