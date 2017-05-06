// tuple_utils.h
#pragma once

//#pragma warning(push)
//#pragma warning(disable:4503) // decorated name length exceeded, name was truncated

#include <tuple>
#include <utility>

#pragma pack(push, 8)

namespace utils
{

///////////////////////////////////////////////////////////////////////////////

// enumerate tuple
namespace
{

template<typename TupleType, typename FunctionType>
static void for_each_helper(TupleType&&, FunctionType, std::integral_constant<size_t, std::tuple_size<typename std::remove_reference<TupleType>::type >::value>)
{
}

template<const size_t I, typename TupleType, typename FunctionType,
	typename = typename std::enable_if<I != std::tuple_size<typename std::remove_reference<TupleType>::type>::value>::type >
	static void for_each_helper(TupleType&& t, FunctionType F, std::integral_constant<size_t, I> K)
{
	F(K, std::get<I>(t));
	for_each_helper(std::forward<TupleType>(t), F, std::integral_constant<size_t, I + 1>());
}

template<typename TupleType, typename FunctionType>
static size_t find_if_helper(TupleType&&, FunctionType, std::integral_constant<size_t, std::tuple_size<typename std::remove_reference<TupleType>::type >::value> v)
{
	return v.value;
}

template<const size_t I, typename TupleType, typename FunctionType,
	typename = typename std::enable_if<I != std::tuple_size<typename std::remove_reference<TupleType>::type>::value>::type >
	static size_t find_if_helper(TupleType&& t, FunctionType F, std::integral_constant<size_t, I> K)
{
	if (F(K, std::get<I>(t))) return I;
	return find_if_helper(std::forward<TupleType>(t), F, std::integral_constant<size_t, I + 1>());
}

} // noname namespace

/*
Using tuple enumeration:
//struct my_functor
//{ // functor called from tuple eumeration
//	template<typename T> void operator() (const size_t index, T&& v)
//	{ std::cout << "index: " << index << ", value: " << v << std::endl; }
//}; // end
//auto t = std::make_tuple(1, 2, "abc", "def", 4.0f);
//for_each(t, my_functor());
*/
template<typename TupleType, typename FunctionType> void for_each(TupleType&& t, FunctionType f)
{
	for_each_helper(std::forward<TupleType>(t), f, std::integral_constant<size_t, 0>());
}

/*
Returns tuple index if found, or tuple size if not found
//Using tuple enumeration:
//struct my_functor
//{
//	template<typename T> size_t operator() (const size_t index, const T& v)
//	{
//		std::cout << "index: " << index << ", value: " << v << std::endl;
//		return std::is_same<decltype(v), const int&>::value;
//	}
//};
//auto t = std::make_tuple(1hu, 2, "abc", "def", 4.0f);
//size_t i = find_if(t, my_functor());
//bool bFound = i < std::tuple_size<decltype(t)>::value;
*/
template<typename TupleType, typename FunctionType> size_t find_if(TupleType&& t, FunctionType f)
{
	return find_if_helper(std::forward<TupleType>(t), f, std::integral_constant<size_t, 0>());
}

///////////////////////////////////////////////////////////////////////////////

}

#pragma pack(pop)

//#pragma warning(pop)
