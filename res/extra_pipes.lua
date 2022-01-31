local blocked_dirs = { -- map from `animation_frame` to blocked direction
    [1] = 1,
    [2] = 2,
    [4] = 4,
    [5] = 8
}
local dirs = { -- map from player directional inputs to direction
    [INPUTS.LEFT] = 1,
    [INPUTS.RIGHT] = 2,
    [INPUTS.DOWN] = 4,
    [INPUTS.UP] = 8
}
local extra_pipes_texture_id
do
    local extra_pipes_tex = TextureDefinition:new()
    extra_pipes_tex.texture_path = "Data/Textures/extra_pipes.png"
    extra_pipes_tex.width = 128 * 3
    extra_pipes_tex.height = 128 * 2
    extra_pipes_tex.tile_width = 128
    extra_pipes_tex.tile_height = 128
    extra_pipes_texture_id = define_texture(extra_pipes_tex)
end

-- map player input to a direction
local function get_to_from_input(pipe, player, from)
    if player.inventory ~= nil and player.inventory.player_slot ~= -1 and pipe.animation_frame ~= 0 then
        local input = state.player_inputs.player_slots[player.inventory.player_slot].buttons
        for i, v in pairs(dirs) do
            if (input & i) > 0 and v ~= from then
                return v
            end
        end
    end
    return 0
end

-- adjust direction if the current direction is blocked
local function adjust_to_if_blocked(pipe, from, to)
    local real_to = (function()
        if to > 0 then
            return to
        elseif from < 3 then
            return 3 - from
        else
            return 12 - from
        end
    end)()

    local blocked_dir = blocked_dirs[pipe.animation_frame]
    if real_to == blocked_dir then
        if real_to == dirs[INPUTS.LEFT] then
            local fixed_to = dirs[INPUTS.UP]
            return fixed_to ~= from and fixed_to or 12 - from
        elseif real_to == dirs[INPUTS.RIGHT] then
            local fixed_to = dirs[INPUTS.DOWN]
            return fixed_to ~= from and fixed_to or 12 - from
        elseif real_to == dirs[INPUTS.UP] then
            local fixed_to = dirs[INPUTS.RIGHT]
            return fixed_to ~= from and fixed_to or 3 - from
        elseif real_to == dirs[INPUTS.DOWN] then
            local fixed_to = dirs[INPUTS.LEFT]
            return fixed_to ~= from and fixed_to or 3 - from
        end
    end

    return nil
end

-- animation frames of new pipes:
-- - 0 == unconnected 4-way (forces straight)
-- - 1 == left-blocked 3-way (defaults straight, unless left, allows input)
-- - 2 == right-blocked 3-way (defaults straight, unless right, allows input)
-- - 3 == connected 4-way (defaults straight, allows input)
-- - 4 == bottom-blocked 3-way (defaults straight, unless bottom, allows input)
-- - 5 == top-blocked 3-way (defaults straight, unless top, allows input)

local function spawn_4_way_pipe_with_decos(x, y, layer)
    local uid = spawn_grid_entity(ENT_TYPE.FLOOR_PIPE, x, y, layer)
    do
        local pipe = get_entity(uid)
        pipe:set_texture(extra_pipes_texture_id)
        pipe.animation_frame = 3
    end

    set_timeout(function()
        local pipe = get_entity(uid)

        local offsets = {{-1, 0, 143, -1, 1}, {1, 0, 143, 1, 1}, {0, -1, 131, -1, -1}, {0, 1, 131, 1, 1}}
        for _, offset in ipairs(offsets) do
            local ox, oy, tex, w, h = table.unpack(offset)
            local other = get_entity(get_grid_entity_at(x + ox, y + oy, layer))
            if other == nil or other.type.id ~= ENT_TYPE.FLOOR_PIPE then
                local deco_uid = spawn_entity_over(ENT_TYPE.DECORATION_PIPE, pipe.uid, 0.5 * ox, 0.5 * oy)
                local deco = get_entity(deco_uid)
                deco.animation_frame = tex
                deco.width = w
                deco.height = h
            end
        end
    end, 1)

    return get_entity(uid)
end

define_tile_code("pipe_intersection")
define_tile_code("pipe_crossing")
set_pre_tile_code_callback(function(x, y, layer)
    local pipe = spawn_4_way_pipe_with_decos(x, y, layer)
    pipe:set_texture(extra_pipes_texture_id)
end, "pipe_intersection")
set_pre_tile_code_callback(function(x, y, layer)
    local pipe = spawn_4_way_pipe_with_decos(x, y, layer)
    pipe:set_texture(extra_pipes_texture_id)
    set_timeout(function()
        pipe.animation_frame = 0
    end, 1)
end, "pipe_crossing")

