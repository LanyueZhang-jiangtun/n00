#include "../include/cust.h"
#include "../include/menu.h"
#include "../include/order.h"
#include "../include/data.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <curses.h>

extern OrderManager* orderManager;
extern RouteHashTable* routeTable;

// AVL树结构体与操作
typedef struct AVLNode {
    int flightIndex;
    int dateInt; // yyyymmdd
    float price;
    struct AVLNode* left;
    struct AVLNode* right;
    int height;
} AVLNode;

// 获取高度
static int avl_height(AVLNode* n) { return n ? n->height : 0; }
// 最大值
static int avl_max(int a, int b) { return a > b ? a : b; }
// 右旋
static AVLNode* avl_right_rotate(AVLNode* y) {
    AVLNode* x = y->left;
    AVLNode* T2 = x->right;
    x->right = y;
    y->left = T2;
    y->height = avl_max(avl_height(y->left), avl_height(y->right)) + 1;
    x->height = avl_max(avl_height(x->left), avl_height(x->right)) + 1;
    return x;
}
// 左旋
static AVLNode* avl_left_rotate(AVLNode* x) {
    AVLNode* y = x->right;
    AVLNode* T2 = y->left;
    y->left = x;
    x->right = T2;
    x->height = avl_max(avl_height(x->left), avl_height(x->right)) + 1;
    y->height = avl_max(avl_height(y->left), avl_height(y->right)) + 1;
    return y;
}
// 平衡因子
static int avl_get_balance(AVLNode* n) { return n ? avl_height(n->left) - avl_height(n->right) : 0; }
// 插入
static AVLNode* avl_insert(AVLNode* node, int flightIndex, int dateInt, float price) {
    if (!node) {
        AVLNode* n = (AVLNode*)malloc(sizeof(AVLNode));
        n->flightIndex = flightIndex; n->dateInt = dateInt; n->price = price;
        n->left = n->right = NULL; n->height = 1;
        return n;
    }
    if (dateInt < node->dateInt || (dateInt == node->dateInt && price < node->price))
        node->left = avl_insert(node->left, flightIndex, dateInt, price);
    else
        node->right = avl_insert(node->right, flightIndex, dateInt, price);
    node->height = 1 + avl_max(avl_height(node->left), avl_height(node->right));
    int balance = avl_get_balance(node);
    if (balance > 1 && (dateInt < node->left->dateInt || (dateInt == node->left->dateInt && price < node->left->price)))
        return avl_right_rotate(node);
    if (balance < -1 && (dateInt > node->right->dateInt || (dateInt == node->right->dateInt && price >= node->right->price)))
        return avl_left_rotate(node);
    if (balance > 1 && (dateInt > node->left->dateInt || (dateInt == node->left->dateInt && price >= node->left->price))) {
        node->left = avl_left_rotate(node->left);
        return avl_right_rotate(node);
    }
    if (balance < -1 && (dateInt < node->right->dateInt || (dateInt == node->right->dateInt && price < node->right->price))) {
        node->right = avl_right_rotate(node->right);
        return avl_left_rotate(node);
    }
    return node;
}
// 中序遍历收集排序结果
static void avl_inorder(AVLNode* root, int* out, int* count, int max) {
    if (!root || *count >= max) return;
    avl_inorder(root->left, out, count, max);
    if (*count < max) out[(*count)++] = root->flightIndex;
    avl_inorder(root->right, out, count, max);
}
// 释放树
static void avl_free(AVLNode* root) {
    if (!root) return;
    avl_free(root->left);
    avl_free(root->right);
    free(root);
}
// 字符串转yyyymmdd
static int date_to_int(const char* depTime) {
    int y, m, d;
    sscanf(depTime, "%2d/%2d/%2d", &y, &m, &d);
    return (y+2000)*10000 + m*100 + d;
}

