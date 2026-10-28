# C Programming - Formative Project 2
This project assignment contains 3 C project and each sub folder contains source code of the project as assigned

## Structure

- **`P1-AcademicRisk/`**: This is a dynamic academic performance and risk analysing engine, which keeps student records in a dynamic allocated array, supports adding, displaying, sorting , searching and removing the records. it also socres every student witha custom academic risk index

- **`P2-TextMining`**: This is a dynamic text mining and linguistic pattern analyzer, which stores a paragraph of any length on the heap, counts word freq, reports memory address and also scoring the text with a custom lexical diversity score
- **`P3-IoTMonitor`**: This is a dynamic IoT Event Monitoring and callback engine which keeps a dynamic list of devices, stores their readings, dispatch everby reading to a callback chosen by device type and finally adds a custom sensor drift detector callback

here's the shape:

```
P1-AcademicRisk/
├── academicrisk.c            the program
├── docs/
│   ├── PoC-2026-10-28.md     what it does, how it works, how it was tested
│   └── evidence/             compiler output, sample runs, sanitizer output
└── outputs/
    └── sample_run.txt        a full recorded session
```

## Instructions
### Documentation
Every sub directory has `docs/` with a **POC** document which explains what the program does, what strcture was used and how the memory is managed, how the function pointers are used and finally how the custom analysis works.

### How to run the C program

Every program is a single C file and it uses the `gcc` compiler :
```shell
# For Project 1 -> Academic Performance and Risk analysis engine
cd P1-AcademicRisk
gcc -Wall -Wextra -std=c11 academicrisk.c -o academicrisk
./academicrisk

# For Project 2 -> Text Mining and Linguistic Pattern Analyser
cd ../P2-TextMining
gcc -Wall -Wextra -str=c11 textmining.c -o textmining
./textmining

# For Project 3 -> IoT Event Monitoring and Callback Engine
cd ../P3-IoTMonitor
gcc -Wall -Wextra -str=c11 iotmonitor.c -o iotmonitor
./iotmonitor
```

All of the three compiled under the `-Wall -Wextra` method. Each prgram is menu accessed and you type a number of the operation you want and press Enter. The last option also exists

## Author: Robert Niyonkuru