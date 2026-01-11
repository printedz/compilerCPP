#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <cstdlib>
#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include "ast_printer.h"

void printUsage() {
    std::cout << "Uso: compiler [opciones] <archivo.c>\n";
    std::cout << "Opciones:\n";
    std::cout << "  --lex      Detenerse después del análisis léxico\n";
    std::cout << "  --parse    Detenerse después del análisis sintáctico\n";
    std::cout << "  --codegen  Detenerse después de la generación de código\n";
}

std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("No se pudo abrir el archivo: " + path);
    }
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

void writeFile(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("No se pudo escribir en el archivo: " + path);
    }
    file << content;
}

void createExecutable(const std::string& sourceFile) {
    std::filesystem::path p(sourceFile);
    std::string baseName = p.stem().string();
    std::string assemblyFile = baseName + ".s";
    std::string outputFile = baseName;

#if defined(__APPLE__)
    std::string command = "gcc -arch x86_64 " + assemblyFile + " -o " + outputFile;
#else
    std::string command = "gcc " + assemblyFile + " -o " + outputFile;
#endif

    std::cout << "Ejecutando linker: " << command << std::endl;
    int exitCode = std::system(command.c_str());

    if (exitCode != 0) {
        std::cerr << "Error: GCC falló con código " << exitCode << std::endl;
        std::exit(1);
    } else {
        if (std::filesystem::exists(outputFile)) {
            std::cout << "Ejecutable creado exitosamente: " << outputFile << std::endl;
        } else {
            std::cerr << "Error: GCC terminó bien pero no veo el archivo " << outputFile << std::endl;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return 0;
    }

    std::string fileName;
    bool lexOnly = false;
    bool parseOnly = false;
    bool codegenOnly = false;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--lex") lexOnly = true;
        else if (arg == "--parse") parseOnly = true;
        else if (arg == "--codegen") codegenOnly = true;
        else if (arg.starts_with("-")) {
            std::cerr << "Error: Opción desconocida " << arg << std::endl;
            return 1;
        } else {
            fileName = arg;
        }
    }

    if (fileName.empty()) {
        std::cerr << "Error: No se proporcionó un archivo de entrada." << std::endl;
        return 1;
    }

    try {
        std::string content = readFile(fileName);

        // 1. Fase de Lexer
        std::vector<Token> tokens = Lexer::tokenize(content);
        if (lexOnly) {
            for (const auto& t : tokens) {
                std::cout << static_cast<int>(t.type) << " : " << t.value << std::endl;
            }
            return 0;
        }

        // 2. Fase de Parser
        Parser parser(tokens);
        auto ast = parser.parseProgram();

        if (parseOnly) {
            std::cout << ASTPrinter::print(*ast) << std::endl;
            return 0;
        }

        // 3. Fase de Generación de Código
        std::string assembly = CodeGenerator::generate(*ast);
        if (codegenOnly) {
            std::cout << assembly << std::endl;
            return 0;
        }

        std::filesystem::path p(fileName);
        std::string assemblyFileName = p.stem().string() + ".s";
        writeFile(assemblyFileName, assembly);

        // 4. Linker
        createExecutable(fileName);

    } catch (const std::exception& e) {
        std::cerr << "Error durante la compilación: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
