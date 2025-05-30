#include <stdint.h>
#include <stdbool.h>

// VGA Constants
#define VGA_WIDTH 320
#define VGA_HEIGHT 200
#define VGA_MEMORY 0xA0000
#define VGA_AC_INDEX 0x3C0
#define VGA_AC_WRITE 0x3C0
#define VGA_AC_READ 0x3C1
#define VGA_MISC_WRITE 0x3C2
#define VGA_SEQ_INDEX 0x3C4
#define VGA_SEQ_DATA 0x3C5
#define VGA_DAC_READ_INDEX 0x3C7
#define VGA_DAC_WRITE_INDEX 0x3C8
#define VGA_DAC_DATA 0x3C9
#define VGA_MISC_READ 0x3CC
#define VGA_GC_INDEX 0x3CE
#define VGA_GC_DATA 0x3CF
#define VGA_CRTC_INDEX 0x3D4
#define VGA_CRTC_DATA 0x3D5
#define VGA_INSTAT_READ 0x3DA

// Color definitions
#define COLOR_BLACK 0x00
#define COLOR_WHITE 0x0F
#define COLOR_RED 0x04
#define COLOR_GREEN 0x02
#define COLOR_BLUE 0x01
#define COLOR_YELLOW 0x0E
#define COLOR_CYAN 0x0B
#define COLOR_MAGENTA 0x0D

// GUI Constants
#define MAX_WINDOWS 16
#define TITLE_BAR_HEIGHT 20
#define BORDER_WIDTH 2

// Hardware I/O functions
static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Memory management
static uint8_t *framebuffer = (uint8_t*)VGA_MEMORY;
static uint8_t backbuffer[VGA_WIDTH * VGA_HEIGHT];

// Font data (8x8 bitmap font)
static const uint8_t font_8x8[256][8] = {
    // Basic ASCII characters - simplified for demo
    [' '] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    ['A'] = {0x18, 0x3C, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x00},
    ['B'] = {0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00},
    ['C'] = {0x3C, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3C, 0x00},
    ['D'] = {0x78, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0x78, 0x00},
    ['E'] = {0x7E, 0x60, 0x60, 0x78, 0x60, 0x60, 0x7E, 0x00},
    ['F'] = {0x7E, 0x60, 0x60, 0x78, 0x60, 0x60, 0x60, 0x00},
    ['G'] = {0x3C, 0x66, 0x60, 0x6E, 0x66, 0x66, 0x3C, 0x00},
    ['H'] = {0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00},
    ['I'] = {0x3C, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00},
    ['J'] = {0x0E, 0x06, 0x06, 0x06, 0x66, 0x66, 0x3C, 0x00},
    ['0'] = {0x3C, 0x66, 0x6E, 0x76, 0x66, 0x66, 0x3C, 0x00},
    ['1'] = {0x18, 0x38, 0x18, 0x18, 0x18, 0x18, 0x7E, 0x00},
    ['2'] = {0x3C, 0x66, 0x06, 0x1C, 0x30, 0x60, 0x7E, 0x00},
    ['3'] = {0x3C, 0x66, 0x06, 0x1C, 0x06, 0x66, 0x3C, 0x00},
};

// Rectangle structure
typedef struct {
    int x, y, width, height;
} rect_t;

// Window structure
typedef struct {
    rect_t bounds;
    char title[32];
    uint8_t bg_color;
    uint8_t border_color;
    bool visible;
    bool focused;
} window_t;

// GUI State
static window_t windows[MAX_WINDOWS];
static int window_count = 0;
static int focused_window = -1;