// 下单（NCURSES模式适配）
void add_order() {
    Order order;
    int y, x;
    get_center_position(&y, &x);

    // 显示标题（居中）
    clear_and_move(y - 7, x - 10); // 标题位置：中心上方7行，左移10列（居中）
    printw("----- 输入购票信息 -----");
    refresh();

    //输入要订票的航班号（居中布局）
    //后面有防错：如果没有此航班号，报错处理
    move(y - 5, x - 30); // 标题下方2行，左移30列（对齐提示文字）
    printw("请输入航班号 (6位，前两位大写字母，后四位数字): ");
    refresh();
    echo();
    getnstr(order.flightID, 7); // 读取6个字符（flightID[7]含终止符）
    noecho();

    // 验证航班号有效性
    if (!validateFlight(order.flightID)) {
        return;
    }

    // 检查航班剩余座位数
    int hashIndex = hashFunction(order.flightID, manager->hashSize);
    HashNode* current = manager->hashTable[hashIndex];
    int flightIdx = -1;
    while (current) {
        if (strcmp(current->flightID, order.flightID) == 0) {
            flightIdx = current->index;
            break;
        }
        current = current->next;
    }
    if (flightIdx == -1) {
        show_operation_result("未查询到该航班信息！");
        return;
    }
    if (manager->flights[flightIdx].remSeats <= 0) {
        // 推荐同航线其他可选航班，先按日期再按价格排序
        char* depCity = manager->flights[flightIdx].depCity;
        char* arrCity = manager->flights[flightIdx].arrCity;
        RouteNode* routeList = findRouteFlights(routeTable, depCity, arrCity);
        int found = 0;
        // 构建AVL树
        AVLNode* root = NULL;
        int orig_dateint = date_to_int(manager->flights[flightIdx].depTime);
        for (RouteNode* p = routeList; p; p = p->next) {
            int idx = p->flightIndex;
            if (idx != flightIdx && manager->flights[idx].remSeats > 0) {
                int dateint = date_to_int(manager->flights[idx].depTime);
                float price = manager->flights[idx].basePrice * manager->flights[idx].discount;
                root = avl_insert(root, idx, dateint, price);
            }
        }
        // 中序遍历收集排序结果
        int sortedIdx[1024], scount = 0;
        avl_inorder(root, sortedIdx, &scount, 1024);
        // 打印美观的推荐航班信息（左对齐风格，与管理员列出航班一致）
        clear();
        bkgd(COLOR_PAIR(3));
        getmaxyx(stdscr, y, x);
        const char* fmt_head = "%-6s | %-8s | %-8s | %-18s | %-18s | %-8s | %-4s | %-6s | %-8s | %-8s";
        const char* fmt_body = "%-6s | %-8s | %-8s | %-18s | %-18s | %-8.2f | %-4.1f | %-6d | %-8d | %-8.2f";
        mvprintw(0, 0, fmt_head, "ID", "DepCity", "ArrCity", "DepTime", "ArrTime", "Price", "Disc", "Total", "Remain", "CurPrice");
        mvprintw(1, 0, "-----------------------------------------------------------------------------");
        int row = 2;
        for (int si = 0; si < scount; ++si) {
            int idx = sortedIdx[si];
            Flight* f = &manager->flights[idx];
            int is_same_day = (date_to_int(f->depTime) == orig_dateint);
            char info[256];
            snprintf(info, sizeof(info), fmt_body,
                f->flightID, f->depCity, f->arrCity, f->depTime, f->arrTime, f->basePrice, f->discount, f->totalSeats, f->remSeats, f->basePrice * f->discount);
            if (is_same_day) {
                attron(COLOR_PAIR(7)); // 青色
                mvprintw(row, 0, "%-6s", f->flightID);
                attroff(COLOR_PAIR(7));
                attron(COLOR_PAIR(3));
                mvprintw(row, 6, "%s", info + 6);
                attroff(COLOR_PAIR(3));
            } else {
                attron(COLOR_PAIR(3));
                mvprintw(row, 0, "%s", info);
                attroff(COLOR_PAIR(3));
            }
            row++;
            found = 1;
        }
        avl_free(root);
        if (found) {
            mvprintw(row + 2, (x - 30) / 2, "该航班已无余票，推荐如上可选航班");
        } else {
            mvprintw(row, (x - 30) / 2, "该航班已无余票，且同航线无可用航班！");
        }
        mvprintw(row + 4, (x - 15) / 2, "按任意键继续...");
        refresh();
        getch();
        return;
    }

    // 输入乘客姓名
    move(y - 3, x - 30); // 上一项下方2行
    printw("请输入乘客姓名: ");
    refresh();
    echo();
    getnstr(order.customerName, 49); // customerName[50]
    order.customerName[strcspn(order.customerName, "\n")] = '\0';
    noecho();

    // 输入身份证号
    move(y - 1, x - 30);
    printw("请输入身份证号: ");
    refresh();
    echo();
    getnstr(order.IDNumber, 18);
    order.IDNumber[strcspn(order.IDNumber, "\n")] = '\0';
    noecho();

    // 输入下单人姓名
    move(y + 1, x - 30);
    printw("请输入下单人姓名：");
    refresh();
    echo();
    getnstr(order.ordererName, 49); // [50]
    order.ordererName[strcspn(order.ordererName, "\n")] = '\0';
    noecho();


    // 调用addOrder函数添加订单并检查结果
    if (addOrder(orderManager, order)) {
        // 订票成功，找到航班并减一
        int hashIndex = hashFunction(order.flightID, manager->hashSize);
        HashNode* current = manager->hashTable[hashIndex];
        while (current) {
            if (strcmp(current->flightID, order.flightID) == 0) {
                manager->flights[current->index].remSeats--;
                break;
            }
            current = current->next;
        }
        // 打印订单详细信息
        int orderIdx = orderManager->count - 1;
        Order* o = &orderManager->orders[orderIdx];
        clear();
        move(y - 6, x - 15);
        printw("----- 订单信息 -----");
        move(y - 4, x - 15);
        printw("订单号：%s", o->orderID);
        move(y - 2, x - 15);
        printw("航班号：%s", o->flightID);
        move(y, x - 15);
        printw("乘客姓名：%s", o->customerName);
        move(y + 2, x - 15);
        printw("身份证号：%s", o->IDNumber);
        move(y + 4, x - 15);
        printw("下单人：%s", o->ordererName);
        move(y + 6, x - 15);
        printw("状态：%s", o->status == 0 ? "正常" : "已退票");
        move(y + 8, x - 15);
        printw("按任意键继续...");
        refresh();
        getch();
    } else {
        show_operation_result("订单添加失败！");
    }

    saveOrdersMemToDat(orderManager, "/home/user/orders.dat");
}