-- handling any of the dynamic direction switching
local function setup_pipe(ent)
    local pre_collision2 = set_pre_collision2(ent.uid, function(pipe, collider)
        -- only care about players colliding with the modified pipes
        if (collider.type.search_flags & MASK.PLAYER) ~= 0 and pipe:get_texture() == extra_pipes_texture_id then
            local cx, cy, _ = get_position(collider.uid)
            local x, y, _ = get_position(pipe.uid)
            local dx, dy = cx - x, cy - y
            if math.abs(dx) > 0.5 or math.abs(dy) > 0.5 then
                -- straight 4-way is simple
                if pipe.animation_frame == 0 then
                    if math.abs(dx) > math.abs(dy) then
                        pipe.direction_type = 3
                    else
                        pipe.direction_type = 12
                    end
                else
                    -- get where player is coming from
                    local from = (function()
                        if math.abs(dx) > math.abs(dy) then
                            return dx < 0 and 1 or 2
                        else
                            return dy < 0 and 4 or 8
                        end
                    end)()

                    -- player is at blocked of side, can't enter here
                    if pipe.animation_frame ~= 3 and adjust_to_if_blocked(pipe, 0, from) then
                        pipe.direction_type = 0
                        return
                    end

                    -- and where player wants to go to
                    local to = get_to_from_input(pipe, collider, from)
                    -- then adjust the position if in a 3-way and the direction is not possible
                    if pipe.animation_frame ~= 3 then
                        local adjusted_to = adjust_to_if_blocked(pipe, from, to)
                        if adjusted_to then
                            to = adjusted_to
                        end
                    end

                    -- and just set the resulting direciton
                    if to > 0 then
                        pipe.direction_type = from + to
                    else
                        if from < 3 then
                            pipe.direction_type = 3
                        else
                            pipe.direction_type = 12
                        end
                    end
                end
            end
        end
    end)

    local post_statemachine -- can't be an upvalue unless forward declared
    local registered_on_kill = false
    post_statemachine = set_post_statemachine(ent.uid, function(pipe)
        if pipe:get_texture() == extra_pipes_texture_id then
            -- need to set this every frame, otherwise we don't get pre_collision2
            pipe.end_pipe = true

            if not registered_on_kill then
                registered_on_kill = true

                local x, y, l = get_position(pipe.uid)

                -- contains info for spawning deco when neighbouring pipes break {dx, dy, animation_frame, width, height}
                local offsets = {{-1, 0, 143, -1, 1}, {1, 0, 143, 1, 1}, {0, -1, 131, -1, -1}, {0, 1, 131, 1, 1}}
                for _, offset in ipairs(offsets) do
                    local ox, oy, tex, w, h = table.unpack(offset)
                    local other = get_entity(get_grid_entity_at(x + ox, y + oy, l))
                    if other ~= nil and other.type.id == ENT_TYPE.FLOOR_PIPE then
                        set_on_kill(other.uid, function()
                            -- only spawn deco if the modified pipe still exists
                            if get_entity(pipe.uid) then
                                local deco_uid = spawn_entity_over(ENT_TYPE.DECORATION_PIPE, pipe.uid, 0.5 * ox,
                                    0.5 * oy)
                                local deco = get_entity(deco_uid)
                                deco.animation_frame = tex
                                deco.width = w
                                deco.height = h
                            end
                        end)
                    end
                end
            end
        else
            clear_entity_callback(pipe.uid, pre_collision2)
            clear_entity_callback(pipe.uid, post_statemachine)
        end
    end)
end

set_post_entity_spawn(setup_pipe, SPAWN_TYPE.ANY, 0, ENT_TYPE.FLOOR_PIPE)
set_callback(function()
    local pipes = get_entities_by_type(ENT_TYPE.PIPE)
    for _, uid in pairs(pipes) do
        local pipe = get_entity(uid)
        if pipe:get_texture() == extra_pipes_texture_id then
            setup_pipe(pipe)
        end
    end
end, ON.LOAD)

-- debug draw `direction_type`
--[[
set_callback(function(draw_ctx)
    local pipes = get_entities_by_type(ENT_TYPE.PIPE)
    for _, uid in pairs(pipes) do
        local pipe_ent = get_entity(uid)
        if pipe_ent ~= nil then
            local text = inspect(pipe_ent.direction_type)

            local x, y, _ = get_render_position(uid)
            local sx, sy = screen_position(x, y - pipe_ent.hitboxy + pipe_ent.offsety)
            local tx, ty = draw_text_size(35, text)
            draw_ctx:draw_text(sx - tx / 2, sy - ty * 2, 35, text, rgba(255, 255, 255, 255))
        end
    end

    for _, player in pairs(players) do
        local aabb = screen_aabb(get_render_hitbox(player.uid))
        draw_ctx:draw_rect(aabb, 4, 1, rgba(255, 255, 255, 255))
    end
end, ON.GUIFRAME)
--[[]]
