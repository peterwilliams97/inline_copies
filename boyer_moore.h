#include <list>
#include "BinString.h"

const byte* boyer_moore(const byte *text, size_t textlen, const byte *pat, size_t patlen);
std::vector<const byte *> boyer_moore_all(const byte *text, size_t textlen, const byte *pat, size_t patlen, size_t min_gap);
