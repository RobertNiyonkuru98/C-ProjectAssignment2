#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int gradeDescCompare(void *a, void *b) {
  Student *sa = (Student *)a;
  Student *sb = (Student *)b;

  if (sa->grade != sb->grade) {
    return (sa->grade < sb->grade) - (sa->grade > sb->grade);
  }
  return sa->id - sb->id;
}

int nameAscCompare(void *a, void *b) {
  Student *sa = (Student *)a;
  Student *sb = (Student *)b;
  int by_name = str_compare_insensitive(sa->name, sb->name);

  if (by_name != 0) {
    return by_name;
  }
  return sa->id - sb->id;
}

int idAscCompare(void *a, void *b) {
  Student *sa = (Student *)a;
  Student *sb = (Student *)b;
  return sa->id - sb->id;
}

int read_line(char *buf, int size) {
  int len;
  int ch;
  int overflow = 0;

  if (fgets(buf, size, stdin) == NULL) {
    buf[0] = '\0';
    return 0;
  }

  len = strlen(buf);
  if (len > 0 && buf[len - 1] == '\n') {
    buf[len - 1] = '\0';
    return 1;
  }

  while ((ch = getchar()) != '\n' && ch != EOF) {
    overflow = 1;
  }
  return overflow ? -1 : 1;
}

int read_int(const char *prompt, int *out) {
  char buf[64];
  char *end;
  long value;
  int status;

  printf("%s", prompt);
  status = read_line(buf, sizeof buf);

  if (status == 0) {
    return 0;
  }
  if (status == -1) {
    return -1;
  }

  value = strtol(buf, &end, 10);
  if (end == buf) {
    return -1;
  }
  while (*end != '\0' && isspace((unsigned char)*end)) {
    end++;
  }
  if (*end != '\0') {
    return -1;
  }

  *out = (int)value;
  return 1;
}

int is_valid_name(const char *s) {
  int i;
  int has_letter = 0;

  if (s[0] == '\0') {
    return 0;
  }
  for (i = 0; s[i] != '\0'; i++) {
    unsigned char c = (unsigned char)s[i];
    if (isalpha(c)) {
      has_letter = 1;
    } else if (c != ' ' && c != '-' && c != '\'') {
      return 0;
    }
  }
  return has_letter;
}

void add_student(StudentList *list) {
  Student record;
  int status;
  int value;
  int i;

  printf("\n  --- Add a new student record ---\n");

  if (!ensure_capacity(list, list->count + 1)) {
    return;
  }

  for (;;) {
    status = read_int("  Student ID: ", &value);
    if (status == 0) {
      printf("\n  Input closed - add cancelled.\n");
      return;
    }
    if (status == -1) {
      printf("  Invalid: enter a whole number.\n");
      continue;
    }
    if (value <= 0) {
      printf("  Invalid: the ID must be a positive integer.\n");
      continue;
    }
    i = find_index_by_id(list, value);
    if (i >= 0) {
      printf("  Rejected: ID %d is already used by %s.\n", value,
             list->items[i].name);
      continue;
    }
    record.id = value;
    break;
  }

  // name
  for (;;) {
    char buffer[48];

    printf("  Full name: ");
    status = read_line(buffer, 48);
    if (status == 0) {
      printf("\n  Input closed - add cancelled.\n");
      return;
    }
    if (status == -1) {
      printf("  Invalid: name is too long (maximum %d characters).\n", 48 - 1);
      continue;
    }
    if (!is_valid_name(buffer)) {
      printf("  Invalid: use letters, spaces, hyphens or apostrophes.\n");
      continue;
    }
    strcpy(record.name, buffer);
    break;
  }

  // course
  for (;;) {
    char buffer[40];

    printf("  Course: ");
    status = read_line(buffer, 40);
    if (status == 0) {
      printf("\n  Input closed - add cancelled.\n");
      return;
    }
    if (status == -1) {
      printf("  Invalid: course name is too long (maximum %d characters).\n",
             40 - 1);
      continue;
    }
    if (buffer[0] == '\0') {
      printf("  Invalid: the course name cannot be empty.\n");
      continue;
    }
    strcpy(record.course, buffer);
    break;
  }

  // grade
  for (;;) {
    status = read_int("  Grade (0-100): ", &value);
    if (status == 0) {
      printf("\n  Input closed - add cancelled.\n");
      return;
    }
    if (status == -1) {
      printf("  Invalid: enter a whole number between 0 and 100.\n");
      continue;
    }
    if (value < 0 || value > 100) {
      printf("  Invalid: the grade must be between 0 and 100.\n");
      continue;
    }
    record.grade = value;
    break;
  }

  list->items[list->count] = record;
  list->count++;

  printf("  Added: %s (ID %d, %s, %d). Records now: %d.\n", record.name,
         record.id, record.course, record.grade, list->count);
}

