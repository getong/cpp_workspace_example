#pragma once

// 这个头文件被多个编译单元包含（greeting.cpp 和 main.cpp）。
// 修改它时，Ninja 会根据 depfile 找出所有包含它的编译单元并重新编译，
// 而没有包含它的 add.cpp / mul.cpp 不会被动到。
inline constexpr auto demo_version = "1.0.0";
