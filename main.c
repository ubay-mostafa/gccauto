#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>
#include <urlmon.h>

#define MSYS2_URL "https://github.com/msys2/msys2-installer/releases/latest/download/msys2-x86_64-latest.exe"
#define MSYS2_ROOT "C:\\msys64"
#define GCC_BIN_PATH "C:\\msys64\\ucrt64\\bin"
#define CONFIG_FILE "flags.cfg"
#define LOG_FILE "gccauto_log.txt"

/* ---------- download ---------- */

int download_with_retry(const char* url, const char* outfile, int max_attempts) {
    for (int attempt = 1; attempt <= max_attempts; attempt++) {
        printf("Download attempt %d...\n", attempt);
        HRESULT result = URLDownloadToFile(NULL, url, outfile, 0, NULL);
        if (result == S_OK) {
            printf("Download succeeded.\n");
            return 1;
        }
        printf("  failed (code: %lx)\n", result);
    }
    printf("All download attempts failed.\n");
    return 0;
}

/* ---------- setup ---------- */

int msys2_already_installed(void) {
    char check_path[MAX_PATH];
    snprintf(check_path, sizeof(check_path), "%s\\usr\\bin\\pacman.exe", MSYS2_ROOT);
    FILE* f = fopen(check_path, "r");
    if (f) { fclose(f); return 1; }
    return 0;
}

int gcc_already_installed(void) {
    char check_path[MAX_PATH];
    snprintf(check_path, sizeof(check_path), "%s\\gcc.exe", GCC_BIN_PATH);
    FILE* f = fopen(check_path, "r");
    if (f) { fclose(f); return 1; }
    return 0;
}

/* ---------- run a command quietly, with a dot spinner instead of raw output ---------- */

int run_command_with_spinner(const char* command) {
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    SECURITY_ATTRIBUTES sa;
    ZeroMemory(&sa, sizeof(sa));
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    HANDLE hLog = CreateFileA(LOG_FILE, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
                               &sa, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdOutput = hLog;
    si.hStdError = hLog;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

    char full_cmd[1200];
    snprintf(full_cmd, sizeof(full_cmd), "cmd.exe /c %s", command);

    BOOL ok = CreateProcessA(NULL, full_cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
    if (hLog != INVALID_HANDLE_VALUE) CloseHandle(hLog);

    if (!ok) {
        printf("\nCould not start command (error %lu)\n", GetLastError());
        return -1;
    }

    while (WaitForSingleObject(pi.hProcess, 500) == WAIT_TIMEOUT) {
        printf(".");
        fflush(stdout);
    }

    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    printf("\n");
    return (int)exitCode;
}

int run_setup(void) {
    if (msys2_already_installed()) {
        printf("MSYS2 already found at %s, skipping installer.\n", MSYS2_ROOT);
    } else {
        if (!download_with_retry(MSYS2_URL, "msys2-installer.exe", 3)) {
            return 0;
        }

        char cmd[512];
        snprintf(cmd, sizeof(cmd),
            "msys2-installer.exe install --confirm-command --root \"%s\"", MSYS2_ROOT);
        printf("Installing MSYS2");
        int result = run_command_with_spinner(cmd);
        if (result != 0) {
            printf("MSYS2 install failed (exit code %d). See %s for details.\n", result, LOG_FILE);
            return 0;
        }
        printf("MSYS2 installed.\n");
    }

    if (gcc_already_installed()) {
        printf("gcc toolchain already installed, skipping package install.\n");
    } else {
        printf("Installing gcc toolchain (this takes a few minutes)");
        char pacman_cmd[600];
        snprintf(pacman_cmd, sizeof(pacman_cmd),
            "\"\"%s\\usr\\bin\\bash.exe\" -lc \"pacman -S --noconfirm --needed base-devel mingw-w64-ucrt-x86_64-toolchain\"\"",
            MSYS2_ROOT);
        int pacman_result = run_command_with_spinner(pacman_cmd);
        if (pacman_result != 0) {
            printf("pacman step returned exit code %d. See %s for details.\n", pacman_result, LOG_FILE);
        } else {
            printf("gcc toolchain installed.\n");
        }
    }

    printf("Adding gcc to PATH...\n");
    char path_cmd[1024];
    snprintf(path_cmd, sizeof(path_cmd),
        "powershell -NoProfile -Command \""
        "$p = [Environment]::GetEnvironmentVariable('Path','User'); "
        "if ($p -notlike '*%s*') { [Environment]::SetEnvironmentVariable('Path', $p + ';%s', 'User') }\"",
        GCC_BIN_PATH, GCC_BIN_PATH);
    run_command_with_spinner(path_cmd);

    printf("Setup complete.\n");
    printf("'gccauto build' will work right away in this window.\n");
    printf("Typing 'gcc' directly still needs a new terminal window (Windows limitation).\n");
    return 1;
}

/* ---------- config (saved flags) ---------- */

void load_flags(char* buffer, size_t size) {
    buffer[0] = '\0';
    FILE* f = fopen(CONFIG_FILE, "r");
    if (!f) return;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0] == '\0') continue;
        strncat(buffer, line, size - strlen(buffer) - 2);
        strncat(buffer, " ", size - strlen(buffer) - 1);
    }
    fclose(f);
}

