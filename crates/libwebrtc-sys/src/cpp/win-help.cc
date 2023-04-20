
#include "win-help.h"
#include "dwmapi.h"

log_handler_t log_handler = def_log_handler;
void (*crash_handler)(const char *, va_list, void *) = def_crash_handler;
const char *astrblank = "";
const char *generic_class_substrings[] = {
	"Chrome",
	NULL,
};
long num_allocs = 0;
int crashing = 0;
void *log_param = NULL;
void *crash_param = NULL;
/* not capturable or internal windows, exact executable names */
const char *internal_microsoft_exes_exact[] = {
	"startmenuexperiencehost.exe",
	"applicationframehost.exe",
	"peopleexperiencehost.exe",
	"shellexperiencehost.exe",
	"microsoft.notes.exe",
	"systemsettings.exe",
	"textinputhost.exe",
	"searchapp.exe",
	"video.ui.exe",
	"searchui.exe",
	"lockapp.exe",
	"cortana.exe",
	"gamebar.exe",
	"tabtip.exe",
	"time.exe",
	NULL,
};
/* partial matches start from the beginning of the executable name */
const char *internal_microsoft_exes_partial[] = {
	"windowsinternal",
	NULL,
};




void add_window(HWND hwnd)
{
	struct dstr class_ = {0};
	struct dstr title = {0};
	struct dstr exe = {0};
	struct dstr encoded = {0};
	struct dstr desc = {0};

	if (!ms_get_window_exe(&exe, hwnd))
		return;
	if (is_microsoft_internal_window_exe(exe.array)) {
		dstr_free(&exe);
		return;
	}

	ms_get_window_title(&title, hwnd);
	if (dstr_cmp(&exe, "explorer.exe") == 0 && dstr_is_empty(&title)) {
		dstr_free(&exe);
		dstr_free(&title);
		return;
	}

	ms_get_window_class(&class_, hwnd);

	// if (callback && !callback(title.array, class_.array, exe.array)) {
	// 	dstr_free(&title);
	// 	dstr_free(&class_);
	// 	dstr_free(&exe);
	// 	return;
	// }

	dstr_printf(&desc, "[%s]: %s", exe.array, title.array);

	encode_dstr(&title);
	encode_dstr(&class_);
	encode_dstr(&exe);

	dstr_cat_dstr(&encoded, &title);
	dstr_cat(&encoded, ":");
	dstr_cat_dstr(&encoded, &class_);
	dstr_cat(&encoded, ":");
	dstr_cat_dstr(&encoded, &exe);


	// obs_property_list_add_string(p, desc.array, encoded.array);

	dstr_free(&encoded);
	dstr_free(&desc);
	dstr_free(&class_);
	dstr_free(&title);
	dstr_free(&exe);
}


	// struct dstr class = {0};
	// struct dstr title = {0};
	// struct dstr exe = {0};
	// struct dstr encoded = {0};
	// struct dstr desc = {0};

	// if (!ms_get_window_exe(&exe, hwnd))
	// 	return;
	// if (is_microsoft_internal_window_exe(exe.array)) {
	// 	dstr_free(&exe);
	// 	return;
	// }

	// ms_get_window_title(&title, hwnd);
	// if (dstr_cmp(&exe, "explorer.exe") == 0 && dstr_is_empty(&title)) {
	// 	dstr_free(&exe);
	// 	dstr_free(&title);
	// 	return;
	// }

	// ms_get_window_class(&class, hwnd);

	// if (callback && !callback(title.array, class.array, exe.array)) {
	// 	dstr_free(&title);
	// 	dstr_free(&class);
	// 	dstr_free(&exe);
	// 	return;
	// }

	// dstr_printf(&desc, "[%s]: %s", exe.array, title.array);

	// encode_dstr(&title);
	// encode_dstr(&class);
	// encode_dstr(&exe);

	// dstr_cat_dstr(&encoded, &title);
	// dstr_cat(&encoded, ":");
	// dstr_cat_dstr(&encoded, &class);
	// dstr_cat(&encoded, ":");
	// dstr_cat_dstr(&encoded, &exe);

	// obs_property_list_add_string(p, desc.array, encoded.array);

	// dstr_free(&encoded);
	// dstr_free(&desc);
	// dstr_free(&class);
	// dstr_free(&title);
	// dstr_free(&exe);


