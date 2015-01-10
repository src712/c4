#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <errno.h>
#include <iostream>

#include "pos.h"
#include "diagnostic.h"

static void verrorf(c4::Pos const*, char const* fmt, va_list);

static unsigned nErrors   = 0;
static bool     newErrors = false;

void errorErrno(c4::Pos const& pos)
{
	errorf(pos, "%s", strerror(errno));
}

void errorf(c4::Pos const& pos, char const* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	verrorf(&pos, fmt, ap);
	va_end(ap);
}

void errorf(char const* fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	verrorf(nullptr, fmt, ap);
	va_end(ap);
}

bool hasErrors()
{
  return newErrors;
}

bool hasNewErrors()
{
	auto const res = newErrors;
	newErrors = false;
	return res;
}

int printDiagnosticSummary()
{
	if (nErrors != 0) {
		fprintf(stderr, "%u error(s)\n", nErrors);
		return 1;
	}
	return 0;
}

static void verrorf(c4::Pos const* const pos, char const* fmt, va_list ap)
{
#if __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-extra-args"
#endif
	newErrors = true;
	++nErrors;

        // flush std::cout before printing anything to stderr to avoid
        // interleaving, since we turned of io sync and never flush
        // cout
        std::cout.flush();

	auto const out = stderr;

	if (pos) {
		auto const posFmt =
			pos->column != 0 ? "%s:%u:%u: " :
			pos->line   != 0 ? "%s:%u: "    :
			"%s: ";
		fprintf(out, posFmt, pos->name, pos->line, pos->column);
	}
	fputs("error: ", out);

	for (; auto f = strchr(fmt, '%'); fmt = f) {
		fwrite(fmt, 1, f - fmt, out);
		++f; // Skip '%'.
		switch (*f++) {
		case '%':
			fputc('%', out);
			break;

		case 'c': {
			auto const c = (char)va_arg(ap, int);
			fputc(c, out);
			break;
		}

		case 's': {
			auto const s = va_arg(ap, char const*);
			fputs(s, out);
			break;
		}

		default:
			PANIC("invalid format specifier");
		}
	}
	fputs(fmt, out);

	fputc('\n', out);
#if __clang__
#pragma clang diagnostic pop
#endif
}
