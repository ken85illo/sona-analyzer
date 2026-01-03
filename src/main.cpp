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
    scanner.output();
    std::cout << "\n\n";

    Parser parser(tokens);
    Ref<Expr> expression = parser.parse();

    if (hadError()) {
        exit(-1);
    }

    std::cout << MakeRef<AstPrinter>()->print(expression);
}
