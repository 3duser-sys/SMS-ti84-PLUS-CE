#include <tice.h>
#include <graphx.h>
#include <keypadc.h>
#include <fileioc.h>
#include <stdio.h>
#include <string.h>
#include "core/sms_single.c" // Single-file SMS core

#define MAX_ROM_FILES 32
#define ROM_PATH_MAX 32

// Framebuffer for SMS (256x192)
uint8_t sms_framebuffer[192][256];

// Map CE key -> SMS button
uint8_t sms_joypad_state()
{
    kb_scan();
    uint8_t state = 0;

    if (kb_Data[7] & kb_Up)
        state |= 0x01; // UP
    if (kb_Data[7] & kb_Down)
        state |= 0x02; // DOWN
    if (kb_Data[7] & kb_Left)
        state |= 0x04; // LEFT
    if (kb_Data[7] & kb_Right)
        state |= 0x08; // RIGHT
    if (kb_Data[6] & kb_2nd)
        state |= 0x10; // Button 1
    if (kb_Data[6] & kb_Mode)
        state |= 0x20; // Button 2

    return state;
}

// Draw SMS framebuffer to CE screen
void sms_render_frame_to_gfx(uint8_t framebuffer[192][256])
{
    for (int y = 0; y < 192; y++)
    {
        for (int x = 0; x < 256; x++)
        {
            gfx_SetColor(framebuffer[y][x]);
            gfx_FillRectangle(x, y, 1, 1);
        }
    }
    gfx_SwapDraw();
}

// Scan CE storage for .sms files
int scan_roms(char rom_list[MAX_ROM_FILES][ROM_PATH_MAX])
{
    ti_var_t f;
    char name[ROM_PATH_MAX];
    int count = 0;

    f = ti_Open("", "r"); // root directory
    while (ti_DirectoryGetNext(name) && count < MAX_ROM_FILES)
    {
        if (strstr(name, ".sms") || strstr(name, ".SMS"))
        {
            strncpy(rom_list[count], name, ROM_PATH_MAX - 1);
            rom_list[count][ROM_PATH_MAX - 1] = '\0';
            count++;
        }
    }
    return count;
}

// Load ROM from file
int load_sms_rom(const char *filename)
{
    ti_var_t rom = ti_Open(filename, "r");
    if (!rom)
        return 0;

    long size = ti_GetSize(rom);
    if (size > 0x40000)
        size = 0x40000; // 256 KB max

    uint8_t *rom_buffer = sms_get_rom_buffer(); // from SMS core
    ti_Read(rom_buffer, 1, size, rom);
    ti_Close(rom);

    return 1;
}

// Simple file browser to pick a ROM
int pick_rom(char rom_list[MAX_ROM_FILES][ROM_PATH_MAX], int count)
{
    int index = 0;
    while (1)
    {
        gfx_ZeroScreen();
        for (int i = 0; i < count; i++)
        {
            if (i == index)
                gfx_SetColor(2); // highlight
            else
                gfx_SetColor(15);
            gfx_PrintStringXY(rom_list[i], 10, 10 + i * 10);
        }
        gfx_SwapDraw();

        kb_scan();
        if (kb_Data[7] & kb_Up)
        {
            index--;
            if (index < 0)
                index = count - 1;
        }
        if (kb_Data[7] & kb_Down)
        {
            index++;
            if (index >= count)
                index = 0;
        }
        if (kb_Data[6] & kb_Enter)
            return index; // select ROM
        if (kb_Data[6] & kb_Clear)
            return -1; // cancel
        delay(100);
    }
}

int main(void)
{
    // Initialize graphics
    gfx_Begin();
    gfx_SetDrawBuffer();

    // Scan for ROMs
    char rom_list[MAX_ROM_FILES][ROM_PATH_MAX];
    int rom_count = scan_roms(rom_list);
    if (rom_count == 0)
    {
        gfx_PrintStringXY("No .SMS ROMs found!", 10, 10);
        gfx_SwapDraw();
        while (!kb_IsDown(kb_Clear))
            ;
        gfx_End();
        return 1;
    }

    // Pick a ROM
    int selected = pick_rom(rom_list, rom_count);
    if (selected < 0)
    {
        gfx_End();
        return 1;
    }

    // Initialize SMS core
    sms_init();

    // Load selected ROM
    if (!load_sms_rom(rom_list[selected]))
    {
        gfx_PrintStringXY("Failed to load ROM!", 10, 10);
        gfx_SwapDraw();
        while (!kb_IsDown(kb_Clear))
            ;
        gfx_End();
        return 1;
    }

    // Main loop
    while (!kb_IsDown(kb_Clear))
    {
        uint8_t joy = sms_joypad_state();
        sms_set_joypad(joy);                      // pass input
        sms_step();                               // run one frame
        sms_render_frame_to_gfx(sms_framebuffer); // render frame
    }

    gfx_End();
    return 0;
}
