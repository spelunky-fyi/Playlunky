#include "sprite_painter.h"

#include "dds_conversion.h"
#include "extract_game_assets.h"
#include "image_processing.h"
#include "known_files.h"
#include "log.h"
#include "playlunky_settings.h"
#include "sprite_sheet_merger.h"
#include "util/algorithms.h"
#include "util/on_scope_exit.h"
#include "util/span_util.h"
#include "virtual_filesystem.h"

#include <numeric>

#include <Windows.h>
#include <base64pp.hpp>
#include <d3d11.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <spel2.h>

inline constexpr std::uint32_t c_RepaintDelay = 1000;

template<size_t N>
auto encode_base_64(const std::vector<ColorRGB8>& colors, char (&out)[N])
{
    std::string encoding = base64pp::encode(std::span<const std::uint8_t>{
        reinterpret_cast<const std::uint8_t*>(colors.data()),
        colors.size() * sizeof(ColorRGB8),
    });
    const size_t copy_size = std::min(N - 2, encoding.size());
    std::memcpy(out, encoding.data(), copy_size);
    out[copy_size] = '1';
    out[copy_size + 1] = '\0';
}
template<size_t N>
std::vector<ColorRGB8> decode_base_64(char (&in)[N])
{
    std::string_view encoding = in;
    if (encoding.ends_with('1'))
    {
        encoding = encoding.substr(0, encoding.size() - 1);

        if (std::optional<std::vector<std::uint8_t>> data = base64pp::decode(encoding))
        {
            std::span<std::uint8_t> data_span{ data->begin(), data->end() };
            std::span<ColorRGB8> color_span{ span::bit_cast<ColorRGB8>(data_span) };
            return { color_span.begin(), color_span.end() };
        }
    }
    return {};
}
std::filesystem::path append_to_stem(std::filesystem::path path, std::string addage)
{
    const auto old_file_name = path.filename();
    const auto old_stem = path.stem().string();
    const auto new_file_name = old_stem.substr(0, old_stem.size()) + addage + path.extension().string();
    return path.replace_filename(new_file_name);
}

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

SpritePainter::SpritePainter(SpriteSheetMerger& merger, VirtualFilesystem& vfs, const PlaylunkySettings& settings, const std::filesystem::path& original_data_folder)
    : m_Merger{ merger }
    , m_Vfs{ vfs }
    , m_EnableLuminanceScaling{ settings.GetBool("sprite_settings", "enable_luminance_scaling", true) }
    , m_OriginalDataFolder{ original_data_folder }
{
}
SpritePainter::~SpritePainter() = default;

void SpritePainter::RegisterSheet(std::filesystem::path full_path, std::filesystem::path db_destination, bool outdated, bool deleted)
{
    if (outdated || deleted)
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

        std::filesystem::remove(db_destination);

        for (size_t i = 0;; i++)
        {
            const auto part_path = append_to_stem(db_destination, std::to_string(i));
            if (std::filesystem::exists(part_path))
            {
                std::filesystem::remove(part_path);
            }
            else
            {
                break;
            }
        }

        std::filesystem::remove(append_to_stem(std::filesystem::path{ db_destination }.replace_extension(".txt"), "_sprite_coords"));
    }

    if (!deleted)
    {
        const auto [real_path, real_db_destination] = ConvertToRealFilePair(full_path, db_destination);
        if (algo::contains(s_KnownTextureFiles, std::filesystem::path{ real_path }.replace_extension("").filename().string()))
        {
            const auto target_sheet_dds = std::filesystem::path{ real_path }.replace_extension(".DDS");
            ExtractGameAssets(std::array{ target_sheet_dds }, m_OriginalDataFolder);
        }

        std::lock_guard lock{ m_RegisteredColorModSheetsMutex }; // Not really necessary but doesn't hurt to be sure
        m_RegisteredColorModSheets.emplace_back(new RegisteredColorModSheet{ std::move(full_path), std::move(db_destination), outdated });
    }
}

