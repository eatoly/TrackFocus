#ifndef shared_h
#define shared_h

#define unused(a) (void)a
#define cast(T) (T)
#define asize(a) (int)(sizeof(a)/sizeof(*a))
#define slen(s) (int)(sizeof(s)/sizeof(*s) - 1)
#ifndef __linux__
#define inl __forceinline
#else
#define inl __attribute__((always_inline)) inline
#endif
#define null NULL
#define fuct(type) \
	struct type; \
	typedef struct type type; \
	struct type

#define cunt(type) \
	union type; \
	typedef union type type; \
	union type

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#define zerom(d) memset((d), 0, sizeof(*(d)))

typedef size_t    zuint;
typedef ptrdiff_t zint;

inl int mini(const int x, const int y) { return x > y ? y : x; }
inl int maxi(const int x, const int y) { return x < y ? y : x; }
inl int clampi(const int v, const int lo, const int hi) { return mini(maxi(v, lo), hi); }

inl zint minzi(const zint x, const zint y) { return x > y ? y : x; }
inl zint maxzi(const zint x, const zint y) { return x < y ? y : x; }
inl zint clampzi(const zint v, const zint lo, const zint hi) { return minzi(maxzi(v, lo), hi); }

inl float minf(const float x, const float y) { return x > y ? y : x; }
inl float maxf(const float x, const float y) { return x < y ? y : x; }
inl float clampf(const float v, const float lo, const float hi) { return minf(maxf(v, lo), hi); }

inl void strcpyn(char* dest, const char* source, const size_t capacity) { strncpy(dest, source, capacity); dest[capacity - 1] = '\0'; }

inl bool c_space(const char ch) { return ch <= 32 || 127 <= ch; }
inl bool c_separator(const char ch) { return ch == '/' || ch == '\\'; }

#define path_n 260
typedef struct { char s[path_n]; } path_t;
#define pathi { '\0' }
inl path_t path() { const path_t p = pathi; return p; }
inl zint p_len(const path_t path) { return (zint)strlen(path.s); }
inl path_t p_cstr(const char* str) { path_t p; strcpyn(p.s, str, path_n); return p; }
inl path_t p_cstrn(const char* str, const zint str_n) { path_t p; strcpyn(p.s, str, str_n); return p; }
inl path_t p_cstrlen(const char* str, const zint str_len) { path_t p; strcpyn(p.s, str, str_len + 1); return p; }
inl path_t p_fmt(const char* fmt, ...) { 
	path_t path;
	va_list args; 
	va_start(args, fmt); 
	vsnprintf(path.s, asize(path.s), fmt, args); 
	va_end(args);
	return path;
}

inl path_t p_slash(path_t p, const char bad, const char good) {
	const zint len = p_len(p);
	zint i = 0;
	for(zint z = 0; z < len; ++z) {
		if(p.s[z] == bad || p.s[z] == good) {
			p.s[i] = good;
		} else {
			if(p.s[i] == good) { ++i; }
			p.s[i] = p.s[z];
			++i;
		}
	}
	if(p.s[i] == good) { ++i; }
	p.s[i] = '\0';
	return p;
}
inl path_t p_slashb(const path_t p) { return p_slash(p, '/', '\\'); }
inl path_t p_slashf(const path_t p) { return p_slash(p, '\\', '/'); }
inline path_t path_trim(path_t trimmed) {
	path_t tmp = trimmed;
	const zint a_len = p_len(tmp);
	zint start_i = 0;
	for(; start_i < a_len; ++start_i) {
		const char ch = tmp.s[start_i];
		if(!c_separator(ch) && !c_space(ch)) { break; }
	}
	strcpyn(trimmed.s, tmp.s + start_i, path_n);

	const zint trimmed_len = p_len(trimmed);
	zint end_len = trimmed_len;
	for(; end_len > 0; --end_len) {
		const char ch = trimmed.s[end_len - 1];
		if(!c_separator(ch) && !c_space(ch)) { break; }
	}

	trimmed.s[end_len] = '\0';
	return trimmed;
}
inl path_t p_cat(path_t a, path_t b, const char sep) {
	a = path_trim(a);
	b = path_trim(b);
	if(p_len(a) <= 0) { return b; }
	if(p_len(b) <= 0) { return a; }
	path_t r;
	snprintf(r.s, asize(r.s), "%s%c%s", a.s, sep, b.s);
	return r;
}
inl path_t p_catb(const path_t a, const path_t b) { return p_cat(a, b, '\\'); }
inl path_t p_catf(const path_t a, const path_t b) { return p_cat(a, b, '/'); }
inl path_t p_app(const path_t a, const path_t b) { path_t r; snprintf(r.s, asize(r.s), "%s%s", a.s, b.s); return r; }
inl path_t p_ext(path_t a) {
	const zint len = p_len(a);
	for(zint i = 0; i < len; ++i) {
		if(a.s[i] == '.') { return p_cstr(&a.s[i]); }
	}
	return a;
}
inl path_t p_file(const path_t a) {
	const zint len = p_len(a);
	for(zint i = len - 1; i >= 0; --i) {
		if(c_separator(a.s[i])) { return p_cstr(&a.s[i + 1]); }
	}
	return a;
}
inl path_t p_folder(path_t a) {
	const zint len = p_len(a);
	for(zint i = len - 1; i >= 0; --i) {
		if(c_separator(a.s[i])) { a.s[i] = '\0'; return a; }
	}
	return a;
}

// printf("%s\n", p_abs(p_cstr("C:/sample/include.c"), p_cstr("./folder/../../bin/./folder/snapcode.dll")).s); // => C:/bin/folder/snapcode.dll 
inl path_t p_abslen(const path_t abs, const char* rel, const zint rel_len) {
	const char period = '.';

	path_t ret = p_folder(abs); 
	zint last_i = 0;
	for(zint cur_i = 0; cur_i < rel_len; ++cur_i) {
		const char ch = rel[cur_i];
		if(!c_separator(ch)) { continue; }
		const zint len = cur_i - last_i;
		const char* str = &rel[last_i];
		last_i = cur_i + 1;
		if(len == 1 && str[0] == period) { continue; }
		if(len == 2 && str[0] == period && str[1] == period) { ret = p_folder(ret); continue; }
		ret = p_catf(ret, p_cstrn(str, len + 1));
	}
	ret = p_catf(ret, p_cstrn(&rel[last_i], rel_len - last_i + 1));

	return ret;
}
inl path_t p_abs(const path_t abs, const char* rel) { return p_abslen(abs, rel, strlen(rel)); }

inl bool s_ends_with(const char* s, const char* end) {
	const zint s_len = strlen(s);
	const zint end_len = strlen(end);
	if(s_len < end_len) { return false; }
	for(int i = 0; i < end_len; ++i) {
		if(s[s_len - i - 1] != end[end_len - i - 1]) { return false; }
	}
	return true;
}

inl bool s_ends_with_any_of(const char* s, const char** ends, const int ends_n) {
	for(int end_i = 0; end_i < ends_n; ++end_i) { if(s_ends_with(s, ends[end_i])) { return true; } }
	return false;
}

#endif
