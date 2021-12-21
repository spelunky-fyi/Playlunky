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
    switch (mod_type)
    {
    case SaveGameMod::Block:
        return;
    case SaveGameMod::FromMod:
        if (const auto sav_replacement = s_SaveGameVfs->GetDifferentFilePath(save_game_sav))
        {
            const std::string sav_replacement_str = sav_replacement.value().string();
            orig(sav_replacement_str.c_str(), out_buf, out_size);
        }
        else
        {
            orig(save_game_sav, out_buf, out_size);
        }
        return;
    }
}

void OnWriteToFile(SaveGameMod mod_type, [[maybe_unused]] const char* save_game_bak, const char* save_game_sav, void* buf, size_t size, OnWriteToFileOrig* orig)
{
    switch (mod_type)
    {
    case SaveGameMod::Block:
        return;
    case SaveGameMod::FromMod:
        if (const auto sav_replacement = s_SaveGameVfs->GetDifferentFilePath(save_game_sav))
        {
            const std::string sav_replacement_str = sav_replacement.value().string();
            const auto bak_replacement_str = std::filesystem::path{ sav_replacement.value() }.replace_extension(".bak").string();
            orig(bak_replacement_str.c_str(), sav_replacement_str.c_str(), buf, size);
        }
        else
        {
            orig(save_game_bak, save_game_sav, buf, size);
        }
        return;
    }
}