void SpritePainter::FinalizeSetup(const std::filesystem::path& source_folder, const std::filesystem::path& destination_folder)
{
    std::lock_guard lock{ m_RegisteredColorModSheetsMutex }; // Not really necessary but doesn't hurt to be sure
    for (auto& sheet_ptr : m_RegisteredColorModSheets)
    {
        auto& sheet = *sheet_ptr;
        if (sheet.outdated)
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
    const auto item_spacing = ImGui::GetStyle().ItemSpacing.x;
    const auto frame_padding = ImGui::GetStyle().FramePadding.x;
    const auto window_width = ImGui::GetContentRegionMax().x;

    for (auto& sheet_ptr : m_RegisteredColorModSheets)
    {
        std::lock_guard lock{ m_RegisteredColorModSheetsMutex };
        auto& sheet = *sheet_ptr;
        if (sheet.chosen_colors.size() > 0)
        {
            static const auto trigger_repaint = [this](auto& sheet)
            {
                m_HasPendingRepaints = true;
                m_RepaintTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
                sheet.needs_repaint = true;
            };
            static const auto trigger_texture_upload = [](auto& sheet, bool do_luminance_scale)
            {
                for (size_t i = 0; i < sheet.preview_sprites.size(); i++)
                {
                    Image& preview_sprite = sheet.preview_sprites[i];
                    preview_sprite = LuminanceBlend(sheet.source_sprites[i].Copy(), std::move(preview_sprite));
                    for (Image& color_mod_sprite : sheet.color_mod_sprites[i])
                    {
                        preview_sprite = ColorBlend(color_mod_sprite.Copy(), std::move(preview_sprite));
                        if (do_luminance_scale)
                        {
                            preview_sprite = LuminanceScale(color_mod_sprite.Copy(), std::move(preview_sprite));
                        }
                    }
                    ChangeD3D11Texture(sheet.textures[i], preview_sprite.GetData(), preview_sprite.GetWidth(), preview_sprite.GetHeight());
                }
            };
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

            const bool reloading = sheet.needs_reload || sheet.doing_reload;
            if (reloading)
            {
                ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
                ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
            }

            const auto button_width = ImGui::GetFrameHeight();
            const auto button_plus_spacing = button_width + item_spacing;
            const auto num_pickers_per_row = static_cast<std::size_t>(std::floor(window_width / button_plus_spacing)) - 1;
            for (size_t i = 0; i < sheet.chosen_colors.size(); i++)
            {
                static const auto trigger_partial_texture_upload = [](auto& sheet, auto j, bool do_blend, bool do_luminance_scale)
                {
                    for (size_t i = 0; i < sheet.preview_sprites.size(); i++)
                    {
                        Image& preview_sprite = sheet.preview_sprites[i];
                        if (do_blend)
                        {
                            Image& color_mod_sprite = sheet.color_mod_sprites[i][j];
                            preview_sprite = ColorBlend(color_mod_sprite.Copy(), std::move(preview_sprite));
                            if (do_luminance_scale)
                            {
                                preview_sprite = LuminanceScale(color_mod_sprite.Copy(), sheet.source_sprites[i].Copy(), std::move(preview_sprite));
                            }
                        }
                        ChangeD3D11Texture(sheet.textures[i], preview_sprite.GetData(), preview_sprite.GetWidth(), preview_sprite.GetHeight());
                    }
                };

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
                    for (std::vector<Image>& color_mod_sprites : sheet.color_mod_sprites)
                    {
                        color_mod_sprites[i] = ReplaceColor(std::move(color_mod_sprites[i]), color, new_color);
                    }
                    trigger_partial_texture_upload(sheet, i, true, m_EnableLuminanceScaling);
                    color = new_color;
                    trigger_repaint(sheet);
                }
                else if (ImGui::IsItemHovered() && !sheet.color_picker_hovered[i])
                {
                    for (size_t j = 0; j < sheet.preview_sprites.size(); j++)
                    {
                        Image color_only = sheet.color_mod_sprites[j][i].Copy();
                        if (!color_only.IsEmpty())
                        {
                            Image preview_sprite = sheet.preview_sprites[j].Clone();
                            preview_sprite = AlphaBlend(std::move(preview_sprite), std::move(color_only));
                            ChangeD3D11Texture(sheet.textures[j], preview_sprite.GetData(), preview_sprite.GetWidth(), preview_sprite.GetHeight());
                        }
                    }
                    sheet.color_picker_hovered[i] = true;
                }
                else if (!ImGui::IsItemHovered() && sheet.color_picker_hovered[i])
                {
                    trigger_partial_texture_upload(sheet, i, false, m_EnableLuminanceScaling);
                    sheet.color_picker_hovered[i] = false;
                }
            }

            const auto reset_width = ImGui::CalcTextSize("Reset").x + frame_padding * 2.0f;
            const auto random_width = ImGui::CalcTextSize("Random").x + frame_padding * 2.0f;
            const auto share_width = ImGui::CalcTextSize("Share").x + frame_padding * 2.0f;
            const auto full_buttons_width = reset_width + item_spacing + random_width + item_spacing + share_width;
            ImGui::SetCursorPosX((window_width - full_buttons_width) * 0.5f);

            const std::string reset_id = "Reset##" + sheet.full_path.string();
            const std::string random_id = "Random##" + sheet.full_path.string();
            const std::string share_id = "Share##" + sheet.full_path.string();

            if (ImGui::Button(reset_id.c_str()))
            {
                if (std::filesystem::exists(sheet.db_destination) && std::filesystem::is_regular_file(sheet.db_destination))
                {
                    std::filesystem::remove(sheet.db_destination);
                }
                m_HasPendingReloads = true;
                sheet.needs_reload = true;
                trigger_repaint(sheet);
            }
            ImGui::SameLine();
            if (ImGui::Button(random_id.c_str()))
            {
                const auto prev_colors = std::move(sheet.chosen_colors);
                sheet.chosen_colors = GenerateDistinctRandomColors(prev_colors.size());
                for (std::vector<Image>& color_mod_sprites : sheet.color_mod_sprites)
                {
                    for (size_t i = 0; i < prev_colors.size(); i++)
                    {
                        color_mod_sprites[i] = ReplaceColor(std::move(color_mod_sprites[i]), prev_colors[i], sheet.chosen_colors[i]);
                    }
                }
                trigger_texture_upload(sheet, m_EnableLuminanceScaling);
                trigger_repaint(sheet);
            }
            ImGui::SameLine();
            if (ImGui::Button(share_id.c_str()))
            {
                encode_base_64(sheet.chosen_colors, sheet.colors_base64);
                sheet.share_popup_open = true;
                ImGui::OpenPopup("SharePopup");
            }

            if (reloading)
            {
                ImGui::PopItemFlag();
                ImGui::PopStyleVar();
            }

            if (sheet.share_popup_open && ImGui::BeginPopupModal("SharePopup", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove))
            {
                const auto popup_width = 400;
                ImGui::SetWindowSize(ImVec2{ popup_width, 0 });

                const auto base64_width = ImGui::CalcTextSize(sheet.colors_base64).x + frame_padding * 2.0f;
                ImGui::SetCursorPosX((popup_width - base64_width) * 0.5f);
                ImGui::TextWrapped("%s", sheet.colors_base64);

                const auto copy_width = ImGui::CalcTextSize("Copy").x + frame_padding * 2.0f;
                const auto paste_width = ImGui::CalcTextSize("Paste").x + frame_padding * 2.0f;
                const auto full_popup_buttons_width = copy_width + item_spacing + paste_width;
                ImGui::SetCursorPosX((popup_width - full_popup_buttons_width) * 0.5f);

                if (ImGui::Button("Copy"))
                {
                    ImGui::SetClipboardText(sheet.colors_base64);
                    ImGui::CloseCurrentPopup();
                    sheet.share_popup_open = false;
                }
                ImGui::SameLine();
                if (ImGui::Button("Paste"))
                {
                    const char* clipboard_data = ImGui::GetClipboardText();
                    strcpy_s(sheet.colors_base64, clipboard_data);

                    std::vector<ColorRGB8> prev_colors = sheet.chosen_colors;
                    sheet.chosen_colors = decode_base_64(sheet.colors_base64);
                    if (sheet.chosen_colors.size() < sheet.unique_colors.size())
                    {
                        for (size_t i = sheet.chosen_colors.size(); i < sheet.unique_colors.size(); i++)
                        {
                            sheet.chosen_colors.push_back(sheet.unique_colors[i]);
                        }
                    }
                    else
                    {
                        while (sheet.chosen_colors.size() > sheet.unique_colors.size())
                        {
                            sheet.chosen_colors.pop_back();
                        }
                    }

                    for (std::vector<Image>& color_mod_sprites : sheet.color_mod_sprites)
                    {
                        for (size_t i = 0; i < prev_colors.size(); i++)
                        {
                            color_mod_sprites[i] = ReplaceColor(std::move(color_mod_sprites[i]), prev_colors[i], sheet.chosen_colors[i]);
                        }
                    }

                    trigger_texture_upload(sheet, m_EnableLuminanceScaling);
                    trigger_repaint(sheet);

                    ImGui::CloseCurrentPopup();
                    sheet.share_popup_open = false;
                }

                ImGui::EndPopup();
            }
            else
            {
                sheet.share_popup_open = false;
            }
        }
    }
}

