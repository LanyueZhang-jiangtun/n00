#include "../include/admin.h"
#include "../include/menu.h"
#include "../include/data.h"
#include "../include/order.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <curses.h>
extern RouteHashTable* routeTable;


// 录入航班信息（NCURSES模式适配）
void add_flight() {
    Flight flight;
    int y, x;
    get_center_position(&y, &x); // 获取屏幕中心位置

    //显示标题（居中）
    clear_and_move(y - 7, x - 10);
    printw("----- 录入航班信息 -----");
    refresh();

    //输入航班号（居中布局）
    move(y - 5, x - 30);
    printw("请输入航班号 (6位，前两位大写字母，后四位数字): ");
    refresh();
    echo(); // 回显
    getnstr(flight.flightID, 7); // 读取6个字符（flightID[7]含终止符）
    noecho();

    //起飞城市
    move(y - 3, x - 30);
    printw("请输入起飞城市: ");
    refresh();
    echo();
    getnstr(flight.depCity, 49); // depCity[50]
    flight.depCity[strcspn(flight.depCity, "\n")] = '\0'; // 移除换行符
    noecho();

    //抵达城市
    move(y - 1, x - 30);
    printw("请输入抵达城市: ");
    refresh();
    echo();
    getnstr(flight.arrCity, 49);
    flight.arrCity[strcspn(flight.arrCity, "\n")] = '\0';
    noecho();

    //起飞时间
    move(y + 1, x - 30);
    printw("请输入起飞时间 (格式：年/月/日 小时:分钟，如25/08/01 23:00): ");
    refresh();
    echo();
    getnstr(flight.depTime, 19); // depTime[20]
    flight.depTime[strcspn(flight.depTime, "\n")] = '\0';
    noecho();

    //抵达时间
    move(y + 3, x - 30);
    printw("请输入抵达时间 (格式：年/月/日 小时:分钟，如25/08/01 23:00): ");
    refresh();
    echo();
    getnstr(flight.arrTime, 19);
    flight.arrTime[strcspn(flight.arrTime, "\n")] = '\0';
    noecho();

    //基础票价（数字输入用scanw）
    move(y + 5, x - 30);
    printw("请输入基础票价: ");
    refresh();
    echo();
    scanw("%f", &flight.basePrice); // 读取浮点数
    noecho();

    //票价折扣
    move(y + 7, x - 30);
    printw("请输入票价折扣 (例如0.9表示9折): ");
    refresh();
    echo();
    scanw("%f", &flight.discount);
    noecho();

    //总座位数
    move(y + 9, x - 30);
    printw("请输入总座位数: ");
    refresh();
    echo();
    scanw("%d", &flight.totalSeats);
    noecho();

    //剩余座位数
    move(y + 11, x - 30);
    printw("请输入剩余座位数: ");
    refresh();
    echo();
    scanw("%d", &flight.remSeats);
    noecho();

    // 调用addFlight函数添加航班
    addFlight(flight);
}

// 添加航班
int addFlight(Flight flight) {
    // 检查航班号格式
    if (!isValidFlightNumber(flight.flightID)) {
        show_operation_result("无效的航班号格式！前两位应为大写字母，后四位应为数字");
        return 0;
    }

    // 检查时间格式
    if (!isValidTime(flight.depTime)) {
        show_operation_result("无效的起飞时间格式！请使用 年/月/日 小时:分钟 格式（如 25/08/01 23:00 ）");
        return 0;
    }
    if (!isValidTime(flight.arrTime)) {
        show_operation_result("无效的抵达时间格式！请使用 年/月/日 小时:分钟 格式（如 25/08/01 23:00 ）");
        return 0;
    }

    // 检查航班号是否已存在
    int hashIndex = hashFunction(flight.flightID, manager->hashSize);
    HashNode* current = manager->hashTable[hashIndex];
    while (current) {
        if (strcmp(current->flightID, flight.flightID) == 0) {
            show_operation_result("航班号已存在！");
            return 0;
        }
        current = current->next;
    }

    // 数组扩容
    if (manager->count >= manager->capacity) {
        resizeFlightsArray(manager);
    }

    // 添加到主存储数组
    int flightIndex = manager->count;
    manager->flights[flightIndex] = flight;
    manager->count++;

    // 添加到哈希表
    HashNode* newNode = (HashNode*)malloc(sizeof(HashNode));
    if (!newNode) {
        show_operation_result("内存分配失败");
        return 0;
    }
    strcpy(newNode->flightID, flight.flightID);
    newNode->index = flightIndex;
    newNode->next = manager->hashTable[hashIndex];
    manager->hashTable[hashIndex] = newNode;

    // 添加到航线哈希表
    insertRouteHashTable(routeTable, flight.depCity, flight.arrCity, flightIndex);

    show_operation_result("航班添加成功！");
    return 1;
}

