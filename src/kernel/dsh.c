#include <kernel/dsh.h>
#include <kernel/vga.h>
#include <kernel/io.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

extern size_t terminal_column;

#define PROMPT "dsh> "
#define PROMPT_LEN (sizeof(PROMPT) - 1)

static void refresh_input_line(const char *buffer, size_t buf_len, size_t cursor_pos) {
    size_t row = terminal_row;
    terminal_column = PROMPT_LEN;
    terminal_update_cursor();
    
    for (size_t i = PROMPT_LEN; i < VGA_WIDTH; i++) {
        vga_buffer[row * VGA_WIDTH + i] = vga_entry(' ', terminal_color);
    }
    

    terminal_column = PROMPT_LEN;
    terminal_update_cursor();
    for (size_t i = 0; i < buf_len; i++) {
        vga_buffer[row * VGA_WIDTH + terminal_column] = vga_entry(buffer[i], terminal_color);
        terminal_column++;
    }
    
    terminal_column = PROMPT_LEN + cursor_pos;
    terminal_update_cursor();
}


static int caps_lock = 0;
static int num_lock = 0;

static void serial_write(const char* str) {
    while (*str) {
        outb(0x3F8, *str++);  // COM1 port
    }
}

static inline void outw(uint16_t port, uint16_t data) {
    __asm__ volatile ("outw %0, %1" : : "a"(data), "Nd"(port));
}

static inline void *dex_memmove(void *dest, const void *src, size_t n) {
    if (dest == src || n == 0)
        return dest;

    unsigned char *d = dest;
    const unsigned char *s = src;
    
    if (d < s) {
        for (size_t i = 0; i < n; i++)
            d[i] = s[i];
    } else {
        for (size_t i = n; i > 0; i--)
            d[i-1] = s[i-1];
    }
    return dest;
}

static inline char *dex_strncpy(char *dest, const char *src, size_t n) {
    if (!dest || !src || n == 0)
        return dest;

    char *start = dest;
    while (n && (*dest++ = *src++))
        n--;
    if (n)
        while (--n)
            *dest++ = '\0';
    return start;
}

static inline size_t dex_strlen(const char *s) {
    if (!s)
        return 0;
    const char *p = s;
    while (*p)
        p++;
    return (size_t)(p - s);
}

static void delay(volatile int count) {
    while(count--) {
        io_wait();
    }
}


#define DSH_BUFFER_SIZE 128
#define MAX_COMMAND_HISTORY 10

#define INITIAL_REPEAT_DELAY 2500000
#define SUBSEQUENT_REPEAT_DELAY 500000

static char command_history[MAX_COMMAND_HISTORY][DSH_BUFFER_SIZE];
static int history_count = 0;
static int history_position = 0;

static int string_equal(const char *s1, const char *s2);
static int string_starts_with(const char *str, const char *prefix);
static void add_to_history(const char *cmd);

