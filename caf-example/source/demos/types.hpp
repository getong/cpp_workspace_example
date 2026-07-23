#pragma once

// 本文件演示 CAF 的"自定义消息类型"机制:
//
// 1. CAF 中 actor 之间只能通过消息通信, 消息里的每一个类型都必须先注册一个
//    全局唯一的 type ID, 这样 CAF 才能对消息做类型检查/打印/(网络)序列化。
// 2. "原子"(atom) 是无载荷的标签类型, 常用来表达一个"动词"(add/get/open...),
//    使消息协议既自描述又能参与编译期匹配。
// 3. 自定义 struct 需要提供 inspect() 函数, CAF 用它实现序列化与 to_string。

#include <cstdint>
#include <string>

#include <caf/type_id.hpp>

namespace demo
{

/// 自定义消息类型: 一张订单。可以像内置类型一样在 actor 之间传递。
struct order
{
  uint32_t id = 0;
  std::string item;
  int32_t quantity = 0;
  double unit_price = 0.0;
};

/// inspect() 让 CAF 能够检查/序列化/打印 order 类型
template<class Inspector>
auto inspect(Inspector& inspector, order& obj) -> bool
{
  return inspector.object(obj).fields(
      inspector.field("id", obj.id),
      inspector.field("item", obj.item),
      inspector.field("quantity", obj.quantity),
      inspector.field("unit_price", obj.unit_price));
}

/// 自定义消息类型: 订单回执, 由商店 actor 作为应答返回
struct receipt
{
  uint32_t order_id = 0;
  double total = 0.0;
};

template<class Inspector>
auto inspect(Inspector& inspector, receipt& obj) -> bool
{
  return inspector.object(obj).fields(inspector.field("order_id", obj.order_id),
                                      inspector.field("total", obj.total));
}

}  // namespace demo

// 类型 ID 块: 从 caf::first_custom_type_id 开始为项目内的自定义类型分配 ID。
// main.cpp 中通过 CAF_MAIN(caf::id_block::caf_example) 完成运行时注册。
CAF_BEGIN_TYPE_ID_BLOCK(caf_example, caf::first_custom_type_id)

// ---- 原子(atom): 无载荷的"动词"消息 ----
CAF_ADD_ATOM(caf_example, demo, add_atom)
CAF_ADD_ATOM(caf_example, demo, sub_atom)
CAF_ADD_ATOM(caf_example, demo, mul_atom)
CAF_ADD_ATOM(caf_example, demo, div_atom)
CAF_ADD_ATOM(caf_example, demo, inc_atom)
CAF_ADD_ATOM(caf_example, demo, dec_atom)
CAF_ADD_ATOM(caf_example, demo, get_atom)
CAF_ADD_ATOM(caf_example, demo, open_atom)
CAF_ADD_ATOM(caf_example, demo, close_atom)
CAF_ADD_ATOM(caf_example, demo, status_atom)
CAF_ADD_ATOM(caf_example, demo, start_atom)
CAF_ADD_ATOM(caf_example, demo, tick_atom)
CAF_ADD_ATOM(caf_example, demo, ping_atom)
CAF_ADD_ATOM(caf_example, demo, pong_atom)
CAF_ADD_ATOM(caf_example, demo, buy_atom)
CAF_ADD_ATOM(caf_example, demo, join_atom)
CAF_ADD_ATOM(caf_example, demo, chat_atom)
CAF_ADD_ATOM(caf_example, demo, say_atom)

// ---- 自定义结构体 ----
CAF_ADD_TYPE_ID(caf_example, (demo::order))
CAF_ADD_TYPE_ID(caf_example, (demo::receipt))

CAF_END_TYPE_ID_BLOCK(caf_example)
