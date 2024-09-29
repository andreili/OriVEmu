#include "sim_top.h"
#include "tb.h"
#include CONCAT5(V,TOP_NAME,_,TOP_NAME,.h)
#include CONCAT5(V,TOP_NAME,_,orion_pro_top,.h)

double sc_time_stamp() { return 0; }

#define TICK_TIME 2
#define TICK_PERIOD (TICK_TIME / 2)
#define SIM_TIME_MAX (1000*10)
#define SIM_TIME_MAX_TICK (TICK_TIME * SIM_TIME_MAX)
#define RGB(b,g,r) (((r) << 16) | ((g) << 8) | (b))

static TB* obj_tb;

static int on_step_cb(uint64_t time, TOP_CLASS* p_top)
{
    p_top->i_clk = !p_top->i_clk;
    return 0;
}

SIM_TOP::SIM_TOP(int argc, const char** argv, thread_cb_t cb_to_draw, thread_cb_t cb_resize)
{
    m_active = true;
    m_cur_width = 0;
    m_cb_start_draw = cb_to_draw;
    m_cb_resize = cb_resize;
    //this->p_gui = p_gui;
    //this->p_kbd = p_kbd;
    obj_tb = new TB(TOP_NAME_STR, argc, argv);
    obj_tb->init(on_step_cb);
    TOP_CLASS* top = obj_tb->get_top();
    p_storage       = (uint8_t*)top->TOP_NAME->u_orion_core->ram.m_storage;
    p_video_mode    = (video_mode_u*)   &top->TOP_NAME->u_orion_core->video_mode;
    p_screen_mode   = (screen_mode_u*)  &top->TOP_NAME->u_orion_core->screen_mode;
    p_colors_pseudo = (colors_pseudo_u*)&top->TOP_NAME->u_orion_core->colors_pseudo;
    p_kbd_input     = (uint8_t*)&top->TOP_NAME->u_orion_core->kbd_input;
    p_kbd_output    = (uint8_t*)&top->TOP_NAME->u_orion_core->kbd_output;
    p_thr = new std::thread(&SIM_TOP::thread_main, this);
    //p_thr->join();
}

void SIM_TOP::thread_main()
{
    TOP_CLASS* top = obj_tb->get_top();

    // wait for reset
    top->i_reset_n = 0;
    obj_tb->run_steps(20 * TICK_TIME);
    top->i_reset_n = 1;

    const uint32_t tick_speed = (10 * 1000 * 1000); // 10MHZ
    const uint32_t screen_period = tick_speed / 50;
    //float sim_time;
    uint32_t screen_cycle = 0;
    uint32_t sec_cycle = 0;
    time_t time_prev = time(0);
    while (m_active)
    {
        obj_tb->run_steps(2);
        //sim_time = (cycle * 1.f) / cycle_len;

        //p_instance->p_gui->draw(sim_time);
        if (++screen_cycle == screen_period)
        {
            screen_proc();
            m_cb_start_draw();
            screen_cycle = 0;
        }
        if (++sec_cycle == tick_speed)
        {
            time_t time_new = time(0);
            time_t delta = time_new - time_prev;
            time_prev = time_new;
            printf("Sim time for 1 second: %ld\n", delta);
            sec_cycle = 0;
        }
    }

    obj_tb->finish();
    top->final();
}

