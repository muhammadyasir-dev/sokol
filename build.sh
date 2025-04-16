#!/bin/bash
# build.sh - Build and run RustOS

echo "🦀 Building RustOS kernel..."

# Check if cargo and qemu are installed
if ! command -v cargo &> /dev/null; then
    echo "Error: cargo not found. Please install Rust toolchain."
    exit 1
fi

if ! command -v qemu-system-x86_64 &> /dev/null; then
    echo "Error: qemu-system-x86_64 not found. Please install QEMU."
    exit 1
fi

# Install bootimage if not installed
if ! cargo install bootimage --version "^0.10.3" 2>/dev/null; then
    echo "Installing bootimage tool..."
    cargo install bootimage --version "^0.10.3"
fi

# Install rust-src component if not installed
if ! rustup component list --installed | grep -q "rust-src"; then
    echo "Installing rust-src component..."
    rustup component add rust-src
fi

# Build the kernel
echo "Compiling kernel..."
cargo bootimage

# Check if build was successful
if [ $? -ne 0 ]; then
    echo "❌ Build failed"
    exit 1
fi

echo "✅ Build successful!"
echo "🚀 Starting RustOS in QEMU..."

# Run the kernel in QEMU
qemu-system-x86_64 \
    -drive format=raw,file=target/x86_64-rust_os/debug/bootimage-rust-os.bin \
    -m 128M \
    -serial stdio \
    -display gtk,gl=on \
    -device isa-debug-exit,iobase=0xf4,iosize=0x04 \
    -accel kvm

echo "QEMU session ended"