std::vector<AudioSourceInfo> ms_fill_window_list(enum window_search_mode mode)
{
	std::vector<AudioSourceInfo> result;
	HWND parent;
	bool use_findwindowex = false;

	HWND window = first_window(mode, &parent, &use_findwindowex);


	while (window) {
		struct dstr title = {0};
		struct dstr exe = {0};
		if (!ms_get_window_exe(&exe, window))
		{
			window = next_window(window, mode, &parent, use_findwindowex);
			continue;
		}

		if (is_microsoft_internal_window_exe(exe.array)) {
			dstr_free(&exe);
			window = next_window(window, mode, &parent, use_findwindowex);
			continue;
		}

		ms_get_window_title(&title, window);
		if (dstr_cmp(&exe, "explorer.exe") == 0 && dstr_is_empty(&title)) {
			dstr_free(&exe);
			dstr_free(&title);
			window = next_window(window, mode, &parent, use_findwindowex);
			continue;
		}

		DWORD dwProcessId = 0;
		if (GetWindowThreadProcessId(window, &dwProcessId)) {
			ms_get_window_title(&title, window);
			AudioSourceInfo info(dwProcessId , std::string(title.array));
			result.push_back(info);
		}

		window = next_window(window, mode, &parent, use_findwindowex);
		dstr_free(&exe);
		dstr_free(&title);
	}

	return result;
}


inline bool IsWindowCloaked(HWND window)
{
	DWORD cloaked;
	HRESULT hr = DwmGetWindowAttribute(window, DWMWA_CLOAKED, &cloaked,
					   sizeof(cloaked));
	return SUCCEEDED(hr) && cloaked;
}


bool check_window_valid(HWND window, enum window_search_mode mode)
{
	DWORD styles, ex_styles;
	RECT rect;

	if (!IsWindowVisible(window) ||
	    (mode == EXCLUDE_MINIMIZED &&
	     (IsIconic(window) || IsWindowCloaked(window))))
		return false;

	GetClientRect(window, &rect);
	styles = (DWORD)GetWindowLongPtr(window, GWL_STYLE);
	ex_styles = (DWORD)GetWindowLongPtr(window, GWL_EXSTYLE);

	if (ex_styles & WS_EX_TOOLWINDOW)
		return false;
	if (styles & WS_CHILD)
		return false;
	if (mode == EXCLUDE_MINIMIZED && (rect.bottom == 0 || rect.right == 0))
		return false;

	return true;
}


bool ms_is_uwp_window(HWND hwnd)
{
	wchar_t name[256];

	name[0] = 0;
	if (!GetClassNameW(hwnd, name, sizeof(name) / sizeof(wchar_t)))
		return false;

	return wcscmp(name, L"ApplicationFrameWindow") == 0;
}

HWND ms_get_uwp_actual_window(HWND parent)
{
	DWORD parent_id = 0;
	HWND child;

	GetWindowThreadProcessId(parent, &parent_id);
	child = FindWindowEx(parent, NULL, NULL, NULL);

	while (child) {
		DWORD child_id = 0;
		GetWindowThreadProcessId(child, &child_id);

		if (child_id != parent_id)
			return child;

		child = FindWindowEx(parent, child, NULL, NULL);
	}

	return NULL;
}

HWND next_window(HWND window, enum window_search_mode mode, HWND *parent,
			bool use_findwindowex)
{
	if (*parent) {
		window = *parent;
		*parent = NULL;
	}

	while (true) {
		if (use_findwindowex)
			window = FindWindowEx(GetDesktopWindow(), window, NULL,
					      NULL);
		else
			window = GetNextWindow(window, GW_HWNDNEXT);

		if (!window || check_window_valid(window, mode))
			break;
	}

	if (ms_is_uwp_window(window)) {
		HWND child = ms_get_uwp_actual_window(window);
		if (child) {
			*parent = window;
			return child;
		}
	}

	return window;
}

