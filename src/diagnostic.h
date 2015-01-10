#ifndef DIAGNOSTIC_H
#define DIAGNOSTIC_H

namespace c4 {
struct Pos;
}

void errorf(const c4::Pos &, const char * fmt, ...);

void errorf(const char* fmt, ...);

void errorErrno(const c4::Pos& pos);

//! Returns true, if there are errors without resetting the flag.
bool hasErrors();

bool hasNewErrors();

int printDiagnosticSummary();

#endif
