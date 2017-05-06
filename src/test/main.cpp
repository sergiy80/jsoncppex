// main.cpp

#include <iostream>
#include "jsonex.h"

using namespace utils;

class CSubObjType; // forward declaration required

// define your json's data specialization class
template<> struct Json::JsonExDataTraits<CSubObjType>
{
	enum data_enum : size_t
	{
		AttrA = 0, AttrB = 1, AttrV = 2
	};

	using data_type = std::tuple<int, int, Nullable<std::array<int, 3>> >;
	using attr_type = JsonExAttributes::attr_type;
	using data_attrs = std::array<attr_type, std::tuple_size<data_type>::value>;

	static const data_attrs& attributes()
	{
		static const data_attrs attrs
		{
			{
				attr_type(std::string("a")), attr_type(std::string("b")), attr_type(std::string("v"))
			}
		};
		return attrs;
	}
};

// define your json's sub object type
class CSubObjType : public Json::JsonEx<CSubObjType>
{
public:
	CSubObjType() = default;
	explicit CSubObjType(const data_type& other) : base_type(other) {}
	explicit CSubObjType(data_type&& other) : base_type(std::forward<data_type>(other)) {}
};

class CMainType; // forward declaration	required

// define your main json's data specialization class
template<> struct Json::JsonExDataTraits<CMainType>
{
	enum data_enum : size_t
	{
		AttrBoolValue = 0, AttrUIntValue = 1, AttrVec = 2,
		AttrObj = 3, AttrVecObj = 4, AttrVecObjFixedSize = 5
	};

	using data_type = std::tuple<
		bool, Nullable<unsigned int>, std::vector<int>,
		Nullable<CSubObjType>, Nullable< std::vector<CSubObjType>>, Nullable< std::array<CSubObjType, 2>> >;

	using attr_type = JsonExAttributes::attr_type;

	using data_attrs = std::array<attr_type, std::tuple_size<data_type>::value>;

	static const data_attrs& attributes()
	{
		static const data_attrs attrs
		{
			{
				attr_type(std::string("boolVal")), attr_type(std::string("uintVal")), attr_type(std::string("vec")),
				attr_type(std::string("obj")), attr_type(std::string("vecObj")), attr_type(std::string("vecObjFixedSize"))
			}
		};
		return attrs;
	}
};

// define your main json's object type
class CMainType : public Json::JsonEx<CMainType>
{
public:
	CMainType() = default;
	explicit CMainType(const data_type& other) : base_type(other) {}
	explicit CMainType(data_type&& other) : base_type(std::forward<data_type>(other)) {}
};


void TestJsonEx()
{
	CMainType jsonObj;

	std::string sjson =
		"{"
		" \"boolVal\": true, \"uintVal\": \"123\","
		" \"vec\": [1,2,3],"
		//" \"obj\": {\"a\": 11, \"b\": 22},"
		//" \"vecObj\": [{\"a\": 44, \"b\": 55, \"v\": [6,7,8]},{\"a\": 66, \"b\": 77},{\"a\": 88, \"b\": 99, \"v\": null}],"
		//" \"vecObjFixedSize\": [{\"a\": 444, \"b\": 545},{\"a\": 666, \"b\": 777}]"
		" \"foo\": null"
		"}";

	bool b = jsonObj.load(sjson);

	std::cout << "JSON load: Ok = " << std::boolalpha << b << ", Last error: " << jsonObj.lastError() << std::endl;

	if (b)
	{
		std::cout << "JSON string: " << jsonObj.getJsonString() << std::endl;
	}
	else
	{
		std::cout << "JSON Error Attribute: " << jsonObj.errorInfo() << std::endl;

	}

	std::cout << std::endl << "Creating JSON:" << std::endl;

	CMainType j2;
	std::get<CMainType::data_enum::AttrBoolValue>(j2.data()) = true;
	std::get<CMainType::data_enum::AttrUIntValue>(j2.data()) = 23456;
	std::get<CMainType::data_enum::AttrVec>(j2.data()) = { 8, 7, 6, 5, 4, 3, 2, 1 };
	std::get<CMainType::data_enum::AttrVecObj>(j2.data()) = nullptr;
	std::get<CMainType::data_enum::AttrVecObjFixedSize>(j2.data()) = nullptr;

	std::string s = j2.getJsonString();
	b = j2.lastError().empty();
	if (b)
	{
		std::cout << "JSON string: " << s << std::endl;
	}
	else
	{
		std::cout << "JSON Error Attribute: " << j2.errorInfo() << std::endl;

	}
	std::cout << std::endl;

	auto s0data = std::make_tuple(123, 456, std::array<int, 3>{ {7, 6, 5}});
	CSubObjType s0(s0data);

	std::cout << "CTestSubObjType JSON string: " << s0.getJsonString(false) << std::endl;

	CMainType j3(std::make_tuple(true, 7890, std::vector<int>({ 11, 22, 33, 44, 55 }), nullptr, nullptr, nullptr));
	std::cout << "\nCMainType JSON string, streamed (styled): \n" << Json::styled << j3 << std::endl;

	//Nullable<CSubObjType> nt(CSubObjType(CSubObjType::data_type(111, 222, std::array<int, 3>{{ 1, 2, 3 }})));
	Nullable<CSubObjType> nt(CSubObjType(CSubObjType::data_type(111, 222, nullptr)));
	std::cout << "Nullable test, streamed (non-styled): " << std::endl << Json::nostyled << nt << std::endl;
}

int main(int /*argc*/, char* /*argv*/[])
{
	std::cout << "Begin." << std::endl;

	TestJsonEx();

	std::cout << std::endl << "End. Press enter to exit." << std::endl;
	getchar();
	return 0;
}
