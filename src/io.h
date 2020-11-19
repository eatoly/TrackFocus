#pragma once

#include "pch.h"

typedef int zint;
#define path_n 260
#define pathi { '\0' }

inline bool s_endswith(const char* s, const char* end) {
	const zint s_len = strlen(s);
	const zint end_len = strlen(end);
	if(s_len < end_len) { return false; }
	for(int i = 0; i < end_len; ++i) {
		if(s[s_len - i - 1] != end[end_len - i - 1]) { return false; }
	}
	return true;
}

struct io_t {
	FILE* fp;

	bool write;

	char* line;
	int line_n;

	char* error;
	int error_n;
};

inl void io_init(io_t* io, FILE* fp, bool write, char* out_line, int out_line_n, char* out_error, int out_error_n) {
	io->fp = fp;
	io->write = write;
	io->line = out_line;
	io->line_n = out_line_n;
	io->error = out_error;
	io->error_n = out_error_n;

	io->line[0] = '\0';
	io->error[0] = '\0';
}

#define IO_SHARED \
	;

#define IO_READ \
	fgets(io->line, io->line_n, io->fp)

#define IO_READ_EXTRA { \
	const char last_io_line_ch = io->line[strlen(io->line) - 1]; \
	if(last_io_line_ch != '\n' && last_io_line_ch != '\0') { \
		while(fgets(io->line, io->line_n, io->fp)) { \
			const char last_io_line_ch = io->line[strlen(io->line) - 1]; \
			if(last_io_line_ch == '\n' || last_io_line_ch == '\0') break; \
		} \
	} \
}

#define IO_WRITE \
	va_list vargs; \
	va_start(vargs, fmt); \
	vsnprintf(io->line, io->line_n, fmt, vargs); \
	va_end(vargs)

#define IO_ERROR \
	snprintf(io->error, io->error_n, "failed to %s %s (%s %d)", (io->write ? "write" : "parse"), datum_str, file, line); \
	return false


/* todo: this io system is a bit flawed on the basis of the vargs. right now it's tough to have vargs passed through down to smaller units. ie (...) -> (...) doesn't convert. we'll need to make it pass in va_args for composition to work. */

#define io_int8(io, datum, ...) _io_int8(io, &(datum), #datum, "int8", __FILE__, __LINE__, "" __VA_ARGS__)
bool _io_int8(io_t* io, int8_t* datum, const char* datum_str, const char* type, const char* file, const int line, const char* fmt, ...);

#define io_int16(io, datum, ...) _io_int16(io, &(datum), #datum, "int16", __FILE__, __LINE__, "" __VA_ARGS__)
bool _io_int16(io_t* io, int16_t* datum, const char* datum_str, const char* type, const char* file, const int line, const char* fmt, ...);

#define io_int(io, datum, ...) _io_int(io, &(datum), #datum, "int", __FILE__, __LINE__, "" __VA_ARGS__)
bool _io_int(io_t* io, int32_t* datum, const char* datum_str, const char* type, const char* file, const int line, const char* fmt, ...);

#if 0
#define io_int64(io, datum, ...) _io_int64(io, &(datum), #datum, "int64", __FILE__, __LINE__, "" __VA_ARGS__)
#endif

#define io_uint8(io, datum, ...) _io_uint8(io, &(datum), #datum, "uint8", __FILE__, __LINE__, "" __VA_ARGS__)
bool _io_uint8(io_t* io, uint8_t* datum, const char* datum_str, const char* type, const char* file, const int line, const char* fmt, ...);

#define io_uint16(io, datum, ...) _io_uint16(io, &(datum), #datum, "uint16", __FILE__, __LINE__, "" __VA_ARGS__)
bool _io_uint16(io_t* io, uint16_t* datum, const char* datum_str, const char* type, const char* file, const int line, const char* fmt, ...);

#define io_uint(io, datum, ...) _io_uint(io, &(datum), #datum, "uint", __FILE__, __LINE__, "" __VA_ARGS__)
bool _io_uint(io_t* io, uint32_t* datum, const char* datum_str, const char* type, const char* file, const int line, const char* fmt, ...);

#if 0
#define io_uint64(io, datum, ...) _io_uint(io, &(datum), #datum, "uint64", __FILE__, __LINE__, "" __VA_ARGS__)
#endif

#define io_float(io, datum, ...) _io_float(io, &(datum), #datum, "float", __FILE__, __LINE__, "" __VA_ARGS__)
bool _io_float(io_t* io, float* datum, const char* datum_str, const char* type, const char* file, const int line, const char* fmt, ...);

#define io_float2(io, datum, ...) _io_float2(io, datum, #datum, "float2", __FILE__, __LINE__, "" __VA_ARGS__)
bool _io_float2(io_t* io, float* datum, const char* datum_str, const char* type, const char* file, const int line, const char* fmt, ...);

#define io_float3(io, datum, ...) _io_float3(io, datum, #datum, "float3", __FILE__, __LINE__, "" __VA_ARGS__)
bool _io_float3(io_t* io, float* datum, const char* datum_str, const char* type, const char* file, const int line, const char* fmt, ...);

#define io_float4(io, datum, ...) _io_float4(io, datum, #datum, "float4", __FILE__, __LINE__, "" __VA_ARGS__)
#define io_color4(io, datum, ...) _io_float4(io, datum.v, #datum, "color4", __FILE__, __LINE__, "" __VA_ARGS__)
bool _io_float4(io_t* io, float* datum, const char* datum_str, const char* type, const char* file, const int line, const char* fmt, ...);

#if 0
#define io_double(io, datum, ...) _io_double(io, &(datum), #datum, "double", __FILE__, __LINE__, "" __VA_ARGS__)
#endif

#define io_string(io, datum, datum_n, ...) _io_string(io, datum, datum_n, #datum, "string", __FILE__, __LINE__, "" __VA_ARGS__)
bool _io_string(io_t* io, char* datum, const int datum_n, const char* datum_str, const char* type, const char* file, const int line, const char* fmt, ...);


/* custom io */
#define io_bool(io, datum, ...) _io_uint8(io, (uint8_t*)&(datum), #datum, "bool", __FILE__, __LINE__, "" __VA_ARGS__)
#define io_vec2(io, datum, ...) _io_float2(io, (float*)&(datum.x), #datum, "vec2", __FILE__, __LINE__, "" __VA_ARGS__)
#define io_vec3(io, datum, ...) _io_float3(io, (float*)&(datum.x), #datum, "vec3", __FILE__, __LINE__, "" __VA_ARGS__)
#define io_vec4(io, datum, ...) _io_float4(io, (float*)&(datum.x), #datum, "vec4", __FILE__, __LINE__, "" __VA_ARGS__)