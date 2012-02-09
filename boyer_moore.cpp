#include <list>
#include <stdlib.h>
#include "boyer_moore.h"

using namespace std;

#ifdef  _WIN64
typedef  __int64    ssize_t;
#elif defined(WIN32)
typedef _W64  int   ssize_t;
#endif

#define ALPHABET_LEN 255
#define NOT_FOUND patlen
#define max(a, b) ((a < b) ? b : a)
 
// delta1 table: delta1[c] contains the distance between the last
// character of pat and the rightmost occurence of c in pat.
// If c does not occur in pat, then delta1[c] = patlen.
// If c is at string[i] and c != pat[patlen-1], we can
// safely shift i over by delta1[c], which is the minimum distance
// needed to shift pat forward to get string[i] lined up 
// with some character in pat.
// this algorithm runs in alphabet_len+patlen time.
void make_delta1(size_t *delta1, const byte *pat, size_t patlen) {
    size_t i;
    for (i = 0; i < ALPHABET_LEN; i++) {
        delta1[i] = NOT_FOUND;
    }
    for (i = 0; i < patlen - 1; i++) {
        delta1[pat[i]] = patlen - 1 - i;
    }
}
 
// true if the suffix of word starting from word[pos] is a prefix 
// of word
int is_prefix(const byte *word, size_t wordlen, size_t pos) {
    size_t i;
    size_t suffixlen = wordlen - pos;
    // could also use the strncmp() library function here
    for (i = 0; i < suffixlen; i++) {
        if (word[i] != word[pos+i]) {
            return 0;
        }
    }
    return 1;
}
 
// length of the longest suffix of word ending on word[pos].
// suffix_length("dddbcabc", 8, 4) = 2
size_t suffix_length(const byte *word, size_t wordlen, size_t pos) {
    size_t i;
    // increment suffix length i to the first mismatch or beginning
    // of the word
    for (i = 0; (word[pos-i] == word[wordlen-1-i]) && (i < pos); i++)
        ;;;
    return i;
}
 
// delta2 table: given a mismatch at pat[pos], we want to align 
// with the next possible full match could be based on what we
// know about pat[pos+1] to pat[patlen-1].
//
// In case 1:
// pat[pos+1] to pat[patlen-1] does not occur elsewhere in pat,
// the next plausible match starts at or after the mismatch.
// If, within the substring pat[pos+1 .. patlen-1], lies a prefix
// of pat, the next plausible match is here (if there are multiple
// prefixes in the substring, pick the longest). Otherwise, the
// next plausible match starts past the character aligned with 
// pat[patlen-1].
// 
// In case 2:
// pat[pos+1] to pat[patlen-1] does occur elsewhere in pat. The
// mismatch tells us that we are not looking at the end of a match.
// We may, however, be looking at the middle of a match.
// 
// The first loop, which takes care of case 1, is analogous to
// the KMP table, adapted for a 'backwards' scan order with the
// additional restriction that the substrings it considers as 
// potential prefixes are all suffixes. In the worst case scenario
// pat consists of the same letter repeated, so every suffix is
// a prefix. This loop alone is not sufficient, however:
// Suppose that pat is "ABYXCDEYX", and text is ".....ABYXCDEYX".
// We will match X, Y, and find B != E. There is no prefix of pat
// in the suffix "YX", so the first loop tells us to skip forward
// by 9 characters.
// Although superficially similar to the KMP table, the KMP table
// relies on information about the beginning of the partial match
// that the BM algorithm does not have.
//
// The second loop addresses case 2. Since suffix_length may not be
// unique, we want to take the minimum value, which will tell us
// how far away the closest potential match is.
void make_delta2(size_t *delta2, const byte *pat, size_t patlen) {
    ssize_t p;
    size_t last_prefix_index = patlen-1;
 
    // first loop
    for (p = patlen-1; p >= 0; p--) {
        if (is_prefix(pat, patlen, p+1)) {
            last_prefix_index = p+1;
        }
        delta2[p] = last_prefix_index + (patlen-1 - p);
    }
 
    // second loop
    for (p = 0; p < (ssize_t)(patlen-1); p++) {
        size_t slen = suffix_length(pat, patlen, p);
        if (pat[p - slen] != pat[patlen-1 - slen]) {
            delta2[patlen-1 - slen] = patlen-1 - p + slen;
        }
    }
}

static const byte *scan_text(const byte *text, size_t textlen, const byte *pat, size_t patlen,
                        const size_t *delta1,  const size_t *delta2) {
    size_t i = patlen - 1;
    while (i < textlen) {
        ssize_t j = patlen-1;
        while (j >= 0 && (text[i] == pat[j])) {
            --i;
            --j;
        }
        if (j < 0) {
            return text + i + 1;
        }
 
        i += max(delta1[text[i]], delta2[j]);
    }
    return NULL;
}

const byte *boyer_moore(const byte *text, size_t textlen, const byte *pat, size_t patlen) {
 
    size_t delta1[ALPHABET_LEN];
    size_t *delta2 = (size_t *)malloc(patlen * sizeof(size_t));
    make_delta1(delta1, pat, patlen);
    make_delta2(delta2, pat, patlen);

    const byte *result = scan_text(text, textlen, pat, patlen, delta1, delta2); 
 
    free(delta2);
    return result;
}

vector<const byte *> boyer_moore_all(const byte *text, size_t textlen, const byte *pat, size_t patlen, size_t min_gap) {
  
    size_t delta1[ALPHABET_LEN];
    size_t *delta2 = (size_t *)malloc(patlen * sizeof(size_t));
    make_delta1(delta1, pat, patlen);
    make_delta2(delta2, pat, patlen);

    list<const byte *> matches;

    const byte *end = text + textlen;
    const byte *p = text;
    while (p + patlen <= end) {
        const byte *m = scan_text(p, end - p, pat, patlen, delta1, delta2);
        if (!m) {
            break;
        }
        matches.push_back(m);
        // Skip to end of match, always skip at least min_grap
        p = max(p + min_gap, m + patlen); 
    }
 
    free(delta2);
    return vector<const byte *>(matches.begin(), matches.end());
}