int astrcmpi(const char *str1, const char *str2)
{
	if (!str1)
		str1 = astrblank;
	if (!str2)
		str2 = astrblank;

	do {
		char ch1 = (char)toupper(*str1);
		char ch2 = (char)toupper(*str2);

		if (ch1 < ch2)
			return -1;
		else if (ch1 > ch2)
			return 1;
	} while (*str1++ && *str2++);

	return 0;
}

void dstr_ncat(struct dstr *dst, const char *array, const size_t len)
{
	size_t new_len;
	if (!array || !*array || !len)
		return;

	new_len = dst->len + len;

	dstr_ensure_capacity(dst, new_len + 1);
	memcpy(dst->array + dst->len, array, len);

	dst->len = new_len;
	dst->array[new_len] = 0;
}

inline void dstr_cat(struct dstr *dst, const char *array)
{
	size_t len;
	if (!array || !*array)
		return;

	len = strlen(array);
	dstr_ncat(dst, array, len);
}

void dstr_cat_dstr(struct dstr *dst, const struct dstr *str)
{
	size_t new_len;
	if (!str->len)
		return;

	new_len = dst->len + str->len;

	dstr_ensure_capacity(dst, new_len + 1);
	memcpy(dst->array + dst->len, str->array, str->len + 1);
	dst->len = new_len;
}

void encode_dstr(struct dstr *str)
{
	dstr_replace(str, "#", "#22");
	dstr_replace(str, ":", "#3A");
}

void dstr_vprintf(struct dstr *dst, const char *format, va_list args)
{
	va_list args_cp;
	va_copy(args_cp, args);

	int len = vsnprintf(NULL, 0, format, args_cp);
	va_end(args_cp);

	if (len < 0)
		len = 4095;

	dstr_ensure_capacity(dst, ((size_t)len) + 1);
	len = vsnprintf(dst->array, ((size_t)len) + 1, format, args);

	if (!*dst->array) {
		dstr_free(dst);
		return;
	}

	dst->len = len < 0 ? strlen(dst->array) : (size_t)len;
}


void dstr_printf(struct dstr *dst, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	dstr_vprintf(dst, format, args);
	va_end(args);
}

inline int dstr_cmp(const struct dstr *str1, const char *str2)
{
	return strcmp(str1->array, str2);
}

bool is_microsoft_internal_window_exe(const char *exe)
{
	if (!exe)
		return false;

	for (const char **vals = internal_microsoft_exes_exact; *vals; vals++) {
		if (astrcmpi(exe, *vals) == 0)
			return true;
	}

	for (const char **vals = internal_microsoft_exes_partial; *vals;
	     vals++) {
		if (astrcmpi_n(exe, *vals, strlen(*vals)) == 0)
			return true;
	}

	return false;
}


HWND first_window(enum window_search_mode mode, HWND *parent,
			 bool *use_findwindowex)
{
	HWND window = FindWindowEx(GetDesktopWindow(), NULL, NULL, NULL);

	if (!window) {
		*use_findwindowex = false;
		window = GetWindow(GetDesktopWindow(), GW_CHILD);
	} else {
		*use_findwindowex = true;
	}

	*parent = NULL;

	if (!check_window_valid(window, mode)) {
		window = next_window(window, mode, parent, *use_findwindowex);

		if (!window && *use_findwindowex) {
			*use_findwindowex = false;

			window = GetWindow(GetDesktopWindow(), GW_CHILD);
			if (!check_window_valid(window, mode))
				window = next_window(window, mode, parent,
						     *use_findwindowex);
		}
	}

	if (ms_is_uwp_window(window)) {
		HWND child = ms_get_uwp_actual_window(window);
		if (child) {
			*parent = window;
			return child;
		}
	}

	return window;
}


bool is_uwp_class(const char *window_class)
{
	return strcmp(window_class, "Windows.UI.Core.CoreWindow") == 0;
}

int astrcmpi_n(const char *str1, const char *str2, size_t n)
{
	if (!n)
		return 0;
	if (!str1)
		str1 = astrblank;
	if (!str2)
		str2 = astrblank;

	do {
		char ch1 = (char)toupper(*str1);
		char ch2 = (char)toupper(*str2);

		if (ch1 < ch2)
			return -1;
		else if (ch1 > ch2)
			return 1;
	} while (*str1++ && *str2++ && --n);

	return 0;
}