void config_add(const char* flag) {
    FILE* f = fopen(CONFIG_FILE, "a");
    if (!f) { printf("Could not open %s\n", CONFIG_FILE); return; }
    fprintf(f, "%s\n", flag);
    fclose(f);
    printf("Saved flag: %s\n", flag);
}

void config_list(void) {
    FILE* f = fopen(CONFIG_FILE, "r");
    if (!f) { printf("No flags saved yet.\n"); return; }
    char line[256];
    int i = 1;
    while (fgets(line, sizeof(line), f)) {
        printf("%d: %s", i++, line);
    }
    fclose(f);
}

void config_clear(void) {
    FILE* f = fopen(CONFIG_FILE, "w");
    if (f) fclose(f);
    printf("Cleared all saved flags.\n");
}

/* ---------- build ---------- */

void ensure_gcc_in_path(void) {
    char current[4096];
    DWORD len = GetEnvironmentVariableA("PATH", current, sizeof(current));
    if (len == 0 || len >= sizeof(current)) return;

    if (strstr(current, GCC_BIN_PATH) == NULL) {
        char updated[4096 + 64];
        snprintf(updated, sizeof(updated), "%s;%s", current, GCC_BIN_PATH);
        SetEnvironmentVariableA("PATH", updated);
    }
}

void build_file(const char* srcfile) {
    ensure_gcc_in_path();

    char flags[512];
    load_flags(flags, sizeof(flags));

    char outname[256];
    strncpy(outname, srcfile, sizeof(outname) - 1);
    outname[sizeof(outname) - 1] = '\0';
    char* dot = strrchr(outname, '.');
    if (dot) *dot = '\0';
    strncat(outname, ".exe", sizeof(outname) - strlen(outname) - 1);

    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "gcc %s \"%s\" -o \"%s\"", flags, srcfile, outname);

    printf("Running: %s\n", cmd);
    int result = system(cmd);
    if (result == 0) {
        printf("Build succeeded: %s\n", outname);
    } else {
        printf("Build failed (exit code %d)\n", result);
    }
}

int prompt_yes_no(const char* question) {
    char line[16];
    while (1) {
        printf("%s (y/n): ", question);
        fgets(line, sizeof(line), stdin);
        line[strcspn(line, "\r\n")] = '\0';

        if (strlen(line) == 1 && (line[0] == 'y' || line[0] == 'Y')) return 1;
        if (strlen(line) == 1 && (line[0] == 'n' || line[0] == 'N')) return 0;

        printf("Invalid entry '%s'. Please type y or n.\n", line);
    }
}

/* ---------- wizard (double-click friendly) ---------- */

typedef struct { const char* flag; const char* desc; } FlagOption;

static const FlagOption common_flags[] = {
    {"-Wall",    "Enable common warnings"},
    {"-Wextra",  "Enable extra warnings"},
    {"-g",       "Include debug symbols"},
    {"-O2",      "Optimize for speed"},
    {"-std=c11", "Use the C11 standard"},
    {"-lm",      "Link the math library"}
};
#define NUM_COMMON_FLAGS (sizeof(common_flags) / sizeof(common_flags[0]))

