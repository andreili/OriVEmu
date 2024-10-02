#pragma once
#include <thread>
#include <functional>
#include <mutex>

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

typedef union
{
    struct
    {
        uint8_t pen : 4;
        uint8_t back: 4;
    };
    uint8_t bt;
} cfg_sw_u;

typedef union
{
    uint32_t dw;
    struct
    {
        uint8_t PA;
        uint8_t PB;
        uint8_t PC;
        uint8_t res;
    } bt;
} kbd_port_u;

#define SCREEN_HEIGHT 256
#define SCREEN_WIDTH_NORMAL 384
#define SCREEN_WIDTH_WIDE 512

enum class SIM_STATE
{
    INIT,
    IDLE,
    RUN,
    RUN_STEP,
    EXIT,
};

typedef std::function<void()> thread_cb_t;

class SIM_TOP
{
public:
    SIM_TOP(int argc, const char** argv, thread_cb_t cb_to_draw, thread_cb_t cb_resize);
    ~SIM_TOP();
    void run_cont()  { m_state = SIM_STATE::RUN;      m_mtx.unlock(); }
    void run_step()  { m_state = SIM_STATE::RUN_STEP; m_mtx.unlock(); }
    void run_pause() { m_state = SIM_STATE::IDLE;     m_mtx.lock();   }
    int get_width() { return m_cur_width; }
    int get_height() { return SCREEN_HEIGHT; }
    uint32_t* get_screen() { return p_screen; }
    void stop() { m_state = SIM_STATE::EXIT; }

    void load_rom1(std::string fn);
    void load_rom2(std::string fn);
    void load_rom_disk(std::string fn);

    void key_press(uint32_t key);
    void key_release(uint32_t key);
private:
    std::thread*    p_thr;
    int             m_cur_width;
    uint32_t        p_screen[SCREEN_HEIGHT*SCREEN_WIDTH_WIDE];
    uint8_t*        p_storage;
    cfg_sw_u*       p_cfg_sw;
    video_mode_u*   p_video_mode;
    screen_mode_u*  p_screen_mode;
    colors_pseudo_u*p_colors_pseudo;
    kbd_port_u*     p_kbd_input;
    kbd_port_u*     p_kbd_output;
    thread_cb_t     m_cb_start_draw;
    thread_cb_t     m_cb_resize;
    SIM_STATE       m_state;
    std::mutex      m_mtx;
    uint32_t        m_keys_matrix[20];

    uint8_t*        p_rom1;
    uint32_t*       p_rom1_raddr;
    uint32_t*       p_rom1_rdata;
    uint32_t        m_rom1_size;
    uint8_t*        p_rom2;
    uint32_t*       p_rom2_raddr;
    uint32_t*       p_rom2_rdata;
    uint32_t        m_rom2_size;
    uint8_t*        p_rom_disk;
    uint32_t*       p_rom_disk_raddr;
    uint32_t*       p_rom_disk_rdata;
    uint32_t        m_rom_disk_size;

    void thread_main();
    void screen_proc();
    void kbd_proc();
    void rom_proc();
};