char *astrstri(const char *str, const char *find)
{
	size_t len;

	if (!str || !find)
		return NULL;

	len = strlen(find);

	do {
		if (astrcmpi_n(str, find, len) == 0)
			return (char *)str;
	} while (*str++);

	return NULL;
}

bool is_generic_class(const char *current_class)
{
	const char **class_ = generic_class_substrings;
	while (*class_) {
		if (astrstri(current_class, *class_) != NULL) {
			return true;
		}
		class_ ++;
	}

	return false;
}

inline long os_atomic_dec_long(volatile long* val)
{
	return _InterlockedDecrement(val);
}

void a_free(void *ptr)
{
#ifdef ALIGNED_MALLOC
	_aligned_free(ptr);
#elif ALIGNMENT_HACK
	if (ptr)
		free((char *)ptr - ((char *)ptr)[-1]);
#else
	free(ptr);
#endif
}

void bfree(void *ptr)
{
	if (ptr) {
		os_atomic_dec_long(&num_allocs);
		a_free(ptr);
	}
}


inline void dstr_free(struct dstr *dst)
{
	bfree(dst->array);
	dst->array = NULL;
	dst->len = 0;
	dst->capacity = 0;
}

HMODULE kernel32(void)
{
	HMODULE kernel32_handle = NULL;
	if (!kernel32_handle)
		kernel32_handle = GetModuleHandleA("kernel32");
	return kernel32_handle;
}


void deobfuscate_str(char *str, uint64_t val)
{
	uint8_t *dec_val = (uint8_t *)&val;
	int i = 0;

	while (*str != 0) {
		int pos = i / 2;
		bool bottom = (i % 2) == 0;
		uint8_t *ch = (uint8_t *)str;
		uint8_t xor = bottom ? LOWER_HALFBYTE(dec_val[pos])
				     : UPPER_HALFBYTE(dec_val[pos]);

		*ch ^= xor;

		if (++i == sizeof(uint64_t) * 2)
			i = 0;

		str++;
	}
}


void *ms_get_obfuscated_func(HMODULE module, const char *str, uint64_t val)
{
	char new_name[128];
	strcpy(new_name, str);
	deobfuscate_str(new_name, val);
	return GetProcAddress(module, new_name);
}


inline int dstr_cmpi(const struct dstr *str1, const char *str2)
{
	return astrcmpi(str1->array, str2);
}

inline HANDLE open_process(DWORD desired_access, bool inherit_handle,
				  DWORD process_id)
{
	typedef HANDLE(WINAPI * PFN_OpenProcess)(DWORD, BOOL, DWORD);
	PFN_OpenProcess open_process_proc = NULL;
	if (!open_process_proc)
		open_process_proc = (PFN_OpenProcess)ms_get_obfuscated_func(
			kernel32(), "B}caZyah`~q", 0x2D5BEBAF6DDULL);

	return open_process_proc(desired_access, inherit_handle, process_id);
}

inline long os_atomic_inc_long(volatile long *val)
{
	return _InterlockedIncrement(val);
}

void blogva(int log_level, const char *format, va_list args)
{
	log_handler(log_level, format, args, log_param);
}

void blog(int log_level, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	blogva(log_level, format, args);
	va_end(args);
}

void def_log_handler(int log_level, const char *format, va_list args,
			    void *param)
{
	char out[4096];
	vsnprintf(out, sizeof(out), format, args);

	switch (log_level) {
	case LOG_DEBUG:
		fprintf(stdout, "debug: %s\n", out);
		fflush(stdout);
		break;

	case LOG_INFO:
		fprintf(stdout, "info: %s\n", out);
		fflush(stdout);
		break;

	case LOG_WARNING:
		fprintf(stdout, "warning: %s\n", out);
		fflush(stdout);
		break;

	case LOG_ERROR:
		fprintf(stderr, "error: %s\n", out);
		fflush(stderr);
	}

	UNUSED_PARAMETER(param);
}


void os_breakpoint(void)
{
	__debugbreak();
}

