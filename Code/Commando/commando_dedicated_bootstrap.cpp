#include "commando_dedicated_bootstrap.h"

#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

#include "ConsoleMode.h"
#include "mainloop.h"
#include "ServerSettings.h"
#include "singletoninstancekeeper.h"
#include "slavemaster.h"
#include "useroptions.h"
#include "../wwlib/osdep.h"

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

std::string Detect_Server_Config_Path(int argc, char **argv)
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

std::string Build_Command_Line(int argc, char **argv)
{
    std::ostringstream command_line;

    for (int i = 1; i < argc; ++i) {
        if (argv[i] == nullptr || argv[i][0] == '\0') {
            continue;
        }

        if (command_line.tellp() > 0) {
            command_line << ' ';
        }

        command_line << argv[i];
    }

    return command_line.str();
}

void Apply_Dedicated_Defaults()
{
    ConsoleBox.Set_Exclusive(true);

    if (!SlaveMaster.Am_I_Slave() && !ServerSettingsClass::Is_Server_Settings_File_Set()) {
        char default_settings[] = "STARTSERVER=server.ini";
        cUserOptions::Set_Server_INI_File(default_settings);
    }
}

} // namespace

int Renegade_Dedicated_Bootstrap(int argc, char **argv)
{
    Set_Working_Directory_From_Executable(argv);

    const std::string command_line = Build_Command_Line(argc, argv);
    if (!cUserOptions::Parse_Command_Line(command_line.c_str())) {
        return 0;
    }

    Apply_Dedicated_Defaults();

    SingletonInstanceKeeperClass instance_keeper;
    if (!instance_keeper.Verify_Safe_To_Execute()) {
        return 0;
    }

    const bool smoke_test = HasArgument(argc, argv, "--headless-smoke");
    const std::string server_config = ServerSettingsClass::Is_Server_Settings_File_Set()
        ? std::string(ServerSettingsClass::Get_Settings_File_Name())
        : Detect_Server_Config_Path(argc, argv);

    if (!smoke_test && !renegade_osdep::Path_Exists(server_config)) {
        std::cerr
            << "Dedicated server configuration file not found: " << server_config << '\n'
            << "Provide STARTSERVER=<file>, /STARTSERVER=<file>, --startserver=<file>, or --server-config <file>.\n";
        return 1;
    }

    std::cout << "Dedicated server mode enabled (FREEDEDICATEDSERVER).\n";
    std::cout << "  exclusive console mode: on\n";
    std::cout << "  resolved server config: " << server_config << '\n';

    if (smoke_test) {
        if (ServerSettingsClass::Is_Server_Settings_File_Set() && !ServerSettingsClass::Parse(false)) {
            std::cerr << "  startup validation: failed\n";
            return 1;
        }

        std::cout << "  headless smoke: success\n";
        return 0;
    }

    return Game_Main_Loop();
}
