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

int ensure_device_capacity(Monitor *m, int needed) {
  int new_cap = m->capacity;
  Device *grown;

  if (needed <= m->capacity)
    return 1;
  while (new_cap < needed)
    new_cap *= 2;
  grown = realloc(m->devices, new_cap * sizeof(Device));
  if (grown == NULL) {
    printf("  [mem] could not grow the device list - operation cancelled.\n");
    return 0;
  }
  printf("  [mem] device list %d -> %d slots\n", m->capacity, new_cap);
  m->devices = grown;
  m->capacity = new_cap;
  return 1;
}

void shrink_device_list(Monitor *m) {
  int new_cap;
  Device *shrunk;

  if (m->capacity <= 4 || m->count > m->capacity / 4)
    return;
  new_cap = m->capacity / 2 < 4 ? 4 : m->capacity / 2;
  shrunk = realloc(m->devices, new_cap * sizeof(Device));
  if (shrunk == NULL)
    return;
  printf("  [mem] device list %d -> %d slots (shrunk)\n", m->capacity, new_cap);
  m->devices = shrunk;
  m->capacity = new_cap;
}

int find_device_index(const Monitor *m, int id) {
  int i;

  for (i = 0; i < m->count; i++)
    if (m->devices[i].id == id)
      return i;
  return -1;
}

void add_alert(Device *d, const char *expected, const char *reason,
               const char *callback, float value) {
  Monitor *m = g_monitor;
  Alert *grown;
  Alert *a;

  if (m == NULL)
    return;
  if (m->alert_count >= m->alert_capacity) {
    int new_cap = m->alert_capacity * 2;

    grown = realloc(m->alerts, new_cap * sizeof(Alert));
    if (grown == NULL) {
      printf("  [mem] could not grow the alert list - alert not stored.\n");
      return;
    }
    printf("  [mem] alert list %d -> %d slots\n", m->alert_capacity, new_cap);
    m->alerts = grown;
    m->alert_capacity = new_cap;
  }

  a = &m->alerts[m->alert_count++];
  a->device_id = d->id;
  strcpy(a->device_name, d->name);
  a->type = d->type;
  a->value = value;
  strcpy(a->unit, type_unit(d->type));
  strcpy(a->expected, expected);
  strcpy(a->reason, reason);
  strcpy(a->callback, callback);
  m->alerts_by_type[d->type]++;
}

void temperature_monitor(Device *d) {
  float v = d->reading.temperature;

  if (v >= 18.0f && v <= 35.0f) {
    d->status = 0;
    printf("  normal %-20s %6.1f C\n", d->name, v);
    return;
  }
  d->status = 1;
  if (v < 18.0f) {
    printf("  ALERT  %-20s %6.1f C  below %.1f C\n", d->name, v, 18.0f);
    add_alert(d, "18.0 .. 35.0 C", "temperature too low", "temperature_monitor",
              v);
  } else {
    printf("  ALERT  %-20s %6.1f C  above %.1f C\n", d->name, v, 35.0f);
    add_alert(d, "18.0 .. 35.0 C", "temperature too high",
              "temperature_monitor", v);
  }
}

void pressure_monitor(Device *d) {
  int v = d->reading.pressure;

  if (v >= 98 && v <= 108) {
    d->status = 0;
    printf("  normal %-20s %6d kPa\n", d->name, v);
    return;
  }
  d->status = 1;
  if (v < 98) {
    printf("  ALERT  %-20s %6d kPa below %d kPa\n", d->name, v, 98);
    add_alert(d, "98 .. 108 kPa", "pressure too low", "pressure_monitor",
              (float)v);
  } else {
    printf("  ALERT  %-20s %6d kPa above %d kPa\n", d->name, v, 108);
    add_alert(d, "98 .. 108 kPa", "pressure too high", "pressure_monitor",
              (float)v);
  }
}

