#include <assert.h>
#include <cmath>

#include "worldle.h"


char most_common_letter(const AlphaC& a) {
  u32 most_common_count = a.counts[0];
  char most_common_0 = 'A';
  for (u32 i = 1; i < 26; i++) {
    if (a.counts[i] > most_common_count) {
      most_common_count = a.counts[i];
      most_common_0 = 'A' + i;
    }
  }

  return most_common_0;
}

inline bool valid_option(const Options& opt, char c) {
  return opt.can_be[c - 'A'];
}

f32 entropy_word_score(const uPATTERN* pts, const u32* valid_words, u32 num_valid_words, u32 total_words, u32 guess_index) {
  u32 pattern_count[NUM_PATTERNS] ={};

  u32 total_count = (u32)patterns_needed(total_words);

  for (u32 i = 0; i < num_valid_words; i++) {
    uPATTERN p = pts[guess_index * total_words + valid_words[i]];

    //Pattern is actually the key! Wow
    pattern_count[p] += 1;
  }

  f32 res = 0;

  //All patterns checked
  for (u32 i = 0; i < NUM_PATTERNS; i++) {
    if (pattern_count[i] != 0) {
      f32 prob = (f32)pattern_count[i] / (f32)num_valid_words;

      res += prob * (-log2f(prob));
    }
  }

  return res;
}

u32 min_pattern_count(const uPATTERN* pts, const u32* valid_words, u32 num_valid_words, u32 total_words, u32 guess_index) {
  u32 pattern_count[NUM_PATTERNS] ={};

  u32 total_count = (u32)patterns_needed(total_words);

  for (u32 i = 0; i < num_valid_words; i++) {
    uPATTERN p = pts[guess_index * total_words + valid_words[i]];

    //Pattern is actually the key! Wow
    pattern_count[p] += 1;
  }

  u32 max = 0;

  //All patterns checked
  for (u32 i = 0; i < NUM_PATTERNS; i++) {
    u32 test_max = pattern_count[i];

    if (test_max > max) {
      max = test_max;
    }
  }

  return max;
}

uPATTERN pattern_from_result(const char* res) {
  uPATTERN p = 0;

  for (u32 i = 0; i < WORD_LEN; i++) {
    switch (res[i]) {
      case 'g': p |= (PAT_g << (i * 2u)); break;
      case 'y': p |= (PAT_y << (i * 2u)); break;
      case 'r': p |= (PAT_r << (i * 2u)); break;
      default: assert(false);//error
    }
  }

  return p;
}

uPATTERN single_pattern(const WordN* const actual, const WordN* const guess) {
  //everything starts false as nothing has been matched
  bool matched[WORD_LEN] ={};

  //The resulting characters
  uPATTERN pattern = 0;

  //Greens and reds
  for (u32 u = 0; u < WORD_LEN; u++) {
    if (actual->characters[u] == guess->characters[u]) {
      pattern |= PAT_g << (u * 2u);
      matched[u] = true;
    }
  }

  //Yellows
  for (u32 i = 0; i < WORD_LEN; i++) {
    if (((pattern >> (i * 2)) & PAT_mask) == PAT_r) {
      for (u32 j = 0; j < WORD_LEN; j++) {
        if (!matched[j] && actual->characters[j] == guess->characters[i]) {
          matched[j] = true;
          pattern |= PAT_y << (i * 2u);
          goto NEXT_LETTER;
        }
      }

    NEXT_LETTER:
      continue;
    }
  }

  return pattern;
}

AllWords load_patterns(uPATTERN* pt, const WordN* all_words, u32 num_words) {
  assert(pt != nullptr);

  u64 pattern_index = 0;

  for (u32 j = 0; j < num_words; j++) {
    const WordN* guess = all_words + j;

    for (u32 i = 0; i < num_words; i++) {
      const WordN* actual = all_words + i;

      uPATTERN pattern = 0;

      pt[pattern_index++] = single_pattern(actual, guess);
    }
  }

  assert(pattern_index == patterns_needed(num_words));

  AllWords words ={};
  words.patterns = pt;
  words.all_words = all_words;
  words.total_words = num_words;

  return words;
}

u32 add_include(char* includes, u32* counts, u32 num_includes, char c) {
  for (u32 u = 0; u < num_includes; u++) {
    if (includes[u] == c) {
      counts[u] += 1;
      return num_includes;
    }
  }

  includes[num_includes] = c;
  counts[num_includes] = 1;
  return num_includes + 1;
}

