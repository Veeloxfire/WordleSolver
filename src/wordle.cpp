#include <stdio.h>
#include <assert.h>

#include "worldle.h"

struct AlphaC {
  u32 counts[26];
};

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

struct Options {
  bool can_be[26];
};

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


void solve_wordle_cli_safe(WordN* words, u32 num_words) {
  WordN* copy = new WordN[num_words];
  for (u32 i = 0; i < num_words; i++) {
    copy[i] = words[i];
  }

  solve_wordle_cli(copy, num_words);

  delete[]copy;
}

void solve_wordle_cli(WordN* words, u32 num_words) {
  printf("Wordle Solver Started\nThis solver is compiled for " STR(WORD_LEN) " letters long words\nThe solver will provide you with a guess and you must input the response to each letter of the guess\n\n"
         "g: green (correct). y: yellow (somewhere in the word). r: grey/red (not in the word)\n\n");
  printf("For example:\nIf the solver guesses GREAT and the word is GEARS you would have to input gyyyr\nG is correct, REA are in the word but wrong positions, T is not in the word\n\n");


  char fixed[WORD_LEN] ={ 0 };

  u32 must_inc_count = 0;
  char must_include[WORD_LEN] ={ 0 };
  u32 must_include_count[WORD_LEN] ={ 0 };
  Options options[WORD_LEN] ={ 0 };

  //Set all options to be true
  for (u32 opt = 0; opt < WORD_LEN; opt++) {
    for (u32 i = 0; i < 26; i++) {
      options[opt].can_be[i] = true;
    }
  }


  u32 total_words_start = num_words;

  while (true) {
    AlphaC alpha_count[WORD_LEN] ={ 0 };

    assert(must_inc_count <= WORD_LEN);

    if (num_words == 0) {
      printf("ERROR: No valid words");
      return;
    }
    else if (num_words == 1) {
      char finalv[WORD_LEN + 1] ={ 0 };

      for (u32 u = 0; u < WORD_LEN; u++) {
        finalv[u] = words[0].characters[u];
      }

      printf("%." STR(WORD_LEN) "s    is the only remaining valid word\n", finalv);
      return;
    }

    for (u32 i = 0; i < num_words; i++) {
      WordN* w = words + i;


      for (u32 u = 0; u < WORD_LEN; u++) {
        alpha_count[u].counts[w->characters[u] - 'A'] += 1;
      }
    }

    //Scale the scores???
    //TODO: This might no longer be needed due to change in how scores are calculated
    for (u32 i = 0; i < WORD_LEN; i++) {
      for (u32 a = 0; a < 26; a++) {
        alpha_count[i].counts[a] = (u32)((float)alpha_count[i].counts[a] * ((float)total_words_start / (float)num_words));
      }
    }

    char guess[WORD_LEN + 1] ={ 0 };

    {
      WordN* best = words;
      u32 best_score = word_score(fixed, best, alpha_count);

      for (u32 i = 1; i < num_words; i++) {
        u32 common = word_score(fixed, words + i, alpha_count);
        if (common > best_score) {
          best = words + i;
          best_score = common;
        }
      }

      for (u32 u = 0; u < WORD_LEN; u++) {
        guess[u] = best->characters[u];
      }
    }



  TRY_READ_AGAIN:

    printf("%." STR(WORD_LEN) "s    Num valid words: %u\n", guess, num_words);

    char res[WORD_LEN] ={ 0 };

    char nl;
    scanf_s("%" STR(WORD_LEN) "c%c", res, WORD_LEN, &nl, 1);
    if (nl != '\n') {
      printf("Invalid input length ... Try again\n\n");
      while (fgetc(stdin) != '\n') {};
      goto TRY_READ_AGAIN;
    }

    //Verify input
    for (u32 i = 0; i < WORD_LEN; i++) {
      char c = res[i];
      if (c != 'g' && c != 'y' && c != 'r') {
        printf("%c is not a valid option (of 'g', 'y' and 'r') ... Try Again\n\n", c);
        goto TRY_READ_AGAIN;
      }
    }

    {
      //Greens first
      for (u32 j = 0; j < WORD_LEN; j++) {
        if (res[j] == 'g') {
          //Fix this to have 1 option
          fixed[j] = guess[j];

          //Remove from includes
          for (u32 u = 0; u < must_inc_count; u++) {
            if (must_include[u] == fixed[j] && must_include_count[u] > 0) {
              must_include_count[u] -= 1;
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
          num_new_includes = add_include(new_includes, new_include_counts, num_new_includes, guess[j]);

          options[j].can_be[guess[j] - 'A'] = false;
        }
      }

      //Combine the includes
      {
        u32 to_add = 0;
        for (u32 u2 = 0; u2 < num_new_includes; u2++) {
          for (u32 u1 = 0; u1 < must_inc_count; u1++) {
            if (new_includes[u2] == must_include[u1]) {
              if (must_include_count[u1] <= new_include_counts[u2]) {
                must_include_count[u1] = new_include_counts[u2];
              }
              goto FOUND;
            }
          }

          must_include[must_inc_count + to_add] = new_includes[u2];
          must_include_count[must_inc_count + to_add] = new_include_counts[u2];
          to_add += 1;

        FOUND:
          continue;
        }

        must_inc_count += to_add;
      }

      //Finally reds
      for (u32 j = 0; j < WORD_LEN; j++) {
        if (res[j] == 'r') {
          //Definitely cant be here
          options[j].can_be[guess[j] - 'A'] = false;


          for (u32 u = 0; u < must_inc_count; u++) {
            if (must_include[u] == guess[j] && must_include[u] > 0) {
              //This exists somewhere and we dont know where
              goto CANNOT_REMOVE;
            }
          }

          //Can remove everywhere!
          for (u32 i = 0; i < WORD_LEN; i++) {
            options[i].can_be[guess[j] - 'A'] = false;
          }

        CANNOT_REMOVE:
          continue;
        }
      }
    }

    //Filter words
    u32 base = 0;
    for (u32 i = 0; i < num_words; i++) {
      WordN* w = words + i;

      {
        for (u32 u = 0; u < WORD_LEN; u++) {
          if (fixed[u] != '\0') {
            if (fixed[u] != w->characters[u]) {
              goto INVALID;
            }
          }
          else if (!valid_option(options[u], w->characters[u])) {
            goto INVALID;
          }
        }

        u32 include_counts[WORD_LEN] ={ 0 };

        for (u32 u1 = 0; u1 < must_inc_count; u1++) {
          for (u32 u2 = 0; u2 < WORD_LEN; u2++) {
            if (must_include[u1] == w->characters[u2]) {
              include_counts[u1]++;
              goto FOUND_INC;
            }
          }
          goto INVALID;

        FOUND_INC:
          continue;
        }

        for (u32 u = 0; u < must_inc_count; u++) {
          if (include_counts[u] < must_include_count[u]) {
            goto INVALID;
          }
        }

        //VALID
        if (base != i) {
          *(words + base) = *(words + i);
        }
        base++;
      }

    INVALID:
      continue;
    }

    num_words = base;
  }
}