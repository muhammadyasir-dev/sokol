// src/animation.rs
use crate::vga_buffer;
use crate::Color;
use core::fmt::Write;

// RustOS Logo ASCII Art
const LOGO: &[&str] = &[
    "                                                  ",
    "              *****************************       ",
    "          *********************************      ",
    "       ******       *******************           ",
    "     ******        *******************            ",
    "    *****         *******************             ",
    "   *****         ****                             ",
    "   ****         ****                              ",
    "  ****         ****                               ",
    "  ****        *****                               ",
    "  ****       ******                               ",
    "  ****      *******   ******************          ",
    "  *****    ****************************************",
    "   *****  ************************************** ",
    "   *********************************               ",
    "    *******************************                ",
    "     *****************************                 ",
    "       *************************                   ",
    "          ********************                     ",
    "                                                  ",
];

// Constants for animation
const FRAME_DELAY: u64 = 5000000;
const ANIMATION_CYCLES: usize = 8;

// Function to create a delay (busy waiting)
fn delay(count: u64) {
    for _ in 0..count {
        core::hint::spin_loop();
    }
}

// Animation colors for the Rust logo (rust-orange gradient)
const LOGO_COLORS: [Color; 7] = [
    Color::LightRed,
    Color::Red,
    Color::Brown,
    Color::Yellow,
    Color::Brown,
    Color::Red,
    Color::LightRed,
];

// Helper to draw the logo with a specific color
fn draw_logo(color: Color) {
    let mut writer = vga_buffer::WRITER.lock();
    writer.set_cursor(2, 0);

    for line in LOGO {
        writer.set_color(vga_buffer::ColorCode::new(color, Color::Black));
        writeln!(writer, "{}", line).unwrap();
    }
}

// Clear the logo area
fn clear_logo_area() {
    let mut writer = vga_buffer::WRITER.lock();
    writer.set_color(vga_buffer::ColorCode::new(Color::White, Color::Black));

    for row in 2..(2 + LOGO.len()) {
        writer.set_cursor(row, 0);
        for _ in 0..80 {
            writer.write_byte(b' ');
        }
    }
}

// Main animation function
pub fn animate_logo() {
    // Initial loading animation
    let loading_text = "Loading RustOS...";
    {
        let mut writer = vga_buffer::WRITER.lock();
        writer.set_cursor(12, 30);
        writer.set_color(vga_buffer::ColorCode::new(Color::White, Color::Black));
        writer.write_string(loading_text);
    }

    // Progress bar animation
    for progress in 0..=40 {
        let mut writer = vga_buffer::WRITER.lock();
        writer.set_cursor(14, 20);
        writer.set_color(vga_buffer::ColorCode::new(Color::White, Color::Black));
        writer.write_string("[");

        for i in 0..40 {
            if i < progress {
                writer.set_color(vga_buffer::ColorCode::new(Color::Green, Color::Black));
                writer.write_byte(b'=');
            } else {
                writer.set_color(vga_buffer::ColorCode::new(Color::DarkGray, Color::Black));
                writer.write_byte(b' ');
            }
        }

        writer.set_color(vga_buffer::ColorCode::new(Color::White, Color::Black));
        writer.write_string("]");
        writer.set_cursor(14, 62);
        write!(writer, "{:3}%", progress * 100 / 40).unwrap();

        delay(FRAME_DELAY / 2);
    }

    // Clear loading text and progress bar
    {
        let mut writer = vga_buffer::WRITER.lock();
        for row in 12..15 {
            writer.set_cursor(row, 0);
            for _ in 0..80 {
                writer.write_byte(b' ');
            }
        }
    }

    // Animate the logo with color cycling
    for cycle in 0..ANIMATION_CYCLES {
        for (i, &color) in LOGO_COLORS.iter().enumerate() {
            draw_logo(color);

            // Add animation effect - color pulse
            if cycle == ANIMATION_CYCLES - 1 && i == LOGO_COLORS.len() - 1 {
                // For the last frame, hold longer
                delay(FRAME_DELAY * 2);
            } else {
                delay(FRAME_DELAY / 4);
            }
        }
    }

    // Final logo with static color
    draw_logo(Color::Brown);
    delay(FRAME_DELAY);

    // Clear the logo for the main interface
    clear_logo_area();
}
