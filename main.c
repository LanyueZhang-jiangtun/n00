#include <stdio.h>
#include <stdlib.h>
#include <curses.h>
#include <string.h>
#include <locale.h>       // 必须：支持 setlocale
#include <wchar.h>
#include <time.h>
#include "../include/menu.h"
#include "../include/data.h"
#include "../include/order.h"

FlightManager* manager;
RouteHashTable* routeTable;

OrderManager* orderManager;

//日期处理函数和删除、打折函数
void removePastFlights(FlightManager* manager);
void discountTodayFlights(FlightManager* manager);

// 验证管理员密码
int verify_admin_password();

int main() {
    //系统启动时，将flights.dat文件一次性加载到内存，并在内存中构建上述所有索引
    // 初始化系统，初始容量 10，哈希表大小 101
    manager = initManager(10, 101);
    routeTable = initRouteHashTable(101, 53);

    orderManager = initOrderManager(10, 101);

    //导入dat文件
    loadDatToMem(manager, "/home/user/fl0.dat");
    // 加载订单文件（若无则新建）
    FILE* f = fopen("/home/user/orders0.dat", "rb");
    if (f) {
        fclose(f);
        loadOrdersDatToMem(orderManager, "/home/user/orders0.dat");
    } else {
        // 文件不存在则新建空文件
        FILE* nf = fopen("/home/user/orders0.dat", "wb");
        if (nf) fclose(nf);
    }

    // 删除到达日期为昨天及以前的航班
    removePastFlights(manager);

    // 对今天的航班打五折
    discountTodayFlights(manager);

    setlocale(LC_ALL, "zh_CN.UTF-8");
    // 初始化ncurses
    initscr();
    cbreak(); // 禁用行缓冲，直接读取字符
    noecho(); // 默认不回显输入
    keypad(stdscr, TRUE); // 启用功能键
    curs_set(1); // 显示光标
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_YELLOW, COLOR_YELLOW); // 黄色字，黄底
        init_pair(2, COLOR_BLUE, COLOR_YELLOW);   // 蓝色字，黄底
        init_pair(3, COLOR_BLACK, COLOR_YELLOW);  // 黑色字，黄底（主色调）
        init_pair(4, COLOR_RED, COLOR_YELLOW);    // 红色字，黄底
        init_pair(5, COLOR_GREEN, COLOR_YELLOW);  // 绿色字，黄底
        init_pair(6, COLOR_MAGENTA, COLOR_YELLOW);// 品红字，黄底
        init_pair(7, COLOR_CYAN, COLOR_YELLOW);   // 青色字，黄底
    }
    bkgd(COLOR_PAIR(3));

    int running = 1;
    int current_page = 0; // 0: 初始页面, 1: 管理员页面, 2: 客户页面

    while (running) {
        switch (current_page) {
            case 0: // 初始页面
                show_initial_page();
                echo();
                char choice_str[10];
                getnstr(choice_str, sizeof(choice_str) - 1);
                noecho();

                int choice = atoi(choice_str);

                if (choice == 1) {
                    // 管理员登录
                    if (verify_admin_password()) {
                        current_page = 1;
                    } else {
                        show_operation_result("密码错误，返回初始页面");
                    }
                } else if (choice == 2) {
                    // 客户登录
                    current_page = 2;
                } else if (choice == 0) {
                    // 退出系统
                    running = 0;
                } else {
                    show_operation_result("无效选择，请重试");
                }
                break;

            case 1: // 管理员页面
                show_admin_menu();
                echo();
                getnstr(choice_str, sizeof(choice_str) - 1);
                noecho();

                if (strcmp(choice_str, "00") == 0) {
                    current_page = 0;
                } else {
                    int choice = atoi(choice_str);
                    if (choice == 0) {
                        running = 0;
                    } else if (choice >= 1 && choice <= 7) {
                        handle_admin_operation(choice);
                    } else {
                        show_operation_result("无效选择，请重试");
                    }
                }
                break;

            case 2: // 客户页面
                show_customer_menu();
                echo();
                getnstr(choice_str, sizeof(choice_str) - 1);
                noecho();

                if (strcmp(choice_str, "00") == 0) {
                    current_page = 0;
                } else {
                    int choice = atoi(choice_str);
                    if (choice == 0) {
                        running = 0;
                    } else if (choice >= 1 && choice <= 6) {
                        handle_customer_operation(choice);
                    } else {
                        show_operation_result("无效选择，请重试");
                    }
                }
                break;
        }
    }
    //导出到.dat文件
    saveMemToDat(manager, "/home/user/fl0.dat");
    saveOrdersMemToDat(orderManager, "/home/user/orders0.dat");

    // 清理ncurses
    endwin();
    freeRouteHashTable(routeTable);
    return 0;
}

int verify_admin_password() {
    int y, x;
    get_center_position(&y, &x);

    clear_and_move(y - 1, x - 15);
    printw("请输入管理员密码: ");
    refresh();

    char password[20];
    noecho();
    getnstr(password, sizeof(password) - 1);

    return strcmp(password, "www") == 0;
}

// 获取日期
time_t getDate(const char* timeStr) {
    struct tm tm = {0};
    sscanf(timeStr, "%d/%d/%d", &tm.tm_year, &tm.tm_mon, &tm.tm_mday);
    tm.tm_year += 100; // 年份从 1900 开始
    tm.tm_mon--;       // 月份从 0 开始
    tm.tm_hour = 0;
    tm.tm_min = 0;
    tm.tm_sec = 0;
    return mktime(&tm);
}

// 删除到达日期为昨天及以前的航班
void removePastFlights(FlightManager* manager) {
    time_t now = time(NULL);
    struct tm* local = localtime(&now);
    local->tm_mday--;
    time_t yesterday = mktime(local);

    int newCount = 0;
    for (int i = 0; i < manager->count; i++) {
        time_t arrTime = getDate(manager->flights[i].arrTime);
        if (arrTime > yesterday) {
            manager->flights[newCount] = manager->flights[i];
            newCount++;
        }
    }
    manager->count = newCount;
}

// 对今天的航班打五折
void discountTodayFlights(FlightManager* manager) {
    time_t now = time(NULL);
    struct tm* local = localtime(&now);
    local->tm_hour = 0;
    local->tm_min = 0;
    local->tm_sec = 0;
    time_t today = mktime(local);

    for (int i = 0; i < manager->count; i++) {
        time_t depTime = getDate(manager->flights[i].depTime);
        if (depTime == today) {
            manager->flights[i].discount = 0.5;
        }
    }
}