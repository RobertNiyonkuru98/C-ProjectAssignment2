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

int read_line(char *buf, int size) {
  int len, ch, overflow = 0;

  if (fgets(buf, size, stdin) == NULL)
    return buf[0] = '\0', 0;
  len = strlen(buf);
  if (len > 0 && buf[len - 1] == '\n')
    return buf[len - 1] = '\0', 1;
  while ((ch = getchar()) != '\n' && ch != EOF)
    overflow = 1;
  return overflow ? -1 : 1;
}

void load_text(Text *t) {
  char line[512];
  int status;

  printf("\n  Paste or type the paragraph. Finish with a line containing only "
         "a dot (.)\n");
  t->text_len = 0;
  t->text[0] = '\0';

  for (;;) {
    printf("  > ");
    status = read_line(line, 512);
    if (status == 0)
      break;
    if (status == -1) {
      printf("  That line was too long and was skipped.\n");
      continue;
    }
    if (strcmp(line, ".") == 0)
      break;
    if (!append_text(t, line) || !append_text(t, " "))
      return;
  }

  build_stats(t);
  printf("  Text loaded: %d characters, %d words, %d distinct words.\n",
         t->text_len, t->total_words, t->distinct);
}

void count_words(Text *t) {
  if (t->total_words == 0) {
    printf("\n  The text has no words.\n");
    return;
  }
  printf("\n  Total words: %d\n", t->total_words);
  printf("  Characters in the text: %d\n", t->text_len);
  if (t->truncated_words > 0)
    printf("  Words longer than %d characters were shortened: %d\n", 40 - 1,
           t->truncated_words);
}

void longest_word(Text *t) {
  int i, best = 0, best_len = 0;

  if (t->distinct == 0) {
    printf("\n  The text has no words.\n");
    return;
  }
  for (i = 0; i < t->distinct; i++) {
    int len = (int)strlen(t->stats[i].name);
    if (len > best_len) {
      best_len = len;
      best = i;
    }
  }
  printf("\n  Longest word: \"%s\" (%d characters)\n", t->stats[best].name,
         best_len);
  if (best_len >= 40 - 1)
    printf("  Note: this word may have been shortened to fit the %d "
           "character field.\n",
           40 - 1);
}

void most_frequent(Text *t) {
  int i, best = 0;

  if (t->distinct == 0) {
    printf("\n  The text has no words.\n");
    return;
  }
  for (i = 1; i < t->distinct; i++)
    if (t->stats[i].count > t->stats[best].count)
      best = i;
  printf("\n  Most frequent word: \"%s\" used %d times.\n", t->stats[best].name,
         t->stats[best].count);
}

void display_word_statistics(Text *t) {
  const wordStat *p;

  if (t->distinct == 0) {
    printf("\n  There are no words to show.\n");
    return;
  }
  printf("\n  --- Word statistics (%d distinct words) ---\n", t->distinct);
  printf("  %-30s %s\n  %-30s %s\n", "Word", "Count",
         "------------------------------", "-----");
  for (p = t->stats; p < t->stats + t->distinct; p++)
    printf("  %-30s %5d\n", p->name, p->count);
}

int cmp_alpha(const void *a, const void *b) {
  return strcmp(((const wordStat *)a)->name, ((const wordStat *)b)->name);
}

int cmp_count_desc(const void *a, const void *b) {
  const wordStat *sa = a, *sb = b;
  int d = sb->count - sa->count;
  return d != 0 ? d : strcmp(sa->name, sb->name);
}