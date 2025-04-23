#ifndef DEFINE_H
#define DEFINE_H

#include <type_traits>

#define UNUSED(var_) (void)var_

// 宏:arg_转字符串
#define __MACRO_TO_STR(arg_) #arg_
// 宏:arg_0与arg_1拼接形成新的符号(arg_0与arg_1不可以是宏)
#define MACRO_CONCAT(arg_0, arg_1) arg_0##arg_1
// 宏:arg_0与arg_1拼接形成新的符号(arg_0与arg_1可以是宏)
#define MACRO_TO_SYMBOL(arg_0, arg_1) MACRO_CONCAT(arg_0, arg_1)

// C++17: 替换方案  https://zh.cppreference.com/w/cpp/iterator/size
// //计算arrar数据的长度
// template<class _Type, std::size_t N>
// constexpr auto SIZE_ARRAY(_Type(&)[N]) noexcept {
// 	return N;
// }

//获取枚举类型的底层类型得值
template <class _Type>
constexpr auto toUType(_Type enumerator) noexcept
{
    static_assert(std::is_enum_v<_Type>, "Must be an enumerated type.");
    return static_cast<std::underlying_type_t<_Type>>(enumerator);
}

//禁用COPY操作
#define DISALLOW_COPY_OPERATOR(TypeName)                                                                               \
    TypeName(const TypeName &) = delete;                                                                               \
    TypeName &operator=(const TypeName &) = delete;

//禁用MOVE操作
#define DISALLOW_MOVE_OPERATOR(TypeName)                                                                               \
    TypeName(TypeName &&) = delete;                                                                                    \
    TypeName &operator=(TypeName &&) = delete;

//禁用COPY和MOVE操作
#define DISALLOW_COPY_AND_MOVE_OPERATOR(TypeName)                                                                      \
    DISALLOW_COPY_OPERATOR(TypeName)                                                                                   \
    DISALLOW_MOVE_OPERATOR(TypeName)

//默认MOVE操作
#define DEFAULT_COPY_OPERATOR(TypeName)                                                                                \
    TypeName(const TypeName &) = default;                                                                              \
    TypeName &operator=(const TypeName &) = default;

//默认MOVE操作
#define DEFAULT_MOVE_OPERATOR(TypeName)                                                                                \
    TypeName(TypeName &&) = default;                                                                                   \
    TypeName &operator=(TypeName &&) = default;

//默认COPY和MOVE操作
#define DEFAULT_COPY_AND_MOVE_OPERATOR(TypeName)                                                                       \
    DEFAULT_COPY_OPERATOR(TypeName)                                                                                    \
    DEFAULT_MOVE_OPERATOR(TypeName)

#endif // DEFINE_H