#include "cachelab.h"
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

int hitcnt = 0, misscnt = 0, evictioncnt = 0;
char* tracename;
void HelpMenu();
//通过结构体模拟cache的大体框架：
//行的模拟（Line）
//因为程序仅仅需要模拟命中、不命中、驱逐的功能，具体块中存储什么并不重要，因此省去。
typedef struct {
    int val;    //表示有效位(1表示有效）
    int tag;    //表示标识位
    int LRUcnt; //表示距离上一次调用该行有多久
} Line;
//组的模拟（Set）
typedef struct {
    Line* lines;
} Set;
//cache的模拟
typedef struct {
    Set* sets;
    int set_num;  //表示组数
    int line_num; //表示行数
} SimCache;
//获取cache 相关参数的函数
void Get_Opt(int argc, char** argv, int* verbose, int* s, int* E, int* b) {
    char order;
    while ((order = getopt(argc, argv, "hvs:E:b:t:")) != -1) {
        switch (order) {
        case 'h':
            HelpMenu();
            break;
        case 'v':
            *verbose = 1;
            break;
        case 's':
            *s = atoi(optarg);
            break;
        case 'E':
            *E = atoi(optarg);
            break;
        case 'b':
            *b = atoi(optarg);
            break;
        case 't':
            tracename = (char*)optarg;
            break;
        default:
            HelpMenu();
            exit(0);
        }
    }
}
void HelpMenu() {
    printf("Usage: [-hv] -s <num> -E <num> -b <num> -t <file>\n\
Options:\n\
-h\t\tPrint this help message.\n\
-v\t\tOptional verbose flag.\n\
-s <num>\tNumber of set index bits.\n\
-E <num>\tNumber of lines per set.\n\
-b <num>\tNumber of block offset bits.\n\
-t <file>\tTrace file.\n\n\
Examples:\n");
    printf("\tlinux> -s 4 -E 1 -b 4 -t traces/yi.trace\n");
    printf("\tlinux> -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
    exit(-1); //终止程序
}
void updateLRU(SimCache* simcache, int set, int line);
//判断命中与否 返回值为1表示命中，0表示不命中
int hit(SimCache* simcache, int set, int tag) {
    for (int i = 0; i < simcache->line_num; i++) {
        if (simcache->sets[set].lines[i].val == 1 && simcache->sets[set].lines[i].tag == tag) {
            //表示成功命中
            updateLRU(simcache, set, i);
            return 1;
        }
    }
    return 0;
}
//判断该组号下还有无空行 如有空行返回组号，若无空行返回-1
int full_or_not(SimCache* simcache, int set) {
    for (int i = 0; i < simcache->line_num; i++) {
        if (simcache->sets[set].lines[i].val == 0) {
            //表示有空行
            return i;
        }
    }
    return -1;
}
//在函数full_or_not返回值为-1的前提下，寻找在LRU规则下需要被替换的行
//返回值为将要被替换的行的行号
int eviction(SimCache* simcache, int set) {
    int LRUmax = 0;
    int replace = 0;
    for (int i = 0; i < simcache->line_num; i++) {
        if (simcache->sets[set].lines[i].LRUcnt > LRUmax) {
            LRUmax = simcache->sets[set].lines[i].LRUcnt;
            replace = i;
        }
    }
    return replace;
}
//对LRU进行批量操作的函数
//LRU越大表示越久没有调用该行
void updateLRU(SimCache* simcache, int set, int line) {
    for (int i = 0; i < simcache->line_num; i++) {
        simcache->sets[set].lines[i].LRUcnt++; //这一组其他的所有行
    }
    simcache->sets[set].lines[line].LRUcnt = 0; //最后将调用的那一行LRUcnt置零
}
//将以上三种操作按照逻辑链封装为一个函数
void cacahe_simulator(SimCache* simcache, int set, int tag, int verbose) {
    if (hit(simcache, set, tag)) { //如果命中进行如下操作
        hitcnt++;
        if (verbose) printf("hit ");
    } else {
        misscnt++;
        if (verbose) printf("miss ");
        //未命中先判断有无空行
        int replace = full_or_not(simcache, set);
        if (replace != -1) { //若有空行的操作：
            simcache->sets[set].lines[replace].val = 1;
            simcache->sets[set].lines[replace].tag = tag;
            updateLRU(simcache, set, replace);
        } else { //若无空行的操作：
            evictioncnt++;
            if (verbose) printf("eviction ");
            replace = eviction(simcache, set);
            simcache->sets[set].lines[replace].val = 1;
            simcache->sets[set].lines[replace].tag = tag;
            updateLRU(simcache, set, replace);
        }
    }
}
//初始化函数
void inition(int s, int E, int b, SimCache* simcache) {
    //判断输入的正确性
    if (s <= 0 || E <= 0) exit(0);
    //初始化cache：
    simcache->set_num = 2 << s;
    simcache->line_num = E;
    simcache->sets = (Set*)malloc(sizeof(Set) * simcache->set_num);
    //如果内存不足，退出
    if (simcache->sets == NULL) exit(0);
    //初始化set：
    for (int i = 0; i < simcache->set_num; i++) {
        simcache->sets[i].lines = (Line*)malloc(sizeof(Line) * simcache->line_num);
        for (int j = 0; j < E; j++) {
            simcache->sets[i].lines[j].val = 0;
            simcache->sets[i].lines[j].tag = 0;
        }
    }
}
//获取tag值的函数
//s 和 b的值分别表示地址中后（s+b）位为组号索引和块偏移量，右移该大小即可获得标识号。
int GetTag(int addr, int s, int b) {
    addr = (unsigned)addr;
    return addr >> (s + b);
}
//获取组号索引的函数
//右移b位，使低s位恰好为组号索引，再与低s位为1，其余位为0的掩码的按位与即可
int GetSet(int addr, int s, int b) {
    int set = addr >> b;
    int mask = (1 << s) - 1;
    return mask & set;
}
int main(int argc, char** argv) {
    int s = 0, E = 0, b = 0, verbose = 0;
    char order;
    SimCache simcache;
    unsigned long long addr;
    int size;
    //获取cache相关参数
    Get_Opt(argc, argv, &verbose, &s, &E, &b);
    inition(s, E, b, &simcache);
    printf("%s", tracename);
    //读取文件
    FILE* pFile = fopen(tracename, "r");
    while (fscanf(pFile, "%c %llx,%d", &order, &addr, &size) > 0) {
        //如果仅读，不涉及命中不命中，忽略即可。
        if (order == 'I') continue;
        int set = GetSet(addr, s, b);
        int tag = GetTag(addr, s, b);
        //如果是读取和存储，访问一次cache即可。
        if (order == 'L' || order == 'S')
            cacahe_simulator(&simcache, set, tag, verbose);
        //如果是修改操作，需要先读取后存储，访问两次cache。
        if (order == 'M') {
            cacahe_simulator(&simcache, set, tag, verbose);
            cacahe_simulator(&simcache, set, tag, verbose);
        }
        if (verbose == 1) printf("\n");
    }
    printSummary(hitcnt, misscnt, evictioncnt);
    return 0;
}