void SpritePainter::Update(const std::filesystem::path& source_folder, const std::filesystem::path& destination_folder)
{
    if (m_HasPendingReloads)
    {
        for (auto& sheet : m_RegisteredColorModSheets)
        {
            std::lock_guard lock{ m_RegisteredColorModSheetsMutex };
            if (sheet->needs_reload && !sheet->doing_reload && !sheet->doing_repaint)
            {
                sheet->needs_reload = false;
                sheet->doing_reload = true;
                std::thread([&sheet, this]()
                            {
                                std::unique_ptr<RegisteredColorModSheet> new_sheet{ new RegisteredColorModSheet{ sheet->full_path, sheet->db_destination, true } };
                                new_sheet->needs_repaint = sheet->needs_repaint.load();
                                SetupSheet(*new_sheet);

                                std::lock_guard lock{ m_RegisteredColorModSheetsMutex };
                                std::swap(new_sheet, sheet); })
                    .detach();
            }
        }

        std::lock_guard lock{ m_RegisteredColorModSheetsMutex };
        if (algo::all_of(m_RegisteredColorModSheets, [](auto& sheet) -> bool
                         { return !(sheet->needs_reload || sheet->doing_reload); }))
        {
            m_HasPendingReloads = false;
        }
    }
    else if (m_HasPendingRepaints)
    {
        // In here we don't need to lock m_RegisteredColorModSheetsMutex since we can't get here while another thread touches the sheets
        const std::size_t now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        if (now - m_RepaintTimestamp > c_RepaintDelay)
        {
            for (auto& sheet : m_RegisteredColorModSheets)
            {
                if (sheet->needs_repaint && !sheet->doing_repaint)
                {
                    sheet->needs_repaint = false;
                    sheet->doing_repaint = true;
                    std::thread([&sheet, this]()
                                {
                                    Image color_mod_image = ReplaceColor(sheet->color_mod_images[0].Clone(), sheet->unique_colors[0], sheet->chosen_colors[0]);
                                    color_mod_image.Write(append_to_stem(sheet->db_destination, "0"));
                                    for (size_t i = 1; i < sheet->color_mod_images.size(); i++)
                                    {
                                        Image color_mod_blend_image = ReplaceColor(sheet->color_mod_images[i].Clone(), sheet->unique_colors[i], sheet->chosen_colors[i]);
                                        color_mod_blend_image.Write(append_to_stem(sheet->db_destination, std::to_string(i)));
                                        color_mod_image = AlphaBlend(std::move(color_mod_image), std::move(color_mod_blend_image));
                                    }
                                    color_mod_image.Write(sheet->db_destination);
                                    RepaintImage(sheet->full_path, sheet->db_destination);
                                    sheet->doing_repaint = false; })
                        .detach();
                }
            }
            if (algo::all_of(m_RegisteredColorModSheets, [](auto& sheet) -> bool
                             { return !(sheet->needs_repaint || sheet->doing_repaint); }))
            {
                for (auto& sheet : m_RegisteredColorModSheets)
                {
                    ReloadSheet(sheet->full_path, sheet->db_destination);
                }
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

        sheet.color_mod_images.clear();

        if (std::filesystem::exists(sheet.db_destination))
        {
            // Read in all color masks, the order is already correct
            std::vector<ColorRGB8> all_colors;
            for (size_t i = 0;; i++)
            {
                const auto part_path = append_to_stem(sheet.db_destination, std::to_string(i));
                if (std::filesystem::exists(part_path))
                {
                    Image& partial_image = sheet.color_mod_images.emplace_back();
                    partial_image.Load(part_path);
                    all_colors.push_back(partial_image.GetFirstColor().value());
                }
                else
                {
                    break;
                }
            }

            // Read the sprite coordinates from file
            std::vector<ImageSubRegion> sprite_regions;
            if (auto coord_file = std::ifstream(append_to_stem(std::filesystem::path{ sheet.db_destination }.replace_extension(".txt"), "_sprite_coords")))
            {
                size_t num_sprites;
                coord_file >> num_sprites;
                sprite_regions.resize(num_sprites);
                for (ImageSubRegion& region : sprite_regions)
                {
                    coord_file >> region.x >> region.y >> region.width >> region.height;
                }
            }

            // Fetch the sprites
            for (auto sub_region : sprite_regions)
            {
                sheet.source_sprites.push_back(sheet.source_image.GetSubImage(sub_region));
                std::vector<Image> color_mod_sprite_split;
                for (auto& image : sheet.color_mod_images)
                {
                    color_mod_sprite_split.push_back(image.GetSubImage(sub_region));
                }
                Image preview_sprite = sheet.source_sprites.back().Clone();
                for (Image& color_mod_sprite : color_mod_sprite_split)
                {
                    preview_sprite = ColorBlend(color_mod_sprite.Copy(), std::move(preview_sprite));
                }
                sheet.preview_sprites.push_back(std::move(preview_sprite));
                sheet.color_mod_sprites.push_back(std::move(color_mod_sprite_split));
            }

            // Setup colors, order is already corresponding to the sprites they appear in
            sheet.unique_colors = std::move(all_colors);
            sheet.chosen_colors = sheet.unique_colors;
            sheet.color_picker_hovered = std::vector<bool>(sheet.chosen_colors.size(), false);
        }
        else
        {
            std::vector<ColorRGB8> all_colors;
            {
                Image color_mod_image;
                color_mod_image.Load(sheet.full_path);
                if (color_mod_image.GetWidth() != sheet.source_image.GetWidth() || color_mod_image.GetHeight() != sheet.source_image.GetHeight())
                {
                    color_mod_image.Resize(ImageSize{ sheet.source_image.GetWidth(), sheet.source_image.GetHeight() }, ScalingFilter::Nearest);
                }

                all_colors = color_mod_image.GetUniqueColors();
                for (auto& color : all_colors)
                {
                    Image extracted_image = ExtractColor(color_mod_image.Clone(), color);
                    sheet.color_mod_images.push_back(std::move(extracted_image));
                }
            }

            sheet.source_sprites.clear();
            sheet.preview_sprites.clear();
            sheet.color_mod_sprites.clear();

            // Get sprites containing all colors for proper preview
            std::vector<ImageSubRegion> sprite_regions;
            {
                auto colors = all_colors;
                auto all_sprites = sheet.source_image.GetSprites();
                for (auto& [sprite, sub_region] : all_sprites)
                {
                    std::vector<Image> color_mod_sprite_split;
                    std::vector<ColorRGB8> this_sprite_colors;
                    for (auto& color_mod_image : sheet.color_mod_images)
                    {
                        auto color_mod_sprite = color_mod_image.GetSubImage(sub_region);
                        color_mod_sprite_split.push_back(color_mod_sprite.Copy());
                        if (auto color_mod_sprite_color = color_mod_sprite.GetFirstColor())
                        {
                            this_sprite_colors.push_back(color_mod_sprite_color.value());
                        }
                    }
                    for (auto color : this_sprite_colors)
                    {
                        if (algo::contains(colors, color))
                        {
                            algo::erase(colors, color);
                            if (!color_mod_sprite_split.empty())
                            {
                                sheet.source_sprites.push_back(sprite.Clone());
                                Image preview_sprite = sprite.Clone();
                                for (Image& color_mod_sprite : color_mod_sprite_split)
                                {
                                    preview_sprite = ColorBlend(color_mod_sprite.Clone(), std::move(preview_sprite));
                                }
                                sheet.preview_sprites.push_back(std::move(preview_sprite));
                                sheet.color_mod_sprites.push_back(std::move(color_mod_sprite_split));
                                sprite_regions.push_back(sub_region);
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
                std::vector<ColorRGB8> i_colors;
                for (const auto& color_mod_sprite : sheet.color_mod_sprites[i])
                {
                    if (auto color = color_mod_sprite.GetFirstColor())
                    {
                        i_colors.push_back(color.value());
                    }
                }
                bool removed_i_colors = false;
                for (size_t j = i + 1; j < sheet.source_sprites.size();)
                {
                    std::vector<ColorRGB8> j_colors;
                    for (auto& color_mod_sprite : sheet.color_mod_sprites[j])
                    {
                        if (auto color = color_mod_sprite.GetFirstColor())
                        {
                            j_colors.push_back(color.value());
                        }
                    }
                    if (algo::is_sub_set(i_colors, j_colors))
                    {
                        sheet.source_sprites.erase(sheet.source_sprites.begin() + i);
                        sheet.preview_sprites.erase(sheet.preview_sprites.begin() + i);
                        sheet.color_mod_sprites.erase(sheet.color_mod_sprites.begin() + i);
                        sprite_regions.erase(sprite_regions.begin() + i);
                        removed_i_colors = true;
                        break;
                    }
                    else if (algo::is_sub_set(j_colors, i_colors))
                    {
                        sheet.source_sprites.erase(sheet.source_sprites.begin() + j);
                        sheet.preview_sprites.erase(sheet.preview_sprites.begin() + j);
                        sheet.color_mod_sprites.erase(sheet.color_mod_sprites.begin() + j);
                        sprite_regions.erase(sprite_regions.begin() + j);
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

            // Store the sprite coordinates to file
            if (auto coord_file = std::ofstream(append_to_stem(std::filesystem::path{ sheet.db_destination }.replace_extension(".txt"), "_sprite_coords")))
            {
                coord_file << sprite_regions.size() << " ";
                for (ImageSubRegion region : sprite_regions)
                {
                    coord_file << region.x << " " << region.y << " " << region.width << " " << region.height << " ";
                }
            }

            // Sort unique colors based on which image they appear in
            {
                std::vector<std::ptrdiff_t> color_indices;
                sheet.unique_colors.clear();
                for (const auto& color_mod_sprite_split : sheet.color_mod_sprites)
                {
                    std::vector<std::ptrdiff_t> this_sprite_color_indices;
                    for (size_t i = 0; i < color_mod_sprite_split.size(); i++)
                    {
                        if (color_mod_sprite_split[i].GetFirstColor())
                        {
                            this_sprite_color_indices.push_back(i);
                        }
                    }
                    for (std::ptrdiff_t color_index : this_sprite_color_indices)
                    {
                        if (!algo::contains(color_indices, color_index))
                        {
                            color_indices.push_back(color_index);
                            sheet.unique_colors.push_back(all_colors[color_index]);
                        }
                    }
                }

                auto color_mod_images = std::move(sheet.color_mod_images);
                auto color_mod_sprites = std::move(sheet.color_mod_sprites);
                sheet.color_mod_sprites.resize(sheet.preview_sprites.size());
                for (std::ptrdiff_t i : color_indices)
                {
                    sheet.color_mod_images.push_back(std::move(color_mod_images[i]));
                    for (size_t j = 0; j < color_mod_sprites.size(); j++)
                    {
                        sheet.color_mod_sprites[j].push_back(std::move(color_mod_sprites[j][i]));
                    }
                }
            }

            // Flatten luminance of all colors
            for (size_t i = 0; i < sheet.unique_colors.size(); i++)
            {
                auto& color = sheet.unique_colors[i];
                const float r = color.r / 255.0f;
                const float g = color.g / 255.0f;
                const float b = color.b / 255.0f;
                const auto [fr, fg, fb] = SetLuminance(r, g, b, 0.5f);
                const ColorRGB8 fixed_color{
                    .r{ static_cast<std::uint8_t>(std::clamp(fr * 255.0f, 0.0f, 255.0f)) },
                    .g{ static_cast<std::uint8_t>(std::clamp(fg * 255.0f, 0.0f, 255.0f)) },
                    .b{ static_cast<std::uint8_t>(std::clamp(fb * 255.0f, 0.0f, 255.0f)) },
                };
                sheet.color_mod_images[i] = ReplaceColor(std::move(sheet.color_mod_images[i]), color, fixed_color);
                for (auto& images : sheet.color_mod_sprites)
                {
                    images[i] = ReplaceColor(std::move(images[i]), color, fixed_color);
                }
                color = fixed_color;
            }

            sheet.chosen_colors = sheet.unique_colors;
            sheet.color_picker_hovered = std::vector<bool>(sheet.chosen_colors.size(), false);
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
    const auto [real_path, real_db_destination] = ConvertToRealFilePair(full_path, db_destination);

    if (const auto source_path = GetSourcePath(real_path))
    {
        Image repainted_image;
        repainted_image.Load(source_path.value());

        Image color_mod_image;
        bool do_luminance_scale = false;
        if (std::filesystem::exists(db_destination))
        {
            color_mod_image.Load(db_destination);
            do_luminance_scale = true;
        }
        else
        {
            color_mod_image.Load(full_path);
        }

        repainted_image = ColorBlend(color_mod_image.Copy(), std::move(repainted_image));

        const auto luminance_image_path = ReplaceColExtension(full_path, "_lumin");
        if (std::filesystem::exists(luminance_image_path))
        {
            Image luminance_mod_image;
            luminance_mod_image.Load(luminance_image_path);
            repainted_image = LuminanceBlend(std::move(luminance_mod_image), std::move(repainted_image));
        }

        if (m_EnableLuminanceScaling && do_luminance_scale)
        {
            repainted_image = LuminanceScale(color_mod_image.Copy(), std::move(repainted_image));
        }

        if (algo::contains(s_KnownTextureFiles, std::filesystem::path{ real_path }.replace_extension("").filename().string()))
        {
            // Save to .DDS
            const auto dds_db_destination = std::filesystem::path{ real_db_destination }.replace_extension(".DDS");
            ConvertRBGAToDds(repainted_image.GetData(), repainted_image.GetWidth(), repainted_image.GetHeight(), dds_db_destination);
        }
        else
        {
            // Save to .png (or possibly other source format, should work too)
            repainted_image.Write(real_db_destination);
        }
    }

    return true;
}
bool SpritePainter::ReloadSheet(const std::filesystem::path& full_path, const std::filesystem::path& db_destination)
{
    LogInfo("Repainting image {}...", full_path.string());

    const auto [real_path, real_db_destination] = ConvertToRealFilePair(full_path, db_destination);

    if (algo::contains(s_KnownTextureFiles, std::filesystem::path{ real_path }.replace_extension("").filename().string()))
    {
        // Make game reload directly
        std::string dds_path = std::filesystem::path{ real_path }.replace_extension(".DDS").string();
        std::replace(dds_path.begin(), dds_path.end(), '\\', '/');
        Spelunky_ReloadTexture(dds_path.c_str());
    }
    else
    {
        // Do the reload through the sprite sheet merger
        m_Merger.RegisterSheet(real_path, true, false);
    }

    LogInfo("File {} was successfully repainted...", full_path.string());

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
    std::optional<std::filesystem::path> vfs_path = m_Vfs.GetFilePathFilterExt(relative_path, Image::AllowedExtensions, VfsType::User);
    if (!vfs_path && algo::contains(s_KnownTextureFiles, std::filesystem::path{ relative_path }.replace_extension("").filename().string()))
    {
        vfs_path = m_OriginalDataFolder / relative_path;
        if (!std::filesystem::exists(vfs_path.value()))
        {
            vfs_path.reset();
        }
    }
    return vfs_path;
}
std::filesystem::path SpritePainter::ReplaceColExtension(std::filesystem::path path, std::string_view replacement)
{
    const auto old_file_name = path.filename();
    const auto old_stem = path.stem().string();
    const auto new_file_name = old_stem.substr(0, old_stem.size() - 4) + std::string{ replacement } + path.extension().string();
    return path.replace_filename(new_file_name);
}