void battery_monitor(Device *d) {
  float v = d->reading.voltage;

  if (v >= 3.3f && v <= 4.2f) {
    d->status = 0;
    printf("  normal %-20s %6.2f V\n", d->name, v);
    return;
  }
  d->status = 1;
  if (v < 3.3f) {
    printf("  ALERT  %-20s %6.2f V  below %.2f V\n", d->name, v, 3.3f);
    add_alert(d, "3.30 .. 4.20 V", "battery voltage critically low",
              "battery_monitor", v);
  } else {
    printf("  ALERT  %-20s %6.2f V  above %.2f V\n", d->name, v, 4.2f);
    add_alert(d, "3.30 .. 4.20 V", "battery voltage too high",
              "battery_monitor", v);
  }
}

void drift_monitor(Device *d) {
  float value, change, limit;

  if (d->type == 1) {
    value = d->reading.temperature;
    limit = 5.0f;
  } else if (d->type == 2) {
    value = (float)d->reading.pressure;
    limit = (float)3;
  } else {
    value = d->reading.voltage;
    limit = 0.3f;
  }

  if (!d->has_last) {
    d->last_value = value;
    d->has_last = 1;
    return;
  }
  change = value - d->last_value;
  if (change < 0)
    change = -change;
  if (change > limit) {
    if (g_monitor != NULL)
      g_monitor->drift_flags++;
    printf("  DRIFT  %-20s moved %.2f %s since the last reading (limit "
           "%.2f)\n",
           d->name, change, type_unit(d->type), limit);
    add_alert(d, "change between readings", "sensor drift suspected",
              "drift_monitor", value);
  }
  d->last_value = value;
}

void process_device(Device *device, void (*callback)(Device *)) {
  if (device != NULL && callback != NULL)
    callback(device);
}

DeviceCallback callback_for_type(int type) {
  if (type == 1)
    return temperature_monitor;
  if (type == 2)
    return pressure_monitor;
  return battery_monitor;
}

void register_device(Monitor *m) {
  Device d;
  int value, i;

  printf("\n  --- Register a new device ---\n");
  if (!ensure_device_capacity(m, m->count + 1))
    return;

  for (;;) {
    if (!ask_int("  Device ID: ", 1, 2000000000,
                 "  Invalid: enter a whole number.\n",
                 "  Invalid: the ID must be positive.\n", &value)) {
      printf("\n  Input closed - registration cancelled.\n");
      return;
    }
    i = find_device_index(m, value);
    if (i >= 0) {
      printf("  Rejected: ID %d is already used by %s.\n", value,
             m->devices[i].name);
      continue;
    }
    d.id = value;
    break;
  }

  for (;;) {
    int name_status;
    printf("  Device name: ");
    name_status = read_line(d.name, 40);
    if (name_status == 0) {
      printf("\n  Input closed - registration cancelled.\n");
      return;
    }
    if (name_status == -1) {
      printf("  Invalid: name too long (maximum %d characters).\n", 40 - 1);
      continue;
    }
    if (d.name[0] == '\0') {
      printf("  Invalid: the name cannot be empty.\n");
      continue;
    }
    break;
  }

  printf("  Device type: 1 = temperature, 2 = pressure, 3 = battery\n");
  if (!ask_int("  Type: ", 1, 3, "  Invalid: enter 1, 2 or 3.\n",
               "  Invalid: choose 1, 2 or 3.\n", &value)) {
    printf("\n  Input closed - registration cancelled.\n");
    return;
  }
  d.type = value;

  if (d.type == 1)
    d.reading.temperature = 0.0f;
  else if (d.type == 2)
    d.reading.pressure = 0;
  else
    d.reading.voltage = 0.0f;

  d.status = 0;
  d.last_value = 0.0f;
  d.has_last = 0;

  m->devices[m->count++] = d;
  printf("  Registered %s (ID %d, %s). Devices now: %d.\n", d.name, d.id,
         type_name(d.type), m->count);
}

