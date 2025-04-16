// src/main.rs
#![no_std] // don't link the Rust standard library
#![no_main] // disable all Rust-level entry points
#![feature(abi_x86_interrupt)] // enable x86 interrupt support

use bootloader::{entry_point, BootInfo};
use core::fmt::Write;
use core::panic::PanicInfo;
use lazy_static::lazy_static;
use spin::Mutex;
use x86_64::instructions::interrupts;
use x86_64::instructions::port::Port;

mod animation;
mod gdt;
mod interrupts_mod;
mod vga_buffer;

// Define custom entry point since we don't use main
entry_point!(kernel_main);

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum Color {
    Black = 0,
    Blue = 1,
    Green = 2,
    Cyan = 3,
    Red = 4,
    Magenta = 5,
    Brown = 6,
    LightGray = 7,
    DarkGray = 8,
    LightBlue = 9,
    LightGreen = 10,
    LightCyan = 11,
    LightRed = 12,
    Pink = 13,
    Yellow = 14,
    White = 15,
}

/// The kernel's main function, called by the bootloader
fn kernel_main(boot_info: &'static BootInfo) -> ! {
    // Initialize our OS components
    gdt::init();
    interrupts_mod::init_idt();

    // Clear the screen
    vga_buffer::WRITER.lock().clear_screen();

    // Draw logo and animate
    animation::animate_logo();

    // Print a hello world message
    interrupts::without_interrupts(|| {
        let mut writer = vga_buffer::WRITER.lock();
        writer.set_color(vga_buffer::ColorCode::new(Color::Green, Color::Black));
        writeln!(writer, "\n\n").unwrap();
        writeln!(
            writer,
            "██████╗ ██╗   ██╗███████╗████████╗ ██████╗ ███████╗"
        )
        .unwrap();
        writeln!(
            writer,
            "██╔══██╗██║   ██║██╔════╝╚══██╔══╝██╔═══██╗██╔════╝"
        )
        .unwrap();
        writeln!(
            writer,
            "██████╔╝██║   ██║███████╗   ██║   ██║   ██║███████╗"
        )
        .unwrap();
        writeln!(
            writer,
            "██╔══██╗██║   ██║╚════██║   ██║   ██║   ██║╚════██║"
        )
        .unwrap();
        writeln!(
            writer,
            "██║  ██║╚██████╔╝███████║   ██║   ╚██████╔╝███████║"
        )
        .unwrap();
        writeln!(
            writer,
            "╚═╝  ╚═╝ ╚═════╝ ╚══════╝   ╚═╝    ╚═════╝ ╚══════╝"
        )
        .unwrap();
        writeln!(writer, "\n").unwrap();
        writer.set_color(vga_buffer::ColorCode::new(Color::White, Color::Black));
        writeln!(
            writer,
            "                                             v0.1.0"
        )
        .unwrap();
        writeln!(writer, "\n").unwrap();
        writeln!(writer, "Hello World from RustOS!").unwrap();
        writeln!(writer, "System is operational...").unwrap();
    });

    // Loop forever to keep the kernel alive
    loop {
        x86_64::instructions::hlt(); // CPU sleep until next interrupt
    }
}

/// This function is called on panic.
#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    interrupts::without_interrupts(|| {
        let mut writer = vga_buffer::WRITER.lock();
        writer.set_color(vga_buffer::ColorCode::new(Color::Red, Color::Black));
        writeln!(writer, "KERNEL PANIC: {}", info).unwrap();
    });

    loop {
        x86_64::instructions::hlt();
    }
}
