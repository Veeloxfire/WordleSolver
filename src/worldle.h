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

struct AlphaC {
  u32 counts[26];
};

struct Options {
  bool can_be[26];
};

struct WordleState {
  char fixed[WORD_LEN] ={ 0 };

  u32 must_inc_count = 0;
  char must_include[WORD_LEN] ={ 0 };
  u32 must_include_count[WORD_LEN] ={ 0 };
  Options options[WORD_LEN] ={ 0 };

  WordN* words;
  u32 num_words;
  u32 total_words_start;
};

WordleState init_wordle(WordN* words, u32 num_words);

WordN make_guess(const WordleState* state);
void update_state(WordleState* state, WordN* guess, char* res);