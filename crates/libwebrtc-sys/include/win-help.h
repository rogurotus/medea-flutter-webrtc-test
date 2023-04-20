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

enum {
	/**
	 * Use if there's a problem that can potentially affect the program,
	 * but isn't enough to require termination of the program.
	 *
	 * Use in creation functions and core subsystem functions.  Places that
	 * should definitely not fail.
	 */
	LOG_ERROR = 100,

	/**
	 * Use if a problem occurs that doesn't affect the program and is
	 * recoverable.
	 *
	 * Use in places where failure isn't entirely unexpected, and can
	 * be handled safely.
	 */
	LOG_WARNING = 200,

	/**
	 * Informative message to be displayed in the log.
	 */
	LOG_INFO = 300,

	/**
	 * Debug message to be used mostly by developers.
	 */
	LOG_DEBUG = 400
};

#define LOWER_HALFBYTE(x) ((x)&0xF)
#define UPPER_HALFBYTE(x) (((x) >> 4) & 0xF)

typedef void (*log_handler_t)(int lvl, const char *msg, va_list args, void *p);

struct dstr {
	char *array;
	size_t len; /* number of characters, excluding null terminator */
	size_t capacity;
};

void add_window(HWND hwnd); // todo
std::vector<AudioSourceInfo> ms_fill_window_list(enum window_search_mode mode); // todo



bool is_generic_class(const char *current_class);
void dstr_ncat(struct dstr *dst, const char *array, const size_t len);
inline void dstr_cat(struct dstr *dst, const char *array);
void dstr_cat_dstr(struct dstr *dst, const struct dstr *str);
inline void encode_dstr(struct dstr *str);
void dstr_vprintf(struct dstr *dst, const char *format, va_list args);
void dstr_printf(struct dstr *dst, const char *format, ...);
inline int dstr_cmp(const struct dstr *str1, const char *str2);
bool is_microsoft_internal_window_exe(const char *exe);
inline bool dstr_is_empty(const struct dstr *str);
void *a_malloc(size_t size);
void *bmalloc(size_t size);
void strlist_free(char **strlist);
void dstr_replace(struct dstr *str, const char *find, const char *replace);
inline char *decode_str(const char *src);
char **strlist_split(const char *str, char split_ch, bool include_empty);
void ms_build_window_strings(const char *str, char **class_, char **title,
			     char **exe);
void ms_get_window_class(struct dstr *class_, HWND hwnd);
void ms_get_window_title(struct dstr *name, HWND hwnd);
EXPORT void dstr_copy(struct dstr *dst, const char *array);
OBS_NORETURN void def_crash_handler(const char *format, va_list args,
					   void *param);
OBS_NORETURN void bcrash(const char *format, ...);
void *a_realloc(void *ptr, size_t size);
void os_breakpoint(void);
void def_log_handler(int log_level, const char *format, va_list args,
			    void *param);
void blogva(int log_level, const char *format, va_list args);
void blog(int log_level, const char *format, ...);
inline long os_atomic_inc_long(volatile long *val);
void *brealloc(void *ptr, size_t size);
inline void dstr_ensure_capacity(struct dstr *dst, const size_t new_size);
inline void dstr_resize(struct dstr *dst, const size_t num);
void dstr_from_wcs(struct dstr *dst, const wchar_t *wstr);
void deobfuscate_str(char *str, uint64_t val);
void *ms_get_obfuscated_func(HMODULE module, const char *str, uint64_t val);
HMODULE kernel32(void);
inline HANDLE open_process(DWORD desired_access, bool inherit_handle,
				  DWORD process_id);
bool ms_get_window_exe(struct dstr *name, HWND window);
HWND ms_get_uwp_actual_window(HWND parent);
bool ms_is_uwp_window(HWND hwnd);
inline bool IsWindowCloaked(HWND window);
bool check_window_valid(HWND window, enum window_search_mode mode);
HWND next_window(HWND window, enum window_search_mode mode, HWND *parent,
			bool use_findwindowex);
int astrcmpi(const char *str1, const char *str2);
HWND first_window(enum window_search_mode mode, HWND *parent,
			 bool *use_findwindowex);
bool is_uwp_class(const char *window_class);
int astrcmpi_n(const char *str1, const char *str2, size_t n);
char *astrstri(const char *str, const char *find);


inline long os_atomic_dec_long(volatile long* val);
void a_free(void *ptr);
void bfree(void *ptr);
inline void dstr_free(struct dstr *dst);
inline int dstr_cmpi(const struct dstr *str1, const char *str2);
int window_rating(HWND window, enum window_priority priority,
			 const char *class_, const char *title, const char *exe,
			 bool uwp_window, bool generic_class);
HWND ms_find_window(enum window_search_mode mode, enum window_priority priority,
		    const char *class_, const char *title, const char *exe);
