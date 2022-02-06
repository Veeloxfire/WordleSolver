#pragma once
#include <stdint.h>

#define WORD_LEN 5

#define STR2(a) #a
#define STR(a) STR2(a)

using u64 = uint64_t;
using u32 = uint32_t;

struct WordN {
  char characters[WORD_LEN];
};

void solve_wordle_cli_safe(WordN* words, u32 num_words);
void solve_wordle_cli(WordN* words, u32 num_words);