// VGA Mode 13h initialization
void init_vga_mode13h(void) {
    // VGA registers configuration for Mode 13h (320x200x256)
    static const uint8_t g_320x200x256[] = {
        // MISC
        0x63,
        // SEQ
        0x03, 0x01, 0x0F, 0x00, 0x0E,
        // CRTC
        0x5F, 0x4F, 0x50, 0x82, 0x54, 0x80, 0xBF, 0x1F,
        0x00, 0x41, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x9C, 0x0E, 0x8F, 0x28, 0x40, 0x96, 0xB9, 0xA3,
        0xFF,
        // GC
        0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x05, 0x0F,
        0xFF,
        // AC
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
        0x41, 0x00, 0x0F, 0x00, 0x00
    };

    const uint8_t *p = g_320x200x256;
    
    // Write MISC register
    outb(VGA_MISC_WRITE, *p++);
    
    // Write SEQ registers
    for (int i = 0; i < 5; i++) {
        outb(VGA_SEQ_INDEX, i);
        outb(VGA_SEQ_DATA, *p++);
    }
    
    // Unlock CRTC registers
    outb(VGA_CRTC_INDEX, 0x03);
    outb(VGA_CRTC_DATA, inb(VGA_CRTC_DATA) | 0x80);
    outb(VGA_CRTC_INDEX, 0x11);
    outb(VGA_CRTC_DATA, inb(VGA_CRTC_DATA) & ~0x80);
    
    // Write CRTC registers
    for (int i = 0; i < 25; i++) {
        outb(VGA_CRTC_INDEX, i);
        outb(VGA_CRTC_DATA, *p++);
    }
    
    // Write GC registers
    for (int i = 0; i < 9; i++) {
        outb(VGA_GC_INDEX, i);
        outb(VGA_GC_DATA, *p++);
    }
    
    // Write AC registers
    inb(VGA_INSTAT_READ);
    for (int i = 0; i < 21; i++) {
        outb(VGA_AC_INDEX, i);
        outb(VGA_AC_WRITE, *p++);
    }
    
    // Enable video
    inb(VGA_INSTAT_READ);
    outb(VGA_AC_INDEX, 0x20);
}

// Basic graphics primitives
void put_pixel(int x, int y, uint8_t color) {
    if (x >= 0 && x < VGA_WIDTH && y >= 0 && y < VGA_HEIGHT) {
        backbuffer[y * VGA_WIDTH + x] = color;
    }
}

void draw_line(int x0, int y0, int x1, int y1, uint8_t color) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    int steps = (dx > dy ? dx : dy);
    if (steps < 0) steps = -steps;
    
    float x_inc = (float)dx / steps;
    float y_inc = (float)dy / steps;
    
    float x = x0, y = y0;
    for (int i = 0; i <= steps; i++) {
        put_pixel((int)x, (int)y, color);
        x += x_inc;
        y += y_inc;
    }
}

void fill_rect(rect_t rect, uint8_t color) {
    for (int y = rect.y; y < rect.y + rect.height; y++) {
        for (int x = rect.x; x < rect.x + rect.width; x++) {
            put_pixel(x, y, color);
        }
    }
}

void draw_rect(rect_t rect, uint8_t color) {
    // Top and bottom edges
    for (int x = rect.x; x < rect.x + rect.width; x++) {
        put_pixel(x, rect.y, color);
        put_pixel(x, rect.y + rect.height - 1, color);
    }
    
    // Left and right edges
    for (int y = rect.y; y < rect.y + rect.height; y++) {
        put_pixel(rect.x, y, color);
        put_pixel(rect.x + rect.width - 1, y, color);
    }
}

void draw_char(int x, int y, char c, uint8_t color) {
    if (c < 0 || c >= 256) return;
    
    const uint8_t *glyph = font_8x8[(unsigned char)c];
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (glyph[row] & (0x80 >> col)) {
                put_pixel(x + col, y + row, color);
            }
        }
    }
}

void draw_string(int x, int y, const char *str, uint8_t color) {
    while (*str) {
        draw_char(x, y, *str, color);
        x += 8;
        str++;
    }
}

// Window management functions
int create_window(int x, int y, int width, int height, const char *title, uint8_t bg_color) {
    if (window_count >= MAX_WINDOWS) return -1;
    
    int id = window_count++;
    windows[id].bounds.x = x;
    windows[id].bounds.y = y;
    windows[id].bounds.width = width;
    windows[id].bounds.height = height;
    windows[id].bg_color = bg_color;
    windows[id].border_color = COLOR_WHITE;
    windows[id].visible = true;
    windows[id].focused = false;
    
    // Copy title
    int i = 0;
    while (title[i] && i < 31) {
        windows[id].title[i] = title[i];
        i++;
    }
    windows[id].title[i] = '\0';
    
    return id;
}

