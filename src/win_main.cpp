#include <Windows.h>
#include <stdio.h>
#include "worldle.h"

#include <stdlib.h>

struct FileData {
  const char* data;
  u64 size;
};

FileData load_file_to_string(const char* path) {
  HANDLE in_file = CreateFileA(path, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
  if (in_file == INVALID_HANDLE_VALUE) {
    return {
      0, 0
    };
  }

  LARGE_INTEGER la ={};
  GetFileSizeEx(in_file, &la);

  char* buffer = new char[la.QuadPart + 1]{ 0 };

  ReadFile(in_file, buffer, la.QuadPart, 0, 0);

  CloseHandle(in_file);

  return { buffer, (u64)la.QuadPart };
}

void free_file(FileData* f) {
  delete[] f->data;
  f->data = nullptr;
  f->size = 0;
}


WordN* create_N_word_list(const char* inpath, u32* out_count) {
  FileData f = load_file_to_string(inpath);

  if (f.data == nullptr) {
    printf("%s could not be opened\nIt may not be a valid path or it may be being used by another process", inpath);
    return nullptr;
  }

  const char* buffer = f.data;

  u64 itr = 0;
  char c = buffer[itr];

  while (true) {
    u64 base= itr;
    while (c != '\r' && c != '\n' && c != '\0') {
      itr++;
      c = buffer[itr];
    }

    if (itr - base == WORD_LEN) {
      *out_count += 1;
    }

    while (c == '\r' || c == '\n') {
      itr++;
      c = buffer[itr];
    }

    if (c == '\0') {
      break;
    }
  }

  u32 count = 0;
  WordN* words = new WordN[*out_count];

  buffer = f.data;

  itr = 0;
  c = buffer[itr];

  while (true) {
    u64 base= itr;
    while (c != '\r' && c != '\n' && c != '\0') {
      itr++;
      c = buffer[itr];
    }

    if (itr - base == WORD_LEN) {
      memcpy(words[count].characters, buffer + base, WORD_LEN);
      count++;
    }

    while (c == '\r' || c == '\n') {
      itr++;
      c = buffer[itr];
    }

    if (c == '\0') {
      break;
    }
  }

  free_file(&f);
  return words;
}

void solve_wordle_cli(WordleState* state) {
  printf("Wordle Solver Started\nThis solver is compiled for " STR(WORD_LEN) " letters long words\nThe solver will provide you with a guess and you must input the response to each letter of the guess\n\n"
         "g: green (correct). y: yellow (somewhere in the word). r: grey/red (not in the word)\n\n");
  printf("For example:\nIf the solver guesses GREAT and the word is GEARS you would have to input gyyyr\nG is correct, REA are in the word but wrong positions, T is not in the word\n\n");

  while (true) {
    if (state->num_words == 0) {
      printf("ERROR: No valid words");
      return;
    }
    else if (state->num_words == 1) {
      printf("%." STR(WORD_LEN) "s    is the only remaining valid word\n", state->words[0].characters);
      return;
    }

    WordN guess = make_guess(state);

  TRY_READ_AGAIN:

    printf("%." STR(WORD_LEN) "s    Num valid words: %u\n", guess.characters, state->num_words);

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

    update_state(state, &guess, res);
  }
}

void tests(const WordN* words, u32 num_words) {
  WordN* copy = new WordN[num_words];


  float average_guess = 0.0f;

  for (u32 i = 0; i < 1000; i++) {
    //TODO: better random
    WordN to_guess = words[rand() % num_words];

    for (u32 i = 0; i < num_words; i++) {
      copy[i] = words[i];
    }

    WordleState wordle = init_wordle(copy, num_words);

    u32 num_guesses = 0;

    while (true) {
      num_guesses += 1;

      if (wordle.num_words == 0) {
        printf("ERROR: No words matched ... to_guess = %." STR(WORD_LEN) "s\n", to_guess.characters);
        break;
      }
      else if (wordle.num_words == 1) {

        if (memcmp(to_guess.characters, wordle.words[0].characters, 5) != 0) {
          printf("ERROR: SOLUTION DIDNT MATCH!!!!!!! to_guess = %." STR(WORD_LEN) "s     guess = %." STR(WORD_LEN) "s\n", to_guess.characters, wordle.words[0].characters);
          break;
        }

        printf("%." STR(WORD_LEN) "s    %u guesses\n", wordle.words[0].characters, num_guesses);
        break;
      }

      WordN guess = make_guess(&wordle);

      char res[WORD_LEN] ={};

      //Greens and reds
      for (u32 u = 0; u < WORD_LEN; u++) {
        if (to_guess.characters[u] == guess.characters[u]) {
          res[u] = 'g';
        }
        else {
          res[u] = 'r';
        }
      }

      //Needed for stopping duplicate matching
      //Maps to to_guess
      //true if has already been matched
      //everything starts false as nothing has been matched
      bool res_g[WORD_LEN] ={};
      
      //Yellows
      for (u32 u1 = 0; u1 < WORD_LEN; u1++) {
        for (u32 u2 = 0; u2 < WORD_LEN; u2++) {
          if (!res_g[u1] && res[u2] == 'r' && to_guess.characters[u1] == guess.characters[u2]) {
            res_g[u1] = true;
            res[u2] = 'y';
            goto NEXT_LETTER;
          }
        }
      NEXT_LETTER:
        continue;
      }

      update_state(&wordle, &guess, res);
    }

    //update the average guesses

    average_guess = average_guess + (((float)num_guesses - average_guess) / (float)(i + 1));
  }

  printf("Average guess tries = %f", (double)average_guess);

  delete[] copy;
}

void syntax_error(const char** argv) {
  printf("Invalid number of arguments\nSyntax: %s <word list path> or %s -tests <word list path>", argv[0], argv[0]);
}

int main(int argc, const char** argv) {
  if (argc == 2) {
    u32 num_words = 0;
    WordN* words = create_N_word_list(argv[1], &num_words);
    if (words == nullptr) {
      return 0;
    }

    WordleState wordle = init_wordle(words, num_words);
    solve_wordle_cli(&wordle);
  }
  else if (argc == 3) {
    if (strcmp(argv[1], "-tests") == 0) {
      u32 num_words = 0;
      WordN* words = create_N_word_list(argv[2], &num_words);
      if (words == nullptr) {
        return 0;
      }

      tests(words, num_words);
    }
    else {
      syntax_error(argv);
      return 0;      
    }
  }
  else {
    syntax_error(argv);
    return 0;
  }
}