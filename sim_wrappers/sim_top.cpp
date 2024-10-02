#include "sim_top.h"
#include "tb.h"
#include CONCAT5(V,TOP_NAME,_,TOP_NAME,.h)
#include CONCAT5(V,TOP_NAME,_,orion_pro_top,.h)
#include <iostream>
#include <fstream>
#include <QEvent>

double sc_time_stamp() { return 0; }

#define TICK_TIME 2
#define TICK_PERIOD (TICK_TIME / 2)
#define SIM_TIME_MAX (1000*10)
#define SIM_TIME_MAX_TICK (TICK_TIME * SIM_TIME_MAX)
#define RGB(b,g,r) (((r) << 16) | ((g) << 8) | (b))

static TB* obj_tb;

int key_matrix[8][11] = {
    /*r D0*/  { '*',                      Qt::Key_Escape,           '+',                      Qt::Key_F1,               Qt::Key_F2,     Qt::Key_F3,        '4',           Qt::Key_F4,    Qt::Key_F5, '7',          '8' },
    /*e D1*/  { Qt::Key_Minus,            Qt::Key_Tab,              'J' ,                     '1',                      '2',            '3',               'E',           '5',           '6',        '[',          ']' },
    /*s D2*/  { 0,                        Qt::Key_CapsLock,         'F' ,                     'C',                      'U',            'K',               'P',           'N',           'G',        'L',          'D' },
    /*u D3*/  { 0,                        0,                        'Q' ,                     'Y',                      'W',            'A',               'I',           'R',           'O',        'B',          0   },
    /*l D4*/  { Qt::Key_Shift,            Qt::Key_Control,          0,                        0xDE,                     'S',            'M',               ' ',           'T',           'X',        Qt::Key_Left, '<' },
    /*t D5*/  { '7' | Qt::KeypadModifier, '0' | Qt::KeypadModifier, '1' | Qt::KeypadModifier, '4' | Qt::KeypadModifier, Qt::Key_Plus,   Qt::Key_Backspace, Qt::Key_Right,  Qt::Key_Down, '>',        '\\',         'V' },
    /*  D6*/  { '8' | Qt::KeypadModifier, '.',                      '2' | Qt::KeypadModifier, '5' | Qt::KeypadModifier, Qt::Key_F6,     Qt::Key_Home,      Qt::Key_Return, Qt::Key_Up,   '/',        'H',          'Z' },
    /*  D7*/  { '9' | Qt::KeypadModifier, Qt::Key_Return,           '3' | Qt::KeypadModifier, '6' | Qt::KeypadModifier, Qt::Key_Insert, Qt::Key_End,       ';',            '?',          '-',        '0',          '9' }};
/*scancode  D0                        D1                        D2                        D3                        D4              D5                 D6              D7            CD0         CD1           CD2 */


static int on_step_cb(uint64_t time, TOP_CLASS* p_top)
{
    p_top->i_clk = !p_top->i_clk;
    return 0;
}

SIM_TOP::SIM_TOP(int argc, const char** argv, thread_cb_t cb_to_draw, thread_cb_t cb_resize)
{
    m_cur_width = 0;
    m_cb_start_draw = cb_to_draw;
    m_cb_resize = cb_resize;
    m_state = SIM_STATE::INIT;
    //this->p_gui = p_gui;
    //this->p_kbd = p_kbd;
    obj_tb = new TB(TOP_NAME_STR, argc, argv);
    obj_tb->init(on_step_cb);
    TOP_CLASS* top = obj_tb->get_top();
    p_storage        = (uint8_t*)top->TOP_NAME->u_orion_core->ram.m_storage;
    p_cfg_sw         = (cfg_sw_u*)       &top->TOP_NAME->cfg_sw;
    p_rom1_raddr     = (uint32_t*)       &top->TOP_NAME->rom1_addr;
    p_rom1_rdata     = (uint32_t*)       &top->TOP_NAME->rom1_rdata;
    p_rom2_raddr     = (uint32_t*)       &top->TOP_NAME->rom2_addr;
    p_rom2_rdata     = (uint32_t*)       &top->TOP_NAME->rom2_rdata;
    p_rom_disk_raddr = (uint32_t*)       &top->TOP_NAME->rom_disk_addr;
    p_rom_disk_rdata = (uint32_t*)       &top->TOP_NAME->rom_disk_rdata;
    p_video_mode     = (video_mode_u*)   &top->TOP_NAME->u_orion_core->video_mode;
    p_screen_mode    = (screen_mode_u*)  &top->TOP_NAME->u_orion_core->screen_mode;
    p_colors_pseudo  = (colors_pseudo_u*)&top->TOP_NAME->u_orion_core->colors_pseudo;
    p_kbd_input      = (kbd_port_u*)     &top->TOP_NAME->u_orion_core->kbd_input;
    p_kbd_output     = (kbd_port_u*)     &top->TOP_NAME->u_orion_core->kbd_output;
    p_kbd_input->dw = 0xffffffff;
    p_rom1 = nullptr;
    p_rom2 = nullptr;
    p_rom_disk = nullptr;
    m_rom1_size = 0;
    m_rom2_size = 0;
    m_rom_disk_size = 0;
    p_thr = new std::thread(&SIM_TOP::thread_main, this);
    for (int i=0 ; i<20 ; ++i)
    {
        m_keys_matrix[i] = 0;
    }

    // TODO
    p_cfg_sw->bt = 0b00001011;
}

