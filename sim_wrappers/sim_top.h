#pragma once
#include <thread>
#include <functional>

typedef union
{
    struct
    {
        uint8_t mode: 5;
    };
    uint8_t bt;
} video_mode_u;

typedef union
{
    struct
    {
        uint8_t scr_no: 2;
        uint8_t : 4;
        uint8_t ram_reg_off: 1;
        uint8_t wide_scr: 1;
    };
    uint8_t bt;
} screen_mode_u;

typedef union
{
    struct
    {
        uint8_t pen : 4;
        uint8_t back: 4;
    };
    uint8_t bt;
} colors_pseudo_u;

#define SCREEN_HEIGHT 256
#define SCREEN_WIDTH_NORMAL 384
#define SCREEN_WIDTH_WIDE 512

typedef std::function<void()> thread_cb_t;

class SIM_TOP
{
public:
    SIM_TOP(int argc, const char** argv, thread_cb_t cb_to_draw, thread_cb_t cb_resize);
    int get_width() { return m_cur_width; }
    int get_height() { return SCREEN_HEIGHT; }
    uint32_t* get_screen() { return p_screen; }
    void stop() { m_active = false; }
private:
    std::thread*    p_thr;
    bool            m_active;
    int             m_cur_width;
    uint32_t        p_screen[SCREEN_HEIGHT*SCREEN_WIDTH_WIDE];
    uint8_t*        p_storage;
    video_mode_u*   p_video_mode;
    screen_mode_u*  p_screen_mode;
    colors_pseudo_u*p_colors_pseudo;
    uint8_t*        p_kbd_input;
    uint8_t*        p_kbd_output;
    thread_cb_t     m_cb_start_draw;
    thread_cb_t     m_cb_resize;

    void thread_main();
    void screen_proc();
};
