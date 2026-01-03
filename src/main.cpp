#include "AstPrinter.h"
#include "Scanner.h"
#include "TokenType.h"

std::string readFromInput(std::istream &source) {
    return std::string(std::istreambuf_iterator<char>(source), std::istreambuf_iterator<char>());
}

int main(int argc, char *argv[]) {
    // std::string input = readFromInput(std::cin);
    // Scanner scanner(input);
    // scanner.scanTokens();
    // scanner.output();

    Ref<BinaryExpr> expression = MakeRef<BinaryExpr>(
        MakeRef<UnaryExpr>(MakeRef<DefToken>(SUBTRACT_OP, "-", 1), MakeRef<LiteralExpr>(MakeRef<std::string>("123"))),
        MakeRef<DefToken>(MULTIPLY_OP, "*", 1),
        MakeRef<GroupingExpr>(MakeRef<LiteralExpr>(MakeRef<std::string>("45.67")))
    );

    std::cout << MakeRef<AstPrinter>()->print(expression);
}