void *a_realloc(void *ptr, size_t size)
{
#ifdef ALIGNED_MALLOC
	return _aligned_realloc(ptr, size, ALIGNMENT);
#elif ALIGNMENT_HACK
	long diff;

	if (!ptr)
		return a_malloc(size);
	diff = ((char *)ptr)[-1];
	ptr = realloc((char *)ptr - diff, size + diff);
	if (ptr)
		ptr = (char *)ptr + diff;
	return ptr;
#else
	return realloc(ptr, size);
#endif
}


OBS_NORETURN void bcrash(const char *format, ...)
{
	va_list args;

	if (crashing) {
		fputs("Crashed in the crash handler", stderr);
		exit(2);
	}

	crashing = 1;
	va_start(args, format);
	crash_handler(format, args, crash_param);
	va_end(args);
	exit(0);
}

OBS_NORETURN void def_crash_handler(const char *format, va_list args,
					   void *param)
{
	vfprintf(stderr, format, args);
	exit(0);

	UNUSED_PARAMETER(param);
}

void *brealloc(void *ptr, size_t size)
{
	if (!ptr)
		os_atomic_inc_long(&num_allocs);

	if (!size) {
		blog(LOG_ERROR,
		     "brealloc: Allocating 0 bytes is broken behavior, please "
		     "fix your code! This will crash in future versions of "
		     "OBS.");
		size = 1;
	}

	ptr = a_realloc(ptr, size);

	if (!ptr) {
		os_breakpoint();
		bcrash("Out of memory while trying to allocate %lu bytes",
		       (unsigned long)size);
	}

	return ptr;
}

inline void dstr_ensure_capacity(struct dstr *dst, const size_t new_size)
{
	size_t new_cap;
	if (new_size <= dst->capacity)
		return;

	new_cap = (!dst->capacity) ? new_size : dst->capacity * 2;
	if (new_size > new_cap)
		new_cap = new_size;
	dst->array = (char *)brealloc(dst->array, new_cap);
	dst->capacity = new_cap;
}


inline void dstr_resize(struct dstr *dst, const size_t num)
{
	if (!num) {
		dstr_free(dst);
		return;
	}

	dstr_ensure_capacity(dst, num + 1);
	dst->array[num] = 0;
	dst->len = num;
}

void dstr_from_wcs(struct dstr *dst, const wchar_t *wstr)
{
	size_t len = wchar_to_utf8(wstr, 0, NULL, 0, 0);

	if (len) {
		dstr_resize(dst, len);
		wchar_to_utf8(wstr, 0, dst->array, len + 1, 0);
	} else {
		dstr_free(dst);
	}
}

bool ms_get_window_exe(struct dstr *name, HWND window)
{
	wchar_t wname[MAX_PATH];
	struct dstr temp = {0};
	bool success = false;
	HANDLE process = NULL;
	char *slash;
	DWORD id;

	GetWindowThreadProcessId(window, &id);
	if (id == GetCurrentProcessId())
		return false;

	process = open_process(PROCESS_QUERY_LIMITED_INFORMATION, false, id);
	if (!process)
		goto fail;

	if (!GetProcessImageFileNameW(process, wname, MAX_PATH))
		goto fail;

	dstr_from_wcs(&temp, wname);
	slash = strrchr(temp.array, '\\');
	if (!slash)
		goto fail;

	dstr_copy(name, slash + 1);
	success = true;

fail:
	if (!success)
		dstr_copy(name, "unknown");

	dstr_free(&temp);
	CloseHandle(process);
	return true;
}


void dstr_copy(struct dstr *dst, const char *array)
{
	size_t len;

	if (!array || !*array) {
		dstr_free(dst);
		return;
	}

	len = strlen(array);
	dstr_ensure_capacity(dst, len + 1);
	memcpy(dst->array, array, len + 1);
	dst->len = len;
}

void ms_get_window_title(struct dstr *name, HWND hwnd)
{
	int len;

	len = GetWindowTextLengthW(hwnd);
	if (!len)
		return;

	if (len > 1024) {
		wchar_t *temp;

		temp = (wchar_t*)malloc(sizeof(wchar_t) * (len + 1));
		if (!temp)
			return;

		if (GetWindowTextW(hwnd, temp, len + 1))
			dstr_from_wcs(name, temp);

		free(temp);
	} else {
		wchar_t temp[1024 + 1];

		if (GetWindowTextW(hwnd, temp, len + 1))
			dstr_from_wcs(name, temp);
	}
}

