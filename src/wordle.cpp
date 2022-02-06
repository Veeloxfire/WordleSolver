#include <assert.h>

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

u32 word_score(const char* fixed, const WordN* w, const AlphaC* counts) {
  u32 num_chars = 0;
  char dup[WORD_LEN] ={ 0 };

  u32 score = 0;
  for (u32 u = 0; u < WORD_LEN; u++) {
    if (fixed[u] != '\0') {
      goto NO_SCORE;
    }

    for (u32 u2 = 0; u2 < num_chars; u2++) {
      if (dup[u2] == w->characters[u]) {
        goto NO_SCORE;
      }
    }
    score += counts[u].counts[w->characters[u] - 'A'];
    dup[num_chars++] = w->characters[u];

  NO_SCORE:
    continue;
  }

  return score;
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

WordleState init_wordle(WordN* words, u32 num_words) {
  WordleState state ={ 0 };

  state.words = words;
  state.num_words = num_words;
  state.total_words_start = num_words;

  //Set all options to be true
  for (u32 opt = 0; opt < WORD_LEN; opt++) {
    for (u32 i = 0; i < 26; i++) {
      state.options[opt].can_be[i] = true;
    }
  }

  return state;
}

WordN make_guess(const WordleState* state) {
  WordN guess ={ 0 };

  AlphaC alpha_count[WORD_LEN] ={ 0 };

  assert(state->must_inc_count <= WORD_LEN);

  for (u32 i = 0; i < state->num_words; i++) {
    WordN* w = state->words + i;


    for (u32 u = 0; u < WORD_LEN; u++) {
      alpha_count[u].counts[w->characters[u] - 'A'] += 1;
    }
  }

  //Scale the scores???
  //TODO: This might no longer be needed due to change in how scores are calculated
  for (u32 i = 0; i < WORD_LEN; i++) {
    for (u32 a = 0; a < 26; a++) {
      alpha_count[i].counts[a] = (u32)((float)alpha_count[i].counts[a] * ((float)state->total_words_start / (float)state->num_words));
    }
  }


  WordN* best = state->words;
  u32 best_score = word_score(state->fixed, best, alpha_count);

  for (u32 i = 1; i < state->num_words; i++) {
    u32 common = word_score(state->fixed, state->words + i, alpha_count);
    if (common > best_score) {
      best = state->words + i;
      best_score = common;
    }
  }

  for (u32 u = 0; u < WORD_LEN; u++) {
    guess.characters[u] = best->characters[u];
  }

  return guess;
}

void update_state(WordleState* state, WordN* guess, char* res) {
  //Greens first
  for (u32 j = 0; j < WORD_LEN; j++) {
    if (res[j] == 'g') {
      //Fix this to have 1 option
      state->fixed[j] = guess->characters[j];

      //Remove from includes
      for (u32 u = 0; u < state->must_inc_count; u++) {
        if (state->must_include[u] == state->fixed[j] && state->must_include_count[u] > 0) {
          state->must_include_count[u] -= 1;
        }
      }
    }
  }

  u32 num_new_includes = 0;
  char new_includes[WORD_LEN] ={ 0 };
  u32 new_include_counts[WORD_LEN] ={ 0 };

  //Yellows second
  for (u32 j = 0; j < WORD_LEN; j++) {
    if (res[j] == 'y') {
      num_new_includes = add_include(new_includes, new_include_counts, num_new_includes, guess->characters[j]);

      state->options[j].can_be[guess->characters[j] - 'A'] = false;
    }
  }

  //Combine the includes
  {
    u32 to_add = 0;
    for (u32 u2 = 0; u2 < num_new_includes; u2++) {
      for (u32 u1 = 0; u1 < state->must_inc_count; u1++) {
        if (new_includes[u2] == state->must_include[u1]) {
          if (state->must_include_count[u1] <= new_include_counts[u2]) {
            state->must_include_count[u1] = new_include_counts[u2];
          }
          goto FOUND;
        }
      }

      state->must_include[state->must_inc_count + to_add] = new_includes[u2];
      state->must_include_count[state->must_inc_count + to_add] = new_include_counts[u2];
      to_add += 1;

    FOUND:
      continue;
    }

    state->must_inc_count += to_add;
  }

  //Finally reds
  for (u32 j = 0; j < WORD_LEN; j++) {
    if (res[j] == 'r') {
      //Definitely cant be here
      state->options[j].can_be[guess->characters[j] - 'A'] = false;


      for (u32 u = 0; u < state->must_inc_count; u++) {
        if (state->must_include[u] == guess->characters[j] && state->must_include[u] > 0) {
          //This exists somewhere and we dont know where
          goto CANNOT_REMOVE;
        }
      }

      //Can remove everywhere!
      for (u32 i = 0; i < WORD_LEN; i++) {
        state->options[i].can_be[guess->characters[j] - 'A'] = false;
      }

    CANNOT_REMOVE:
      continue;
    }
  }


  //Filter words
  u32 base = 0;
  for (u32 i = 0; i < state->num_words; i++) {
    WordN* w = state->words + i;

    {
      for (u32 u = 0; u < WORD_LEN; u++) {
        if (state->fixed[u] != '\0') {
          if (state->fixed[u] != w->characters[u]) {
            goto INVALID;
          }
        }
        else if (!valid_option(state->options[u], w->characters[u])) {
          goto INVALID;
        }
      }

      u32 include_counts[WORD_LEN] ={ 0 };

      for (u32 u1 = 0; u1 < state->must_inc_count; u1++) {
        for (u32 u2 = 0; u2 < WORD_LEN; u2++) {
          if (state->must_include[u1] == w->characters[u2]) {
            include_counts[u1]++;
            goto FOUND_INC;
          }
        }
        goto INVALID;

      FOUND_INC:
        continue;
      }

      for (u32 u = 0; u < state->must_inc_count; u++) {
        if (include_counts[u] < state->must_include_count[u]) {
          goto INVALID;
        }
      }

      //VALID
      if (base != i) {
        *(state->words + base) = *(state->words + i);
      }
      base++;
    }

  INVALID:
    continue;
  }

  state->num_words = base;
}

