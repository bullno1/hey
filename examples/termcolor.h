#ifndef HEY_TERM_COLOR
#define HEY_TERM_COLOR

#ifdef _WIN32
#	define WIN32_LEAN_AND_MEAN
#	include <windows.h>
#	include <io.h>
#else
#	include <unistd.h>
#endif

#include <stdbool.h>
#include <stdio.h>
#include <varargs.h>

#define ANSI_CODE_RESET      "\033[00m"
#define ANSI_CODE_BOLD       "\033[1m"
#define ANSI_CODE_DARK       "\033[2m"
#define ANSI_CODE_UNDERLINE  "\033[4m"
#define ANSI_CODE_BLINK      "\033[5m"
#define ANSI_CODE_REVERSE    "\033[7m"
#define ANSI_CODE_CONCEALED  "\033[8m"
#define ANSI_CODE_GRAY       "\033[30m"
#define ANSI_CODE_GREY       "\033[30m"
#define ANSI_CODE_RED        "\033[31m"
#define ANSI_CODE_GREEN      "\033[32m"
#define ANSI_CODE_YELLOW     "\033[33m"
#define ANSI_CODE_BLUE       "\033[34m"
#define ANSI_CODE_MAGENTA    "\033[35m"
#define ANSI_CODE_CYAN       "\033[36m"
#define ANSI_CODE_WHITE      "\033[37m"
#define ANSI_CODE_BG_GRAY    "\033[40m"
#define ANSI_CODE_BG_GREY    "\033[40m"
#define ANSI_CODE_BG_RED     "\033[41m"
#define ANSI_CODE_BG_GREEN   "\033[42m"
#define ANSI_CODE_BG_YELLOW  "\033[43m"
#define ANSI_CODE_BG_BLUE    "\033[44m"
#define ANSI_CODE_BG_MAGENTA "\033[45m"
#define ANSI_CODE_BG_CYAN    "\033[46m"
#define ANSI_CODE_BG_WHITE   "\033[47m"

static inline bool
hey_term_is_tty(FILE* std_handle) {
#ifdef _WIN32
	return _isatty(_fileno(std_handle));
#else
	return isatty(fileno(std_handle));
#endif
}

static inline void
hey_term_enable_color(FILE* std_handle) {
	if (!hey_term_is_tty(std_handle)) { return;	}

#ifdef _WIN32
	HANDLE handle;
	if (std_handle == stdout) {
		handle = GetStdHandle(STD_OUTPUT_HANDLE);
	} else if (std_handle == stderr) {
		handle = GetStdHandle(STD_ERROR_HANDLE);
	} else {
		return;
	}

	DWORD mode;
	if (!GetConsoleMode(handle, &mode)) {
		return;
	}

	mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	SetConsoleMode(handle, mode);
#endif
}

static inline void
hey_term_put(FILE* std_handle, const char* format, ...) {
	if (!hey_term_is_tty(std_handle)) { return; }

	va_list args;
	va_start(args, format);
	vfprintf(std_handle, format, args);
	va_end(args);
}

#endif