void display_devices(Monitor *m) {
  const Device *p;

  if (m->count == 0) {
    printf("\n  No devices registered.\n");
    return;
  }
  printf("\n  --- Registered devices (%d) ---\n", m->count);
  printf("  %-5s %-22s %-12s %10s  %s\n  %-5s %-22s %-12s %10s  %s\n", "ID",
         "Name", "Type", "Reading", "Status", "-----", "----------------------",
         "------------", "----------", "------");

  for (p = m->devices; p < m->devices + m->count; p++) {
    char reading[32];

    if (p->type == 1)
      sprintf(reading, "%.1f C", p->reading.temperature);
    else if (p->type == 3)
      sprintf(reading, "%d kPa", p->reading.pressure);
    else
      sprintf(reading, "%.2f V", p->reading.voltage);

    printf("  %-5d %-22s %-12s %10s  %s\n", p->id, p->name, type_name(p->type),
           reading, p->status == 0 ? "normal" : "ALERT");
  }
}

void remove_device(Monitor *m) {
  int value, index, i, status;

  if (m->count == 0) {
    printf("\n  No devices to remove.\n");
    return;
  }
  status = read_int("  Device ID to remove: ", &value);
  if (status == 0) {
    printf("\n  Input closed - removal cancelled.\n");
    return;
  }
  if (status == -1) {
    printf("  Invalid: enter a whole number.\n");
    return;
  }
  index = find_device_index(m, value);
  if (index < 0) {
    printf("  No device with ID %d - nothing removed.\n", value);
    return;
  }

  printf("  Removing %s (ID %d).\n", m->devices[index].name,
         m->devices[index].id);
  for (i = index; i + 1 < m->count; i++)
    m->devices[i] = m->devices[i + 1];
  m->count--;
  shrink_device_list(m);
  printf("  Removed. Devices now: %d (capacity %d).\n", m->count, m->capacity);
}

void simulate_readings(Monitor *m) {
  int r, i, forced;
  Device *d;
  float value;

  if (m->count == 0) {
    printf("\n  No devices registered - nothing to simulate.\n");
    return;
  }
  printf("\n  --- Simulating %d readings ---\n", 12);

  for (r = 0; r < 12; r++) {
    i = r % m->count;
    d = &m->devices[i];

    forced = (r % 4 == 3);

    if (d->type == 1) {
      value = forced ? random_float(36.0f, 45.0f) : random_float(15.0f, 45.0f);
      d->reading.temperature = value;
    } else if (d->type == 3) {
      value = forced ? random_float(95.0f, 97.0f) : random_float(95.0f, 110.0f);
      d->reading.pressure = (int)(value + 0.5f);
    } else {
      value = forced ? random_float(3.0f, 3.2f) : random_float(3.0f, 4.2f);
      d->reading.voltage = value;
    }

    m->readings_processed++;
    printf("\n  reading %2d: %s (ID %d, %s)\n", r + 1, d->name, d->id,
           type_name(d->type));

    process_device(d, callback_for_type(d->type));
    process_device(d, drift_monitor);

    if (d->status == 0)
      m->normal_readings++;
    else
      m->abnormal_readings++;
  }

  printf("\n  %d readings processed: %d normal, %d abnormal.\n", 12,
         m->normal_readings, m->abnormal_readings);
}

void display_alerts(Monitor *m) {
  const Alert *a;

  if (m->alert_count == 0) {
    printf("\n  No alerts have been raised yet.\n");
    return;
  }
  printf("\n  --- Alerts (%d) ---\n", m->alert_count);
  for (a = m->alerts; a < m->alerts + m->alert_count; a++)
    printf("\n  Device   : %s (ID %d)\n"
           "  Type     : %s\n"
           "  Reading  : %.2f %s\n"
           "  Expected : %s\n"
           "  Reason   : %s\n"
           "  Callback : %s\n",
           a->device_name, a->device_id, type_name(a->type), a->value, a->unit,
           a->expected, a->reason, a->callback);
}

