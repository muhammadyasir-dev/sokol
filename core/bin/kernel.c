void main() {
    const char *str = "Hello, World!";
    char *video_memory = (char *)0xb8000; // Video memory starts here
    for (int i = 0; str[i] != '\0'; i++) {
        video_memory[i * 2] = str[i];      // Character
        video_memory[i * 2 + 1] = 0x07;    // Attribute byte (light gray on black)
    }
    while (1); // Loop forever
}
