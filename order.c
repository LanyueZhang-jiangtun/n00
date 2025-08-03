#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../include/order.h"
#include "../include/data.h"

// 初始化订单管理系统
OrderManager* initOrderManager(int initialCapacity, int hashSize) {
    OrderManager* manager = (OrderManager*)malloc(sizeof(OrderManager));
    if (!manager) return NULL;

    manager->orders = (Order*)malloc(sizeof(Order) * initialCapacity);
    if (!manager->orders) {
        free(manager);
        return NULL;
    }

    manager->count = 0;
    manager->capacity = initialCapacity;
    manager->nextOrderNumber = 1; // 新增

    // 初始化哈希表
    manager->hashSize = hashSize;
    manager->hashTable = (OrderHashNode**)calloc(hashSize, sizeof(OrderHashNode*));
    if (!manager->hashTable) {
        free(manager->orders);
        free(manager);
        return NULL;
    }
    return manager;
}

// 哈希函数
int orderHashFunction(const char* orderID, int hashSize) {
    int hash = 0;
    for (int i = 0; i < 5; i++) {
        hash = (hash * 31 + orderID[i]) % hashSize;
    }
    return hash;
}

// 检查订单号格式
int isValidOrderNumber(const char* orderID) {
    if (strlen(orderID) != 5) return 0;
    for (int i = 0; i < 5; i++) {
        if (!isdigit(orderID[i])) return 0;
    }
    return 1;
}

// 数组扩容
void resizeOrdersArray(OrderManager* manager) {
    int newCapacity = manager->capacity * 2;
    Order* newOrders = (Order*)realloc(manager->orders, sizeof(Order) * newCapacity);
    if (!newOrders) {
        printf("内存分配失败，无法添加新订单\n");
        exit(EXIT_FAILURE);
    }
    manager->orders = newOrders;
    manager->capacity = newCapacity;
}

// 从 .dat 文件加载订单到内存
void loadOrdersDatToMem(OrderManager* manager, const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) return; // 文件不存在时直接返回

    // 读 nextOrderNumber
    if (fread(&manager->nextOrderNumber, sizeof(int), 1, file) != 1) {
        fclose(file);
        return;
    }
    // 再读订单数量
    int count;
    if (fread(&count, sizeof(int), 1, file) != 1) {
        fclose(file);
        return;
    }
    // 读订单
    for (int i = 0; i < count; i++) {
        if (manager->count >= manager->capacity) {
            resizeOrdersArray(manager);
        }
        if (fread(&manager->orders[manager->count], sizeof(Order), 1, file) != 1) break;
        // 添加到哈希表
        int hashIndex = orderHashFunction(manager->orders[manager->count].orderID, manager->hashSize);
        OrderHashNode* node = (OrderHashNode*)malloc(sizeof(OrderHashNode));
        if (!node) {
            printf("内存分配失败\n");
            continue;
        }

        strcpy(node->orderID, manager->orders[manager->count].orderID);
        node->index = manager->count;
        node->next = manager->hashTable[hashIndex];
        manager->hashTable[hashIndex] = node;

        manager->count++;
    }
    fclose(file);
}


// 将内存中的订单保存到 .dat 文件
void saveOrdersMemToDat(OrderManager* manager, const char* filename) {
    FILE* file = fopen(filename, "wb");
    if (!file) {
        printf("无法打开文件: %s\n", filename);
        return;
    }
    // 先写 nextOrderNumber
    fwrite(&manager->nextOrderNumber, sizeof(int), 1, file);
    // 再写订单数量
    fwrite(&manager->count, sizeof(int), 1, file);
    // 再写订单数组
    for (int i = 0; i < manager->count; i++) {
        fwrite(&manager->orders[i], sizeof(Order), 1, file);
    }
    fclose(file);
}

// 辅助函数：通过订单号查找订单索引（如果需要）
int findOrderByID(OrderManager* manager, const char* orderID) {
    int hashIndex = orderHashFunction(orderID, manager->hashSize);
    OrderHashNode* node = manager->hashTable[hashIndex];

    while (node) {
        if (strcmp(node->orderID, orderID) == 0) {
            return node->index;
        }
        node = node->next;
    }

    return -1; // 未找到
}



// 添加订单到订单管理系统
int addOrder(OrderManager* manager, Order order) {
    // 检查订单数组是否需要扩容
    if (manager->count >= manager->capacity) {
        resizeOrdersArray(manager);
    }

    // 生成唯一订单号
    char orderID[6];
    snprintf(orderID, sizeof(orderID), "%05d", manager->nextOrderNumber++);
    strcpy(order.orderID, orderID);

    order.status = 0; // 这里加上对status的赋值
    // 添加订单到数组
    manager->orders[manager->count] = order;

    // 添加到哈希表
    int hashIndex = orderHashFunction(order.orderID, manager->hashSize);
    OrderHashNode* node = (OrderHashNode*)malloc(sizeof(OrderHashNode));
    if (!node) {
        printf("内存分配失败\n");
        return 0;
    }

    strcpy(node->orderID, order.orderID);
    node->index = manager->count;
    node->next = manager->hashTable[hashIndex];
    manager->hashTable[hashIndex] = node;

    manager->count++;
    return 1; // 添加成功
}

int refundOrder(OrderManager* manager, const char* orderID) {
    int idx = findOrderByID(manager, orderID);
    if (idx == -1) {
        return 0; // 未找到
    }
    if (manager->orders[idx].status == 1) {
        return 0; // 已退票
    }
    manager->orders[idx].status = 1; // 标记为已退票
    // 这里可以加上航班余票+1的逻辑
    return 1;
}
