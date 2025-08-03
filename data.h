

#ifndef DATA_H
#define DATA_H
// 航班信息结构体
typedef struct {
    char flightID[7];      // 航班号，6 位字符
    char depCity[50];    // 起飞城市
    char arrCity[50];      // 抵达城市
    char depTime[20];     // 改为 "年/月/日 小时:分钟" 的格式，预留足够空间
    char arrTime[20];
    float basePrice;           // 基础票价
    float discount;            // 票价折扣
    int totalSeats;            // 总座位数
    int remSeats;        // 剩余座位数
} Flight;

// 哈希表节点结构体
typedef struct HashNode {
    char flightID[7];  // 键：航班号
    int index;         // 值：航班在主存储数组中的索引
    struct HashNode* next; // 用于处理哈希冲突的链表指针
} HashNode;

// 航班管理系统结构体
typedef struct {
    Flight* flights;       // 主存储：动态数组
    int count;             // 当前航班数量
    int capacity;          // 数组容量
    HashNode** hashTable;  // 哈希表：用于航班号索引   桶的数组
    int hashSize;          // 哈希表大小
} FlightManager;

FlightManager* initManager(int initialCapacity, int hashSize) ;
int hashFunction(const char* flightID, int hashSize);

int isValidFlightNumber(const char* flightID);
int isValidTime(const char* time);

void resizeFlightsArray(FlightManager* manager);
extern FlightManager* manager;

//.dat文件  内存
void loadDatToMem(FlightManager* manager, const char* filename);
void saveMemToDat(FlightManager* manager, const char* filename);

//航线嵌套哈希表结构体
//存储航班索引的链表节点
typedef struct RouteNode {
    int flightIndex; // 航班在主数组中的索引
    struct RouteNode* next;
} RouteNode;

// 抵达城市哈希桶节点
typedef struct ArrCityHashNode {
    char arrCity[50];
    RouteNode* routeList; // 航班索引链表
    struct ArrCityHashNode* next;
} ArrCityHashNode;

// 起飞城市哈希桶节点
typedef struct DepCityHashNode {
    char depCity[50];
    ArrCityHashNode** arrCityTable; // 二级哈希表
    int arrCityHashSize;
    struct DepCityHashNode* next;
} DepCityHashNode;

// 航线嵌套哈希表主结构
typedef struct {
    DepCityHashNode** depCityTable; // 一级哈希表
    int depCityHashSize;
} RouteHashTable;

// 初始化航线哈希表
RouteHashTable* initRouteHashTable(int depCityHashSize, int arrCityHashSize);
// 插入航班索引到航线哈希表
void insertRouteHashTable(RouteHashTable* table, const char* depCity, const char* arrCity, int flightIndex);
// 查找某航线所有航班索引
RouteNode* findRouteFlights(RouteHashTable* table, const char* depCity, const char* arrCity);
// 释放航线哈希表
void freeRouteHashTable(RouteHashTable* table);
// 从航线哈希表中移除指定航班
void removeRouteFromHashTable(RouteHashTable* table, const char* depCity, const char* arrCity, int flightIndex);

#endif //DATA_H
