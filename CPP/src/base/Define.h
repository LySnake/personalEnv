#ifndef DEFINE_H
#define DEFINE_H

#include <type_traits>

#define UNUSED(var_) (void)var_

// 宏:转字符串
#define MAKE_STR(arg_) #arg_

// 宏:label_与L拼接形成新的符号(label_与L不可以是宏)
#define MAKE_LABEL2(label_, L) label_##L
// 宏:label_与L拼接形成新的符号(label_与L可以是宏)
#define MAKE_LABEL1(label_, L)  MAKE_LABEL2(label_, L)
// 宏:生成唯一标识符
#define UNIQUE_IDENTIFIER(label_) MAKE_LABEL1(label_, __LINE__)

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