#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../include/admin.h"
#include "../include/menu.h"
#include "../include/data.h"
#include <assert.h>
#include <strings.h>


// 初始化航班管理系统
FlightManager* initManager(int initialCapacity, int hashSize) {
    FlightManager* manager = (FlightManager*)malloc(sizeof(FlightManager));
    if (!manager) return NULL;

    manager->flights = (Flight*)malloc(sizeof(Flight) * initialCapacity);
    manager->count = 0;
    manager->capacity = initialCapacity;

    // 初始化哈希表
    manager->hashSize = hashSize;
    manager->hashTable = (HashNode**)calloc(hashSize, sizeof(HashNode*));

    return manager;
}

// 哈希函数
int hashFunction(const char* flightID, int hashSize) {
    int hash = 0;
    for (int i = 0; i < 6; i++) {
        hash = (hash * 31 + flightID[i]) % hashSize;
    }
    return hash;
}

// 检查航班号格式
int isValidFlightNumber(const char* flightID) {
    if (strlen(flightID) != 6) return 0;
    if (!isupper(flightID[0]) || !isupper(flightID[1])) return 0;
    for (int i = 2; i < 6; i++) {
        if (!isdigit(flightID[i])) return 0;
    }
    return 1;
}

// 检查时间格式
int isValidTime(const char* time) {
    //长度是 14
    if (strlen(time) != 14) return 0;
    //校验分隔符位置
    if (time[2] != '/' || time[5] != '/' || time[8] != ' ' || time[11] != ':') return 0;
    //提取年月日时分
    int year = (time[0] - '0') * 10 + (time[1] - '0');
    int month = (time[3] - '0') * 10 + (time[4] - '0');
    int day = (time[6] - '0') * 10 + (time[7] - '0');
    int hour = (time[9] - '0') * 10 + (time[10] - '0');
    int minute = (time[12] - '0') * 10 + (time[13] - '0'); // 注意：原格式长度若为14，这里可能越界，需根据实际调整
    //范围校验
    return (year >= 0 && year < 100 //年份是两位
            && month >= 1 && month <= 12
            && day >= 1 && day <= 31
            && hour >= 0 && hour < 24
            && minute >= 0 && minute < 60);
}

void resizeFlightsArray(FlightManager* manager) {
    manager->capacity *= 2;
    Flight* newFlights = (Flight*)realloc(manager->flights, sizeof(Flight) * manager->capacity);
    if (newFlights) {
        manager->flights = newFlights;
    } else {
        show_operation_result("内存分配失败，无法添加新航班");
    }
}

void loadDatToMem(FlightManager* manager, const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) return;

    int count;
    if (fread(&count, sizeof(int), 1, fp) != 1) {
        fclose(fp);
        return;
    }
    for (int i = 0; i < count; i++) {
        Flight flight;
        if (fread(&flight, sizeof(Flight), 1, fp) != 1) break;
        addFlight(flight);
    }
    fclose(fp);
}
void saveMemToDat(FlightManager* manager, const char* filename) {
    FILE* fp = fopen(filename, "wb");
    if (!fp) {
        printf("无法打开文件保存数据！\n");
        return;
    }
    fwrite(&manager->count, sizeof(int), 1, fp);
    for (int i = 0; i < manager->count; i++) {
        fwrite(&manager->flights[i], sizeof(Flight), 1, fp);
    }
    fclose(fp);
}

//航线嵌套哈希表
//字符串哈希函数
static int cityHash(const char* city, int hashSize) {
    unsigned int hash = 0;
    for (int i = 0; city[i]; i++) {
        hash = (hash * 31 + tolower((unsigned char)city[i])) % hashSize;
    }
    return hash;
}

RouteHashTable* initRouteHashTable(int depCityHashSize, int arrCityHashSize) {
    RouteHashTable* table = (RouteHashTable*)malloc(sizeof(RouteHashTable));
    table->depCityHashSize = depCityHashSize;
    table->depCityTable = (DepCityHashNode**)calloc(depCityHashSize, sizeof(DepCityHashNode*));
    // arrCityHashSize 由 DepCityHashNode 记录
    return table;
}

