#include <iostream>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;
int main(int argc, char** argv)
{
    if (fs::current_path().stem() != "Spelunky 2")
    {
        const fs::path exe_path{fs::path{argv[0]}.parent_path()};
        if (exe_path.stem() != "Spelunky 2")
        {
            std::cout << "Not running from game folder, please move this executable to the game folder...\n";
            std::system("pause");
            return 1;
        }
        fs::current_path(exe_path);
    }
    std::cout << "What mod would you like to delete? (case sensitive)\n";
    std::string deletedMod;
    getline(std::cin, deletedMod);
    if (deletedMod == ".db" || deletedMod == "load_order.txt")
    {
        std::cout << "Can't delete those files!";
        return 2;
    }
    deletedMod = "Mods/Packs/" + deletedMod;
    fs::remove_all(deletedMod);
    std::cout << deletedMod << std::endl; 
}