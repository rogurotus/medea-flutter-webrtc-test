#pragma once

#include "c99.h"
#include <Windows.h>
#include <Psapi.h>
#include "utf8.h"
#include <stdio.h>
#include <iostream>
#include "system_audio_module.h"
#include <vector>


enum window_priority {
	WINDOW_PRIORITY_CLASS,
	WINDOW_PRIORITY_TITLE,
	WINDOW_PRIORITY_EXE,
};

enum window_search_mode {
	INCLUDE_MINIMIZED,
	EXCLUDE_MINIMIZED,
};

// enum {
// 	/**
// 	 * Use if there's a problem that can potentially affect the program,
// 	 * but isn't enough to require termination of the program.
// 	 *
// 	 * Use in creation functions and core subsystem functions.  Places that
// 	 * should definitely not fail.
// 	 */
// 	LOG_ERROR = 100,

// 	/**
// 	 * Use if a problem occurs that doesn't affect the program and is
// 	 * recoverable.
// 	 *
// 	 * Use in places where failure isn't entirely unexpected, and can
// 	 * be handled safely.
// 	 */
// 	LOG_WARNING = 200,

// 	/**
// 	 * Informative message to be displayed in the log.
// 	 */
// 	LOG_INFO = 300,

// 	/**
// 	 * Debug message to be used mostly by developers.
// 	 */
// 	LOG_DEBUG = 400
// };


// typedef void (*log_handler_t)(int lvl, const char *msg, va_list args, void *p);


inline long os_atomic_inc_long(volatile long *val);

struct dstr {
	char *array;
	size_t len; /* number of characters, excluding null terminator */
	size_t capacity;
};
inline void dstr_resize(dstr& dst, const size_t num);
inline void dstr_free(dstr& dst);
void dstr_copy(dstr& dst, const char *array);
inline void dstr_ensure_capacity(dstr& dst, const size_t new_size);
inline int dstr_cmp(const dstr& str1, const char *str2);
inline bool dstr_is_empty(const dstr& str);

void bfree(void *ptr);
void *brealloc(void *ptr, size_t size);


#define LOWER_HALFBYTE(x) ((x)&0xF)
#define UPPER_HALFBYTE(x) (((x) >> 4) & 0xF)

std::vector<AudioSourceInfo> ms_fill_window_list(enum window_search_mode mode); // todo
HWND first_window(enum window_search_mode mode, HWND *parent,
			 bool *use_findwindowex);
bool check_window_valid(HWND window, enum window_search_mode mode);
inline bool IsWindowCloaked(HWND window);
bool ms_is_uwp_window(HWND hwnd);
HWND next_window(HWND window, enum window_search_mode mode, HWND *parent,
			bool use_findwindowex);

// todo
bool ms_get_window_exe(dstr& exe, HWND window);
inline HANDLE open_process(DWORD desired_access, bool inherit_handle,
				  DWORD process_id);
void *ms_get_obfuscated_func(HMODULE module, const char *str, uint64_t val);
void deobfuscate_str(char *str, uint64_t val);
bool is_microsoft_internal_window_exe(const char *exe);
void ms_get_window_title(dstr& name, HWND hwnd);

int astrcmpi(const char *str1, const char *str2);
int astrcmpi_n(const char *str1, const char *str2, size_t n);
void *a_realloc(void *ptr, size_t size);



// bool is_generic_class(const char *current_class);
// void dstr_ncat(dstr& dst, const char *array, const size_t len);
// inline void dstr_cat(dstr& dst, const char *array);
// void dstr_cat_dstr(dstr& dst, const dstr& str);
// inline void encode_dstr(dstr& str);
// void dstr_vprintf(dstr& dst, const char *format, va_list args);
// void dstr_printf(dstr& dst, const char *format, ...);
// void *a_malloc(size_t size);
// void *bmalloc(size_t size);
// void strlist_free(char **strlist);
// void dstr_replace(dstr& str, const char *find, const char *replace);
// inline char *decode_str(const char *src);
// char **strlist_split(const char *str, char split_ch, bool include_empty);
// void ms_build_window_strings(const char *str, char **class_, char **title,
// 			     char **exe);
// void ms_get_window_class(dstr& class_, HWND hwnd);
// OBS_NORETURN void def_crash_handler(const char *format, va_list args,
// 					   void *param);
// OBS_NORETURN void bcrash(const char *format, ...);
// void os_breakpoint(void);
// void def_log_handler(int log_level, const char *format, va_list args,
// 			    void *param);
// void blogva(int log_level, const char *format, va_list args);
// void blog(int log_level, const char *format, ...);
// void dstr_from_wcs(dstr& dst, const wchar_t *wstr);
// HMODULE kernel32(void);



// char *astrstri(const char *str, const char *find);

// bool is_uwp_class(const char *window_class);


// inline long os_atomic_dec_long(volatile long* val);
// void a_free(void *ptr);
// inline int dstr_cmpi(const dstr& str1, const char *str2);
// int window_rating(HWND window, enum window_priority priority,
// 			 const char *class_, const char *title, const char *exe,
// 			 bool uwp_window, bool generic_class);
// HWND ms_find_window(enum window_search_mode mode, enum window_priority priority,
// 		    const char *class_, const char *title, const char *exe);
