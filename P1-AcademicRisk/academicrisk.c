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

int count_below_pass(StudentList *list) {
  Student *p;
  int count = 0;

  for (p = list->items; p < list->items + list->count; p++) {
    if (p->grade < 50) {
      count++;
    }
  }
  return count;
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

double academic_risk_index(int grade, double cohort_mean) {
  double deficit;
  double relative_gap;
  double index;

  deficit = (double)50 - (double)grade;
  if (deficit < 0.0) {
    deficit = 0.0;
  }

  relative_gap = cohort_mean - (double)grade;
  if (relative_gap < 0.0) {
    relative_gap = 0.0;
  }

  index = 100.0 * ((0.6 * deficit / (double)50) + (0.4 * relative_gap / 100.0));

  if (index < 0.0) {
    index = 0.0;
  }
  if (index > 100.0) {
    index = 100.0;
  }
  return index;
}

const char *ari_band(double index) {
  if (index < 10.0) {
    return "Secure";
  }
  if (index < 25.0) {
    return "Low";
  }
  if (index < 50.0) {
    return "Moderate";
  }
  return "High";
}

void custom_analysis(StudentList *list) {
  const Student *p;
  const Student *worst = NULL;
  double mean;
  double total_risk = 0.0;
  double highest_risk = -1.0;
  int secure = 0;
  int low = 0;
  int moderate = 0;
  int high = 0;

  if (list->count == 0) {
    printf("\n  No records - the analysis needs at least one student.\n");
    return;
  }

  mean = list_average(list);

  printf("\n  --- Custom analysis: Academic Risk Index (ARI) ---\n");
  printf("  Pass mark: %d    Cohort mean: %.2f\n\n", 50, mean);
  printf("  %-6s %-24s %6s %8s %8s %7s  %s\n", "ID", "Name", "Grade", "Deficit",
         "Rel.Gap", "ARI", "Band");
  printf("  %-6s %-24s %6s %8s %8s %7s  %s\n", "------",
         "------------------------", "------", "--------", "--------",
         "-------", "--------");

  for (p = list->items; p < list->items + list->count; p++) {
    double index = academic_risk_index(p->grade, mean);
    double deficit = (double)50 - (double)p->grade;
    double relative_gap = mean - (double)p->grade;

    if (deficit < 0.0) {
      deficit = 0.0;
    }
    if (relative_gap < 0.0) {
      relative_gap = 0.0;
    }

    printf("  %-6d %-24s %6d %8.1f %8.1f %7.1f  %s\n", p->id, p->name, p->grade,
           deficit, relative_gap, index, ari_band(index));

    total_risk = total_risk + index;
    if (index > highest_risk) {
      highest_risk = index;
      worst = p;
    }
    if (index < 10.0) {
      secure++;
    } else if (index < 25.0) {
      low++;
    } else if (index < 50.0) {
      moderate++;
    } else {
      high++;
    }
  }

  printf("\n  Cohort ARI: mean %.1f\n", total_risk / (double)list->count);
  printf("  Secure: %d    Low: %d    Moderate: %d    High: %d\n", secure, low,
         moderate, high);
  if (worst != NULL) {
    printf("  Highest risk: %s (ID %d, grade %d, ARI %.1f, %s)\n", worst->name,
           worst->id, worst->grade, highest_risk, ari_band(highest_risk));
  }

  printf("\n  Note: the ARI measures the size of each gap. Below pass mark: "
         "%d.\n",
         count_below_pass(list));
}

void generate_summary_report(StudentList *list) {
  const Student *p;
  const Student *highest;
  const Student *lowest;
  int pass_count = 0;
  int fail_count = 0;
  int high_risk = 0;
  double mean;
  double total_risk = 0.0;

  if (list->count == 0) {
    printf("\n  No records - nothing to summarise.\n");
    return;
  }

  mean = list_average(list);
  highest = list->items;
  lowest = list->items;

  for (p = list->items; p < list->items + list->count; p++) {
    double risk = academic_risk_index(p->grade, mean);

    if (p->grade > highest->grade) {
      highest = p;
    }
    if (p->grade < lowest->grade) {
      lowest = p;
    }
    if (p->grade >= 50) {
      pass_count++;
    } else {
      fail_count++;
    }
    if (risk >= 50.0) {
      high_risk++;
    }
    total_risk = total_risk + risk;
  }

  printf("\n");
  printf("  ===================== SUMMARY REPORT =====================\n");
  printf("  Total student records : %d\n", list->count);
  printf("  Average grade         : %.2f\n", mean);
  printf("  Highest grade         : %d (%s, ID %d)\n", highest->grade,
         highest->name, highest->id);
  printf("  Lowest grade          : %d (%s, ID %d)\n", lowest->grade,
         lowest->name, lowest->id);
  printf("  Pass threshold        : %d\n", 50);
  printf("  Students passing      : %d\n", pass_count);
  printf("  Students below pass   : %d\n", fail_count);
  printf("  Memory in use         : %d records in %d slots\n", list->count,
         list->capacity);
  printf("  ----------------------------------------------------------\n");
  printf("  Custom analysis (Academic Risk Index):\n");
  printf("    Cohort ARI mean     : %.1f\n", total_risk / (double)list->count);
  printf("    Moderate/High risk  : %d student(s) with ARI >= 50\n", high_risk);
  printf("  ==========================================================\n");
}

void quit_program(StudentList *list) {
  int g_running = 1;
  (void)list;
  g_running = 0;
  printf("\n  Exiting.\n");
}

Operation operations[] = {
    add_student,       display_students, sort_students,
    calculate_average, find_top_student, search_student,
    remove_student,    custom_analysis,  generate_summary_report,
    quit_program};

const char *menu_labels[] = {"Add a new student record",
                             "Display all student records",
                             "Sort students (by grade / name / ID)",
                             "Calculate the average grade",
                             "Find the top performing student",
                             "Search for a student (by ID / name)",
                             "Remove a student record",
                             "Custom analysis - Academic Risk Index",
                             "Display the summary report",
                             "Exit"};

int MENU_COUNT = sizeof(operations) / sizeof(operations[0]);

void print_header(void) {
  printf("\n");
  printf(
      "  ==================================================================\n");
  printf("  Student ID : %s\n", "1000");
  printf("  Project    : Dynamic Academic Performance and Risk Analysis "
         "Engine\n");
  printf("  Custom     : Academic Risk Index (ARI)\n");
  printf(
      "  ==================================================================\n");
}

void print_menu(void) {
  int i;

  printf("\n  ------------------------- MENU -------------------------\n");
  for (i = 0; i < MENU_COUNT; i++) {
    printf("  %2d. %s\n", i + 1, menu_labels[i]);
  }
  printf("  --------------------------------------------------------\n");
}

Student seed_dataset[] = {
    {1001, "Aline Uwase", "C Programming", 88},
    {1002, "Kevin Niyonzima", "Data Structures", 74},
    {1003, "Diane Mukamana", "Database Systems", 65},
    {1004, "Eric Habimana", "Digital Electronics", 52},
    {1005, "Grace Iradukunda", "C Programming", 47},
    {1006, "Samuel Nkurunziza", "Mathematics for Computing", 39},
    {1007, "Chantal Umutoni", "Operating Systems", 91},
    {1008, "Patrick Bizimana", "Data Structures", 70},
    {1009, "Josiane Ineza", "Database Systems", 58},
    {1010, "Fabrice Nshuti", "Digital Electronics", 33},
    {1011, "Divine Mutoni", "C Programming", 82},
    {1012, "Jean Paul Rwigema", "Mathematics for Computing", 61}};

int SEED_COUNT = 12;

void load_seed_dataset(StudentList *list) {
  int i;

  for (i = 0; i < SEED_COUNT; i++) {
    if (!ensure_capacity(list, list->count + 1)) {
      return;
    }
    list->items[list->count] = seed_dataset[i];
    list->count++;
  }
}

int main() {
  StudentList list;
  int status;
  int choice;
  int g_running = 1;

  list.capacity = 10;
  list.count = 0;
  list.items = calloc(10, sizeof(Student));

  if (list.items == NULL) {
    printf("Error: could not allocate memory for the initial records.\n");
    return 1;
  }

  printf("  [mem] capacity 0 -> %d (initial calloc)\n", 10);

  load_seed_dataset(&list);
  print_header();
  printf("\n  Loaded %d starting records (capacity %d).\n", list.count,
         list.capacity);

  while (g_running) {
    print_menu();
    status = read_int("  Enter choice: ", &choice);

    if (status == 0) {
      printf("\n  Input closed.\n");
      break;
    }
    if (status == -1) {
      printf("  Invalid: enter a number from 1 to %d.\n", MENU_COUNT);
      continue;
    }
    if (choice < 1 || choice > MENU_COUNT) {
      printf("  Invalid: enter a number from 1 to %d.\n", MENU_COUNT);
      continue;
    }

    operations[choice - 1](&list);
  }

  free(list.items);
  list.items = NULL;
  list.count = 0;
  list.capacity = 0;

  printf("\n  All memory released. Goodbye.\n");
  return 0;
}
