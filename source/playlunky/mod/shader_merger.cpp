#include "shader_merger.h"

#include "log.h"
#include "virtual_filesystem.h"
#include "util/algorithms.h"
#include "util/image.h"

#include <ctre.hpp>
#include <fstream>
#include <unordered_map>

bool MergeShaders(const std::filesystem::path& source_folder, const std::filesystem::path& destination_folder,
	const std::filesystem::path& shader_file, VirtualFilesystem& vfs) {

	std::filesystem::path source_shader = vfs.GetFilePath(shader_file).value_or(source_folder / shader_file);
	std::string original_shader_code = [&source_shader]() {
		if (auto original_shader_file = std::ifstream{ source_shader }) {
			return std::string((std::istreambuf_iterator<char>(original_shader_file)), std::istreambuf_iterator<char>());
		}
		return std::string{};
	}();

	if (original_shader_code.empty()) {
		return false;
	}

	const auto shader_mods = vfs.GetAllFilePaths("shaders_mod.hlsl");

	struct ModdedFunction {
		std::string Declaration;
		std::string Body;
	};
	std::vector<ModdedFunction> modded_functions;

	for (const auto& shader_mod : shader_mods) {
		const std::string shader_mod_code = [&shader_mod]() {
			if (auto shader_mod_file = std::ifstream{ shader_mod }) {
				return std::string((std::istreambuf_iterator<char>(shader_mod_file)), std::istreambuf_iterator<char>());
			}
			return std::string{};
		}();

		if (!shader_mod_code.empty()) {
			auto read_char = [&shader_mod_code, parsing_index = size_t{ 0 }]() mutable -> std::optional<char> {
				if (parsing_index < shader_mod_code.size()) {
					char c = shader_mod_code[parsing_index];
					parsing_index++;
					return c;
				}
				return std::nullopt;
			};

			std::string function_decl;
			std::string function_body;
			std::size_t scope_depth = 0;
			while (auto c_opt = read_char()) {
				char c = c_opt.value();

				if (c == '{') {
					if (scope_depth == 0) {
						function_body = '{';
					}
					scope_depth++;
				}
				else if (c == '}') {
					if (scope_depth == 0) {
						LogInfo("Shader {} contains syntax errors...", shader_mod.string());
						break;
					}
					else {
						if (scope_depth == 1) {
							function_body += '}';
							if (!algo::contains_if(modded_functions, [&function_decl](const ModdedFunction& modded_fun) { return modded_fun.Declaration == function_decl; })) {
								modded_functions.push_back(ModdedFunction{
									.Declaration = std::move(function_decl),
									.Body = std::move(function_body)
								});
							}
							function_decl.clear();
							function_body.clear();
						}
						scope_depth--;
					}
				}
				else if (c == '\n' && scope_depth == 0) {
					c = ' ';
				}
				else if (c == '\r') {
					continue;
				}
				else if (scope_depth == 0) {
					function_decl += c;
				}
				else {
					function_body += c;
				}
			}
		}
	}

	for (const ModdedFunction& modded_function : modded_functions) {
		if (modded_function.Declaration.find("struct") != std::string::npos) {
			continue;
		}

		const auto decl_pos = original_shader_code.find(modded_function.Declaration);
		if (decl_pos != std::string::npos) {
			const auto opening_braces_pos = original_shader_code.find('{', decl_pos + modded_function.Declaration.size());
			if (opening_braces_pos != std::string::npos) {
				const auto closing_braces = [&original_shader_code, &opening_braces_pos]() -> std::size_t {
					std::size_t current_depth{ 0 };
					for (std::size_t i = opening_braces_pos; i < original_shader_code.size(); i++) {
						if (original_shader_code[i] == '{') {
							current_depth++;
						}
						else if (original_shader_code[i] == '}') {
							current_depth--;
							if (current_depth == 0) {
								return i;
							}
						}
					}
					return std::string::npos;
				}();
				if (closing_braces != std::string::npos) {
					original_shader_code.replace(opening_braces_pos, closing_braces - opening_braces_pos + 1, modded_function.Body);
				}
			}
		}
		else {
			LogInfo("Could not place function with declaration '{}' into shaders. "
				"If you are just using this mod report the issue to the mods creator. "
				"If you developed this mod, make sure it's signature matches exactly the original function's signature...", modded_function.Declaration);
		}
	}

	if (!std::filesystem::exists(destination_folder)) {
		std::filesystem::create_directories(destination_folder);
	}

	const auto destination_file = destination_folder / shader_file;
	if (auto merged_shader_file = std::ofstream{ destination_file, std::ios::trunc }) {
		merged_shader_file.write(original_shader_code.data(), original_shader_code.size());
		return true;
	}

	return false;
}
