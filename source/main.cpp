#include "freedom_db.h"

#include <cxxopts.hpp>

int main(int argc, const char **argv)
{
    cxxopts::Options options("freedomdb-tool", 
        "Blockchain powered distributed PostgreSQL database");
    options.positional_help("[optional args]");
    options.show_positional_help();
        
    options.set_width(70)
      .set_tab_expansion()
      .allow_unrecognised_options()
      .add_options()
        ("h,help", "Show Help")
        ("s,server", "Server mode")
        ("i,interactive", "Interactive shell mode")
        ("p,port", "Select P2P network port", cxxopts::value<int>())
        ("n,netid", "Select Network ID", cxxopts::value<int>())
        ("color", "Use color printing", cxxopts::value<bool>()
            ->default_value("true")->implicit_value("false"))
        ("v,verbose", "Verbose Level", cxxopts::value<std::string>()
            ->default_value("INFO")->implicit_value("DEBUG"));

    auto parsed = options.parse(argc, argv);
   
    if (parsed.count("help"))
    {
        std::cout << options.help({"", "Group"}) << std::endl;
        exit(0);
    }

    if (parsed.count("color"))
        Log::mColored = parsed["color"].as<bool>();
    if (parsed.count("verbose")) {
        auto mod = magic_enum::enum_cast<Log::Level>(parsed["v"].as<std::string>());
        if (mod) {
            Log::mVerboseLevel = *mod;
            Log::d(Log::Type::CMD, "Changed verbose to {}", parsed["v"].as<std::string>());
        }
    }

    //Log::e(Log::Type::CMD, "Cactus {}", parsed["v"].as<int>());

    FreedomDB fdb;
    return 0;
}
