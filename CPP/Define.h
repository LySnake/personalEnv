#ifndef __DEFINE_H__
#define __DEFINE_H__

#include <type_traits>

//辅助宏：用于把宏加载了再##操作
#define __MACRO_TO_STR(macro) #macro                         
#define __MACRO_TO_SYMBOL(macro0, macro1)  macro0##macro1


//计算arrar数据的长度
template<class _Type, std::size_t N>
constexpr auto SIZE_ARRAY(_Type(&)[N]) noexcept {
	return N;
}


//获取枚举类型的底层类型
template<class _Type>
constexpr auto toUType(_Type enumerator) noexcept
{
	static_assert(std::is_enum_v<_Type>, "Must be an enumerated type.");
	return static_cast<std::underlying_type_t<_Type>>(enumerator);
}


//禁用COPY操作
#define DISALLOW_COPY_OPERATOR(TypeName)\
    TypeName(const TypeName &) = delete; \
    TypeName &operator =(const TypeName &) = delete;

//禁用MOVE操作
#define DISALLOW_MOVE_OPERATOR(TypeName)\
	TypeName(TypeName &&) = delete;\
	TypeName &operator =(TypeName &&) = delete;

//禁用COPY和MOVE操作
#define DISALLOW_COPY_AND_MOVE_OPERATOR(TypeName) \
	DISALLOW_COPY_OPERATOR(TypeName)\
	DISALLOW_MOVE_OPERATOR(TypeName)

//默认MOVE操作
#define DEFAULT_COPY_OPERATOR(TypeName)\
    TypeName(const TypeName &) = default; \
    TypeName &operator =(const TypeName &) = default;

//默认MOVE操作
#define DEFAULT_MOVE_OPERATOR(TypeName)\
	TypeName(TypeName &&) = default;\
	TypeName &operator =(TypeName &&) = default;

//默认COPY和MOVE操作
#define DEFAULT_COPY_AND_MOVE_OPERATOR(TypeName) \
	DEFAULT_COPY_OPERATOR(TypeName)\
	DEFAULT_MOVE_OPERATOR(TypeName)

#endif //__DEFINE_H__