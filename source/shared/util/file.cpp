#include "file.h"

#include "util/on_scope_exit.h"

#include <functional>

std::string ReadWholeFile(const char* file_path)
{
    FILE* file{ nullptr };
    auto error = fopen_s(&file, file_path, "rb");
    if (error == 0 && file != nullptr)
    {
        auto close_file = OnScopeExit{ std::bind_front(fclose, file) };

        fseek(file, 0, SEEK_END);
        const size_t file_size = ftell(file);
        fseek(file, 0, SEEK_SET);

        std::string code(file_size, '\0');

        const auto size_read = fread(code.data(), 1, file_size, file);
        if (size_read != file_size)
        {
            code.clear();
            return code;
        }

        return code;
    }
    return {};
}
