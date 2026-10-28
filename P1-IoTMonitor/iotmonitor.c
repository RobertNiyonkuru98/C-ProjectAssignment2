#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
  int id;
  char name[40];
  int type;
  union {
    float temperature;
    int pressure;
    float voltage;
  } reading;
  int status;
  float last_value;
  int has_last;
} Device;

typedef struct {
  int device_id;
  char device_name[40];
  int type;
  float value;
  char unit[8];
  char expected[40];
  char reason[64];
  char callback[32];
} Alert;

typedef struct {
  Device *devices;
  int count, capacity;
  Alert *alerts;
  int alert_count, alert_capacity;
  int readings_processed, normal_readings, abnormal_readings;
  int alerts_by_type[4];
  int drift_flags;
} Monitor;

typedef void (*DeviceCallback)(Device *);
typedef void (*Operation)(Monitor *);

int g_running = 1;
Monitor *g_monitor = NULL;

const char *type_name(int type) {
  if (type == 1)
    return "Temperature";
  if (type == 2)
    return "Pressure";
  if (type == 3)
    return "Battery";
  return "Unknown";
}

const char *type_unit(int type) {
  if (type == 1)
    return "C";
  if (type == 2)
    return "kPa";
  if (type == 3)
    return "V";
  return "";
}

float random_float(float low, float high) {
  return low + ((float)rand() / (float)RAND_MAX) * (high - low);
}

// 1 = line read, 0 = end of input, -1 = line longer than the buffer
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

int read_int(const char *prompt, int *out) {
  char buf[32], *end;
  int status;

  printf("%s", prompt);
  status = read_line(buf, 32);
  if (status != 1)
    return status == 0 ? 0 : -1;
  *out = (int)strtol(buf, &end, 10);
  while (*end == ' ' || *end == '\t')
    end++;
  return (end == buf || *end != '\0') ? -1 : 1;
}

int ask_int(const char *prompt, int low, int high, const char *not_number,
            const char *out_of_range, int *out) {
  int status;

  for (;;) {
    status = read_int(prompt, out);
    if (status == 0)
      return 0;
    if (status == -1) {
      printf("%s", not_number);
      continue;
    }
    if (*out < low || *out > high) {
      printf("%s", out_of_range);
      continue;
    }
    return 1;
  }
}