void ms_get_window_class(struct dstr *class_, HWND hwnd)
{
	wchar_t temp[256];

	temp[0] = 0;
	if (GetClassNameW(hwnd, temp, sizeof(temp) / sizeof(wchar_t)))
		dstr_from_wcs(class_, temp);
}


char **strlist_split(const char *str, char split_ch, bool include_empty)
{
	const char *cur_str = str;
	const char *next_str;
	char *out = NULL;
	size_t count = 0;
	size_t total_size = 0;

	if (str) {
		char **table;
		char *offset;
		size_t cur_idx = 0;
		size_t cur_pos = 0;

		next_str = strchr(str, split_ch);

		while (next_str) {
			size_t size = next_str - cur_str;

			if (size || include_empty) {
				++count;
				total_size += size + 1;
			}

			cur_str = next_str + 1;
			next_str = strchr(cur_str, split_ch);
		}

		if (*cur_str || include_empty) {
			++count;
			total_size += strlen(cur_str) + 1;
		}

		/* ------------------ */

		cur_pos = (count + 1) * sizeof(char *);
		total_size += cur_pos;
		out = (char*) bmalloc(total_size);
		offset = out + cur_pos;
		table = (char **)out;

		/* ------------------ */

		next_str = strchr(str, split_ch);
		cur_str = str;

		while (next_str) {
			size_t size = next_str - cur_str;

			if (size || include_empty) {
				table[cur_idx++] = offset;
				strncpy(offset, cur_str, size);
				offset[size] = 0;
				offset += size + 1;
			}

			cur_str = next_str + 1;
			next_str = strchr(cur_str, split_ch);
		}

		if (*cur_str || include_empty) {
			table[cur_idx++] = offset;
			strcpy(offset, cur_str);
		}

		table[cur_idx] = NULL;
	}

	return (char **)out;
}


void *a_malloc(size_t size)
{
#ifdef ALIGNED_MALLOC
	return _aligned_malloc(size, ALIGNMENT);
#elif ALIGNMENT_HACK
	void *ptr = NULL;
	long diff;

	ptr = malloc(size + ALIGNMENT);
	if (ptr) {
		diff = ((~(long)ptr) & (ALIGNMENT - 1)) + 1;
		ptr = (char *)ptr + diff;
		((char *)ptr)[-1] = (char)diff;
	}

	return ptr;
#else
	return malloc(size);
#endif
}

void *bmalloc(size_t size)
{
	if (!size) {
		blog(LOG_ERROR,
		     "bmalloc: Allocating 0 bytes is broken behavior, please "
		     "fix your code! This will crash in future versions of "
		     "OBS.");
		size = 1;
	}

	void *ptr = a_malloc(size);

	if (!ptr) {
		os_breakpoint();
		bcrash("Out of memory while trying to allocate %lu bytes",
		       (unsigned long)size);
	}

	os_atomic_inc_long(&num_allocs);
	return ptr;
}

inline char *decode_str(const char *src)
{
	struct dstr str = {0};
	dstr_copy(&str, src);
	dstr_replace(&str, "#3A", ":");
	dstr_replace(&str, "#22", "#");
	return str.array;
}

inline bool dstr_is_empty(const struct dstr *str)
{
	if (!str->array || !str->len)
		return true;
	if (!*str->array)
		return true;

	return false;
}