static char scancode_to_ascii(uint8_t sc) {
    static int shift_pressed = 0;

    // Handle special keys
    if (sc == 0x2A || sc == 0x36) {
        shift_pressed = 1;
        return 0;
    }
    if (sc == 0xAA || sc == 0xB6) {
        shift_pressed = 0;
        return 0;
    }
    if (sc == 0x3A) { // Caps Lock press
        caps_lock = !caps_lock;
        return 0;
    }
    if (sc == 0x45) { // Num Lock press
        num_lock = !num_lock;
        return 0;
    }

    if (sc & 0x80) {
        return 0;
    }

    // Handle numpad with Num Lock
    if (num_lock) {
        switch (sc) {
            case 0x47: return '7';
            case 0x48: return '8';
            case 0x49: return '9';
            case 0x4B: return '4';
            case 0x4C: return '5';
            case 0x4D: return '6';
            case 0x4F: return '1';
            case 0x50: return '2';
            case 0x51: return '3';
            case 0x52: return '0';
            case 0x53: return '.';
            case 0x37: return '*';
            case 0x4A: return '-';
            case 0x4E: return '+';
        }
    }

    // Handle regular keys with Shift or Caps Lock
    if (shift_pressed || (caps_lock && ((sc >= 0x10 && sc <= 0x19) || (sc >= 0x1E && sc <= 0x26) || (sc >= 0x2C && sc <= 0x32)))) {
        switch (sc) {
            case 0x02: return '!';
            case 0x03: return '@';
            case 0x04: return '#';
            case 0x05: return '$';
            case 0x06: return '%';
            case 0x07: return '^';
            case 0x08: return '&';
            case 0x09: return '*';
            case 0x0A: return '(';
            case 0x0B: return ')';
            case 0x0C: return '_';
            case 0x0D: return '+';
            case 0x10: return 'Q';
            case 0x11: return 'W';
            case 0x12: return 'E';
            case 0x13: return 'R';
            case 0x14: return 'T';
            case 0x15: return 'Y';
            case 0x16: return 'U';
            case 0x17: return 'I';
            case 0x18: return 'O';
            case 0x19: return 'P';
            case 0x1A: return '{';
            case 0x1B: return '}';
            case 0x1E: return 'A';
            case 0x1F: return 'S';
            case 0x20: return 'D';
            case 0x21: return 'F';
            case 0x22: return 'G';
            case 0x23: return 'H';
            case 0x24: return 'J';
            case 0x25: return 'K';
            case 0x26: return 'L';
            case 0x27: return ':';
            case 0x28: return '"';
            case 0x29: return '~';
            case 0x2B: return '|';
            case 0x2C: return 'Z';
            case 0x2D: return 'X';
            case 0x2E: return 'C';
            case 0x2F: return 'V';
            case 0x30: return 'B';
            case 0x31: return 'N';
            case 0x32: return 'M';
            case 0x33: return '<';
            case 0x34: return '>';
            case 0x35: return '?';
            case 0x39: return ' ';
            case 0x0E: return '\b';
            case 0x1C: return '\n';
            default: return 0;
        }
    }

    switch (sc) {
        case 0x02: return '1';
        case 0x03: return '2';
        case 0x04: return '3';
        case 0x05: return '4';
        case 0x06: return '5';
        case 0x07: return '6';
        case 0x08: return '7';
        case 0x09: return '8';
        case 0x0A: return '9';
        case 0x0B: return '0';
        case 0x0C: return '-';
        case 0x0D: return '=';
        case 0x10: return 'q';
        case 0x11: return 'w';
        case 0x12: return 'e';
        case 0x13: return 'r';
        case 0x14: return 't';
        case 0x15: return 'y';
        case 0x16: return 'u';
        case 0x17: return 'i';
        case 0x18: return 'o';
        case 0x19: return 'p';
        case 0x1A: return '[';
        case 0x1B: return ']';
        case 0x1E: return 'a';
        case 0x1F: return 's';
        case 0x20: return 'd';
        case 0x21: return 'f';
        case 0x22: return 'g';
        case 0x23: return 'h';
        case 0x24: return 'j';
        case 0x25: return 'k';
        case 0x26: return 'l';
        case 0x27: return ';';
        case 0x28: return '\'';
        case 0x29: return '`';
        case 0x2B: return '\\';
        case 0x2C: return 'z';
        case 0x2D: return 'x';
        case 0x2E: return 'c';
        case 0x2F: return 'v';
        case 0x30: return 'b';
        case 0x31: return 'n';
        case 0x32: return 'm';
        case 0x33: return ',';
        case 0x34: return '.';
        case 0x35: return '/';
        case 0x39: return ' ';
        case 0x0E: return '\b';
        case 0x1C: return '\n';
        default:   return 0;
    }
}

