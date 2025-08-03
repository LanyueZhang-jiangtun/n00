#include "../include/admin.h"
#include "../include/menu.h"
#include "../include/data.h"
#include "../include/order.h"
extern OrderManager* orderManager;
#include "../include/cust.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <curses.h>
#include <wchar.h>
#include <locale.h>

extern RouteHashTable* routeTable;

// 清除当前屏幕并将光标移动到指定位置
void clear_and_move(int y, int x) {
    clear();
    bkgd(COLOR_PAIR(3));
    move(y, x);
    refresh();
}

// 获取屏幕中心位置
void get_center_position(int *y, int *x) {
    getmaxyx(stdscr, *y, *x);
    *y /= 2;
    *x /= 2;
}

void clearInputBuffer() {
    int c;
    while ((c = getch()) != '\n' && c != EOF );
}

// 显示初始页面（选择角色）
void show_initial_page() {
    int y, x;
    get_center_position(&y, &x);
    clear();
    bkgd(COLOR_PAIR(3));
    setlocale(LC_ALL, ""); // 保证宽字符支持
    // 顶部彩色打印“民航订票系统”
    const wchar_t* title = L"民航订票系统";
    int colors[6] = {3, 2, 4, 5, 6, 7}; // 黄——黑、蓝、红、绿、品红、青
    int title_len = 6;
    int start_col = x - title_len; // 每个汉字宽度2
    for (int i = 0; i < title_len; ++i) {
        attron(COLOR_PAIR(colors[i]));
        mvaddnwstr(y - 7, start_col + i * 2, &title[i], 1);
        attroff(COLOR_PAIR(colors[i]));
    }
    // 菜单项
    move(y - 3, x - 10);
    printw("1 - 管理员");
    move(y - 1, x - 10);
    printw("2 - 客户");
    move(y + 1, x - 10);
    printw("0 - 退出系统");
    move(y + 3, x - 15);
    printw("请输入选择: ");
    refresh();
}

// 显示管理员菜单
void show_admin_menu() {
    int y, x;
    get_center_position(&y, &x);
    ++y;
    clear_and_move(y - 6, x - 12);
    printw("1 - 录入航班");
    move(y - 4, x - 12);
    printw("2 - 修改航班信息");
    move(y - 2, x - 12);
    printw("3 - 按航班号查询航班");
    move(y, x - 12);
    printw("4 - 按航线查询航班");
    move(y + 2, x - 12);
    printw("5 - 删除航班");
    move(y + 4, x - 12);
    printw("6 - 列出航班");
    move(y + 6, x - 12);
    printw("7 - 列出订单");
    move(y + 8, x - 12);
    printw("0 - 退出系统");
    move(y + 10, x - 12);
    printw("00 - 返回初始页面");
    move(y + 12, x - 15);
    printw("请输入选择: ");
    refresh();
}

// 显示客户菜单
void show_customer_menu() {
    int y, x;
    get_center_position(&y, &x);
    clear_and_move(y - 6, x - 14);
    printw("1 - 按航班号查询航班");
    move(y - 4, x - 14);
    printw("2 - 按航线查询航班");
    move(y - 2, x - 14);
    printw("3 - 订票");
    move(y, x - 14);
    printw("4 - 退票");
    move(y + 2, x - 14);
    printw("5 - 按下单人查询订单");
    move(y + 4, x - 14);
    printw("6 - 按订单号查询订单");
    move(y + 6, x - 14);
    printw("0 - 退出系统");
    move(y + 8, x - 14);
    printw("00 - 返回初始页面");
    move(y + 10, x - 15);
    printw("请输入选择: ");
    refresh();
}

// 显示操作结果页面
void show_operation_result(const char* message) {
    int y, x;
    get_center_position(&y, &x);
    
    clear_and_move(y, x - strlen(message) / 2);
    printw("%s", message);
    
    move(y + 2, x - 15);
    printw("按任意键继续...");
    refresh();
    getch(); // 等待用户按键
}

// 处理管理员操作
void handle_admin_operation(int choice) {
    switch(choice) {
        case 1:
            add_flight();
            break;
        case 2:
            modify_flight();
            break;
        case 3:
            find_flight_by_id();
            break;
        case 4:
            find_flight_by_route();
            break;
        case 5:
            del_flight();
            break;
        case 6:
            list_flight(manager);
            break;
        case 7:
            list_order();
            break;
    }
}

