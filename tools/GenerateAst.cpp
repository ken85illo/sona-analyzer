#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

using Str = std::string;
using StrRef = const Str &;
using namespace std::literals::string_literals;

Str trim(Str &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    s.erase(
        std::find_if(
            s.rbegin(), s.rend(),
            [](unsigned char ch) {
        return !std::isspace(ch);
    }
        ).base(),
        s.end()
    );

    return s;
}

std::vector<Str> split(StrRef s, char delim) {
    std::vector<Str> strList;
    std::stringstream str(s);
    Str token;

    while (getline(str, token, delim)) {
        strList.push_back(token);
    }

    return strList;
}

void defineType(std::ofstream &file, StrRef baseName, StrRef className, StrRef fieldList) {
    file << "class " << className << baseName << " : public " << baseName << "{\n";
    file << "public:\n";

    auto fields = split(fieldList, ',');
    for (auto &field: fields) {
        trim(field);
    }

    file << "    " << className << baseName << "(";
    for (int i = 0; i < fields.size(); i++) {
        auto strList = split(fields[i], ' ');
        auto type = trim(strList[0]);
        auto name = trim(strList[1]);

        file << "const Ref<" << type << "> " << name;

        if (i < fields.size() - 1) {
            file << ", ";
        }
    }
    file << ")\n";
    file << "    " << ": ";

    for (int i = 0; i < fields.size(); i++) {
        auto strList = split(fields[i], ' ');
        auto name = trim(strList[1]);

        file << name << "(" << name << ")";

        if (i < fields.size() - 1) {
            file << ", ";
        }
    }
    file << " {}\n\n";

    file << "    std::string accept(Visitor& visitor) override {\n";
    file << "        return visitor.visit" << className << baseName << "(*this);\n";
    file << "    }\n\n";

    for (auto field: fields) {
        auto strList = split(field, ' ');
        auto type = trim(strList[0]);
        auto name = trim(strList[1]);

        file << "    const Ref<" << type << "> " << name << ";\n";
    }

    file << "};\n\n";
}

void defineVisitor(std::ofstream &file, StrRef baseName, const std::vector<Str> &types) {
    file << "class Visitor {\n";
    file << "public:\n";

    for (auto type: types) {
        auto typeName = trim(split(type, '=')[0]);
        auto lowerBaseName = baseName;
        std::transform(lowerBaseName.begin(), lowerBaseName.end(), lowerBaseName.begin(), [](unsigned char c) {
            return std::tolower(c);
        });

        file << "    virtual std::string visit" << typeName << baseName << "(";
        file << "const " << typeName << baseName << "& " << lowerBaseName << ") = 0;\n";
    }

    file << "    virtual ~Visitor() = default;\n";
    file << "};\n\n";
}

void defineAst(StrRef outputDir, StrRef baseName, const std::vector<Str> &types) {
    std::filesystem::path path = outputDir;
    path /= (baseName + ".h"s);

    std::ofstream file(path);

    file << "#pragma once" << "\n";
    file << "#include \"Ref.h\"" << "\n";
    file << "#include \"Token.h\"" << "\n\n";

    for (auto type: types) {
        auto typeName = trim(split(type, '=')[0]);
        file << "class " << typeName << baseName << ";\n";
    }
    file << "class Visitor;\n\n";

    file << "class " << baseName << "{\n";
    file << "public:\n";
    file << "    virtual std::string accept(Visitor& visitor) = 0;\n";
    file << "    virtual ~" << baseName << "() = default;\n";
    file << "};\n\n";

    defineVisitor(file, baseName, types);

    // AST Classes
    for (auto type: types) {
        auto strList = split(type, '=');
        Str className = trim(strList[0]);
        Str fields = trim(strList[1]);
        defineType(file, baseName, className, fields);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cout << "Usage: generate_ast <output_directory>\n";
        exit(1);
    }

    Str outputDir = argv[1];

    std::vector<Str> strList;
    std::ifstream file("grammar.txt");
    Str str;

    Str baseName;
    std::getline(file, baseName); // first line
    trim(baseName);
    std::cout << "Base Name: " << baseName << "\n";

    int i = 0;
    while (std::getline(file, str)) {
        strList.push_back(str);
        std::cout << "[" << i++ << "]: " << str << "\n";
    }

    defineAst(outputDir, baseName, strList);
}