static void add_to_history(const char *cmd) {
    if (!cmd || cmd[0] == '\0') return;
    
    if (history_count > 0 && string_equal(command_history[history_count - 1], cmd)) {
        history_position = history_count;
        return;
    }
    
    if (history_count == MAX_COMMAND_HISTORY) {
        dex_memmove(command_history[0], command_history[1], 
                (MAX_COMMAND_HISTORY - 1) * DSH_BUFFER_SIZE);
        history_count--;
    }
    
    dex_strncpy(command_history[history_count], cmd, DSH_BUFFER_SIZE - 1);
    command_history[history_count][DSH_BUFFER_SIZE - 1] = '\0';
    history_count++;
    history_position = history_count;
}

static int string_equal(const char *s1, const char *s2) {
    if (!s1 || !s2) return 0;
    while (*s1 && *s2) {
        if (*s1 != *s2)
            return 0;
        s1++; s2++;
    }
    return (*s1 == '\0' && *s2 == '\0');
}

static int string_starts_with(const char *str, const char *prefix) {
    if (!str || !prefix) return 0;
    while (*prefix) {
        if (*prefix++ != *str++)
            return 0;
    }
    return 1;
}

static void read_line(char *buffer, size_t buffer_size) {
    if (!buffer || buffer_size == 0) return;
    
    size_t index = 0;
    size_t cursor_pos = 0;
    uint8_t last_processed_sc = 0;
    int delay_counter = 0;
    int current_repeat_delay = INITIAL_REPEAT_DELAY;
    int enter_pressed = 0;

    buffer[0] = '\0';

    while (1) {
        uint8_t sc = inb(0x60);

        if (sc & 0x80) {
            uint8_t key_released = sc & 0x7F;
            if (key_released == 0x2A || key_released == 0x36) {
                scancode_to_ascii(sc);
            }
            if (key_released == last_processed_sc) {
                last_processed_sc = 0;
                continue;
            }
            if (sc == 0x9C && enter_pressed) {  // Enter key released (0x1C | 0x80)
                buffer[index] = '\0';
                terminal_putchar('\n');
                add_to_history(buffer);
                break;
            }
            if ((sc & 0x7F) == last_processed_sc)
                last_processed_sc = 0;
            continue;
        }

        if (sc == last_processed_sc) {
            delay_counter++;
            if (delay_counter < current_repeat_delay)
                continue;
            delay_counter = 0;
            current_repeat_delay = SUBSEQUENT_REPEAT_DELAY;
        } else {
            last_processed_sc = sc;
            delay_counter = 0;
            current_repeat_delay = INITIAL_REPEAT_DELAY;
        }

        if (sc == 0x4B) {  // Arrow Left
            if (cursor_pos > 0) {
                cursor_pos--;
                refresh_input_line(buffer, index, cursor_pos);
            }
            continue;
        }
        if (sc == 0x4D) {  // Arrow Right
            if (cursor_pos < index) {
                cursor_pos++;
                refresh_input_line(buffer, index, cursor_pos);
            }
            continue;
        }
        // Home (0x47) – moving the cursor to the beginning of the line (cursor_pos = 0)
        if (sc == 0x47) {
            cursor_pos = 0;
            refresh_input_line(buffer, index, cursor_pos);
            continue;
        }
        // End (0x4F) – moving the cursor to the end of the line (cursor_pos = index)
        if (sc == 0x4F) {
            cursor_pos = index;
            refresh_input_line(buffer, index, cursor_pos);
            continue;
        }
        if (sc == 0x48) {  // Arrow Up
            if (history_count > 0) {
                while (index > 0) {
                    terminal_write("\b \b");
                    index--;
                }
                if (history_position > 0)
                    history_position--;
                dex_strncpy(buffer, command_history[history_position], buffer_size - 1);
                buffer[buffer_size - 1] = '\0';
                terminal_write(buffer);
                index = dex_strlen(buffer);
                cursor_pos = index;
                refresh_input_line(buffer, index, cursor_pos);
            }
            continue;
        }
        if (sc == 0x50) {  // Arrow Down
            if (history_count > 0) {
                while (index > 0) {
                    terminal_write("\b \b");
                    index--;
                }
                if (history_position < history_count - 1) {
                    history_position++;
                    dex_strncpy(buffer, command_history[history_position], buffer_size - 1);
                    buffer[buffer_size - 1] = '\0';
                    terminal_write(buffer);
                    index = dex_strlen(buffer);
                    cursor_pos = index;
                } else {
                    history_position = history_count;
                    buffer[0] = '\0';
                    index = 0;
                    cursor_pos = 0;
                }
                refresh_input_line(buffer, index, cursor_pos);
            }
            continue;
        }
        if (sc == 0x1C) {
            enter_pressed = 1;
            continue;
        }
        
        char c = scancode_to_ascii(sc);
        if (c == 0)
            continue;
        
        if (c == '\b') {  // Backspace
            if (cursor_pos > 0) {
                // Move the cursor one position to the left
                for (size_t j = cursor_pos - 1; j < index; j++) {
                    buffer[j] = buffer[j + 1];
                }
                index--; 
                cursor_pos--;
                refresh_input_line(buffer, index, cursor_pos);
            }
        } else {
            if (index < buffer_size - 1) {
                // If the buffer is not full, insert the character at the cursor position
                if (cursor_pos < index) {
                    // Move the characters to the right to make space for the new character
                    for (size_t j = index; j > cursor_pos; j--) {
                        buffer[j] = buffer[j - 1];
                    }
                    buffer[cursor_pos] = c;
                    index++;
                    cursor_pos++;
                } else {
                    // If the buffer is full, replace the last character with the new character
                    buffer[index++] = c;
                    cursor_pos++;
                }
                buffer[index] = '\0';
                refresh_input_line(buffer, index, cursor_pos);
            }
        }
        delay(10000);
    }
}

