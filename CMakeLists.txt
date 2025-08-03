cmake_minimum_required(VERSION 3.10)
project(n C)

#set(CMAKE_C_STANDARD 11)

#add_executable(f main.c)

# 这三行放在find_package前面
SET(NCURSES_LIBRARY_NAME "ncurses")
set(CURSES_NEED_NCURSES TRUE)
set(CURSES_NEED_WIDE TRUE)

# 寻找 ncurses 库
find_package(Curses REQUIRED)
include_directories(${CURSES_INCLUDE_DIR})

# 包含所有源文件
aux_source_directory(. SOURCES)

# 源文件列表
set(SOURCES
        src/main.c
        src/menu.c
        src/admin.c
        src/cust.c
        src/data.c
        src/order.c
)

add_executable(n ${SOURCES})

# 链接 ncurses 库
target_link_libraries(n ${CURSES_LIBRARIES})

# 设置 C 标准
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

#aio