AvailableWords load_available_words(u32* indexes, u32 total_words) {
  for (u32 i = 0; i < total_words; i++) {
    indexes[i] = i;
  }

  AvailableWords words ={};
  words.valid_word_indexes = indexes;
  words.num_valid_words = total_words;
  words.solved = false;

  return words;
}

u32 make_guess_entropy(const AllWords* state, const AvailableWords* available) {
  u32 best_guess = 0;
  f32 best_score = entropy_word_score(state->patterns, available->valid_word_indexes, available->num_valid_words, state->total_words, 0);

  for (u32 i = 1; i < state->total_words; i++) {
    f32 common = entropy_word_score(state->patterns, available->valid_word_indexes, available->num_valid_words, state->total_words, i);
    if (common > best_score) {
      best_guess = i;
      best_score = common;
    }
  }

  return best_guess;
}

u32 make_guess_min_max(const AllWords* state, const AvailableWords* available) {
  u32 best_guess = 0;
  u32 min_count = min_pattern_count(state->patterns, available->valid_word_indexes, available->num_valid_words, state->total_words, 0);

  for (u32 i = 1; i < state->total_words; i++) {
    u32 common = min_pattern_count(state->patterns, available->valid_word_indexes, available->num_valid_words, state->total_words, i);
    if (common < min_count) {
      best_guess = i;
      min_count = common;
    }
  }

  return best_guess;
}

f32 n_score_entropy(const AllWords* state, const AvailableWords* available, u32 n_available, u32 index) {
  f32 sc = 0;
  for (u32 i = 0; i < n_available; i++) {
    if (!available[i].solved) {
      sc += entropy_word_score(state->patterns, available[i].valid_word_indexes, available[i].num_valid_words, state->total_words, index);
    }
  }

  return sc;
}

u32 make_guess_n_entropy(const AllWords* state, const AvailableWords* available, u32 n_available) {
  u32 best_guess = 0;

  //If we can solve, we will solve
  for (u32 i = 0; i < n_available; i++) {
    if (!available[i].solved && available[i].num_valid_words == 1) {
      return available[i].valid_word_indexes[0];
    }
  }

  f32 best_score = n_score_entropy(state, available, n_available, 0);

  for (u32 i = 1; i < state->total_words; i++) {
    f32 common = n_score_entropy(state, available, n_available, i);
    if (common > best_score) {
      best_guess = i;
      best_score = common;
    }
  }

  return best_guess;
}

u32 n_min_count(const AllWords* state, const AvailableWords* available, u32 n_available, u32 index) {
  u32 count = 0;
  for (u32 i = 0; i < n_available; i++) {
    if (!available[i].solved) {
      count += min_pattern_count(state->patterns, available[i].valid_word_indexes, available[i].num_valid_words, state->total_words, index);
    }
  }

  return count;
}

u32 make_guess_n_min_max(const AllWords* state, const AvailableWords* available, u32 n_available) {
  u32 best_guess = 0;

  //If we can solve, we will solve
  for (u32 i = 0; i < n_available; i++) {
    if (!available[i].solved && available[i].num_valid_words == 1) {
      return available[i].valid_word_indexes[0];
    }
  }

  u32 min_count = n_min_count(state, available, n_available, 0);

  for (u32 i = 1; i < state->total_words; i++) {
    u32 c = n_min_count(state, available, n_available, i);
    if (c < min_count) {
      best_guess = i;
      min_count = c;
    }
  }

  return best_guess;
}

void update_available(const AllWords* all_words, AvailableWords* available, u32 guess_index, uPATTERN pattern) {
  u32 max = available->num_valid_words;

  if (available->solved) {
    return;
  }

  if (pattern == SOLVED_PATTERN) {
    //solved yay
    available->solved = true;
    return;
  }

  //Filter words - only keep if the patterns match
  u32 base = 0;
  for (u32 i = 0; i < max; i++) {
    if (all_words->patterns[guess_index * all_words->total_words + available->valid_word_indexes[i]] == pattern) {
      //Keep
      available->valid_word_indexes[base] = available->valid_word_indexes[i];
      base++;
    }
  }

  assert(base != 0);
  available->num_valid_words = base;
}