static void execute_command(const char *cmd) {
    if (!cmd || cmd[0] == '\0')
        return;
    
    if (string_equal(cmd, "shutdown")) {
        terminal_write("\nShutting down...\n");
        serial_write("\nShutting down...\n");
        outw(0x604, 0x2000);
        while (1) {
            __asm__ volatile("cli; hlt");
        }
    }
    if (string_equal(cmd, "reboot")) {
        terminal_write("\nRebooting...\n");
        serial_write("\nRebooting...\n");
        uint8_t good = 0x02;
        while (good & 0x02)
            good = inb(0x64);
        outb(0x64, 0xFE);
        while (1) {
            __asm__ volatile("cli; hlt");
        }
    }
    if (string_equal(cmd, "cleanup")) {
        terminal_initialize();
        return;
    }
    if (string_equal(cmd, "sysabout")) {
        terminal_write("\ndsh (DexisShell) v0.1.0\n");
        terminal_write("Author: ShLKV (The Shlyukov)\n");
        terminal_write("License: MIT\n");
        terminal_write("https://github.com/TheShlyukov/DexisCore");
        terminal_write("\nType 'help' for available commands list\n\n");
        return;
    }
    if (string_equal(cmd, "echo") || string_starts_with(cmd, "echo ")) {
        terminal_write("\n");
        if (dex_strlen(cmd) > 5)
            terminal_write(cmd + 5);
        terminal_write("\n\n");
        return;
    }
    if (string_equal(cmd, "help")) {
        terminal_write("\nAvailable commands:\n");
        terminal_write("shutdown - shutdown system\n");
        terminal_write("reboot - reboot system\n");
        terminal_write("cleanup - clear terminal\n");
        terminal_write("sysabout - about system\n");
        terminal_write("echo - echo string\n");
        terminal_write("help - available commands list\n\n");
        return;
    }
    terminal_write("\nUnknown command!\nType 'help' for available commands list\n\n");
}

void dsh_run(void) {
    char buffer[DSH_BUFFER_SIZE];
    terminal_write("\nRunning dsh (DexShell) v0.0.1\n");
    while (1) {
        terminal_write(PROMPT);
        read_line(buffer, DSH_BUFFER_SIZE);
        execute_command(buffer);
    }
}
