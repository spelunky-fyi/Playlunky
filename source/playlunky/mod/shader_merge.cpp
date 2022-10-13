#include "shader_merge.h"

#include "detour/imgui.h"
#include "known_files.h"
#include "log.h"
#include "util/algorithms.h"
#include "util/file.h"
#include "util/file_watch.h"
#include "util/image.h"
#include "util/on_scope_exit.h"
#include "util/regex.h"
#include "util/tokenize.h"
#include "virtual_filesystem.h"

#pragma comment(lib, "d3dcompiler")
#include <d3dcompiler.h>

#include "spel2.h"

#include <fstream>
#include <imgui.h>
#include <unordered_map>

template<class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

bool MergeShadersImpl(
    const std::filesystem::path& destination_folder,
    const std::filesystem::path& shader_file,
    std::variant<std::filesystem::path, std::string> source_shader,
    const std::vector<std::filesystem::path>& shader_mods)
{
    std::string source_shader_code = std::visit(overloaded{
                                                    [](const std::filesystem::path& source_shader_path)
                                                    {
                                                        if (auto original_shader_file = std::ifstream{ source_shader_path })
                                                        {
                                                            return std::string((std::istreambuf_iterator<char>(original_shader_file)), std::istreambuf_iterator<char>());
                                                        }
                                                        return std::string{};
                                                    },
                                                    [](std::string& source_shader_code)
                                                    {
                                                        return std::move(source_shader_code);
                                                    },
                                                },
                                                source_shader);

    if (source_shader_code.empty())
    {
        return false;
    }

    auto find_decl_in_original = [&source_shader_code](std::string_view decl)
    {
        return source_shader_code.find(decl) != std::string::npos;
    };

    struct ModdedFunction
    {
        std::string Preamble;
        std::string Declaration;
        std::string Body;
    };
    std::vector<ModdedFunction> modded_functions;

    struct ExtendedFunction
    {
        std::string FunctionName;
        std::vector<ModdedFunction> Extensions;
    };
    std::vector<ExtendedFunction> extended_functions;

    for (const auto& shader_mod : shader_mods)
    {
        const std::string shader_mod_code = [&shader_mod]()
        {
            if (auto shader_mod_file = std::ifstream{ shader_mod })
            {
                return std::string((std::istreambuf_iterator<char>(shader_mod_file)), std::istreambuf_iterator<char>());
            }
            return std::string{};
        }();

        if (!shader_mod_code.empty())
        {
            auto parsing_index = size_t{ 0 };
            auto read_char = [&shader_mod_code, &parsing_index]() -> std::optional<char>
            {
                if (parsing_index < shader_mod_code.size())
                {
                    char c = shader_mod_code[parsing_index];
                    parsing_index++;
                    return c;
                }
                return std::nullopt;
            };
            auto peek_char = [&shader_mod_code, &parsing_index]() -> std::optional<char>
            {
                if (parsing_index < shader_mod_code.size())
                {
                    char c = shader_mod_code[parsing_index];
                    return c;
                }
                return std::nullopt;
            };

            enum class CommentState
            {
                None,
                SingleLine,
                MultiLine
            };
            CommentState comment_state = CommentState::None;

            std::string function_preamble;
            std::string current_line;
            std::string function_body;
            bool is_shader_extension{ false };
            std::size_t scope_depth = 0;
            while (auto c_opt = read_char())
            {
                char c = c_opt.value();

                if (comment_state == CommentState::None && c == '/')
                {
                    if (peek_char().value_or('?') == '/')
                    {
                        read_char();
                        comment_state = CommentState::SingleLine;
                        continue;
                    }
                    else if (peek_char().value_or('?') == '*')
                    {
                        read_char();
                        comment_state = CommentState::MultiLine;
                        continue;
                    }
                }
                else if (comment_state == CommentState::SingleLine)
                {
                    if (c == '\n')
                    {
                        comment_state = CommentState::None;
                    }
                    continue;
                }
                else if (comment_state == CommentState::MultiLine)
                {
                    if (c == '*' && peek_char().value_or('?') == '/')
                    {
                        read_char();
                        comment_state = CommentState::None;
                    }
                    continue;
                }

                if (c == '{')
                {
                    if (scope_depth == 0)
                    {
                        function_body.clear();
                    }
                    scope_depth++;
                }
                else if (c == '}')
                {
                    if (scope_depth == 0)
                    {
                        LogError("Shader {} contains syntax errors...", shader_mod.string());
                        break;
                    }
                    else
                    {
                        if (scope_depth == 1)
                        {
                            if (algo::trim(current_line).find("struct") == 0 || (!is_shader_extension && !find_decl_in_original(current_line)))
                            {
                                function_preamble += current_line + function_body;
                            }
                            else if (is_shader_extension)
                            {
                                function_body += '}';
                                const auto first_space_pos = current_line.find(' ');
                                const auto first_parens_pos = current_line.find('(');
                                if (first_space_pos != std::string::npos && first_parens_pos != std::string::npos)
                                {
                                    std::string function_name = algo::trim(current_line.substr(first_space_pos, first_parens_pos - first_space_pos));
                                    ExtendedFunction* extended_function = algo::find(extended_functions, &ExtendedFunction::FunctionName, function_name);
                                    if (extended_function == nullptr)
                                    {
                                        extended_functions.push_back(ExtendedFunction{
                                            .FunctionName{ std::move(function_name) } });
                                        extended_function = &extended_functions.back();
                                    }
                                    extended_function->Extensions.push_back(ModdedFunction{
                                        .Preamble = std::move(function_preamble),
                                        .Declaration = std::move(current_line),
                                        .Body = std::move(function_body) });
                                }
                                current_line.clear();
                                function_body.clear();
                                is_shader_extension = false;
                                scope_depth--;
                                continue;
                            }
                            else if (!algo::contains(modded_functions, &ModdedFunction::Declaration, current_line))
                            {
                                function_body += '}';
                                modded_functions.push_back(ModdedFunction{
                                    .Preamble = std::move(function_preamble),
                                    .Declaration = std::move(current_line),
                                    .Body = std::move(function_body) });
                                scope_depth--;
                                continue;
                            }
                            current_line.clear();
                            function_body.clear();
                            is_shader_extension = false;
                        }
                        scope_depth--;
                    }
                }
                else if (c == '\n' && scope_depth == 0)
                {
                    if (current_line == "#extends")
                    {
                        is_shader_extension = true;
                    }
                    else
                    {
                        function_preamble += current_line + '\n';
                        is_shader_extension = false;
                    }
                    current_line.clear();
                }

                if (scope_depth == 0)
                {
                    if (!current_line.empty() || !std::isspace(c))
                    {
                        current_line += c;
                    }
                }
                else
                {
                    function_body += c;
                }
            }
        }
    }

    for (const ModdedFunction& modded_function : modded_functions)
    {
        const auto decl_pos = source_shader_code.find(modded_function.Declaration);
        if (decl_pos != std::string::npos)
        {
            const auto opening_braces_pos = source_shader_code.find('{', decl_pos + modded_function.Declaration.size());
            if (opening_braces_pos != std::string::npos)
            {
                const auto closing_braces = [&source_shader_code, &opening_braces_pos]() -> std::size_t
                {
                    std::size_t current_depth{ 0 };
                    for (std::size_t i = opening_braces_pos; i < source_shader_code.size(); i++)
                    {
                        if (source_shader_code[i] == '{')
                        {
                            current_depth++;
                        }
                        else if (source_shader_code[i] == '}')
                        {
                            current_depth--;
                            if (current_depth == 0)
                            {
                                return i;
                            }
                        }
                    }
                    return std::string::npos;
                }();
                if (closing_braces != std::string::npos)
                {
                    source_shader_code.replace(opening_braces_pos, closing_braces - opening_braces_pos + 1, modded_function.Body);
                    source_shader_code.insert(decl_pos, modded_function.Preamble);
                }
            }
        }
        else
        {
            LogError("Could not place function with declaration '{}' into shaders. "
                     "If you are just using this mod report the issue to the mods creator. "
                     "If you developed this mod, make sure it's signature matches exactly the original function's signature...",
                     modded_function.Declaration);
        }
    }

    for (ExtendedFunction& extended_function : extended_functions)
    {
        std::reverse(extended_function.Extensions.begin(), extended_function.Extensions.end());
    }

    for (const ExtendedFunction& extended_function : extended_functions)
    {
        const auto name_pos = source_shader_code.find(extended_function.FunctionName);
        if (name_pos != std::string::npos)
        {
            const auto opening_braces_pos = source_shader_code.find('{', name_pos + extended_function.FunctionName.size());
            if (opening_braces_pos != std::string::npos)
            {
                const auto closing_braces = [&source_shader_code, &opening_braces_pos]() -> std::size_t
                {
                    std::size_t current_depth{ 0 };
                    for (std::size_t i = opening_braces_pos; i < source_shader_code.size(); i++)
                    {
                        if (source_shader_code[i] == '{')
                        {
                            current_depth++;
                        }
                        else if (source_shader_code[i] == '}')
                        {
                            current_depth--;
                            if (current_depth == 0)
                            {
                                return i;
                            }
                        }
                    }
                    return std::string::npos;
                }();
                if (closing_braces != std::string::npos)
                {
                    const auto newline_pos = source_shader_code.rfind('\n', name_pos);
                    if (newline_pos != std::string::npos)
                    {
                        const auto space_pos = source_shader_code.find(' ', newline_pos);
                        if (space_pos != std::string::npos)
                        {
                            const auto return_type = algo::trim(source_shader_code.substr(newline_pos + 1, name_pos - newline_pos - 1));
                            const auto arg_list = [&source_shader_code, &name_pos, &opening_braces_pos]() -> std::string
                            {
                                const auto opening_parens = source_shader_code.find('(', name_pos);
                                if (opening_parens != std::string::npos)
                                {
                                    const auto closing_parens = source_shader_code.rfind(')', opening_braces_pos);
                                    if (closing_parens != std::string::npos)
                                    {
                                        const auto param_list = source_shader_code.substr(opening_parens, closing_parens - opening_parens);
                                        std::string arg_list;
                                        for (const auto param : Tokenize<','>(param_list))
                                        {
                                            const auto tokens = algo::split<' '>(param);
                                            if (!tokens.empty())
                                            {
                                                arg_list += tokens.back();
                                                arg_list += ", ";
                                            }
                                        }
                                        return arg_list;
                                    }
                                }

                                return "";
                            }();

                            std::string additional_code = fmt::format("\n\t{} return_value;", return_type);
                            for (size_t i = 0; i < extended_function.Extensions.size(); i++)
                            {
                                const ModdedFunction& modded_function = extended_function.Extensions[i];

                                const auto real_name = extended_function.FunctionName + "_ext" + std::to_string(i);
                                const auto decl = std::string{ modded_function.Declaration }.replace(
                                    modded_function.Declaration.find(extended_function.FunctionName),
                                    extended_function.FunctionName.size(),
                                    real_name);

                                additional_code += fmt::format("\n\tif ({}({}return_value))\n\t\treturn return_value;", real_name, arg_list);
                            }

                            source_shader_code.insert(opening_braces_pos + 1, additional_code);
                            for (size_t i = 0; i < extended_function.Extensions.size(); i++)
                            {
                                const ModdedFunction& modded_function = extended_function.Extensions[i];

                                const auto real_name = extended_function.FunctionName + "_ext" + std::to_string(i);
                                const auto decl = std::string{ modded_function.Declaration }.replace(
                                    modded_function.Declaration.find(extended_function.FunctionName),
                                    extended_function.FunctionName.size(),
                                    real_name);

                                source_shader_code.insert(newline_pos, fmt::format("{} {}\n", decl, modded_function.Body));
                                source_shader_code.insert(newline_pos, modded_function.Preamble);
                            }
                        }
                    }
                }
            }
        }
        else
        {
            LogError("Could not extend function with name '{}' into shaders. "
                     "If you are just using this mod report the issue to the mods creator. "
                     "If you developed this mod, make sure it's name matches exactly the original function's signatunamere...",
                     extended_function.FunctionName);
        }
    }

    namespace fs = std::filesystem;

    if (!fs::exists(destination_folder))
    {
        fs::create_directories(destination_folder);
    }

    const auto destination_file = destination_folder / shader_file;
    if (auto merged_shader_file = std::ofstream{ destination_file, std::ios::trunc })
    {
        merged_shader_file.write(source_shader_code.data(), source_shader_code.size());
        return true;
    }

    return false;
}

