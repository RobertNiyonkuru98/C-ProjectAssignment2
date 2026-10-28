#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
  int id;
  char name[NAME_MAX];
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
  char device_name[NAME_MAX];
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
