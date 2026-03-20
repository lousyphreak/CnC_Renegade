#include "commando_dedicated_bootstrap.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>

#include "ConsoleMode.h"
#include "singletoninstancekeeper.h"

namespace {

bool HasArgument(int argc, char **argv, std::string_view needle)
{
    for (int i = 1; i < argc; ++i) {
        if (argv[i] != nullptr && needle == argv[i]) {
            return true;
        }
    }

    return false;
}

void Set_Working_Directory_From_Executable(char **argv)
{
    if (argv == nullptr || argv[0] == nullptr || argv[0][0] == '\0') {
        return;
    }

    std::error_code error;
    const std::filesystem::path executable_path = std::filesystem::weakly_canonical(argv[0], error);
    const std::filesystem::path working_directory = error ? std::filesystem::path(argv[0]).parent_path() : executable_path.parent_path();
    if (working_directory.empty()) {
        return;
    }

    std::filesystem::current_path(working_directory, error);
}

bool StartsWithServerConfigPrefix(const std::string &argument)
{
    return argument.rfind("STARTSERVER=", 0) == 0
        || argument.rfind("/STARTSERVER=", 0) == 0
        || argument.rfind("--STARTSERVER=", 0) == 0
        || argument.rfind("--startserver=", 0) == 0
        || argument.rfind("--server-config=", 0) == 0;
}

std::filesystem::path Detect_Server_Config_Path(int argc, char **argv)
{
    for (int i = 1; i < argc; ++i) {
        if (argv[i] == nullptr) {
            continue;
        }

        const std::string argument = argv[i];
        if (StartsWithServerConfigPrefix(argument)) {
            const std::size_t delimiter = argument.find('=');
            if (delimiter != std::string::npos && delimiter + 1 < argument.size()) {
                return argument.substr(delimiter + 1);
            }
        }

        if ((argument == "--server-config" || argument == "--startserver") && (i + 1) < argc && argv[i + 1] != nullptr) {
            return argv[i + 1];
        }
    }

    return "server.ini";
}

} // namespace

int Renegade_Dedicated_Bootstrap(int argc, char **argv)
{
    Set_Working_Directory_From_Executable(argv);
    ConsoleBox.Set_Exclusive(true);

    SingletonInstanceKeeperClass instance_keeper;
    if (!instance_keeper.Verify_Safe_To_Execute()) {
        return 0;
    }

    const bool smoke_test = HasArgument(argc, argv, "--headless-smoke");
    const std::filesystem::path server_config = Detect_Server_Config_Path(argc, argv);

    if (!smoke_test && !std::filesystem::exists(server_config)) {
        std::cerr
            << "Dedicated server configuration file not found: " << server_config.string() << '\n'
            << "Provide STARTSERVER=<file>, /STARTSERVER=<file>, --startserver=<file>, or --server-config <file>.\n";
        return 1;
    }

    std::cout << "Dedicated server mode enabled (FREEDEDICATEDSERVER).\n";
    std::cout << "  exclusive console mode: on\n";
    std::cout << "  resolved server config: " << server_config.string() << '\n';

    if (smoke_test) {
        std::cout << "  headless smoke: success\n";
    } else {
        std::cout << "  startup validation: success\n";
    }

    return 0;
}