// 处理客户操作
void handle_customer_operation(int choice) {
    switch(choice) {
        case 1:
            find_flight_by_id();
            break;
        case 2:
            find_flight_by_route();
            break;
        case 3:
            add_order();
            break;
        case 4:
            del_order();
            break;
        case 5:
            find_orders_by_user();
            break;
        case 6:
            find_orders_by_id();
            break;
    }
}

// 列出所有航班信息（左对齐、严格列对齐，变量名规范）
void list_flight(FlightManager* manager) {
    if (manager->count == 0) { // flightCount 改为 count
        show_operation_result("暂无航班信息！");
        return;
    }

    // 获取窗口尺寸（y=行数, x=列数）
    int y, x;
    getmaxyx(stdscr, y, x);

    // 定义标题和列宽（固定宽度格式化，确保对齐）

    const char* title = "航班号 | 起飞城市 | 抵达城市 |      起飞时间      |      抵达时间      | 基础票价 | 折扣 | 总座位 | 剩余座位 | 当前票价";
    const char* fmt = "%-6s | %-8s | %-8s | %-18s | %-18s | %-8.2f | %-4.1f | %-6d | %-8d | %.2f";
    // 分页参数（每页显示行数 = 窗口行数 - 标题/底部预留行）
    const int per_page = y - 6; // 预留标题、分隔线、底部选项的空间
    int current_page = 1;
    // 计算总页数，向上取整
    int total_pages = (manager->count + per_page - 1) / per_page;

    // 循环分页显示航班信息
    while (1) {
        clear();

        // 打印标题（左对齐，第0行顶格显示）
        mvprintw(0, 0, "%s", title);
        refresh();

        // 打印分隔线（增强可读性，第1行显示）
        mvprintw(1, 0, "-----------------------------------------------------------------------------");
        refresh();

        // 确定当前页要显示的航班起始和结束索引
        int start = (current_page - 1) * per_page;
        int end = start + per_page;
        if (end > manager->count) {
            end = manager->count;
        }

        // 逐行打印当前页的航班数据（从第3行开始显示内容）
        for (int i = start, line = 3; i < end; i++, line++) {
            Flight* f = &manager->flights[i];
            char info[200];
            // 按照格式字符串拼接航班信息
            snprintf(info, sizeof(info), fmt,
                     f->flightID,
                     f->depCity,
                     f->arrCity,
                     f->depTime,
                     f->arrTime,
                     f->basePrice,
                     f->discount,
                     f->totalSeats,
                     f->remSeats,
                     f->basePrice * f->discount);
            // 在指定行打印航班信息，左对齐顶格
            mvprintw(line, 0, "%s", info);
        }

        // 打印底部交互选项（固定在倒数第2行，简单居中）
        char options[100] = "";
        if (current_page > 1) {
            strcat(options, "上一页(p) | ");
        }
        if (current_page < total_pages) {
            strcat(options, "下一页(n) | ");
        }
        strcat(options, "下一步(q)");
        // 计算选项居中显示的起始列，然后打印
        mvprintw(y - 2, (x - strlen(options)) / 2, "%s", options);

        refresh();

        // 处理用户输入，实现分页和退出功能
        int key = getch();
        if (key == 'p' && current_page > 1) {
            current_page--;
        } else if (key == 'n' && current_page < total_pages) {
            current_page++;
        } else if (key == 'q') {
            break;
        }
    }

    show_operation_result("已结束航班列表浏览");
}

