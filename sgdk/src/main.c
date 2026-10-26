#include <genesis.h>
#include "generated/rooms.h"
#include "player.h"
#include "generated/player_assets.h"

/* Room explorer plus a movement prototype in the first room. */
static const u16 hud_palette[16] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0x0EEE
};

static void show_room(u16 stage, u16 room, bool atlas)
{
    const VKStage *group = &vk_stages[stage - 1];
    const u16 index = group->first + room;
    char label[40];
    u16 i;

    VDP_setEnable(FALSE);
    VDP_resetSprites();
    VDP_updateSprites(1, CPU);
    VDP_clearPlane(BG_A, TRUE);
    VDP_loadTileData(vk_tiles[group->tileset], TILE_USER_INDEX, VK_TILE_COUNT, CPU);
    PAL_setPalette(PAL0, vk_palettes[index], CPU);
    PAL_setPalette(PAL1, hud_palette, CPU);
    VDP_setTextPalette(PAL1);
    sprintf(label, "STAGE %02u  ROOM %02u/%02u", stage, room, group->count - 1);
    VDP_drawText(label, 3, 1);
    VDP_drawText("B PLAY | START RESET", 6, 2);
    if (atlas)
    {
        VDP_drawText("TILESET / 192 TILES", 5, 4);
        for (i = 0; i < VK_TILE_COUNT; i++)
            VDP_setTileMapXY(BG_A, TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE,
                             TILE_USER_INDEX + i), 8 + (i % 16), 6 + (i / 16));
        VDP_drawText("STATIC ASSETS / NO GAMEPLAY", 3, 22);
    }
    else
        VDP_setTileMapDataRectEx(BG_A, vk_maps[index],
            TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, TILE_USER_INDEX),
            0, 4, 32, 22, 32, CPU);
    VDP_drawText("L/R ROOM  U/D STAGE  A TILES", 2, 27);
    VDP_setEnable(TRUE);
}

#define PLAYER_TILE_INDEX (TILE_USER_INDEX + VK_TILE_COUNT)

static void show_player(const VKPlayer *player)
{
    VDP_setSprite(0, (s16) player->x - 8, (s16) player->y - 31, SPRITE_SIZE(3, 4),
        TILE_ATTR_FULL(PAL2, TRUE, FALSE, FALSE,
                       PLAYER_TILE_INDEX + player->pose * VK_PLAYER_TILES_PER_POSE));
    VDP_updateSprites(1, CPU);
}

static void enter_play(VKPlayer *player)
{
    show_room(1, 0, FALSE);
    vk_player_reset(player);
    VDP_loadTileData((const u32 *) vk_player_tiles, PLAYER_TILE_INDEX,
                    VK_PLAYER_POSES * VK_PLAYER_TILES_PER_POSE, CPU);
    PAL_setPalette(PAL2, vk_palettes[0], CPU);
    VDP_drawText("FIRST ROOM MOVEMENT TEST  ", 3, 1);
    VDP_drawText("   NO ENEMIES / NO ATTACK   ", 2, 2);
    VDP_drawText(" L/R MOVE  C JUMP  B EXIT    ", 2, 27);
    show_player(player);
}

static VKPlayer player;

int main(bool hardReset)
{
    u16 previous = 0;
    u16 stage = 1;
    u16 room = 0;
    bool atlas = FALSE;
    bool play = FALSE;
    (void) hardReset;
    VDP_setScreenWidth256();
    VDP_setScreenHeight224();
    JOY_init();
    show_room(stage, room, atlas);
    while (TRUE)
    {
        u16 keys, pressed;
        bool dirty = FALSE;
        SYS_doVBlankProcess();
        keys = JOY_readJoypad(JOY_1);
        pressed = keys & ~previous;
        if (pressed & BUTTON_B)
        {
            play = !play;
            stage = 1;
            room = 0;
            atlas = FALSE;
            if (play) enter_play(&player);
            else show_room(stage, room, atlas);
            previous = keys;
            continue;
        }
        if (play)
        {
            if (pressed & BUTTON_START) enter_play(&player);
            else
            {
                const int direction = !!(keys & BUTTON_RIGHT) - !!(keys & BUTTON_LEFT);
                vk_player_tick(&player, direction, !!(pressed & BUTTON_C));
                show_player(&player);
            }
            previous = keys;
            continue;
        }
        if (pressed & BUTTON_UP)
        {
            stage = stage == 18 ? 1 : stage + 1;
            room = 0;
            dirty = TRUE;
        }
        else if (pressed & BUTTON_DOWN)
        {
            stage = stage == 1 ? 18 : stage - 1;
            room = 0;
            dirty = TRUE;
        }
        if (pressed & BUTTON_RIGHT)
        {
            room = (room + 1) % vk_stages[stage - 1].count;
            dirty = TRUE;
        }
        else if (pressed & BUTTON_LEFT)
        {
            room = room == 0 ? vk_stages[stage - 1].count - 1 : room - 1;
            dirty = TRUE;
        }
        if (pressed & BUTTON_A)
        {
            atlas = !atlas;
            dirty = TRUE;
        }
        if (pressed & BUTTON_START)
        {
            stage = 1;
            room = 0;
            atlas = FALSE;
            dirty = TRUE;
        }
        if (dirty) show_room(stage, room, atlas);
        previous = keys;
    }
    return 0;
}
