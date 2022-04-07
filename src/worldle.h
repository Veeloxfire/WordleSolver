#pragma once
#include <stdint.h>

#define WORD_LEN 5

#define STR2(a) #a
#define STR(a) STR2(a)

using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8 = uint8_t;
using f32 = float;

constexpr u64 round_to_int_size(u64 u) {
  if (u == 0) {
    return 0;
  }
  else if (u <= UINT8_MAX) {
    return UINT8_MAX;
  }
  else if (u <= UINT16_MAX) {
    return UINT16_MAX;
  }
  else if (u <= UINT32_MAX) {
    return UINT32_MAX;
  }
  else if (u <= UINT64_MAX) {
    return UINT64_MAX;
  }
  else {
    return 0;
  }
}

template<u64 SIZE>
struct MIN_UINT_SIZE;

template<>
struct MIN_UINT_SIZE<UINT8_MAX> {
  using TY = u8;
};

template<>
struct MIN_UINT_SIZE<UINT16_MAX> {
  using TY = u16;
};

template<>
struct MIN_UINT_SIZE<UINT32_MAX> {
  using TY = u32;
};

template<>
struct MIN_UINT_SIZE<UINT64_MAX> {
  using TY = u64;
};


#define BITS_PER_OPTION (2)

static constexpr u64 BITS_FOR_TOTAL_PATTERN = (u64)WORD_LEN * (u64)BITS_PER_OPTION;
static_assert(BITS_FOR_TOTAL_PATTERN < 64, "Cannot store the pattern in an unsigned integer if shift by 64 (or more)");
static constexpr u64 NUM_PATTERNS = (1llu << BITS_FOR_TOTAL_PATTERN);

using uPATTERN = MIN_UINT_SIZE<round_to_int_size(NUM_PATTERNS)>::TY;

constexpr uPATTERN PAT_mask = (uPATTERN)0b11u;
constexpr uPATTERN PAT_g = (uPATTERN)0b10u;
constexpr uPATTERN PAT_y = (uPATTERN)0b01u;
constexpr uPATTERN PAT_r = 0;

constexpr uPATTERN _solve_pattern() {
  uPATTERN p = 0;

  for (u32 i = 0; i < WORD_LEN; i++) {
    p |= static_cast<uPATTERN>(static_cast<u32>(PAT_g) << (i * 2u));
  }

  return p;
}
constexpr uPATTERN SOLVED_PATTERN = _solve_pattern();

struct WordN {
  char characters[WORD_LEN];
};

struct AlphaC {
  u32 counts[26];
};

struct Options {
  bool can_be[26];
};

struct AvailableWords {
  bool solved;
  u32* valid_word_indexes;
  u32 num_valid_words;
};

struct AllWords {
  u32 total_words;

  const uPATTERN* patterns;
  const WordN* all_words;
};

typedef u32 (*MakeGuessFn) (const AllWords* state, const AvailableWords* available);
typedef u32 (*MakeGuessNFn) (const AllWords* state, const AvailableWords* available, u32 n_available);

constexpr u64 patterns_needed(u32 num_words) {
  return num_words * num_words;
}

uPATTERN single_pattern(const WordN* actual, const WordN* guess);
uPATTERN pattern_from_result(const char* res);

AllWords load_patterns(uPATTERN* pt, const WordN* all_words, u32 total_words);
AvailableWords load_available_words(u32* indexes, u32 total_words);

u32 make_guess_entropy(const AllWords* state, const AvailableWords* available);
u32 make_guess_min_max(const AllWords* state, const AvailableWords* available);
u32 make_guess_n_entropy(const AllWords* state, const AvailableWords* available, u32 n_available);
u32 make_guess_n_min_max(const AllWords* state, const AvailableWords* available, u32 n_available);

void update_available(const AllWords* all_words, AvailableWords* state, u32 guess_index, uPATTERN);