void insertRouteHashTable(RouteHashTable* table, const char* depCity, const char* arrCity, int flightIndex) {
    int depHash = cityHash(depCity, table->depCityHashSize);
    DepCityHashNode* depNode = table->depCityTable[depHash];
    while (depNode) {
        if (strcasecmp(depNode->depCity, depCity) == 0) break;
        depNode = depNode->next;
    }
    if (!depNode) {
        depNode = (DepCityHashNode*)malloc(sizeof(DepCityHashNode));
        strcpy(depNode->depCity, depCity);
        depNode->arrCityHashSize = 53; // 固定二级哈希表大小
        depNode->arrCityTable = (ArrCityHashNode**)calloc(depNode->arrCityHashSize, sizeof(ArrCityHashNode*));
        depNode->next = table->depCityTable[depHash];
        table->depCityTable[depHash] = depNode;
    }
    int arrHash = cityHash(arrCity, depNode->arrCityHashSize);
    ArrCityHashNode* arrNode = depNode->arrCityTable[arrHash];
    while (arrNode) {
        if (strcasecmp(arrNode->arrCity, arrCity) == 0) break;
        arrNode = arrNode->next;
    }
    if (!arrNode) {
        arrNode = (ArrCityHashNode*)malloc(sizeof(ArrCityHashNode));
        strcpy(arrNode->arrCity, arrCity);
        arrNode->routeList = NULL;
        arrNode->next = depNode->arrCityTable[arrHash];
        depNode->arrCityTable[arrHash] = arrNode;
    }
    // 插入航班索引到链表头
    RouteNode* routeNode = (RouteNode*)malloc(sizeof(RouteNode));
    routeNode->flightIndex = flightIndex;
    routeNode->next = arrNode->routeList;
    arrNode->routeList = routeNode;
}

RouteNode* findRouteFlights(RouteHashTable* table, const char* depCity, const char* arrCity) {
    int depHash = cityHash(depCity, table->depCityHashSize);
    DepCityHashNode* depNode = table->depCityTable[depHash];
    while (depNode) {
        if (strcasecmp(depNode->depCity, depCity) == 0) break;
        depNode = depNode->next;
    }
    if (!depNode) return NULL;
    int arrHash = cityHash(arrCity, depNode->arrCityHashSize);
    ArrCityHashNode* arrNode = depNode->arrCityTable[arrHash];
    while (arrNode) {
        if (strcasecmp(arrNode->arrCity, arrCity) == 0) break;
        arrNode = arrNode->next;
    }
    if (!arrNode) return NULL;
    return arrNode->routeList;
}

void removeRouteFromHashTable(RouteHashTable* table, const char* depCity, const char* arrCity, int flightIndex) {
    int depHash = cityHash(depCity, table->depCityHashSize);
    DepCityHashNode* depNode = table->depCityTable[depHash];
    while (depNode) {
        if (strcasecmp(depNode->depCity, depCity) == 0) break;
        depNode = depNode->next;
    }
    if (!depNode) return;
    int arrHash = cityHash(arrCity, depNode->arrCityHashSize);
    ArrCityHashNode* arrNode = depNode->arrCityTable[arrHash];
    while (arrNode) {
        if (strcasecmp(arrNode->arrCity, arrCity) == 0) break;
        arrNode = arrNode->next;
    }
    if (!arrNode) return;
    RouteNode* prev = NULL;
    RouteNode* curr = arrNode->routeList;
    while (curr) {
        if (curr->flightIndex == flightIndex) {
            if (prev) prev->next = curr->next;
            else arrNode->routeList = curr->next;
            free(curr);
            break;
        }
        prev = curr;
        curr = curr->next;
    }
}

void freeRouteHashTable(RouteHashTable* table) {
    for (int i = 0; i < table->depCityHashSize; i++) {
        DepCityHashNode* depNode = table->depCityTable[i];
        while (depNode) {
            for (int j = 0; j < depNode->arrCityHashSize; j++) {
                ArrCityHashNode* arrNode = depNode->arrCityTable[j];
                while (arrNode) {
                    RouteNode* r = arrNode->routeList;
                    while (r) {
                        RouteNode* tmp = r;
                        r = r->next;
                        free(tmp);
                    }
                    ArrCityHashNode* tmpArr = arrNode;
                    arrNode = arrNode->next;
                    free(tmpArr);
                }
            }
            free(depNode->arrCityTable);
            DepCityHashNode* tmpDep = depNode;
            depNode = depNode->next;
            free(tmpDep);
        }
    }
    free(table->depCityTable);
    free(table);
}