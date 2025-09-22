#include <iostream>
#include "tomlparser.h"

int main() {
    SimpleTomlParser parser;
    auto config = parser.parse("lmstudio_config.toml");

    std::cout << "Parsed TOML configuration:" << std::endl;
    std::cout << "==========================" << std::endl;

    for (auto it = config.begin(); it != config.end(); ++it) {
        std::cout << "Key: " << it.key().toStdString() << std::endl;
        std::cout << "Value: " << it.value().left(100).toStdString();
        if (it.value().length() > 100) {
            std::cout << "... (truncated)";
        }
        std::cout << std::endl << std::endl;
    }

    std::cout << "\nSpecific checks:" << std::endl;
    std::cout << "Has prompts.keywords? " << (config.contains("prompts.keywords") ? "YES" : "NO") << std::endl;
    std::cout << "Has prompts.summary? " << (config.contains("prompts.summary") ? "YES" : "NO") << std::endl;

    return 0;
}