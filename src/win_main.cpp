#include <Windows.h>
#include <stdio.h>
#include "worldle.h"

#include <assert.h>

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

void solve_wordle_cli(MakeGuessFn guess_fn, AllWords* all_words, AvailableWords* available) {
  printf("Wordle Solver Started\nThis solver is compiled for " STR(WORD_LEN) " letters long words\nThe solver will provide you with a guess and you must input the response to each letter of the guess\n\n"
         "g: green (correct). y: yellow (somewhere in the word). r: grey/red (not in the word)\n\n");
  printf("For example:\nIf the solver guesses GREAT and the word is GEARS you would have to input gyyyr\nG is correct, REA are in the word but wrong positions, T is not in the word\n\n");

  while (true) {
    if (available->num_valid_words == 0) {
      printf("ERROR: No valid words");
      return;
    }
    else if (available->num_valid_words == 1) {
      printf("%." STR(WORD_LEN) "s    is the only remaining valid word\n", all_words->all_words[available->valid_word_indexes[0]].characters);
      return;
    }

    u32 guess_index = guess_fn(all_words, available);
    WordN guess = all_words->all_words[guess_index];

  TRY_READ_AGAIN:

    printf("%." STR(WORD_LEN) "s    Num valid words: %u\n", guess.characters, available->num_valid_words);

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

    uPATTERN p = pattern_from_result(res);
    update_available(all_words, available, guess_index, p);
  }
}

void solve_wordle_cli_n(MakeGuessNFn guess_n_fn, AllWords* all_words, AvailableWords* available, u32 n_available) {
  printf("Wordle Solver Started\nThis solver is compiled for " STR(WORD_LEN) " letters long words\nThe solver will provide you with a guess and you must input the response to each letter of the guess\n\n"
         "g: green (correct). y: yellow (somewhere in the word). r: grey/red (not in the word)\n\n");
  printf("For example:\nIf the solver guesses GREAT and the word is GEARS you would have to input gyyyr\nG is correct, REA are in the word but wrong positions, T is not in the word\n\n");

  while (true) {
    u32 n_solved = 0;

    for (u32 i = 0; i < n_available; i++) {
      if (available[i].num_valid_words == 0) {
        printf("Wordle %u ERROR: No valid words", i);
        return;
      }
      
      n_solved += (int)available[i].solved;
    }

    if (n_solved == n_available) {
      printf("\nSolved all\n"); return;
    }

    u32 guess_index = guess_n_fn(all_words, available, n_available);
    WordN guess = all_words->all_words[guess_index];

    for (u32 i = 0; i < n_available; i++) {
      if (!available[i].solved) {
        printf("%." STR(WORD_LEN) "s ", guess.characters);
      }
    }

    for (u32 i = 0; i < n_available; i++) {
      if (!available[i].solved) {
        printf("%u ", available[i].num_valid_words);
      }
    }

    printf("\n");

    char res[WORD_LEN] ={ 0 };

    char sp = '\0';

    for (u32 i = 0; i < n_available; i++) {
      if (!available[i].solved) {
        if (sp != '\0') {
          assert(sp == ' ');
        }

        scanf_s("%" STR(WORD_LEN) "c%c", res, WORD_LEN, &sp, 1);

        uPATTERN p = pattern_from_result(res);
        update_available(all_words, available + i, guess_index, p);
      }
    }

    assert(sp == '\n');
  }
}


struct WordleTestRes {
  u32 num_failed;
  f32 average_guess;
};

WordleTestRes wordle_test(MakeGuessFn guess_fn, const AllWords* words, u32 start_index, u32 end_index) {
  u32* indexes = new u32[words->total_words];

  u32 num_failed = 0;
  f32 average_guess = 0.0f;

  for (u32 i = start_index; i < end_index; i++) {
    const WordN* actual = words->all_words + i;
    u32 num_guesses = 0;

    AvailableWords available = load_available_words(indexes, words->total_words);


    while (true) {
      num_guesses += 1;

      if (num_guesses > 20) {
        printf("... ERROR: was on guess 20 (should be done by now)");
        num_failed += 1;
        break;
      }

      if (available.num_valid_words == 0) {
        printf("... ERROR: No words matched\n");
        num_failed += 1;
        break;
      }
      else if (available.num_valid_words == 1) {
        //Either error or win

        if (memcmp(actual->characters, words->all_words[available.valid_word_indexes[0]].characters, WORD_LEN) == 0) {
          printf("%." STR(WORD_LEN) "s %u guesses\n", actual->characters, num_guesses);

          //Update average guess
          average_guess = average_guess + (((float)num_guesses - average_guess) / (float)(i + 1));
        }
        else {
          printf("... ERROR: Solution Didnt Match!   guess = %." STR(WORD_LEN) "s\n", words->all_words[available.valid_word_indexes[0]].characters);
          num_failed += 1;
        }

        break;
      }

      u32 guess_index = guess_fn(words, &available);

      uPATTERN p = single_pattern(actual, words->all_words + guess_index);

      update_available(words, &available, guess_index, p);
    }
  }

  delete[] indexes;

  WordleTestRes res ={};
  res.average_guess = average_guess;
  res.num_failed = num_failed;

  return res;
}

struct WordleTestThreadData {
  WordleTestRes thread_out;

  const AllWords* words;
  u32 start_index;
  u32 end_index;
  MakeGuessFn guess_fn;
};

DWORD WINAPI test_single_thread(void* data) {
  WordleTestThreadData* ti = (WordleTestThreadData*)data;
  const AllWords* words = ti->words;
  u32 end_index = ti->end_index;

  ti->thread_out = wordle_test(ti->guess_fn, words, ti->start_index, ti->end_index);
  return 0;
}

