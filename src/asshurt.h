//

/*
"asshurt"ions.

idea: 
	combine the usual horrible pattern:
		int ret = can_fail(var_arg, "inline arg", 123);
		assert(ret != 0);
		if(ret != 0) { fprintf(stderr, "Oh no, ret returned: %d with arguments inline arg & 123", ret); return false; }

	into a single macro:
		int ret = 0;
		asshurt_f(ret = can_fail(var_arg, "inline arg", 123), "Oh no, ret returned: %d!", ret) { return false; }

	asshurt_t (c, varags) { } // assert condition is true in all builds;    run brackets when condition is true
	asshurt_f (c, varags) { } // assert condition is true in all builds;    run brackets when condition is false
	asshurt_st(c, varags) { } // assert condition is true in release build; run brackets when condition is true
	asshurt_sf(c, varags) { } // assert condition is true in release build; run brackets when condition is false

	bonus:
		static_assert implemented in C using a negative subscript trick.

		struct s_t { ... };
		asshurt_static(sizeof(s_t) == 123);
*/

#ifndef ASSHURT_H
#define ASSHURT_H

#include <Windows.h>
#include <stdbool.h>
#include <math.h>

bool _asshurt_proc_b(const bool c, const char* condition_str, const char* file, const int file_line, const char* format, ...);

#if NDEBUG
#define asshurt_t(c, ...)  if( _asshurt_proc_b(c, #c, "", __LINE__, "" __VA_ARGS__))
#define asshurt_f(c, ...)  if(!_asshurt_proc_b(c, #c, "", __LINE__, "" __VA_ARGS__))
#define asshurt_st(c, ...) if(c)
#define asshurt_sf(c, ...) if(!(c))
#define asshurt(c, ...)    (void)(0)
#else
#define asshurt_t(c, ...)  if( _asshurt_proc_b(c, #c, __FILE__, __LINE__, "" __VA_ARGS__))
#define asshurt_f(c, ...)  if(!_asshurt_proc_b(c, #c, __FILE__, __LINE__, "" __VA_ARGS__))
#define asshurt_st(c, ...) if( _asshurt_proc_b(c, #c, __FILE__, __LINE__, "" __VA_ARGS__))
#define asshurt_sf(c, ...) if(!_asshurt_proc_b(c, #c, __FILE__, __LINE__, "" __VA_ARGS__))
#define asshurt(c, ...)        _asshurt_proc_b(c, #c, __FILE__, __LINE__, "" __VA_ARGS__)
#endif

#define _asshurt_static3(c, msg) typedef char asshurt_static_##msg[(!!(c))*2-1]
#define _asshurt_static2(c, line) _asshurt_static3(c, at_line_##line)
#define _asshurt_static1(c, line) _asshurt_static2(c, line)
#define  asshurt_static(c)        _asshurt_static1(c, __LINE__)
#endif

#ifdef ASSHURT_IMPL
#ifndef ASSHURT_IMPL_H
#define ASSHURT_IMPL_H

#define _asizei(a) (int)(sizeof(a)/sizeof(*a))

typedef struct {
	size_t condition_len;
	size_t file_len;
	int file_line;
} asshurt_id_t;
asshurt_id_t _asshurt_ignore_ids[128];
int _asshurt_ignore_ids_n = 0;
bool asshurt_id_eq(asshurt_id_t a, asshurt_id_t b) { return a.condition_len == b.condition_len && a.file_len == b.file_len && a.file_line == b.file_line; }
bool _asshurt_proc_b(const bool c, const char* condition_str, const char* file, const int file_line, const char* format, ...) {

	if(c) { return c; }

	bool ignored = false;
	asshurt_id_t this_asshurt_id;
	this_asshurt_id.condition_len = strlen(condition_str);
	this_asshurt_id.file_len = strlen(file);
	this_asshurt_id.file_line = file_line;
	for(int i = 0; i < _asshurt_ignore_ids_n; ++i) {
		if(asshurt_id_eq(this_asshurt_id, _asshurt_ignore_ids[i])) {
			ignored = true;
			break;
		}
	}

	if(!ignored) {
		static char message[2048] = { 0 };
		int message_n = 0;

		va_list args;
		va_start(args, format);
		message_n += vsnprintf(message + message_n, _asizei(message) - message_n, format, args);
		va_end(args);

		message_n += snprintf(message + message_n, _asizei(message) - message_n, "\n\ncondition failed: '%s'\n", condition_str);
#ifndef NDEBUG
		message_n += snprintf(message + message_n, _asizei(message) - message_n, "\nfile: %s\n", file);
#endif
		message_n += snprintf(message + message_n, _asizei(message) - message_n, "\nline: %d\n", file_line);
		fprintf(stderr, "%s", message);

		bool do_ignore = false;
		bool do_quit = false;
#ifndef __linux__
		message_n += snprintf(message + message_n, _asizei(message) - message_n, "\n\npress CTRL+C to copy this error to your clipboard.");

		const int mb_id = MessageBox(NULL, message, "assertion failed", MB_ABORTRETRYIGNORE | MB_ICONERROR);
		do_quit = mb_id == IDCANCEL || mb_id == IDABORT; 
		do_ignore = mb_id == IDIGNORE;
#else
		fprintf(stderr, "enter 'i' to ignore, 'q' to quit\n");

		char str[64];
		scanf("%63s", str);
		do_quit = str[0] == 'q';
		do_ignore = str[0] == 'i';
#endif
		if(do_quit) { 
			exit(1); 
		}
		if(do_ignore && _asshurt_ignore_ids_n + 1 < _asizei(_asshurt_ignore_ids)) {
			_asshurt_ignore_ids[_asshurt_ignore_ids_n++] = this_asshurt_id;
		}

	}
	return c;
}
#endif
#endif
