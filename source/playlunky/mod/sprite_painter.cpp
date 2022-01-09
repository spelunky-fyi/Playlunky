#include "sprite_painter.h"

#include "dds_conversion.h"
#include "image_processing.h"
#include "known_files.h"
#include "log.h"
#include "playlunky_settings.h"
#include "sprite_sheet_merger.h"
#include "util/algorithms.h"
#include "util/on_scope_exit.h"
#include "virtual_filesystem.h"

#include <numeric>

#include <d3d11.h>
#include <spel2.h>

inline constexpr std::uint32_t c_RepaintDelay = 500;

void CreateD3D11Texture(ID3D11Texture2D** out_texture, ID3D11ShaderResourceView** out_shader_resource_view, std::span<const std::uint8_t> data, std::uint32_t width, std::uint32_t height)
{
    // Create texture
    D3D11_TEXTURE2D_DESC desc;
    ZeroMemory(&desc, sizeof(desc));
    desc.Width = width;
    desc.Height = height;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    D3D11_SUBRESOURCE_DATA sub_resource;
    sub_resource.pSysMem = data.data();
    sub_resource.SysMemPitch = desc.Width * 4;
    sub_resource.SysMemSlicePitch = 0;
    SpelunkyGetD3D11Device()->CreateTexture2D(&desc, &sub_resource, out_texture);

    // Create texture view
    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc;
    ZeroMemory(&srv_desc, sizeof(srv_desc));
    srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MipLevels = desc.MipLevels;
    srv_desc.Texture2D.MostDetailedMip = 0;
    SpelunkyGetD3D11Device()->CreateShaderResourceView(*out_texture, &srv_desc, out_shader_resource_view);
}
void ChangeD3D11Texture(ID3D11Texture2D* texture, std::span<const std::uint8_t> data, std::uint32_t width, std::uint32_t height)
{
    ID3D11DeviceContext* device_context;
    SpelunkyGetD3D11Device()->GetImmediateContext(&device_context);

    D3D11_MAPPED_SUBRESOURCE mapped_resource;
    [[maybe_unused]] HRESULT res = device_context->Map(texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource);
    std::uint8_t* mapped_data = reinterpret_cast<std::uint8_t*>(mapped_resource.pData);
    for (std::uint32_t i = 0; i < height; ++i)
    {
        memcpy(mapped_data, data.data() + 4 * i * width, 4 * width);
        mapped_data += mapped_resource.RowPitch;
    }
    device_context->Unmap(texture, 0);
}

SpritePainter::SpritePainter(SpriteSheetMerger& merger, VirtualFilesystem& vfs, const PlaylunkySettings& /*settings*/)
    : m_Merger{ merger }
    , m_Vfs{ vfs }
{
}
SpritePainter::~SpritePainter() = default;

void SpritePainter::RegisterSheet(std::filesystem::path full_path, std::filesystem::path db_destination, bool outdated, bool deleted)
{
    if (deleted)
    {
        const auto [real_path, real_db_destination] = ConvertToRealFilePair(full_path, db_destination);
        if (algo::contains(s_KnownTextureFiles, std::filesystem::path{ real_path }.replace_extension("").filename().string()))
        {
            const auto dds_db_destination = std::filesystem::path{ real_db_destination }.replace_extension(".DDS");
            std::filesystem::remove(dds_db_destination);
        }
        else
        {
            std::filesystem::remove(real_db_destination);
        }
    }
    else
    {
        m_RegisteredColorModSheets.push_back({ std::move(full_path), std::move(db_destination), outdated });
    }
}

void SpritePainter::FinalizeSetup(const std::filesystem::path& source_folder, const std::filesystem::path& destination_folder)
{
    for (auto& sheet : m_RegisteredColorModSheets)
    {
        if (sheet.outdated || true)
        {
            RepaintImage(sheet.full_path, sheet.db_destination);
        }

        SetupSheet(sheet);
    }
    m_Merger.GenerateRequiredSheets(source_folder, destination_folder, m_Vfs, true);
}

