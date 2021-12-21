#pragma once

void SetSaveGameVfs(class VirtualFilesystem* vfs);

enum class SaveGameMod
{
    None,
    Block,
    SeparateSave,
    FromMod,
    SeparateSaveOrFromMod,
};
using OnReadFromFileOrig = void(const char*, void**, size_t*);
using OnWriteToFileOrig = void(const char*, const char*, void*, size_t);

void OnReadFromFile(SaveGameMod mod_type, const char* save_game_sav, void** out_buf, size_t* out_size, OnReadFromFileOrig* orig);
void OnWriteToFile(SaveGameMod mod_type, const char* save_game_bak, const char* save_game_sav, void* buf, size_t size, OnWriteToFileOrig* orig);