void dstr_replace(struct dstr *str, const char *find, const char *replace)
{
	size_t find_len, replace_len;
	char *temp;

	if (dstr_is_empty(str))
		return;

	if (!replace)
		replace = "";

	find_len = strlen(find);
	replace_len = strlen(replace);
	temp = str->array;

	if (replace_len < find_len) {
		unsigned long count = 0;

		while ((temp = strstr(temp, find)) != NULL) {
			char *end = temp + find_len;
			size_t end_len = strlen(end);

			if (end_len) {
				memmove(temp + replace_len, end, end_len + 1);
				if (replace_len)
					memcpy(temp, replace, replace_len);
			} else {
				strcpy(temp, replace);
			}

			temp += replace_len;
			++count;
		}

		if (count)
			str->len += (replace_len - find_len) * count;

	} else if (replace_len > find_len) {
		unsigned long count = 0;

		while ((temp = strstr(temp, find)) != NULL) {
			temp += find_len;
			++count;
		}

		if (!count)
			return;

		str->len += (replace_len - find_len) * count;
		dstr_ensure_capacity(str, str->len + 1);
		temp = str->array;

		while ((temp = strstr(temp, find)) != NULL) {
			char *end = temp + find_len;
			size_t end_len = strlen(end);

			if (end_len) {
				memmove(temp + replace_len, end, end_len + 1);
				memcpy(temp, replace, replace_len);
			} else {
				strcpy(temp, replace);
			}

			temp += replace_len;
		}

	} else {
		while ((temp = strstr(temp, find)) != NULL) {
			memcpy(temp, replace, replace_len);
			temp += replace_len;
		}
	}
}

void ms_build_window_strings(const char *str, char **class_, char **title,
			     char **exe)
{
	char **strlist;

	*class_ = NULL;
	*title = NULL;
	*exe = NULL;

	if (!str) {
		return;
	}

	strlist = strlist_split(str, ':', true);

	if (strlist && strlist[0] && strlist[1] && strlist[2]) {
		*title = decode_str(strlist[0]);
		*class_ = decode_str(strlist[1]);
		*exe = decode_str(strlist[2]);
	}

	strlist_free(strlist);
}

void strlist_free(char **strlist)
{
	bfree(strlist);
}

int window_rating(HWND window, enum window_priority priority,
			 const char *class_, const char *title, const char *exe,
			 bool uwp_window, bool generic_class)
{
	struct dstr cur_class = {0};
	struct dstr cur_title = {0};
	struct dstr cur_exe = {0};
	int val = 0x7FFFFFFF;

	if (!ms_get_window_exe(&cur_exe, window))
		return 0x7FFFFFFF;
	ms_get_window_title(&cur_title, window);
	ms_get_window_class(&cur_class, window);

	bool class_matches = dstr_cmpi(&cur_class, class_) == 0;
	bool exe_matches = dstr_cmpi(&cur_exe, exe) == 0;
	int title_val = abs(dstr_cmpi(&cur_title, title));

	if (generic_class && (priority == WINDOW_PRIORITY_CLASS))
		priority = WINDOW_PRIORITY_TITLE;

	/* always match by name with UWP windows */
	if (uwp_window) {
		if (priority == WINDOW_PRIORITY_EXE && !exe_matches)
			val = 0x7FFFFFFF;
		else
			val = title_val == 0 ? 0 : 0x7FFFFFFF;

	} else if (priority == WINDOW_PRIORITY_CLASS) {
		val = class_matches ? title_val : 0x7FFFFFFF;
		if (val != 0x7FFFFFFF && !exe_matches)
			val += 0x1000;

	} else if (priority == WINDOW_PRIORITY_TITLE) {
		val = title_val == 0 ? 0 : 0x7FFFFFFF;

	} else if (priority == WINDOW_PRIORITY_EXE) {
		val = exe_matches ? title_val : 0x7FFFFFFF;
	}

	dstr_free(&cur_class);
	dstr_free(&cur_title);
	dstr_free(&cur_exe);

	return val;
}


HWND ms_find_window(enum window_search_mode mode, enum window_priority priority,
		    const char *class_, const char *title, const char *exe)
{
	HWND parent;
	bool use_findwindowex = false;

	HWND window = first_window(mode, &parent, &use_findwindowex);
	HWND best_window = NULL;
	int best_rating = 0x7FFFFFFF;

	if (!class_)
		return NULL;

	const bool uwp_window = is_uwp_class(class_);
	const bool generic_class = is_generic_class(class_);

	while (window) {
		int rating = window_rating(window, priority, class_, title, exe,
					   uwp_window, generic_class);
		if (rating < best_rating) {
			best_rating = rating;
			best_window = window;
			if (rating == 0)
				break;
		}

		window = next_window(window, mode, &parent, use_findwindowex);
	}

	return best_window;
}