//显示所有订单
void list_order() {
    int y, x;
    getmaxyx(stdscr, y, x);
    clear();
    bkgd(COLOR_PAIR(3));
    //读取order.dat
    //每次list都读取一次
    loadOrdersDatToMem(orderManager, "/home/user/orders.dat");
    if (orderManager->count == 0) {
        show_operation_result("暂无订单信息！");
        return;
    }
    // 英文表头左对齐
    const char* fmt = "%-8s %-8s %-10s %-18s %-10s %-6s";
    mvprintw(0, 0, fmt, "OrderID", "FlightID", "Name", "IDNumber", "Orderer", "Status");
    mvprintw(1, 0, "---------------------------------------------------------------");
    int row = 2;
    for (int i = 0; i < orderManager->count; ++i, ++row) {
        Order* o = &orderManager->orders[i];
        const char* status_str = (o->status == 0) ? "正常" : "已退票";
        char info[256];
        snprintf(info, sizeof(info), fmt, o->orderID, o->flightID, o->customerName, o->IDNumber, o->ordererName, status_str);
        mvprintw(row, 0, "%s", info);
    }
    char page_info[64];
    snprintf(page_info, sizeof(page_info), "共%d条订单，按任意键返回", orderManager->count);
    mvprintw(y - 2, (x - strlen(page_info)) / 2, "%s", page_info);
    refresh();
    getch();
}


// 检验航班号格式并检查航班是否存在
int validateFlight(const char* flightID) {
    // 检查航班号格式
    if (!isValidFlightNumber(flightID)) {
        show_operation_result("无效的航班号格式！前两位应为大写字母，后四位应为数字");
        return 0;
    }

    // 检查航班号是否存在，若不存在则报错
    int hashIndex = hashFunction(flightID, manager->hashSize);
    HashNode* current = manager->hashTable[hashIndex];
    while (current) {
        if (strcmp(current->flightID, flightID) == 0) {
            // 找到匹配的航班号，返回成功
            return 1;
        }
        current = current->next;
    }
    // 遍历完所有节点都没找到，返回失败
    show_operation_result("未查询到该航班信息！");
    return 0;
}


//退票函数
void del_order() {
    char orderID[6];
    int y, x;
    get_center_position(&y, &x);
    clear_and_move(y - 2, x - 15);
    printw("请输入要退票的订单号: ");
    refresh();
    echo();
    getnstr(orderID, 6);
    noecho();

    if (refundOrder(orderManager, orderID)) {
        // 退票成功，找到订单对应航班并加一
        int idx = findOrderByID(orderManager, orderID);
        if (idx != -1) {
            char* flightID = orderManager->orders[idx].flightID;
            int hashIndex = hashFunction(flightID, manager->hashSize);
            HashNode* current = manager->hashTable[hashIndex];
            while (current) {
                if (strcmp(current->flightID, flightID) == 0) {
                    manager->flights[current->index].remSeats++;
                    break;
                }
                current = current->next;
            }
        }
        saveOrdersMemToDat(orderManager, "/home/user/orders.dat");
        show_operation_result("退票成功！");
    } else {
        show_operation_result("退票失败，订单号无效或已退票！");
    }
}

