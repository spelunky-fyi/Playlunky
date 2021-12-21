#include "save_game.h"

#include "log.h"
#include "virtual_filesystem.h"

#include <cstddef>
#include <string>

static VirtualFilesystem* s_SaveGameVfs{ nullptr };
void SetSaveGameVfs(VirtualFilesystem* vfs)
{
    s_SaveGameVfs = vfs;
}

void OnReadFromFile(SaveGameMod mod_type, const char* save_game_sav, void** out_buf, size_t* out_size, OnReadFromFileOrig* orig)
{
    auto load_from_separate = [&]()
    {
        using namespace std::string_literals;
        const std::string sav_replacement_str = save_game_sav + ".pl"s;
        orig(sav_replacement_str.c_str(), out_buf, out_size);
    };
    auto try_load_from_mod = [&]()
    {
        if (const auto sav_replacement = s_SaveGameVfs->GetDifferentFilePath(save_game_sav))
        {
            const std::string sav_replacement_str = sav_replacement.value().string();
            orig(sav_replacement_str.c_str(), out_buf, out_size);
            return true;
        }
        return false;
    };

    switch (mod_type)
    {
    case SaveGameMod::Block:
        return;
    case SaveGameMod::SeparateSave:
        load_from_separate();
        return;
    case SaveGameMod::FromMod:
        if (!try_load_from_mod())
        {
            orig(save_game_sav, out_buf, out_size);
        }
        return;
    case SaveGameMod::SeparateSaveOrFromMod:
        if (!try_load_from_mod())
        {
            load_from_separate();
        }
        return;
    }
}

void OnWriteToFile(SaveGameMod mod_type, [[maybe_unused]] const char* save_game_bak, const char* save_game_sav, void* buf, size_t size, OnWriteToFileOrig* orig)
{
    auto save_to_separate = [&]()
    {
        using namespace std::string_literals;
        const std::string bak_replacement_str = save_game_bak + ".pl"s;
        const std::string sav_replacement_str = save_game_sav + ".pl"s;
        orig(bak_replacement_str.c_str(), sav_replacement_str.c_str(), buf, size);
    };
    auto try_save_to_mod = [&]()
    {
        if (const auto sav_replacement = s_SaveGameVfs->GetDifferentFilePath(save_game_sav))
        {
            const std::string sav_replacement_str = sav_replacement.value().string();
            const auto bak_replacement_str = std::filesystem::path{ sav_replacement.value() }.replace_extension(".bak").string();
            orig(bak_replacement_str.c_str(), sav_replacement_str.c_str(), buf, size);
            return true;
        }
        return false;
    };

    switch (mod_type)
    {
    case SaveGameMod::Block:
        return;
    case SaveGameMod::SeparateSave:
        save_to_separate();
        return;
    case SaveGameMod::FromMod:
        if (!try_save_to_mod())
        {
            orig(save_game_bak, save_game_sav, buf, size);
        }
        return;
    case SaveGameMod::SeparateSaveOrFromMod:
        if (!try_save_to_mod())
        {
            save_to_separate();
        }
        return;
    }
}
