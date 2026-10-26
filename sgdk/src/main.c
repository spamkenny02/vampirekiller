#include <genesis.h>
#include "input.h"
#include "generated/rooms.h"
#include "player.h"
#include "generated/player_assets.h"
#include "generated/intro_assets.h"

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

#define PLAYER_TILE_INDEX 1024

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
enum { FRONT_LOGO, FRONT_TITLE, FRONT_CASTLE, FRONT_COURTYARD,
       FRONT_INTERIOR, FRONT_VIEWER, FRONT_TEST };
static u16 front_mode;
static u16 front_timer;
static u16 courtyard_room;

static void show_scene(u16 index)
{
    const VKScene *scene = &vk_scenes[index];
    VDP_setEnable(FALSE);
    VDP_resetSprites();
    VDP_updateSprites(1, CPU);
    VDP_clearPlane(BG_A, TRUE);
    VDP_loadTileData(scene->tiles, TILE_USER_INDEX, scene->count, CPU);
    PAL_setPalette(PAL0, scene->palette, CPU);
    PAL_setPalette(PAL1, hud_palette, CPU);
    VDP_setTextPalette(PAL1);
    VDP_setTileMapDataRectEx(BG_A, scene->map,
        TILE_ATTR_FULL(PAL0, FALSE, FALSE, FALSE, TILE_USER_INDEX), 0, 0, 32, 28, 32, CPU);
    VDP_setEnable(TRUE);
}

static void show_title(void)
{
    u16 x;
    show_scene(1);
    /* The ROM's keyboard prompt is adapted to the Mega Drive controller. */
    for (x = 9; x < 24; x++) VDP_setTileMapXY(BG_A, 0, x, 20);
    VDP_drawText("PRESS START", 10, 20);
    front_mode = FRONT_TITLE;
    front_timer = 0;
}

static void show_castle_frame(u16 frame)
{
    u16 i;
    for (i = 0; i < 8; i++)
    {
        const VKIntroSprite *sprite = &vk_intro_frames[frame][i];
        VDP_setSpriteFull(i, sprite->x, sprite->y, SPRITE_SIZE(2, 2),
            TILE_ATTR_FULL(PAL0, TRUE, FALSE, FALSE, PLAYER_TILE_INDEX + sprite->tile),
            i == 7 ? 0 : i + 1);
    }
    VDP_updateSprites(8, CPU);
}

static void begin_castle(void)
{
    show_scene(2);
    VDP_loadTileData(vk_intro_sprite_tiles, PLAYER_TILE_INDEX, vk_intro_sprite_tile_count, CPU);
    show_castle_frame(0);
    front_mode = FRONT_CASTLE;
    front_timer = 0;
}

static void load_actor(u16 scene)
{
    VDP_loadTileData((const u32 *) vk_player_tiles, PLAYER_TILE_INDEX,
                    VK_PLAYER_POSES * VK_PLAYER_TILES_PER_POSE, CPU);
    PAL_setPalette(PAL2, vk_scenes[scene].palette, CPU);
    show_player(&player);
}

static void show_courtyard(u16 room, u16 x)
{
    courtyard_room = room;
    show_scene(3 + room);
    vk_player_reset(&player);
    player.x = x;
    player.y = 176;
    load_actor(3 + room);
    VDP_drawText(room == 2 ? "L/R MOVE  C JUMP  UP ENTER" : "L/R MOVE C JUMP START TITLE", 2, 27);
    front_mode = FRONT_COURTYARD;
}

static void show_interior(void)
{
    show_scene(6);
    vk_player_reset(&player);
    load_actor(6);
    VDP_drawText("L/R MOVE C JUMP START TITLE", 2, 27);
    front_mode = FRONT_INTERIOR;
}

static void logo_palette(u16 level)
{
    u16 colors[16];
    u16 i;
    for (i = 0; i < 16; i++)
    {
        const u16 source = vk_scenes[0].palette[i];
        u16 r = ((source >> 1) & 7) * level / 16;
        u16 g = ((source >> 5) & 7) * level / 16;
        u16 b = ((source >> 9) & 7) * level / 16;
        colors[i] = (b << 9) | (g << 5) | (r << 1);
    }
    PAL_setPalette(PAL0, colors, CPU);
}

