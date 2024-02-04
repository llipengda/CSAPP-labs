#include "cachelab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

typedef struct {
    int valid; // valid
    long tag;  // tag
    long time; // time
} line;

line** cache;

int hits = 0;
int misses = 0;
int evictions = 0;

int s, E, b;
char trace_file[1000];
int verbose = 0; // false

void init() {
    int S = 1 << s; // 2 ^ s
    cache = (line**)malloc(sizeof(line*) * S);
    for (int i = 0; i < S; i++) {
        cache[i] = (line*)malloc(sizeof(line) * E);
        for (int j = 0; j < E; j++) {
            cache[i][j].valid = 0;
            cache[i][j].tag = -1;
            cache[i][j].time = 0;
        }
    }
}

void update(unsigned long address, char operation, int size) {
    int set = (address >> b) & ((-1U) >> (64 - s));
    int tag = address >> (b + s);

    // hit
    for (int i = 0; i < E; i++) {
        if (cache[set][i].tag == tag) {
            cache[set][i].time = 0;
            hits++;
            if (verbose) {
                printf("%c %lx,%d hit\n", operation, address, size);
            }
            return;
        }
    }

    // miss
    for (int i = 0; i < E; i++) {
        if (cache[set][i].valid == 0) {
            cache[set][i].valid = 1;
            cache[set][i].tag = tag;
            cache[set][i].time = 0;
            misses++;
            if (verbose) {
                printf("%c %lx,%d miss\n", operation, address, size);
            }
            return;
        }
    }

    // miss eviction
    evictions++;
    misses++;
    int max_time = -1;
    int max_time_index = -1;
    for (int i = 0; i < E; i++) {
        if (cache[set][i].time > max_time) {
            max_time = cache[set][i].time;
            max_time_index = i;
        }
    }
    cache[set][max_time_index].tag = tag;
    cache[set][max_time_index].time = 0;
    if (verbose) {
        printf("%c %lx,%d miss eviction\n", operation, address, size);
    }
}

void get_trace() {
    char operation;
    unsigned long address;
    int size;
    FILE* fp = fopen(trace_file, "r");
    if (fp == NULL) exit(-1);
    while (fscanf(fp, " %c %lx,%d\n", &operation, &address, &size) > 0) {

        // do operation
        switch (operation) {
            case 'L':
                update(address, operation, size);
                break;
            case 'S':
                update(address, operation, size);
                break;
            case 'M':
                update(address, operation, size);
                update(address, operation, size);
                break;
        }

        // update time
        for (int i = 0; i < (1 << s); i++) {
            for (int j = 0; j < E; j++) {
                if (cache[i][j].valid) {
                    cache[i][j].time++;
                }
            }
        }
    }
    fclose(fp);
}

void free_cache() {
    for (int i = 0; i < (1 << s); i++) {
        free(cache[i]);
    }
    free(cache);
}

int main(int argc, char* argv[]) {
    char opt;
    const char* usage = "usage: ./csim-ref [-hv] -s <s> -E <E> -b <b> -t <tracefile>";
    while ((opt = getopt(argc, argv, "hvs:E:b:t:")) != EOF) {
        switch (opt) {
            case 'h':
                fprintf(stdout, "%s", usage);
                exit(0);
                break;
            case 'v':
                verbose = 1;
                break;
            case 's':
                s = atoi(optarg);
                break;
            case 'E':
                E = atoi(optarg);
                break;
            case 'b':
                b = atoi(optarg);
                break;
            case 't':
                strcpy(trace_file, optarg);
                break;
            default:
                fprintf(stdout, "%s", usage);
                exit(-1);
                break;
        }
    }
    init();
    get_trace();
    free_cache();
    printSummary(hits, misses, evictions);
    return 0;
}