void SIM_TOP::screen_proc()
{
    int width;
    width = (p_screen_mode->wide_scr) ? 512 : 384;
    if (m_cur_width != width)
    {
        printf("Set width %d->%d\n", m_cur_width, width);
        m_cur_width = width;
        m_cb_resize();
    }
    for (int i=0 ; i<SCREEN_HEIGHT*m_cur_width ; ++i)
    {
        p_screen[i] = 0;
    }
    if ((p_video_mode->mode == 2) || (p_video_mode->mode == 3))
    {
        return;
    }
    int scr_no = (p_video_mode->mode > 15) ? (p_screen_mode->scr_no | 1) : p_screen_mode->scr_no;
    uintptr_t scr_start_addr;
    switch (scr_no)
    {
    case 0: scr_start_addr = 0x0c000;
        break;
    case 1: scr_start_addr = 0x08000;
        break;
    case 2: scr_start_addr = 0x04000;
        break;
    case 3: scr_start_addr = 0x00000;
        break;
    }
    uintptr_t scr_plane_0, scr_plane_1, scr_plane_2, scr_plane_3;
    scr_plane_0 = scr_start_addr;
    scr_plane_1 = scr_start_addr + 0x04000;
    scr_plane_2 = scr_start_addr + 0x10000;
    scr_plane_3 = scr_start_addr + 0x14000;
    uint32_t c0, c1, c2, c3;
    switch (p_video_mode->mode)
    {
    case 0: c0 = RGB(0x00, 0x00, 0x00); c1 = RGB(0x00, 0xff, 0x00);
        break;
    case 1: c0 = RGB(0xc8, 0xb4, 0x28); c1 = RGB(0x32, 0xfa, 0xfa);
        break;
    case 4: c0 = RGB(0x00, 0x00, 0x00); c1 = RGB(0x00, 0x00, 0xc0); c2 = RGB(0x00, 0xc0, 0x00); c3 = RGB(0xc0, 0x00, 0x00);
        break;
    case 5: c0 = RGB(0xc0, 0xc0, 0xc0); c1 = RGB(0x00, 0x00, 0xc0); c2 = RGB(0x00, 0xc0, 0x00); c3 = RGB(0xc0, 0x00, 0x00);
        break;
    }
    uint32_t cur_pxl;
    for (int x=0 ; x<m_cur_width ; x+=8)
    {
        for (int y=0 ; y<SCREEN_HEIGHT ; ++y)
        {
            uint8_t pixels0 = p_storage[(scr_plane_0 + ((x >> 3) << 8)) | y];
            uint8_t pixels1 = p_storage[(scr_plane_1 + ((x >> 3) << 8)) | y];
            uint8_t pixels2 = p_storage[(scr_plane_2 + ((x >> 3) << 8)) | y];
            uint8_t pixels3 = p_storage[(scr_plane_3 + ((x >> 3) << 8)) | y];
            if ((p_video_mode->mode == 14) || (p_video_mode->mode == 15))
            {
                pixels2 = p_colors_pseudo->bt;
            }
            if ((p_video_mode->mode == 6)  || (p_video_mode->mode == 7) ||
                (p_video_mode->mode == 14) || (p_video_mode->mode == 15))
            {
                uint32_t ci, cr, cg, cb;
                ci = (pixels2 & (1 << 7)) >> 1;
                cr = (pixels2 & (1 << 6)) ? (ci | 0xbf) : 0x00;
                cg = (pixels2 & (1 << 5)) ? (ci | 0xbf) : 0x00;
                cb = (pixels2 & (1 << 4)) ? (ci | 0xbf) : 0x00;
                c0 = RGB(cb, cg, cr);
                ci = (pixels2 & (1 << 3)) << 3;
                cr = (pixels2 & (1 << 2)) ? (ci | 0xbf) : 0x00;
                cg = (pixels2 & (1 << 1)) ? (ci | 0xbf) : 0x00;
                cb = (pixels2 & (1 << 0)) ? (ci | 0xbf) : 0x00;
                c1 = RGB(cb, cg, cr);
            }
            for (int i=0 ; i<8 ; ++i)
            {
                uint8_t pxl0 = (pixels0 & 0x80);
                uint8_t pxl1 = (pixels1 & 0x80);
                uint8_t pxl2 = (pixels2 & 0x80);
                uint8_t pxl3 = (pixels3 & 0x80);
                switch (p_video_mode->mode)
                {
                case 0:
                case 1:
                case 6:
                case 7:
                case 14:
                case 15:
                    cur_pxl = (pxl0) ? c1 : c0;
                    break;
                case 4:
                case 5:
                    switch ((pxl0 << 1) | pxl2)
                    {
                    case 0: cur_pxl = c0;
                        break;
                    case 1: cur_pxl = c1;
                        break;
                    case 2: cur_pxl = c2;
                        break;
                    default:cur_pxl = c3;
                        break;
                    }
                    break;
                }
                switch (p_video_mode->mode & 0x14)
                {
                case 0x10:
                    c0 = 0;
                    if (pxl0) { c0 |= RGB(0x00, 0x00, 0xbf); }
                    if (pxl1) { c0 |= RGB(0x00, 0xbf, 0x00); }
                    if (pxl2) { c0 |= RGB(0xbf, 0x00, 0x00); }
                    cur_pxl = c0;
                    break;
                case 0x14:
                    c0 = 0;
                    if (pxl0) { c0 |= RGB(0x00, 0x00, 0xbf); }
                    if (pxl1) { c0 |= RGB(0x00, 0xbf, 0x00); }
                    if (pxl2) { c0 |= RGB(0xbf, 0x00, 0x00); }
                    if (pxl3) { c0 |= RGB(0x40, 0x40, 0x40); }
                    cur_pxl = c0;
                    break;
                }
                //[x+i, y] = cur_pxl
                p_screen[x + i + (y * m_cur_width)] = cur_pxl;
                pixels0 <<= 1;
                pixels1 <<= 1;
                pixels2 <<= 1;
                pixels3 <<= 1;
            }
        }
    }
}