void modify_flight() {
    Flight* flight = NULL;
    char flightID[7];
    int y, x;
    get_center_position(&y, &x);

    //标题
    clear_and_move(y - 8, x - 10);
    printw("----- 修改航班信息 -----");
    refresh();

    //查找航班号
    move(y - 6, x - 30);
    printw("请输入要修改的航班号 (6位，前两位大写字母，后四位数字): ");
    refresh();
    echo();
    getnstr(flightID, 6);
    noecho();

    if (!isValidFlightNumber(flightID)) {
        show_operation_result("无效的航班号格式！");
        return;
    }

    // 哈希查找航班
    int hashIndex = hashFunction(flightID, manager->hashSize);
    HashNode* current = manager->hashTable[hashIndex];
    while (current) {
        if (strcmp(current->flightID, flightID) == 0) {
            flight = &manager->flights[current->index];
            break;
        }
        current = current->next;
    }

    if (!flight) {
        show_operation_result("未找到该航班！");
        return;
    }

    //显示当前航班信息
    move(y - 4, x - 30);
    printw("当前航班信息如下：");
    move(y - 3, x - 30);
    printw("航班号：%s | 起飞城市：%s | 抵达城市：%s", flight->flightID, flight->depCity, flight->arrCity);
    move(y - 2, x - 30);
    printw("起飞时间：%s | 抵达时间：%s", flight->depTime, flight->arrTime);
    move(y - 1, x - 30);
    printw("票价折扣：%.1f | 剩余座位：%d", flight->discount, flight->remSeats);
    move(y, x - 30);
    printw("基础票价：%.2f（默认不变） | 总座位数：%d（默认不变）", flight->basePrice, flight->totalSeats);
    move(y + 1, x - 30);
    printw("请输入新信息（输入'-'表示不修改）：");
    refresh();

    //依次输入修改后的信息
    char input[50];

    //起飞城市
    move(y + 3, x - 30);
    printw("起飞城市（当前：%s）：", flight->depCity);
    refresh();
    echo();
    getnstr(input, 49);
    noecho();
    if (strcmp(input, "-") != 0) {
        strcpy(flight->depCity, input);
    }

    //抵达城市
    move(y + 5, x - 30);
    printw("抵达城市（当前：%s）：", flight->arrCity);
    refresh();
    echo();
    getnstr(input, 49);
    noecho();
    if (strcmp(input, "-") != 0) {
        strcpy(flight->arrCity, input);
    }

    //起飞时间
    move(y + 7, x - 30);
    printw("起飞时间（当前：%s，格式：年/月/日 小时:分钟）：", flight->depTime);
    refresh();
    echo();
    getnstr(input, 19);
    noecho();
    if (strcmp(input, "-") != 0) {
        if (isValidTime(input)) {
            strcpy(flight->depTime, input);
        } else {
            show_operation_result("起飞时间格式无效，保持原时间");
            napms(1000);
        }
    }

    //抵达时间
    move(y + 9, x - 30);
    printw("抵达时间（当前：%s，格式：年/月/日 小时:分钟）：", flight->arrTime);
    refresh();
    echo();
    getnstr(input, 19);
    noecho();
    if (strcmp(input, "-") != 0) {
        if (isValidTime(input)) {
            strcpy(flight->arrTime, input);
        } else {
            show_operation_result("抵达时间格式无效，保持原时间");
            napms(1000);
        }
    }

    //票价折扣
    move(y + 11, x - 30);
    printw("票价折扣（当前：%.1f，例如0.9）：", flight->discount);
    refresh();
    echo();
    getnstr(input, 10);
    noecho();
    if (strcmp(input, "-") != 0) {
        float newDiscount;
        if (sscanf(input, "%f", &newDiscount) == 1) {
            flight->discount = newDiscount;
        } else {
            show_operation_result("折扣格式无效，保持原折扣");
            napms(1000);
        }
    }

    //剩余座位数
    move(y + 13, x - 30);
    printw("剩余座位数（当前：%d）：", flight->remSeats);
    refresh();
    echo();
    getnstr(input, 10);
    noecho();
    if (strcmp(input, "-") != 0) {
        int newRemSeats;
        if (sscanf(input, "%d", &newRemSeats) == 1) {
            flight->remSeats = newRemSeats;
        } else {
            show_operation_result("剩余座位数格式无效，保持原数");
            napms(1000);
        }
    }
    show_operation_result("航班信息修改完成！");
}