void run_wizard(void) {
    char line[256];

    printf("=== GCC Auto Tool - Setup Wizard ===\n\n");
    printf("(Run 'gccauto help' anytime to see all available commands.)\n\n");

    printf("Step 1: Toolchain\n");
    if (prompt_yes_no("Install MSYS2 + gcc now?")) {
        run_setup();
    }

    printf("\nStep 2: Default compiler flags\n");
    printf("These will be applied automatically every time you run 'gccauto build'.\n\n");
    for (size_t i = 0; i < NUM_COMMON_FLAGS; i++) {
        printf("  %zu) %-10s - %s\n", i + 1, common_flags[i].flag, common_flags[i].desc);
    }
    int selections[NUM_COMMON_FLAGS];
    int selection_count;
    int valid;

    do {
        valid = 1;
        selection_count = 0;

        printf("\nEnter the numbers you want, separated by spaces (e.g. 1 2 4),\n");
        printf("or leave blank for none:\n> ");
        fgets(line, sizeof(line), stdin);
        line[strcspn(line, "\r\n")] = '\0';

        char temp[256];
        strncpy(temp, line, sizeof(temp) - 1);
        temp[sizeof(temp) - 1] = '\0';

        char* token = strtok(temp, " \t");
        while (token != NULL) {
            int len = (int)strlen(token);
            int is_number = (len > 0);
            for (int i = 0; i < len; i++) {
                if (!isdigit((unsigned char)token[i])) { is_number = 0; break; }
            }
            int choice = is_number ? atoi(token) : -1;

            if (!is_number || choice < 1 || choice > (int)NUM_COMMON_FLAGS) {
                printf("Invalid entry '%s'. Use only the numbers 1-%zu, separated by single spaces.\n",
                       token, NUM_COMMON_FLAGS);
                valid = 0;
                break;
            }

            selections[selection_count++] = choice;
            token = strtok(NULL, " \t");
        }
    } while (!valid);

    config_clear();
    for (int i = 0; i < selection_count; i++) {
        config_add(common_flags[selections[i] - 1].flag);
    }

    if (prompt_yes_no("\nAdd a custom flag too?")) {
        printf("Enter the flag exactly as gcc expects it (e.g. -Isome/include): ");
        char custom[128];
        fgets(custom, sizeof(custom), stdin);
        custom[strcspn(custom, "\r\n")] = '\0';
        if (custom[0] != '\0') config_add(custom);
    }

    printf("\n=== Setup complete ===\n");
    printf("Saved flags:\n");
    config_list();

    printf("\nTo change these later, open a terminal in this folder and run:\n");
    printf("  gccauto config list          see current flags\n");
    printf("  gccauto config add <flag>    add one more flag\n");
    printf("  gccauto config clear         wipe all flags and start over\n");
    printf("\nTo compile a file:\n  gccauto build yourfile.c\n");
    printf("\nRun 'gccauto help' anytime to see this full command list again.\n");

    printf("\nPress Enter to exit...");
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
}

/* ---------- main / CLI ---------- */

void print_usage(void) {
    printf("Usage:\n");
    printf("  gccauto help                  Show this command list\n");
    printf("  gccauto setup                 Install MSYS2 + gcc toolchain, update PATH\n");
    printf("  gccauto build <file.c>        Compile file.c using saved flags\n");
    printf("  gccauto config add <flag>     Save a flag, e.g. -Wall\n");
    printf("  gccauto config list           List saved flags\n");
    printf("  gccauto config clear          Remove all saved flags\n");
    printf("  gccauto                       (no args) Run the setup wizard\n");
}

int main(int argc, char** argv) {
    if (argc < 2) { run_wizard(); return 0; }

    if (strcmp(argv[1], "help") == 0) {
        print_usage();
    } else if (strcmp(argv[1], "setup") == 0) {
        return run_setup() ? 0 : 1;
    } else if (strcmp(argv[1], "build") == 0) {
        if (argc < 3) { printf("Usage: gccauto build <file.c>\n"); return 1; }
        build_file(argv[2]);
    } else if (strcmp(argv[1], "config") == 0) {
        if (argc < 3) { print_usage(); return 1; }
        if (strcmp(argv[2], "add") == 0 && argc >= 4) {
            config_add(argv[3]);
        } else if (strcmp(argv[2], "list") == 0) {
            config_list();
        } else if (strcmp(argv[2], "clear") == 0) {
            config_clear();
        } else {
            print_usage();
        }
    } else {
        print_usage();
    }
    return 0;
}