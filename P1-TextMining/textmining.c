#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char name[40];
  int count;
} wordStat;

typedef struct {
  char *text;
  int text_len, text_cap;
  wordStat *stats;
  int distinct, stats_cap;
  int total_words, truncated_words;
} Text;

typedef void (*Analysis)(Text *);

int g_running = 1;

int append_text(Text *t, const char *s) {
  int need = t->text_len + (int)strlen(s) + 1;
  int new_cap;
  int old_cap;
  char *grown;

  if (need <= t->text_cap) {
    strcpy(t->text + t->text_len, s);
    t->text_len += (int)strlen(s);
    return 1;
  }
  new_cap = t->text_cap;
  while (new_cap < need)
    new_cap *= 2;
  grown = realloc(t->text, new_cap);
  if (grown == NULL) {
    printf("  [mem] could not grow the text buffer - text not added.\n");
    return 0;
  }
  old_cap = t->text_cap;
  t->text = grown;
  t->text_cap = new_cap;
  printf("  [mem] text buffer %d -> %d bytes\n", old_cap, new_cap);
  strcpy(t->text + t->text_len, s);
  t->text_len += (int)strlen(s);
  return 1;
}

int add_word(Text *t, const char *word) {
  int i;
  int new_cap;
  wordStat *grown;

  for (i = 0; i < t->distinct; i++) {
    if (strcmp(t->stats[i].name, word) == 0) {
      t->stats[i].count++;
      return 1;
    }
  }
  if (t->distinct >= t->stats_cap) {
    new_cap = t->stats_cap * 2;
    grown = realloc(t->stats, new_cap * sizeof(wordStat));
    if (grown == NULL) {
      printf("  [mem] could not grow the word list - word not counted.\n");
      return 0;
    }
    printf("  [mem] word list %d -> %d slots\n", t->stats_cap, new_cap);
    t->stats = grown;
    t->stats_cap = new_cap;
  }
  strcpy(t->stats[t->distinct].name, word);
  t->stats[t->distinct].count = 1;
  t->distinct++;
  return 1;
}

void build_stats(Text *t) {
  const char *p = t->text;
  char word[40];
  int len, truncated;

  t->distinct = 0;
  t->total_words = 0;
  t->truncated_words = 0;

  while (*p != '\0') {
    while (*p != '\0' && !isalpha((unsigned char)*p))
      p++;
    if (*p == '\0')
      break;

    len = 0;
    truncated = 0;
    while (*p != '\0') {
      if (isalpha((unsigned char)*p)) {
        if (len < 40 - 1)
          word[len++] = (char)tolower((unsigned char)*p);
        else
          truncated = 1;
        p++;
      } else if ((*p == '\'' || *p == '-') && len > 0 && p[1] != '\0' &&
                 isalpha((unsigned char)p[1])) {
        if (len < 40 - 1)
          word[len++] = *p;
        p++;
      } else {
        break;
      }
    }
    word[len] = '\0';
    t->total_words++;
    if (truncated)
      t->truncated_words++;
    if (!add_word(t, word))
      return;
  }
}