void tests(MakeGuessFn guess_fn, const WordN* words, const u32 num_words, u32 num_extra_threads) {
  uPATTERN* patterns = new uPATTERN[patterns_needed(num_words)];

  printf("Loading patterns ...\n");
  AllWords all_words = load_patterns(patterns, words, num_words);

  printf("Check these patterns match manually:\n");

  //Test matching
  for (u32 i = 0; i < 10; i++) {
    u32 acc = rand() % num_words;
    u32 guess = rand() % num_words;

    uPATTERN p = patterns[guess * num_words + acc];

    char res[WORD_LEN] ={};
    for (u32 j = 0; j < WORD_LEN; j++) {
      switch ((p >> (2 * j)) & PAT_mask) {
        case PAT_r: res[j] = 'r'; break;
        case PAT_y: res[j] = 'y'; break;
        case PAT_g: res[j] = 'g'; break;
        default: assert(false);
      }
    }

    printf(" - Actual: %." STR(WORD_LEN) "s, Guess: %." STR(WORD_LEN) "s, Res: %." STR(WORD_LEN) "s\n", words[acc].characters, words[guess].characters, res);
  }

  WordleTestThreadData* test_data = new WordleTestThreadData[num_extra_threads];
  HANDLE* handles = new HANDLE[num_extra_threads];

  u32 num_per_thread = num_words / (num_extra_threads + 1);
  u32 current_start = 0;

  for (u32 i = 0; i < num_extra_threads; i++) {
    WordleTestThreadData* td = test_data + i;
    td->start_index = current_start;
    current_start += num_per_thread;

    td->end_index = current_start;
    td->words = &all_words;
    td->guess_fn = guess_fn;

    handles[i] = CreateThread(0, 0, test_single_thread, td, 0, 0);
  }

  WordleTestRes local_res = wordle_test(guess_fn, &all_words, current_start, num_words);

  if (num_extra_threads > 0) {
    WaitForMultipleObjects(num_extra_threads, handles, TRUE, INFINITE);
  }

  f32 average_guess = local_res.average_guess / (f32)(num_extra_threads + 1);
  u32 num_failed = local_res.num_failed;

  for (u32 i = 0; i < num_extra_threads; i++) {
    average_guess += test_data[i].thread_out.average_guess / (f32)(num_extra_threads + 1);
    num_failed += test_data[i].thread_out.num_failed;
  }

  printf("Average guess tries = %f\nFailed: %u", (double)average_guess, num_failed);

  delete[] patterns;
}

void syntax_error(const char** argv) {
  printf("Invalid number of arguments\nSyntax: %s <word list path> or %s -tests <word list path>", argv[0], argv[0]);
}

MakeGuessFn pick_guess_fn() {
  printf("Please select a guess function to use\n1. Entropy\n2. Min Max\nPlease Enter the number of your choice: ");
  u32 i = 0;
  char nl = '\0';
  scanf_s("%u%c", &i, &nl, 1);

  assert(nl == '\n');

  switch (i) {
    case 1: return make_guess_entropy;
    case 2: return make_guess_min_max;
    default: printf("Invalid selection");  return NULL;
  }
}

MakeGuessNFn pick_guess_n_fn() {
  printf("Please select a guess function to use\n1. Entropy\n2. Min Max\nPlease Enter the number of your choice: ");
  u32 i = 0;
  char nl = '\0';
  scanf_s("%u%c", &i, &nl, 1);

  assert(nl == '\n');

  switch (i) {
    case 1: return make_guess_n_entropy;
    case 2: return make_guess_n_min_max;
    default: printf("Invalid selection");  return NULL;
  }
}

int main(int argc, const char** argv) {
  if (argc == 2) {
    MakeGuessFn fn = pick_guess_fn();
    if (fn == NULL) {
      return 0;
    }

    u32 num_words = 0;
    WordN* words = create_N_word_list(argv[1], &num_words);
    if (words == nullptr) {
      return 0;
    }

    u32* indexes = new u32[num_words];

    uPATTERN* patterns = new uPATTERN[patterns_needed(num_words)];

    printf("Loading patterns ...\n");
    AllWords all_worlds = load_patterns(patterns, words, num_words);

    AvailableWords available = load_available_words(indexes, num_words);

    solve_wordle_cli(fn, &all_worlds, &available);
  }
  else if (argc == 3) {
    if (strcmp(argv[1], "-tests") == 0) {
      MakeGuessFn fn = pick_guess_fn();
      if (fn == NULL) {
        return 0;
      }

      u32 num_words = 0;
      WordN* words = create_N_word_list(argv[2], &num_words);
      if (words == nullptr) {
        return 0;
      }

      tests(fn, words, num_words, 3);
    }
    else if (strcmp(argv[1], "-n") == 0) {
      printf("How many wordles? ");
      u32 n_wordles = 0;
      {
        char nl;
        scanf_s("%u%c", &n_wordles, &nl, 1);
        assert(nl == '\n');
      }

      MakeGuessNFn fn = pick_guess_n_fn();

      u32 num_words = 0;
      WordN* words = create_N_word_list(argv[2], &num_words);
      if (words == nullptr) {
        return 0;
      }

      u32* indexes = new u32[num_words * n_wordles];

      uPATTERN* patterns = new uPATTERN[patterns_needed(num_words)];

      printf("Loading patterns ...\n");
      AllWords all_worlds = load_patterns(patterns, words, num_words);

      AvailableWords* available = new AvailableWords[n_wordles];

      for (u32 i = 0; i < n_wordles; i++) {
        available[i] = load_available_words(indexes + (num_words * i), num_words);
      }

      solve_wordle_cli_n(fn, &all_worlds, available, n_wordles);
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