#include <Windows.h>
#include <stdio.h>
#include "worldle.h"

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


int main(int argc, const char** argv) {
  if (argc != 2) {
    printf("Invalid number of arguments (%d)\nSyntax: %s <word list path>", argc, argv[0]);
    return 0;
  }

  u32 num_words = 0;
  WordN* words = create_N_word_list(argv[1], &num_words);

  if (words == nullptr) {
    return 0;
  }

  solve_wordle_cli(words, num_words);
  return 0;
}