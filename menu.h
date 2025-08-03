#ifndef AIRLINERESERVATIONSYSTEM_MENU_H
#define AIRLINERESERVATIONSYSTEM_MENU_H

#include <curses.h>
#include "../include/data.h"

// 清除当前屏幕并将光标移动到指定位置
void clear_and_move(int y, int x);

// 获取屏幕中心位置
void get_center_position(int *y, int *x);

void clearInputBuffer();

// 显示初始页面（选择角色）
void show_initial_page();

// 显示管理员菜单
void show_admin_menu();

// 显示客户菜单
void show_customer_menu();

// 显示操作结果页面
void show_operation_result(const char* message);

// 处理管理员操作
void handle_admin_operation(int choice);

// 处理客户操作
void handle_customer_operation(int choice);

// 列出所有航班
void list_flight(FlightManager* manager);

// 按航班号查询航班
void find_flight_by_id();

// 按航线查询航班
void find_flight_by_route();

// 按下单人查询订单
void find_orders_by_user();

// 按订单号查询订单
void find_orders_by_id();

#endif //AIRLINERESERVATIONSYSTEM_MENU_H