bool SpritePainter::NeedsWindowDraw()
{
    return true;
}
void SpritePainter::WindowDraw()
{
    for (auto& sheet : m_RegisteredColorModSheets)
    {
        if (sheet.chosen_colors.size() > 0)
        {
            static const auto trigger_repaint = [this](auto* sheet)
            {
                m_HasPendingRepaints = true;
                m_RepaintTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                if (!algo::contains(m_PendingRepaints, &PendingRepaint::sheet, sheet))
                {
                    m_PendingRepaints.push_back(PendingRepaint{ sheet });
                }
            };
            static const auto trigger_texture_upload = [](auto& sheet)
            {
                for (size_t i = 0; i < sheet.preview_sprites.size(); i++)
                {
                    Image& preview_sprite = sheet.preview_sprites[i];
                    preview_sprite = ColorBlend(sheet.color_mod_sprites[i].Copy(), sheet.source_sprites[i].Clone());
                    ChangeD3D11Texture(sheet.textures[i], preview_sprite.GetData(), preview_sprite.GetWidth(), preview_sprite.GetHeight());
                }
            };
            const auto item_spacing = ImGui::GetStyle().ItemSpacing.x;
            const auto window_width = ImGui::GetWindowWidth();
            const auto total_image_width = std::accumulate(
                sheet.preview_sprites.begin(),
                sheet.preview_sprites.end(),
                0.0f,
                [item_spacing](const float current, const Image& preview_sprite) -> float
                {
                    return current + static_cast<float>(preview_sprite.GetWidth()) + item_spacing;
                });

            ImGui::Separator();
            ImGui::SetCursorPosX((window_width - total_image_width + item_spacing) * 0.5f);
            for (size_t i = 0; i < sheet.shader_resource_views.size(); i++)
            {
                if (i != 0)
                {
                    ImGui::SameLine();
                }
                ImGui::Image(sheet.shader_resource_views[i], ImVec2{
                                                                 static_cast<float>(sheet.preview_sprites[i].GetWidth()),
                                                                 static_cast<float>(sheet.preview_sprites[i].GetHeight()),
                                                             });
            }
            const auto button_width = ImGui::GetFrameHeight();
            const auto button_plus_spacing = button_width + item_spacing;
            const auto num_pickers_per_row = static_cast<std::size_t>(std::floor(window_width / button_plus_spacing)) - 1;
            for (size_t i = 0; i < sheet.chosen_colors.size(); i++)
            {
                if (i % num_pickers_per_row == 0)
                {
                    const auto num_pickers_left = sheet.chosen_colors.size() - i;
                    if (num_pickers_left > num_pickers_per_row)
                    {
                        ImGui::SetCursorPosX((window_width - button_plus_spacing * num_pickers_per_row + item_spacing) * 0.5f);
                    }
                    else
                    {
                        ImGui::SetCursorPosX((window_width - button_plus_spacing * num_pickers_left + item_spacing) * 0.5f);
                    }
                }
                else
                {
                    ImGui::SameLine();
                }

                ColorRGB8& color = sheet.chosen_colors[i];
                const std::string label = sheet.full_path.string() + std::to_string(i);
                float f_color[3]{ static_cast<float>(color.r) / 255.0f, static_cast<float>(color.g) / 255.0f, static_cast<float>(color.b) / 255.0f };
                if (ImGui::ColorEdit3(label.c_str(), f_color, ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_NoOptions | ImGuiColorEditFlags_NoInputs))
                {
                    ColorRGB8 new_color{
                        static_cast<std::uint8_t>(f_color[0] * 255.0f),
                        static_cast<std::uint8_t>(f_color[1] * 255.0f),
                        static_cast<std::uint8_t>(f_color[2] * 255.0f),
                    };
                    for (Image& color_mod_sprite : sheet.color_mod_sprites)
                    {
                        color_mod_sprite = ReplaceColor(std::move(color_mod_sprite), color, new_color);
                    }
                    trigger_texture_upload(sheet);
                    color = new_color;
                    trigger_repaint(&sheet);
                }
            }

            const auto frame_padding = ImGui::GetStyle().FramePadding.x;
            const auto reset_width = ImGui::CalcTextSize("Reset").x + frame_padding * 2.0f;
            const auto random_width = ImGui::CalcTextSize("Random").x + frame_padding * 2.0f;
            const auto full_buttons_width = reset_width + random_width + item_spacing;
            ImGui::SetCursorPosX((window_width - full_buttons_width) * 0.5f);

            const std::string reset_id = "Reset##" + sheet.full_path.string();
            const std::string random_id = "Random##" + sheet.full_path.string();

            if (ImGui::Button(reset_id.c_str()))
            {
                if (std::filesystem::exists(sheet.db_destination) && std::filesystem::is_regular_file(sheet.db_destination))
                {
                    std::filesystem::remove(sheet.db_destination);
                }
                SetupSheet(sheet);
                trigger_repaint(&sheet);
            }
            ImGui::SameLine();
            if (ImGui::Button(random_id.c_str()))
            {
                const auto prev_colors = std::move(sheet.chosen_colors);
                sheet.chosen_colors = GenerateDistinctRandomColors(prev_colors.size());
                for (Image& color_mod_sprite : sheet.color_mod_sprites)
                {
                    color_mod_sprite = ReplaceColors(std::move(color_mod_sprite), prev_colors, sheet.chosen_colors);
                }
                trigger_texture_upload(sheet);
                trigger_repaint(&sheet);
            }
        }
    }
}