void display_students(StudentList *list) {
  const Student *p;

  if (list->count == 0) {
    printf("\n  No records to display.\n");
    return;
  }

  printf("\n  --- Student records (%d) ---\n", list->count);
  printf("  %-6s %-24s %-26s %s\n", "ID", "Name", "Course", "Grade");
  printf("  %-6s %-24s %-26s %s\n", "------", "------------------------",
         "--------------------------", "-----");

  for (p = list->items; p < list->items + list->count; p++) {
    printf("  %-6d %-24s %-26s %3d\n", p->id, p->name, p->course, p->grade);
  }
}

void sort_students(StudentList *list) {
  int choice;
  int status;
  int (*comparator)(const void *, const void *);

  if (list->count < 2) {
    printf("\n  Nothing to sort (fewer than two records).\n");
    return;
  }

  printf("\n  Sort by:\n    1. Grade (highest first)\n");
  printf("    2. Name (A-Z)\n    3. Student ID (ascending)\n");

  status = read_int("  Choice: ", &choice);
  if (status == 0) {
    printf("\n  Input closed - sort cancelled.\n");
    return;
  }
  if (status == -1) {
    printf("  Invalid choice - sort cancelled.\n");
    return;
  }

  if (choice == 1) {
    comparator = gradeDescCompare;
  } else if (choice == 2) {
    comparator = nameAscCompare;
  } else if (choice == 3) {
    comparator = idAscCompare;
  } else {
    printf("  Invalid choice - sort cancelled.\n");
    return;
  }

  qsort(list->items, list->count, sizeof(Student), comparator);
  printf("  Sorted.\n");
  display_students(list);
}

void calculate_average(StudentList *list) {
  if (list->count == 0) {
    printf("\n  No records - the average is undefined.\n");
    return;
  }
  printf("\n  Average grade across %d records: %.2f\n", list->count,
         list_average(list));
}

void find_top_student(StudentList *list) {
  const Student *p;
  const Student *top;

  if (list->count == 0) {
    printf("\n  No records - there is no top student.\n");
    return;
  }

  top = list->items;
  for (p = list->items + 1; p < list->items + list->count; p++) {
    if (p->grade > top->grade) {
      top = p;
    }
  }

  printf("\n  Top performing student: %s (ID %d, %s) with %d.\n", top->name,
         top->id, top->course, top->grade);
}

void search_student(StudentList *list) {
  const Student *p;
  int mode;
  int status;
  int value;
  int found = 0;

  if (list->count == 0) {
    printf("\n  No records to search.\n");
    return;
  }

  printf("\n  Search by:\n    1. Student ID\n    2. Name\n");
  status = read_int("  Choice: ", &mode);
  if (status == 0) {
    printf("\n  Input closed - search cancelled.\n");
    return;
  }
  if (status == -1) {
    printf("  Invalid choice - search cancelled.\n");
    return;
  }

  if (mode == 1) {
    status = read_int("  Student ID: ", &value);
    if (status == 0) {
      printf("\n  Input closed - search cancelled.\n");
      return;
    }
    if (status == -1) {
      printf("  Invalid: enter a whole number.\n");
      return;
    }

    for (p = list->items; p < list->items + list->count; p++) {
      if (p->id == value) {
        printf("  Found: %s (ID %d, %s, grade %d).\n", p->name, p->id,
               p->course, p->grade);
        found = 1;
        break;
      }
    }
    if (!found) {
      printf("  No student with ID %d.\n", value);
    }
  } else if (mode == 2) {
    char needle[48];

    printf("  Name to search for: ");
    status = read_line(needle, 48);
    if (status == 0) {
      printf("\n  Input closed - search cancelled.\n");
      return;
    }
    if (status == -1) {
      printf("  Invalid: name is too long.\n");
      return;
    }
    if (needle[0] == '\0') {
      printf("  Invalid: enter a name.\n");
      return;
    }

    for (p = list->items; p < list->items + list->count; p++) {
      if (str_compare_insensitive(p->name, needle) == 0) {
        printf("  Found: %s (ID %d, %s, grade %d).\n", p->name, p->id,
               p->course, p->grade);
        found = 1;
      }
    }
    if (!found) {
      printf("  No student named \"%s\".\n", needle);
    }
  } else {
    printf("  Invalid choice - search cancelled.\n");
  }
}

void remove_student(StudentList *list) {
  int status;
  int value;
  int index;
  int i;

  if (list->count == 0) {
    printf("\n  No records to remove.\n");
    return;
  }

  status = read_int("  Student ID to remove: ", &value);
  if (status == 0) {
    printf("\n  Input closed - removal cancelled.\n");
    return;
  }
  if (status == -1) {
    printf("  Invalid: enter a whole number.\n");
    return;
  }

  index = find_index_by_id(list, value);
  if (index < 0) {
    printf("  No student with ID %d - nothing removed.\n", value);
    return;
  }

  printf("  Removing %s (ID %d, grade %d).\n", list->items[index].name,
         list->items[index].id, list->items[index].grade);

  // move every later record one slot down
  for (i = index; i + 1 < list->count; i++) {
    list->items[i] = list->items[i + 1];
  }
  list->count--;

  shrink_if_parse(list);
  printf("  Removed. Records now: %d (capacity %d).\n", list->count,
         list->capacity);
}