#include "cli.h"
#include "command_manager.h"
#include "storage.h"

#include <filesystem>
#include <iostream>
#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {

void printUsage() {
    std::cout << "用法：tcm_prescription.exe [--data-dir 路径] [--reset-demo]\n";
    std::cout << "  --data-dir   指定业务数据目录，默认 data\n";
    std::cout << "  --reset-demo 启动前从 data/seed 恢复演示数据\n";
    std::cout << "  --help       显示本帮助\n";
}

bool askYes(const std::string& prompt) {
    std::cout << prompt << " [y/N]：";
    std::string line;
    if (!std::getline(std::cin, line)) {
        return false;
    }
    return line == "y" || line == "Y" || line == "yes" || line == "YES";
}

}  // namespace

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    std::filesystem::path dataDir = "data";
    bool resetDemo = false;
    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--help" || argument == "-h") {
            printUsage();
            return 0;
        }
        if (argument == "--reset-demo") {
            resetDemo = true;
            continue;
        }
        if (argument == "--data-dir" && i + 1 < argc) {
            dataDir = argv[++i];
            continue;
        }
        std::cout << "未知或缺少参数的选项：" << argument << "\n";
        printUsage();
        return 2;
    }

    tcm::Storage storage(dataDir);
    std::string error;
    if (!storage.initialize(error)) {
        std::cout << "初始化失败：" << error << "\n";
        return 1;
    }
    if (resetDemo) {
        if (!storage.resetToSeed(error)) {
            std::cout << "恢复演示数据失败：" << error << "\n";
            return 1;
        }
    }

    tcm::AppState state;
    if (!storage.load(state, error)) {
        std::cout << "数据加载失败：" << error << "\n";
        if (!askYes("是否从种子数据恢复演示数据")) {
            return 1;
        }
        if (!storage.resetToSeed(error) || !storage.load(state, error)) {
            std::cout << "恢复失败：" << error << "\n";
            return 1;
        }
        std::cout << "演示数据已恢复。\n";
    }

    tcm::CommandManager commands(
        [&storage](const tcm::AppState& current, std::string& saveError) {
            return storage.save(current, saveError);
        });
    tcm::Cli cli(state, storage, commands);
    return cli.run();
}