// 按航班号查询航班（若存在，打印出航班信息）
void find_flight_by_id() {
    char flightID[7];
    Flight* flight = NULL;
    int y, x;
    get_center_position(&y, &x);

    // 显示标题
    clear_and_move(y - 6, x - 10);
    printw("----- 按航班号查询 -----");
    refresh();

    // 输入航班号
    move(y - 4, x - 30);
    printw("请输入要查询的航班号 (6位，前两位大写字母，后四位数字): ");
    refresh();
    echo();
    getnstr(flightID, 6);
    noecho();

    // 验证航班号格式
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

    // 显示查询结果
    if (!flight) {
        show_operation_result("未找到该航班！");
        return;
    }

    // 格式化显示航班信息
    clear();
    move(y - 8, x - 30);
    printw("----- 航班信息 -----");

    move(y - 6, x - 30);
    printw("航班号：%s", flight->flightID);

    move(y - 4, x - 30);
    printw("起飞城市：%s", flight->depCity);

    move(y - 2, x - 30);
    printw("抵达城市：%s", flight->arrCity);

    move(y, x - 30);
    printw("起飞时间：%s", flight->depTime);

    move(y + 2, x - 30);
    printw("抵达时间：%s", flight->arrTime);

    move(y + 4, x - 30);
    printw("基础票价：%.2f 元", flight->basePrice);

    move(y + 6, x - 30);
    printw("票价折扣：%.1f 折", flight->discount * 10);

    move(y + 8, x - 30);
    printw("总座位数：%d 个", flight->totalSeats);

    move(y + 10, x - 30);
    printw("剩余座位数：%d 个", flight->remSeats);

    move(y + 12, x - 30);
    printw("当前票价：%.2f 元", flight->basePrice * flight->discount);

    move(y + 14, x - 15);
    printw("按任意键返回...");
    refresh();
    getch();
}

void find_flight_by_route() {
    char depCity[50], arrCity[50];
    int y, x;
    get_center_position(&y, &x);

    // 输入起飞城市
    clear_and_move(y - 6, x - 10);
    printw("----- 按航线查询航班 -----");
    refresh();
    move(y - 4, x - 30);
    printw("请输入起飞城市: ");
    refresh();
    echo();
    getnstr(depCity, sizeof(depCity) - 1);
    noecho();

    // 输入抵达城市
    move(y - 2, x - 30);
    printw("请输入抵达城市: ");
    refresh();
    echo();
    getnstr(arrCity, sizeof(arrCity) - 1);
    noecho();

    // 查找所有符合条件的航班索引
    RouteNode* routeList = findRouteFlights(routeTable, depCity, arrCity);
    if (!routeList) {
        show_operation_result("未找到该航线的航班！");
        return;
    }

    // 统计数量，准备分页
    int flightIndexes[1024];
    int count = 0;
    for (RouteNode* p = routeList; p && count < 1024; p = p->next) {
        flightIndexes[count++] = p->flightIndex;
    }
    if (count == 0) {
        show_operation_result("未找到该航线的航班！");
        return;
    }

    // 分页展示
    int total_pages, per_page, current_page = 1;
    getmaxyx(stdscr, y, x);
    per_page = y - 6;
    total_pages = (count + per_page - 1) / per_page;
    const char* title = "航班号 | 起飞城市 | 抵达城市 |      起飞时间      |      抵达时间      | 基础票价 | 折扣 | 总座位 | 剩余座位 | 当前票价";
    const char* fmt = "%-6s | %-8s | %-8s | %-18s | %-18s | %-8.2f | %-4.1f | %-6d | %-8d | %.2f";
    while (1) {
        clear();
        mvprintw(0, 0, "%s", title);
        mvprintw(1, 0, "-----------------------------------------------------------------------------" );
        int start = (current_page - 1) * per_page;
        int end = start + per_page;
        if (end > count) end = count;
        for (int i = start, line = 3; i < end; i++, line++) {
            Flight* f = &manager->flights[flightIndexes[i]];
            char info[200];
            snprintf(info, sizeof(info), fmt,
                     f->flightID,
                     f->depCity,
                     f->arrCity,
                     f->depTime,
                     f->arrTime,
                     f->basePrice,
                     f->discount,
                     f->totalSeats,
                     f->remSeats,
                     f->basePrice * f->discount);
            mvprintw(line, 0, "%s", info);
        }
        char options[100] = "";
        if (current_page > 1) strcat(options, "上一页(p) | ");
        if (current_page < total_pages) strcat(options, "下一页(n) | ");
        strcat(options, "下一步(q)");
        mvprintw(y - 2, (x - strlen(options)) / 2, "%s", options);
        refresh();
        int key = getch();
        if (key == 'p' && current_page > 1) current_page--;
        else if (key == 'n' && current_page < total_pages) current_page++;
        else if (key == 'q') break;
    }
    show_operation_result("已结束航线航班浏览");
}