void del_flight() {
    char flightID[7];
    Flight* flight = NULL;
    int y, x;
    get_center_position(&y, &x);

    //标题
    clear_and_move(y - 8, x - 10);
    printw("----- 删除航班信息 -----");
    refresh();

    //航班号
    move(y - 6, x - 30);
    printw("请输入要删除的航班号 (6位，前两位大写字母，后四位数字): ");
    refresh();
    echo();
    getnstr(flightID, 6);
    noecho();

    //验证航班号格式
    if (!isValidFlightNumber(flightID)) {
        show_operation_result("无效的航班号格式！");
        return;
    }

    //查找航班
    int hashIndex = hashFunction(flightID, manager->hashSize);
    HashNode* current = manager->hashTable[hashIndex];
    HashNode* prev = NULL;
    while (current) {
        if (strcmp(current->flightID, flightID) == 0) {
            flight = &manager->flights[current->index];
            break;
        }
        prev = current;
        current = current->next;
    }

    if (!flight) {
        show_operation_result("未找到该航班！");
        return;
    }

    //显示当前航班信息
    move(y - 4, x - 30);
    printw("当前航班信息如下：");
    move(y - 3, x - 30);
    printw("航班号：%s | 起飞城市：%s | 抵达城市：%s", flight->flightID, flight->depCity, flight->arrCity);
    move(y - 2, x - 30);
    printw("起飞时间：%s | 抵达时间：%s", flight->depTime, flight->arrTime);
    move(y - 1, x - 30);
    printw("基础票价：%.2f | 票价折扣：%.1f | 总座位数：%d | 剩余座位：%d",
           flight->basePrice, flight->discount, flight->totalSeats, flight->remSeats);
    refresh();

    //确认删除
    move(y + 1, x - 30);
    printw("确定要删除该航班吗？(y/n)：");
    refresh();
    char confirm;
    echo();
    confirm = getch();
    noecho();

    if (tolower(confirm) != 'y') {
        show_operation_result("已取消删除操作");
        return;
    }

    //从哈希表中删除
    if (prev) {
        prev->next = current->next;
    } else {
        manager->hashTable[hashIndex] = current->next;
    }
    free(current);

    //从航线哈希表中删除
    int deleteIndex = flight - manager->flights;
    removeRouteFromHashTable(routeTable, flight->depCity, flight->arrCity, deleteIndex);

    //从数组中删除（用最后一个元素覆盖要删除的元素）
    if (deleteIndex < manager->count - 1) {
        // 不是最后一个元素，用最后一个元素覆盖
        manager->flights[deleteIndex] = manager->flights[manager->count - 1];
        // 更新被移动元素在哈希表中的索引
        char movedFlightID[7];
        strcpy(movedFlightID, manager->flights[deleteIndex].flightID);
        int movedHashIndex = hashFunction(movedFlightID, manager->hashSize);
        HashNode* moveNode = manager->hashTable[movedHashIndex];
        while (moveNode) {
            if (strcmp(moveNode->flightID, movedFlightID) == 0) {
                moveNode->index = deleteIndex;
                break;
            }
            moveNode = moveNode->next;
        }
        //同步更新航线哈希表中被移动航班的索引
        removeRouteFromHashTable(routeTable, manager->flights[manager->count-1].depCity, manager->flights[manager->count-1].arrCity, manager->count-1);
        insertRouteHashTable(routeTable, manager->flights[deleteIndex].depCity, manager->flights[deleteIndex].arrCity, deleteIndex);
    }
    manager->count--;

    show_operation_result("航班删除成功！");
}

