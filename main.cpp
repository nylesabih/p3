// Project Identifier: 01BD41C3BF016AD7E8B6F837DF18926EC3251350
#include "logmanager.h"
#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

    if (argc != 3) {
        std::cerr << "Usage: ./logman --file <filename>\n";
        return 1;
    }

    std::string arg = argv[1];
    if (arg == "--help" || arg == "-h") {
        std::cout << "Usage: ./logman --file <filename>\n";
        return 0;
    }

    if (arg == "--file" || arg == "-f") {
        std::string filename = argv[2];
        LogManager manager;
        manager.loadLogFile(filename);
        manager.runInteractive();
    } else {
        std::cerr << "Invalid argument.\n";
        return 1;
    }

    return 0;
}