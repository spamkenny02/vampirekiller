#ifndef VK_PLAYER_H
#define VK_PLAYER_H

/* Plain integer types keep the ROM primitives testable on a host compiler. */
typedef unsigned char VKByte;
typedef unsigned short VKWord;

typedef struct {
    VKByte x, y, facing, mode, phase, clock, pose;
} VKPlayer;

VKByte vk_sample_tile(const VKByte *map, VKByte x, VKByte y);
int vk_tile_solid(VKByte tile, VKByte stage, VKByte special);
int vk_grounded(const VKByte *map, VKByte x, VKByte y, VKByte stage, VKByte special);
int vk_wall(const VKByte *map, VKByte x, VKByte y, int direction,
            VKByte crouched, VKByte stage, VKByte special);
void vk_jump_step(VKByte *y, VKByte *phase);
void vk_player_reset(VKPlayer *player);
void vk_player_tick(VKPlayer *player, int direction, int jump_pressed);

#endif
