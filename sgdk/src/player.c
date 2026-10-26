#include "player.h"
#include "generated/player_assets.h"

/* Original 7D2F. Subtraction wraps to 8 bits before row calculation. */
VKByte vk_sample_tile(const VKByte *map, VKByte x, VKByte y)
{
    const VKWord index = ((VKWord)((VKByte)(y - 16) & 248) << 2) + (x >> 3);
    return index < 768 ? map[index] : 0;
}

/* Original 7C5E: carry is set for (tile - 1) < the stage-specific limit. */
int vk_tile_solid(VKByte tile, VKByte stage, VKByte special)
{
    VKByte limit = special ? 6 : vk_collision_limits[stage < 19 ? stage : 0];
    return (VKByte)(tile - 1) < limit;
}

/* Original 7B88: test two points underneath the player, except at y >= D0. */
int vk_grounded(const VKByte *map, VKByte x, VKByte y, VKByte stage, VKByte special)
{
    if (y >= 208) return 0;
    return vk_tile_solid(vk_sample_tile(map, (VKByte)(x + 5), y), stage, special) ||
           vk_tile_solid(vk_sample_tile(map, (VKByte)(x - 5), y), stage, special);
}

/* Original 7BA9/7C05 normal and crouched side probes. */
int vk_wall(const VKByte *map, VKByte x, VKByte y, int direction,
            VKByte crouched, VKByte stage, VKByte special)
{
    VKByte row;
    const VKByte count = crouched ? 3 : 4;
    const VKByte side = (VKByte)(x + (direction < 0 ? -8 : 8));
    for (row = 0; row < count; row++)
        if (vk_tile_solid(vk_sample_tile(map, side, (VKByte)(y - 2 - row * 8)), stage, special))
            return 1;
    return 0;
}

/* Original 6D4D with BC=6D95,D=19. Active jump phases never approach wraparound. */
void vk_jump_step(VKByte *y, VKByte *phase)
{
    VKByte sample;
    (*phase)++;
    sample = *phase > 19 ? 19 : *phase;
    if (sample == 0) sample = 19;
    *y = (VKByte)(*y + vk_jump_deltas[sample - 1]);
}

void vk_player_reset(VKPlayer *p)
{
    p->x = 16;
    p->y = 192;
    p->facing = p->mode = p->phase = p->clock = p->pose = 0;
}

/* Prototype orchestration around the ported primitives, limited to room 1/0.
 * It does not implement the full original state dispatcher, stairs or exits. */
void vk_player_tick(VKPlayer *p, int direction, int jump_pressed)
{
    VKByte was_jumping = p->mode == 1;
    VKByte walking = 0;
    p->clock++;
    if (direction)
    {
        p->facing = direction < 0;
        if (!vk_wall(vk_player_room, p->x, p->y, direction, 0, 1, 0))
        {
            int next = p->x + direction * 2; /* Original normal speed: 0200 in 8.8. */
            if (next >= 8 && next <= 248) { p->x = (VKByte) next; walking = 1; }
        }
    }
    if (p->mode == 0)
    {
        if (!vk_grounded(vk_player_room, p->x, p->y, 1, 0)) { p->mode = 4; p->phase = 0; }
        else if (jump_pressed) { p->mode = 1; p->phase = 0; }
    }
    if (p->mode == 1 && was_jumping)
    {
        vk_jump_step(&p->y, &p->phase);
        if (p->phase >= 9 && vk_grounded(vk_player_room, p->x, p->y, 1, 0))
        { p->y &= 248; p->mode = p->phase = 0; }
    }
    else if (p->mode == 4)
    {
        if (vk_grounded(vk_player_room, p->x, p->y, 1, 0))
        { p->y &= 248; p->mode = p->phase = 0; }
        else { p->y = (VKByte)(p->y + vk_fall_deltas[p->phase]); if (p->phase < 3) p->phase++; }
    }
    if (p->y >= 208) vk_player_reset(p); /* Room-only boundary handling. */
    if (p->mode == 1) p->pose = p->facing ? 7 : 6;
    else
    {
        VKByte frame = walking ? ((p->clock & 4) ? 1 : ((p->clock & 8) ? 0 : 2)) : 0;
        p->pose = frame + (p->facing ? 3 : 0);
    }
}