bool MergeShaders(
    const std::filesystem::path& source_folder,
    const std::filesystem::path& destination_folder,
    const std::filesystem::path& shader_file,
    VirtualFilesystem& vfs)
{
    const auto source_shader = vfs.GetFilePath(shader_file).value_or(source_folder / shader_file);
    std::string source_shader_code = [&source_shader]()
    {
        if (auto original_shader_file = std::ifstream{ source_shader })
        {
            return std::string((std::istreambuf_iterator<char>(original_shader_file)), std::istreambuf_iterator<char>());
        }
        return std::string{};
    }();

    if (source_shader_code.empty())
    {
        return false;
    }

    const auto shader_mods = vfs.GetAllFilePaths("shaders_mod.hlsl");

    return MergeShadersImpl(destination_folder, shader_file, std::move(source_shader_code), shader_mods);
}

std::uint32_t g_ReloadTimer;
std::atomic_uint32_t g_ReloadTimerSignal;
const auto g_ReloadTrigger = []()
{ g_ReloadTimerSignal.store(30, std::memory_order_relaxed); };

struct FileWatchInfo
{
    FileWatchId id;
    std::filesystem::path file_path;
};
std::vector<FileWatchInfo> g_CallbackFiles;
std::function<bool(const std::filesystem::path&, const std::vector<std::filesystem::path>&)> g_ReloadCallback;