void draw_window(int window_id) {
    if (window_id < 0 || window_id >= window_count || !windows[window_id].visible) {
        return;
    }
    
    window_t *win = &windows[window_id];
    
    // Draw window background
    fill_rect(win->bounds, win->bg_color);
    
    // Draw border
    uint8_t border_color = win->focused ? COLOR_YELLOW : win->border_color;
    for (int i = 0; i < BORDER_WIDTH; i++) {
        rect_t border_rect = {
            win->bounds.x - i,
            win->bounds.y - i,
            win->bounds.width + 2 * i,
            win->bounds.height + 2 * i
        };
        draw_rect(border_rect, border_color);
    }
    
    // Draw title bar
    rect_t title_rect = {
        win->bounds.x - BORDER_WIDTH,
        win->bounds.y - TITLE_BAR_HEIGHT - BORDER_WIDTH,
        win->bounds.width + 2 * BORDER_WIDTH,
        TITLE_BAR_HEIGHT
    };
    fill_rect(title_rect, win->focused ? COLOR_BLUE : COLOR_CYAN);
    draw_rect(title_rect, border_color);
    
    // Draw title text
    draw_string(win->bounds.x, win->bounds.y - TITLE_BAR_HEIGHT - BORDER_WIDTH + 6, 
                win->title, COLOR_WHITE);
}

void focus_window(int window_id) {
    if (window_id < 0 || window_id >= window_count) return;
    
    // Unfocus current window
    if (focused_window >= 0) {
        windows[focused_window].focused = false;
    }
    
    // Focus new window
    focused_window = window_id;
    windows[window_id].focused = true;
}

void clear_screen(uint8_t color) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        backbuffer[i] = color;
    }
}

void present_frame(void) {
    // Copy backbuffer to framebuffer
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        framebuffer[i] = backbuffer[i];
    }
}

// Main GUI initialization and demo
void init_gui_driver(void) {
    // Initialize VGA hardware
    init_vga_mode13h();
    
    // Clear screen
    clear_screen(COLOR_BLACK);
    
    // Create some demo windows
    int win1 = create_window(50, 50, 120, 80, "Terminal", COLOR_BLACK);
    int win2 = create_window(100, 80, 150, 100, "File Manager", COLOR_BLUE);
    int win3 = create_window(30, 120, 100, 60, "Calculator", COLOR_GREEN);
    
    focus_window(win1);
    
    // Main render loop (simplified - in real implementation would be event-driven)
    for (int frame = 0; frame < 1000; frame++) {
        clear_screen(COLOR_BLACK);
        
        // Draw desktop background pattern
        for (int y = 0; y < VGA_HEIGHT; y += 20) {
            for (int x = 0; x < VGA_WIDTH; x += 20) {
                put_pixel(x, y, COLOR_CYAN);
            }
        }
        
        // Draw all windows
        for (int i = 0; i < window_count; i++) {
            draw_window(i);
        }
        
        // Draw some demo content in focused window
        if (focused_window >= 0) {
            window_t *win = &windows[focused_window];
            draw_string(win->bounds.x + 5, win->bounds.y + 10, "GUI Demo", COLOR_WHITE);
            draw_string(win->bounds.x + 5, win->bounds.y + 25, "Running!", COLOR_GREEN);
        }
        
        // Simple animation - cycle focus
        if (frame % 200 == 0) {
            focus_window((focused_window + 1) % window_count);
        }
        
        present_frame();
        
        // Simple delay
        for (volatile int i = 0; i < 100000; i++);
    }
}

// Entry point
void _start(void) {
    init_gui_driver();
    
    // Halt system
    while (1) {
        __asm__ volatile ("hlt");
    }
}
