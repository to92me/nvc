#ifndef _IDENT_H
#define _IDENT_H

#include "lib.h"

typedef struct trie *ident_t;

// Intern a string as an identifier.
ident_t ident_new(const char *str);

// Generate a unique identifier with the given prefix.
ident_t ident_uniq(const char *prefix);

// Create a new identifier which is a prepended to b separated
// by a dot.
ident_t ident_prefix(ident_t a, ident_t b);

// Convert an identifier reference to a NULL-terminated string.
// This function is quite slow so its use should be avoid except
// for printing. The pointer returned is only valid for the next
// ISTR_MAX_BUFS calls to istr.
#define ISTR_MAX_BUFS 8
const char *istr(ident_t ident);

void ident_write(ident_t ident, FILE *f);
ident_t ident_read(FILE *f);

#endif // _IDENT_H