void SetupShaderHotReload(
    const std::filesystem::path& source_folder,
    const std::filesystem::path& destination_folder,
    const std::filesystem::path& shader_file,
    VirtualFilesystem& vfs)
{
    const auto modded_source_shader = vfs.GetFilePath(shader_file);
    const auto source_shader = modded_source_shader.value_or(source_folder / shader_file);
    auto shader_mods = vfs.GetAllFilePaths("shaders_mod.hlsl");

    g_ReloadTimer = 0;
    g_ReloadTimerSignal.store(0, std::memory_order_relaxed);
    g_ReloadCallback = std::bind_front(MergeShadersImpl, destination_folder, shader_file);
    g_CallbackFiles.clear();

    if (modded_source_shader.has_value())
    {
        const auto id = AddFileGenericWatch(source_shader, g_ReloadTrigger);
        g_CallbackFiles.push_back({ id, std::move(source_shader) });
    }

    for (auto&& shader_mod : shader_mods)
    {
        const auto id = AddFileGenericWatch(shader_mod, g_ReloadTrigger);
        g_CallbackFiles.push_back({ id, std::move(shader_mod) });
    }
}

void UpdateShaderHotReload(
    const std::filesystem::path& source_folder,
    const std::filesystem::path& shader_file,
    VirtualFilesystem& vfs)
{
    g_ReloadTimer = std::max(g_ReloadTimer, g_ReloadTimerSignal.exchange(0, std::memory_order_relaxed));
    if (g_ReloadTimer > 0 && --g_ReloadTimer == 0)
    {
        const auto modded_source_shader = vfs.GetFilePath(shader_file, VfsType::User);
        const auto source_shader = modded_source_shader.value_or(source_folder / shader_file);
        auto shader_mods = vfs.GetAllFilePaths("shaders_mod.hlsl");

        if (g_ReloadCallback(source_shader, shader_mods))
        {
            const auto shader_code{ ReadWholeFile(vfs.GetFilePath(shader_file, VfsType::Backend).value().string().c_str()) };

            ID3DBlob* shader_out;
            ID3DBlob* errors_out;

            const auto d3d_compile_shader = [&](std::string_view entry_point, const char* target)
            {
                char name[128]{};
                fmt::format_to(name, "{}", entry_point);
                return D3DCompile(shader_code.c_str(), shader_code.size(), nullptr, nullptr, nullptr, name, target, 0x800, 0x0, &shader_out, &errors_out) == S_OK;
            };
            const auto post_compile = [&]()
            {
                if (shader_out)
                {
                    shader_out->Release();
                }
                if (errors_out)
                {
                    errors_out->Release();
                }
            };

            for (const auto vertex_shader : s_VertexShaders)
            {
                OnScopeExit cleanup{ post_compile };
                if (!d3d_compile_shader(vertex_shader, "vs_4_0"))
                {
                    LogError("Failed compiling shader: {}", (const char*)errors_out->GetBufferPointer());
                    return;
                }
            }

            for (const auto pixel_shader : s_PixelShaders)
            {
                OnScopeExit cleanup{ post_compile };
                if (!d3d_compile_shader(pixel_shader, "ps_4_0"))
                {
                    LogError("Failed compiling shader: {}", (const char*)errors_out->GetBufferPointer());
                    return;
                }
            }

            Spelunky_ReloadShaders();
        }

        auto files = std::move(shader_mods);
        if (modded_source_shader.has_value())
        {
            files.push_back(source_shader);
        }

        for (auto it = g_CallbackFiles.begin(); it != g_CallbackFiles.end(); ++it)
        {
            const auto& [id, file] = *it;
            if (!algo::contains(files, file))
            {
                ClearFileWatch(id);
                it = g_CallbackFiles.erase(it);
            }
        }

        for (auto&& file : files)
        {
            if (!algo::contains(g_CallbackFiles, &FileWatchInfo::file_path, file))
            {
                const auto id = AddFileGenericWatch(file, g_ReloadTrigger);
                g_CallbackFiles.push_back({ id, std::move(file) });
            }
        }
    }
}
void DrawShaderHotReload()
{
    if (g_ReloadTimer > 0)
    {
        ImGui::PushFont(ImGuiGetBestFont(80.0f, Alphabet::Emoji));

        using namespace std::string_literals;
        std::string reload_shaders_str = algo::to_utf8(fmt::format(L"🦥 Reloading Shaders in {} 🦥", g_ReloadTimer));
        const char* reload_shaders_msg = reload_shaders_str.c_str();

        const ImVec2 size = ImGui::CalcTextSize(reload_shaders_msg);

        ImGui::SetNextWindowSize({ -1, size.y * 3 });
        ImGui::SetNextWindowPos({ ImGui::GetIO().DisplaySize.x / 2 - size.x / 2, ImGui::GetIO().DisplaySize.y / 2 - size.y * 2.0f });
        ImGui::Begin(
            "Shader Reload Overlay",
            nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBringToFrontOnFocus |
                ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNavInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground);

        ImGui::NewLine();
        ImGui::TextUnformatted(reload_shaders_msg);
        ImGui::End();

        ImGui::PopFont();
    }
}