void SpritePainter::Update(const std::filesystem::path& source_folder, const std::filesystem::path& destination_folder)
{
    if (m_HasPendingRepaints)
    {
        const std::size_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        if (now - m_RepaintTimestamp > c_RepaintDelay)
        {
            algo::erase_if(m_PendingRepaints, [this](PendingRepaint& pending_repaint)
                           {
                               Image color_mod = ReplaceColors(pending_repaint.sheet->color_mod_image.Clone(), pending_repaint.sheet->unique_colors, pending_repaint.sheet->chosen_colors);
                               color_mod.Write(pending_repaint.sheet->db_destination);
                               return RepaintImage(pending_repaint.sheet->full_path, pending_repaint.sheet->db_destination);
                           });
            if (m_HasPendingRepaints && m_PendingRepaints.empty())
            {
                m_Merger.GenerateRequiredSheets(source_folder, destination_folder, m_Vfs, true);
                m_HasPendingRepaints = false;
            }
        }
    }
}

void SpritePainter::SetupSheet(RegisteredColorModSheet& sheet)
{
    const auto [real_path, real_db_destination] = ConvertToRealFilePair(sheet.full_path, sheet.db_destination);
    if (const auto source_path = GetSourcePath(real_path))
    {
        sheet.source_image.Load(source_path.value());

        {
            const auto luminance_image_path = ReplaceColExtension(sheet.full_path, "_lumin");
            if (std::filesystem::exists(luminance_image_path))
            {
                Image luminance_mod_image;
                luminance_mod_image.Load(luminance_image_path);
                sheet.source_image = LuminanceBlend(std::move(luminance_mod_image), std::move(sheet.source_image));
            }
        }

        if (std::filesystem::exists(sheet.db_destination))
        {
            sheet.color_mod_image.Load(sheet.db_destination);
        }
        else
        {
            sheet.color_mod_image.Load(sheet.full_path);
        }

        sheet.unique_colors = sheet.color_mod_image.GetUniqueColors();
        sheet.chosen_colors = sheet.unique_colors;

        sheet.source_sprites.clear();
        sheet.color_mod_sprites.clear();
        sheet.preview_sprites.clear();

        // Get sprites containing all colors for proper preview
        {
            auto colors = sheet.unique_colors;
            auto all_sprites = sheet.source_image.GetSprites();
            for (auto& [sprite, sub_region] : all_sprites)
            {
                Image color_mod_sprite = sheet.color_mod_image.GetSubImage(sub_region);
                const std::vector<ColorRGB8> this_sprite_colors = color_mod_sprite.GetUniqueColors();
                for (ColorRGB8 color : this_sprite_colors)
                {
                    if (algo::contains(colors, color))
                    {
                        algo::erase(colors, color);
                        if (!color_mod_sprite.IsEmpty())
                        {
                            sheet.source_sprites.push_back(sprite.Clone());
                            sheet.color_mod_sprites.push_back(std::move(color_mod_sprite));
                            sheet.preview_sprites.push_back(ColorBlend(sheet.color_mod_sprites.back().Copy(), sprite.Clone()));
                        }
                    }
                }

                if (colors.empty())
                {
                    break;
                }
            }
        }

        // Reduce the amount of preview sprites to what we actually need
        for (size_t i = 0; i < sheet.source_sprites.size();)
        {
            const std::vector<ColorRGB8> i_colors = sheet.color_mod_sprites[i].GetUniqueColors();
            bool removed_i_colors = false;
            for (size_t j = i + 1; j < sheet.source_sprites.size(); j++)
            {
                const std::vector<ColorRGB8> j_colors = sheet.color_mod_sprites[j].GetUniqueColors();
                if (algo::is_sub_set(i_colors, j_colors))
                {
                    sheet.source_sprites.erase(sheet.source_sprites.begin() + i);
                    sheet.color_mod_sprites.erase(sheet.color_mod_sprites.begin() + i);
                    sheet.preview_sprites.erase(sheet.preview_sprites.begin() + i);
                    removed_i_colors = true;
                    break;
                }
                else if (algo::is_sub_set(j_colors, i_colors))
                {
                    sheet.source_sprites.erase(sheet.source_sprites.begin() + j);
                    sheet.color_mod_sprites.erase(sheet.color_mod_sprites.begin() + j);
                    sheet.preview_sprites.erase(sheet.preview_sprites.begin() + j);
                    continue;
                }
                else
                {
                    j++;
                }
            }

            if (!removed_i_colors)
            {
                i++;
            }
        }

        // Create actual preview texture
        for (size_t i = 0; i < sheet.textures.size(); i++)
        {
            sheet.textures[i]->Release();
            sheet.shader_resource_views[i]->Release();
        }

        sheet.textures.clear();
        sheet.shader_resource_views.clear();
        for (const Image& preview_sprite : sheet.preview_sprites)
        {

            CreateD3D11Texture(&sheet.textures.emplace_back(), &sheet.shader_resource_views.emplace_back(), preview_sprite.GetData(), preview_sprite.GetWidth(), preview_sprite.GetHeight());
        }
    }
}
bool SpritePainter::RepaintImage(const std::filesystem::path& full_path, const std::filesystem::path& db_destination)
{
    LogInfo("Repainting image {}...", full_path.string());

    const auto [real_path, real_db_destination] = ConvertToRealFilePair(full_path, db_destination);

    if (const auto source_path = GetSourcePath(real_path))
    {
        Image repainted_image;
        repainted_image.Load(source_path.value());

        Image color_mod_image;
        if (std::filesystem::exists(db_destination))
        {
            color_mod_image.Load(db_destination);
        }
        else
        {
            color_mod_image.Load(full_path);
        }
        repainted_image = ColorBlend(std::move(color_mod_image), std::move(repainted_image));

        const auto luminance_image_path = ReplaceColExtension(full_path, "_lumin");
        if (std::filesystem::exists(luminance_image_path))
        {
            Image luminance_mod_image;
            luminance_mod_image.Load(luminance_image_path);
            repainted_image = LuminanceBlend(std::move(luminance_mod_image), std::move(repainted_image));
        }

        if (algo::contains(s_KnownTextureFiles, std::filesystem::path{ real_path }.replace_extension("").filename().string()))
        {
            // Save to .DDS
            const auto dds_db_destination = std::filesystem::path{ real_db_destination }.replace_extension(".DDS");
            ConvertRBGAToDds(repainted_image.GetData(), repainted_image.GetWidth(), repainted_image.GetHeight(), dds_db_destination);

            // Make game reload
            std::string dds_path = std::filesystem::path{ real_path }.replace_extension(".DDS").string();
            std::replace(dds_path.begin(), dds_path.end(), '\\', '/');
            Spelunky_ReloadTexture(dds_path.c_str());
        }
        else
        {
            // Save to .png (or possibly other source format, should work too)
            repainted_image.Write(real_db_destination);

            m_Merger.RegisterSheet(real_path, true, false);
        }

        LogInfo("File {} was successfully repainted...", full_path.string());
    }

    return true;
}
SpritePainter::FilePair SpritePainter::ConvertToRealFilePair(const std::filesystem::path& full_path, const std::filesystem::path& db_destination)
{
    auto rel_path = [&]()
    {
        if (*full_path.begin() == "Mods")
        {
            auto strip_mod_dir = [](const std::filesystem::path& p) -> std::filesystem::path
            {
                auto strip_mod_dir_impl = [](const std::filesystem::path& p, auto& self) -> std::filesystem::path
                {
                    const std::filesystem::path& parent_path = p.parent_path();
                    if (parent_path.empty() || parent_path.string() == "Mods/Packs")
                    {
                        return std::filesystem::path();
                    }
                    else
                    {
                        return self(parent_path, self) / p.filename();
                    }
                };

                return strip_mod_dir_impl(p, strip_mod_dir_impl);
            };

            std::filesystem::path rel_path{ strip_mod_dir(full_path) };
            return ReplaceColExtension(std::move(rel_path));
        }
        else
        {
            return ReplaceColExtension(full_path);
        }
    }();

    return { rel_path, ReplaceColExtension(db_destination) };
}
std::optional<std::filesystem::path> SpritePainter::GetSourcePath(const std::filesystem::path& relative_path)
{
    static const std::array allowed_extensions{
        std::filesystem::path{ ".png" },
        std::filesystem::path{ ".bmp" },
        std::filesystem::path{ ".jpg" },
        std::filesystem::path{ ".jpeg" },
        std::filesystem::path{ ".jpe" },
        std::filesystem::path{ ".jp2" },
        std::filesystem::path{ ".tif" },
        std::filesystem::path{ ".tiff" },
        std::filesystem::path{ ".pbm" },
        std::filesystem::path{ ".pgm" },
        std::filesystem::path{ ".ppm" },
        std::filesystem::path{ ".sr" },
        std::filesystem::path{ ".ras" },
    };
    return m_Vfs.GetFilePathFilterExt(relative_path, allowed_extensions, VfsType::User);
}
std::filesystem::path SpritePainter::ReplaceColExtension(std::filesystem::path path, std::string_view replacement)
{
    const auto old_file_name = path.filename();
    const auto old_stem = path.stem().string();
    const auto new_file_name = old_stem.substr(0, old_stem.size() - 4) + std::string{ replacement } + path.extension().string();
    return path.replace_filename(new_file_name);
}
