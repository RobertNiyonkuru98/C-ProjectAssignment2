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