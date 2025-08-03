#ifndef ORDER_H
#define ORDER_H

#include "../include/data.h"

//订单信息结构体
typedef struct {
    char orderID[6];      // 订单号，5位字符
    char flightID[7];      // 关联的航班号
    char customerName[50]; // 客户姓名
    char IDNumber[19];    // 身份证号，留一位给终止符
    char ordererName[50]; // 下单人姓名
    int status;           // 订单状态：0-正常，1-已退票
} Order;

//构建订单哈希表的节点
typedef struct OrderHashNode {
    char orderID[6];  // 键：订单号
    int index;         // 值：订单在主存储数组中的索引
    struct OrderHashNode* next; // 用于处理哈希冲突的链表指针
} OrderHashNode;

//订单管理系统结构体
typedef struct {
    Order* orders;       // 主存储，动态数组
    int count;           // 当前订单数量
    int capacity;        // 数组容量
    OrderHashNode** hashTable;  // 哈希表，用于订单号索引，桶的数组
    int hashSize;        // 哈希表大小
    int nextOrderNumber; // 下一订单号
} OrderManager;

//初始化订单管理系统
OrderManager* initOrderManager(int initialCapacity, int hashSize);

//订单哈希函数
int orderHashFunction(const char* orderID, int hashSize);

//验证订单号格式
int isValidOrderNumber(const char* orderID);

//调整订单数组的容量
void resizeOrdersArray(OrderManager* manager);

//从文件加载订单数据到内存
void loadOrdersDatToMem(OrderManager* manager, const char* filename);

//将内存中的订单数据保存到文件
void saveOrdersMemToDat(OrderManager* manager, const char* filename);

//订单号查询函数
int findOrderByID(OrderManager* manager, const char* orderID);

// 添加订单到订单管理系统
int addOrder(OrderManager* manager, Order order);

//退票函数
int refundOrder(OrderManager* manager, const char* orderID);

#endif // ORDER_H