void monitoring_summary(Monitor *m) {
  printf("\n  ================ MONITORING SUMMARY ================\n"
         "  Registered devices     : %d\n"
         "  Readings processed     : %d\n"
         "  Normal readings        : %d\n"
         "  Abnormal readings      : %d\n"
         "  Alerts stored          : %d\n"
         "  ----------------------------------------------------\n"
         "  Alerts by device type:\n"
         "    Temperature sensor   : %d\n"
         "    Pressure sensor      : %d\n"
         "    Battery monitor      : %d\n"
         "  ----------------------------------------------------\n"
         "  Custom callback (Sensor Drift Detector)\n"
         "    Readings flagged as drift: %d\n"
         "  ====================================================\n",
         m->count, m->readings_processed, m->normal_readings,
         m->abnormal_readings, m->alert_count, m->alerts_by_type[1],
         m->alerts_by_type[3], m->alerts_by_type[3], m->drift_flags);
}

void quit_program(Monitor *m) {
  (void)m;
  g_running = 0;
  printf("\n  Exiting.\n");
}

Operation operations[] = {
    register_device, display_devices,    remove_device, simulate_readings,
    display_alerts,  monitoring_summary, quit_program};

const char *menu_labels[] = {"Register a new device",
                             "Display all registered devices",
                             "Remove a device by ID",
                             "Generate simulated readings",
                             "Display detected alerts",
                             "Monitoring summary",
                             "Exit"};

int MENU_COUNT = sizeof(operations) / sizeof(operations[0]);

void print_header(void) {
  printf(
      "\n  ==================================================================\n"
      "  Student ID : %s\n"
      "  Project    : Dynamic IoT Event Monitoring and Callback Engine\n"
      "  Custom     : Sensor Drift Detector (callback)\n"
      "  ==================================================================\n",
      "1000");
}

void print_menu(void) {
  int i;

  printf("\n  ------------------------- MENU -------------------------\n");
  for (i = 0; i < MENU_COUNT; i++)
    printf("  %2d. %s\n", i + 1, menu_labels[i]);
  printf("  --------------------------------------------------------\n");
}

Device seed_devices[] = {{1, "Boiler Room Temp", 1, {0.0f}, 0, 0.0f, 0},
                         {2, "Main Line Pressure", 2, {0}, 0, 0.0f, 0},
                         {3, "Backup Battery", 3, {0.0f}, 0, 0.0f, 0},
                         {4, "Cold Room Temp", 1, {0.0f}, 0, 0.0f, 0}};

int SEED_COUNT = sizeof(seed_devices) / sizeof(seed_devices[0]);

int main() {
  Monitor m;
  int status, choice, i;

  srand((unsigned)time(NULL));

  m.capacity = 4;
  m.count = 0;
  m.devices = calloc(4, sizeof(Device));
  m.alert_capacity = 8;
  m.alert_count = 0;
  m.alerts = calloc(8, sizeof(Alert));
  m.readings_processed = 0;
  m.normal_readings = 0;
  m.abnormal_readings = 0;
  m.drift_flags = 0;
  for (i = 0; i < 4; i++)
    m.alerts_by_type[i] = 0;

  if (m.devices == NULL || m.alerts == NULL) {
    printf("Error: could not allocate memory for the device or alert list.\n");
    return 1;
  }
  g_monitor = &m;

  printf("  [mem] device list 0 -> %d slots\n", 4);
  printf("  [mem] alert list 0 -> %d slots\n", 8);
  for (i = 0; i < SEED_COUNT; i++) {
    if (!ensure_device_capacity(&m, m.count + 1))
      break;
    m.devices[m.count++] = seed_devices[i];
  }

  print_header();
  printf("\n  Loaded %d starting devices (capacity %d).\n", m.count,
         m.capacity);

  while (g_running) {
    print_menu();
    status = read_int("  Enter choice: ", &choice);
    if (status == 0) {
      printf("\n  Input closed.\n");
      break;
    }
    if (status == -1 || choice < 1 || choice > MENU_COUNT) {
      printf("  Invalid: enter a number from 1 to %d.\n", MENU_COUNT);
      continue;
    }
    operations[choice - 1](&m);
  }

  free(m.devices);
  free(m.alerts);
  m.devices = NULL;
  m.alerts = NULL;
  printf("\n  All memory released. Goodbye.\n");
  return 0;
}