// 按下单人查询订单
void find_orders_by_user() {
    char ordererName[50];
    int y, x;
    get_center_position(&y, &x);
    clear();
    bkgd(COLOR_PAIR(3));
    move(y - 4, x - 15);
    printw("----- 按下单人查询订单 -----");
    move(y - 2, x - 30);
    printw("请输入下单人姓名: ");
    refresh();
    echo();
    getnstr(ordererName, sizeof(ordererName) - 1);
    noecho();
    // 收集所有该下单人的订单索引
    int idxs[1024];
    int count = 0;
    for (int i = 0; i < orderManager->count && count < 1024; ++i) {
        if (strcmp(orderManager->orders[i].ordererName, ordererName) == 0) {
            idxs[count++] = i;
        }
    }
    if (count == 0) {
        show_operation_result("未找到该下单人的订单！");
        return;
    }
    getmaxyx(stdscr, y, x);
    clear();
    bkgd(COLOR_PAIR(3));
    // 英文表头左对齐
    const char* fmt = "%-8s %-8s %-10s %-18s %-10s %-6s";
    mvprintw(0, 0, fmt, "OrderID", "FlightID", "Name", "IDNumber", "Orderer", "Status");
    mvprintw(1, 0, "---------------------------------------------------------------");
    int row = 2;
    for (int i = 0; i < count; ++i, ++row) {
        Order* o = &orderManager->orders[idxs[i]];
        const char* status_str = (o->status == 0) ? "正常" : "已退票";
        char info[256];
        snprintf(info, sizeof(info), fmt, o->orderID, o->flightID, o->customerName, o->IDNumber, o->ordererName, status_str);
        mvprintw(row, 0, "%s", info);
    }
    char page_info[64];
    snprintf(page_info, sizeof(page_info), "共%d条订单，按任意键返回", count);
    mvprintw(y - 2, (x - strlen(page_info)) / 2, "%s", page_info);
    refresh();
    getch();
}

// 按订单号查询订单
void find_orders_by_id() {
    char orderID[6];
    int y, x;
    get_center_position(&y, &x);
    clear();
    bkgd(COLOR_PAIR(3));
    move(y - 6, x - 10);
    printw("----- 按订单号查询订单 -----");
    move(y - 4, x - 30);
    printw("请输入要查询的订单号 (5位数字): ");
    refresh();
    echo();
    getnstr(orderID, 6);
    noecho();

    if (!isValidOrderNumber(orderID)) {
        show_operation_result("无效的订单号格式！");
        return;
    }
    int idx = findOrderByID(orderManager, orderID);
    if (idx == -1) {
        show_operation_result("未找到该订单！");
        return;
    }
    Order* o = &orderManager->orders[idx];
    // 查找航班信息
    Flight* f = NULL;
    int hashIndex = hashFunction(o->flightID, manager->hashSize);
    HashNode* cur = manager->hashTable[hashIndex];
    while (cur) {
        if (strcmp(cur->flightID, o->flightID) == 0) {
            f = &manager->flights[cur->index];
            break;
        }
        cur = cur->next;
    }
    clear();
    bkgd(COLOR_PAIR(3));
    move(y - 8, x - 20);
    printw("----- 订单信息 -----");
    move(y - 6, x - 20);
    printw("订单号：%s", o->orderID);
    move(y - 4, x - 20);
    printw("航班号：%s", o->flightID);
    move(y - 2, x - 20);
    printw("乘客姓名：%s", o->customerName);
    move(y, x - 20);
    printw("身份证号：%s", o->IDNumber);
    move(y + 2, x - 20);
    printw("下单人：%s", o->ordererName);
    move(y + 4, x - 20);
    printw("状态：%s", o->status == 0 ? "正常" : "已退票");
    if (f) {
        move(y + 6, x - 20);
        printw("起飞城市：%s", f->depCity);
        move(y + 8, x - 20);
        printw("抵达城市：%s", f->arrCity);
        move(y + 10, x - 20);
        printw("起飞时间：%s", f->depTime);
        move(y + 12, x - 20);
        printw("抵达时间：%s", f->arrTime);
    }
    move(y + 14, x - 15);
    printw("按任意键返回...");
    refresh();
    getch();
}