#ifndef CUST_H
#define CUST_H

#include "order.h"

void add_order();
void list_order();

int validateFlight(const char* flightID);  // 航班验证函数

void del_order();

// 按下单人查询订单
void find_orders_by_user();

// 按订单号查询订单
void find_orders_by_id();


#endif //CUST_H