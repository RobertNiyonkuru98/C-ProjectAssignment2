#include <stdio.h>
#include <stdlib.h>

typedef struct {
  int id;
  char name[48];
  char course[40];
  int grade;
} Student;

typedef struct {
  Student *items;
  int count;
  int capacity;
} StudentList;

typedef void (*Operation)(StudentList *);

int ensure_capacity(StudentList *list, int needed) {
  int new_capacity;
  int old_capacity;
  Student *grown;

  if (needed <= list->capacity) {
    return 1;
  }

  new_capacity = list->capacity;
  while (new_capacity < needed) {
    new_capacity = new_capacity * 2;
  }

  grown = realloc(list->items, new_capacity * sizeof(Student));
  if (grown == NULL) {
    printf("Memory realloc to %d records failed - operation cancelled\n",
           new_capacity);
    return 0;
  }

  old_capacity = list->capacity;
  list->items = grown;
  list->capacity = new_capacity;
  printf("Memory capacity %d -> %d (grew)\n", old_capacity, new_capacity);
  return 1;
}

void shrink_if_parse(StudentList *list) {
  int new_capacity;
  int old_capacity;
  Student *shrunk;

  if (list->capacity <= 10) {
    return;
  }
  if (list->count > list->capacity / 4) {
    return;
  }

  new_capacity = list->capacity / 2;
  if (new_capacity < 10) {
    new_capacity = 10;
  }

  shrunk = realloc(list->items, new_capacity * sizeof(Student));
  if (shrunk == NULL) {
    return;
  }

  old_capacity = list->capacity;
  list->items = shrunk;
  list->capacity = new_capacity;
  printf("  [mem] capacity %d -> %d (shrunk)\n", old_capacity, new_capacity);
}

int find_index_by_id(StudentList *list, int id) {
  Student *p;

  for (p = list->items; p < list->items + list->count; p++) {
    if (p->id == id) {
      return (int)(p - list->items);
    }
  }
  return -1;
}

double list_average(const StudentList *list) {
  const Student *p;
  double total = 0.0;

  if (list->count == 0) {
    return 0.0;
  }
  for (p = list->items; p < list->items + list->count; p++) {
    total = total + (double)p->grade;
  }
  return total / (double)list->count;
}