SIM_TOP::~SIM_TOP()
{
    m_state = SIM_STATE::EXIT;
    // wait for sim thread finished
    p_thr->join();
    if (p_rom1 != nullptr)
    {
        delete p_rom1;
    }
    if (p_rom2 != nullptr)
    {
        delete p_rom2;
    }
    if (p_rom_disk != nullptr)
    {
        delete p_rom_disk;
    }
}

void SIM_TOP::load_rom1(std::string fn)
{
    if (p_rom1 != nullptr)
    {
        delete p_rom1;
    }
    std::ifstream file(fn, std::ios::binary | std::ios::ate);
    m_rom1_size = file.tellg();
    p_rom1 = new uint8_t[m_rom1_size];
    file.seekg(0, std::ios::beg);
    file.read((char*)p_rom1, m_rom1_size);
    file.close();
}

void SIM_TOP::load_rom2(std::string fn)
{
    if (p_rom2 != nullptr)
    {
        delete p_rom2;
    }
    std::ifstream file(fn, std::ios::binary | std::ios::ate);
    m_rom2_size = file.tellg();
    p_rom2 = new uint8_t[m_rom2_size];
    file.seekg(0, std::ios::beg);
    file.read((char*)p_rom2, m_rom2_size);
    file.close();
}

void SIM_TOP::load_rom_disk(std::string fn)
{
    if (p_rom_disk != nullptr)
    {
        delete p_rom_disk;
    }
    std::ifstream file(fn, std::ios::binary | std::ios::ate);
    m_rom_disk_size = file.tellg();
    p_rom_disk = new uint8_t[m_rom_disk_size];
    file.seekg(0, std::ios::beg);
    file.read((char*)p_rom_disk, m_rom_disk_size);
    file.close();
}

void SIM_TOP::key_press(uint32_t key)
{
    for (int j=0 ; j<11 ; ++j)
    {
        uint32_t line = 0;
        for (int i=0 ; i<8 ; ++i)
        {
            if (key_matrix[i][j] == key)
            {
                line |= (1 << i);
            }
        }
        m_keys_matrix[j] = line;
    }
}

void SIM_TOP::key_release(uint32_t key)
{
    for (int j=0 ; j<11 ; ++j)
    {
        uint32_t line = 0;
        for (int i=0 ; i<8 ; ++i)
        {
            if (key_matrix[i][j] == key)
            {
                line |= (1 << i);
            }
        }
        m_keys_matrix[j] &= ~line;
    }
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
    bool screen_refresh;
    while (m_state != SIM_STATE::EXIT)
    {
        m_mtx.lock();
        switch (m_state)
        {
        case SIM_STATE::INIT:
            m_mtx.lock();
            break;
        case SIM_STATE::IDLE:
            m_mtx.lock();
            break;
        case SIM_STATE::RUN:
            obj_tb->run_steps(2);
            screen_refresh = (++screen_cycle == screen_period);
            break;
        case SIM_STATE::RUN_STEP:
            obj_tb->run_steps(2);
            screen_refresh = true;
            break;
        case SIM_STATE::EXIT:
            return;
        }

        //sim_time = (cycle * 1.f) / cycle_len;
        kbd_proc();
        rom_proc();
        if (screen_refresh)
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
        if (m_state == SIM_STATE::RUN_STEP)
        {
            // pause to next step or state change
            //m_mtx.lock();
        }
        m_mtx.unlock();
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

void SIM_TOP::kbd_proc()
{
    uint32_t scancode_msk = ((p_kbd_output->bt.PC & 0x7) << 8) | p_kbd_output->bt.PB;
    if (scancode_msk != 0b11111111111)
    {
        uint32_t result = 0;
        for (int j=0 ; j<11 ; ++j)
        {
            if (((scancode_msk & (1 << j)) == 0) && (m_keys_matrix[j] != 0))
            {
                result |= m_keys_matrix[j];
            }
        }
        p_kbd_input->bt.PA = ~result;
    }
}

void SIM_TOP::rom_proc()
{
    if (*p_rom1_raddr < m_rom1_size)
    {
        *p_rom1_rdata     = p_rom1    [*p_rom1_raddr];
    }
    if (*p_rom2_raddr < m_rom2_size)
    {
        *p_rom2_rdata     = p_rom2    [*p_rom2_raddr];
    }
    if (*p_rom_disk_raddr < m_rom_disk_size)
    {
        *p_rom_disk_rdata = p_rom_disk[*p_rom_disk_raddr];
    }
}