int main(bool hardReset)
{
    VKInput input = {0};
    u16 stage = 1;
    u16 room = 0;
    bool atlas = FALSE;
    (void) hardReset;
    VDP_setScreenWidth256();
    VDP_setScreenHeight224();
    JOY_init();
    front_mode = FRONT_LOGO;
    front_timer = 0;
    show_scene(0);
    logo_palette(0);
    while (TRUE)
    {
        u16 pressed;
        bool dirty = FALSE;
        SYS_doVBlankProcess();
        handleInput(&input);
        pressed = input.pressed;
        if (front_mode == FRONT_LOGO)
        {
            front_timer++;
            if (pressed & BUTTON_START || front_timer >= 120) show_title();
            else if (front_timer <= 16) logo_palette(front_timer);
            else if (front_timer > 104) logo_palette(120 - front_timer);
        }
        else if (front_mode == FRONT_TITLE)
        {
            if (pressed & BUTTON_START) begin_castle();
            else if (pressed & BUTTON_A)
            {
                stage = 1; room = 0; atlas = FALSE;
                show_room(stage, room, atlas); front_mode = FRONT_VIEWER;
            }
        }
        else if (front_mode == FRONT_CASTLE)
        {
            if (pressed & BUTTON_START || ++front_timer >= 320) show_courtyard(0, 16);
            else show_castle_frame(front_timer);
        }
        else if (front_mode == FRONT_COURTYARD || front_mode == FRONT_INTERIOR)
        {
            const int direction = input.direction;
            if (pressed & BUTTON_START) show_title();
            else if (front_mode == FRONT_COURTYARD && courtyard_room == 2 &&
                     player.x >= 192 && player.mode == 0 && (pressed & BUTTON_UP))
                show_interior();
            else
            {
                if (front_mode == FRONT_COURTYARD)
                    vk_player_tick_map(&player, vk_courtyard_maps[courtyard_room], 0,
                                        direction, input.jump_pressed);
                else vk_player_tick(&player, direction, input.jump_pressed);
                if (front_mode == FRONT_COURTYARD && direction > 0 && player.x == 248 &&
                    vk_courtyard_neighbors[courtyard_room][3] != 255)
                    show_courtyard(vk_courtyard_neighbors[courtyard_room][3], 10);
                else if (front_mode == FRONT_COURTYARD && direction < 0 && player.x == 8 &&
                         vk_courtyard_neighbors[courtyard_room][2] != 255)
                    show_courtyard(vk_courtyard_neighbors[courtyard_room][2], 246);
                else show_player(&player);
            }
        }
        else if (front_mode == FRONT_TEST)
        {
            if (pressed & BUTTON_B)
            { show_room(1, 0, FALSE); front_mode = FRONT_VIEWER; }
            else if (pressed & BUTTON_START) enter_play(&player);
            else
            {
                const int direction = input.direction;
                vk_player_tick(&player, direction, input.jump_pressed);
                show_player(&player);
            }
        }
        else /* The asset explorer remains available from the title with A. */
        {
            if (pressed & BUTTON_B)
            { enter_play(&player); front_mode = FRONT_TEST; }
            else
            {
                if (pressed & BUTTON_UP)
                { stage = stage == 18 ? 1 : stage + 1; room = 0; dirty = TRUE; }
                else if (pressed & BUTTON_DOWN)
                { stage = stage == 1 ? 18 : stage - 1; room = 0; dirty = TRUE; }
                if (pressed & BUTTON_RIGHT)
                { room = (room + 1) % vk_stages[stage - 1].count; dirty = TRUE; }
                else if (pressed & BUTTON_LEFT)
                { room = room == 0 ? vk_stages[stage - 1].count - 1 : room - 1; dirty = TRUE; }
                if (pressed & BUTTON_A) { atlas = !atlas; dirty = TRUE; }
                if (pressed & BUTTON_START) { stage = 1; room = 0; atlas = FALSE; dirty = TRUE; }
                if (dirty) show_room(stage, room, atlas);
            }
        }
    }
    return 0;
}
