// jsonex.h
#pragma once

//#pragma warning(push)
//#pragma warning(disable:4503) // decorated name length exceeded, name was truncated

#include <string>
#include <memory>
#include <istream>
#include <ostream>
#include <sstream>
#include <tuple>
#include <vector>
#include <array>
#include <stdexcept>
#include <functional>

#include <json/json.h>

static_assert(JSONCPP_VERSION_HEXA >= ((1 << 24) | (8 << 16) | (0 << 8)), "JsonCPP library must be 1.8.0 or later version.");

#include "details/nullable.h"
#include "details/tuple_utils.h"

#pragma pack(push, 8)

namespace Json
{

// Contain basic load/read/write json files functionality
class JsonExBase
{
public:
	JsonExBase() = default;
	virtual ~JsonExBase() = default;

public:
	// returns string representation of the current json object.
	std::string getJsonString(bool styled = true) const;

	// returns json object of the class instance. By default - empty object
	Json::Value getJsonValue() const;

	// load and parse json object from a stream.
	// returns parse status.
	bool load(std::istream &is);
	// load and parse json object from a string.
	// returns parse status.
	bool load(const std::string &s);

	// write json object to a stream.
	bool write(std::ostream &os, bool styled = false) const;
	// write json object to a string.
	bool write(std::string &s, bool styled = false) const;

	// returns the last error message of load/write json object
	const std::string& lastError() const { return lastError_; }
protected:
	mutable std::string lastError_;

	// Validates this object against input json. Called before parse, and after create methods
	// By default does nothing.
	virtual bool validate(const Json::Value &) const { return true; };

	// Called when input json object should be applied to this object.
	// By default does nothing.
	virtual bool parse(const Json::Value &) { return true; };

	// Called when this object should be converted into a json object.
	// By default does nothing.
	virtual bool create(Json::Value &) const { return true; };
};

inline std::string JsonExBase::getJsonString(bool styled/* = true*/) const
{
	std::string s;
	write(s, styled);
	return std::move(s);
}

// returns json object of the class instance. By default - empty object
inline Json::Value JsonExBase::getJsonValue() const
{
	Json::Value v;
	create(v);
	return std::move(v);
}

inline bool JsonExBase::load(const std::string &s)
{
	std::istringstream iss(s);
	return load(iss);
}

inline bool JsonExBase::load(std::istream &is)
{
	Json::Value value;
	lastError_.clear();
	try
	{
		is >> value;
		if (!validate(value)) throw std::invalid_argument("Input json object is not valid");
		if (!parse(value)) throw std::invalid_argument("Json object cannot be parsed");
	}
	catch (std::exception &e)
	{
		lastError_ = e.what();
		return false;
	}
	return true;
}

inline bool JsonExBase::write(std::string &s, bool styled) const
{
	std::ostringstream oss;
	if (!write(oss, styled)) return false;
	s = oss.str();
	return true;
}

inline bool JsonExBase::write(std::ostream &os, bool styled) const
{
	lastError_.clear();
	Json::Value v;
	try
	{
		if (!create(v)) throw std::runtime_error("Cannot create json object");
		if (!validate(v)) throw std::runtime_error("Created json object is not valid");
	}
	catch (std::exception& e)
	{
		lastError_ = e.what();
		return false;
	}

	Json::StreamWriterBuilder builder;
	builder["indentation"] = styled ? std::string("\t") : std::string();

	std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
	return writer->write(v, &os) == 0;
}

namespace
{

inline int getIndexJsonStyled()
{
	static int indexJsonStyled = std::ios_base::xalloc();
	return indexJsonStyled;
}

}

// stream manipulator to output styled json string
inline std::ios_base& styled(std::ios_base& iob)
{
	iob.iword(getIndexJsonStyled()) = 1;
	return iob;
}

// stream manipulator to output non styled json string, default mode
inline std::ios_base& nostyled(std::ios_base& iob)
{
	iob.iword(getIndexJsonStyled()) = 0;
	return iob;
}

// output json object into is stream
inline std::istream& operator>>(std::istream& is, JsonExBase& obj)
{
	if (!is.good()) return is;

	bool ok = obj.load(is);
	if (!ok) is.setstate(std::ios_base::badbit);
	return is;
}

// input json object from os stream
inline std::ostream& operator<<(std::ostream& os, const JsonExBase& obj)
{
	if (!os.good()) return os;
	bool styled = os.iword(getIndexJsonStyled()) != 0;
	bool ok = obj.write(os, styled);
	if (!ok) os.setstate(std::ios_base::badbit);
	return os;
}

// JsonExDataTraits specialization must be defined before JsonEx specialization
// The _ImplT main class must have forward declaration.
// Must be defined in the specialization:
//   data_type :-> a tuple with data types
//   data_attrs :-> an array of type JsonExAttributes::attr_type of count the tuple's size
//   static const data_attrs& attributes(); :-> static method returning attributes array for each tuple's type
//   enum data_enum: size_t; :-> enum with tuple's field indexes to access using JsonEx::data() method.
template<typename _ImplT> struct JsonExDataTraits
{
};

// Attribute definitions for JsonEx class
struct JsonExAttributes
{
	enum attr_enum: size_t
	{
		// name of json attribute
		AttrIndexName = 0
	};

	// tuple of item attributes
	typedef std::tuple<std::string> attr_type;
};

/*
Usage of the JsonEx class:

//class CSubObjType; // forward declaration required
//template<> struct Json::JsonExDataTraits<CSubObjType>
//{
//	enum data_enum: size_t
//	{
//		AttrA = 0, AttrB = 1, AttrV = 2
//	};
//
//	using data_type = std::tuple<int, int, utils::Nullable<std::array<int, 3>> >;
//	using attr_type = JsonExAttributes::attr_type;
//	using data_attrs = std::array<attr_type, std::tuple_size<data_type>::value>;
//
//	static const data_attrs& attributes()
//	{
//		static const data_attrs attrs
//		{
//			{
//				attr_type(std::string("a")), attr_type(std::string("b")), attr_type(std::string("v"))
//			}
//		};
//		return attrs;
//	}
//
//};
//class CSubObjType: public Json::JsonEx<CSubObjType>
//{
//public:
//	CSubObjType() = default;
//	explicit CSubObjType(const data_type& other): base_type(other) {}
//	explicit CSubObjType(data_type&& other): base_type(std::forward<data_type>(other)) {}
//};
//
//class CMainType; // forward declaration	required
//template<> struct Json::JsonExDataTraits<CMainType>
//{
//	enum data_enum: size_t
//	{
//		AttrBoolValue = 0, AttrUIntValue = 1, AttrVec = 2,
//		AttrObj = 3, AttrVecObj = 4, AttrVecObjFixedSize = 5
//	};
//
//	using data_type = std::tuple<bool, utils::Nullable<unsigned int>, std::vector<int>, utils::Nullable<CSubObjType>, utils::Nullable< std::vector<CSubObjType>>, utils::Nullable< std::array<CSubObjType, 2>> >;
//	using attr_type = JsonExAttributes::attr_type;
//
//	using data_attrs = std::array<attr_type, std::tuple_size<data_type>::value>;
//	static const data_attrs& attributes()
//	{
//		static const data_attrs attrs
//		{
//			{
//				attr_type(std::string("boolVal")), attr_type(std::string("uintVal")), attr_type(std::string("vec")),
//				attr_type(std::string("obj")), attr_type(std::string("vecObj")), attr_type(std::string("vecObjFixedSize"))
//			}
//		};
//		return attrs;
//	}
//};
//
//class CMainType: public Json::JsonEx<CMainType>
//{
//public:
//	CMainType() = default;
//	explicit CMainType(const data_type& other): base_type(other) {}
//	explicit CMainType(data_type&& other): base_type(std::forward<data_type>(other)) {}
//};
//
Code to use:

CMainType mainType;
bool b = mainType.load(jsonString);
std::string s = mainType.getJsonString();
*/

template<typename _ImplT, typename _Dt = JsonExDataTraits<_ImplT> >
class JsonEx: public JsonExBase
{
public:
	typedef _ImplT main_type;
	typedef JsonEx base_type;
	typedef _Dt data_traits;

	// tuple data type
	typedef typename data_traits::data_type data_type;
	// array of atributes for the data types
	typedef typename data_traits::data_attrs data_attrs;
	// enum to access data typle
	typedef typename data_traits::data_enum data_enum;
	// predefined attributes class
	typedef JsonExAttributes attr_traits;
	// attributes tuple type
	typedef typename attr_traits::attr_type attr_type;
	// enum to access attributes tuple
	typedef typename attr_traits::attr_enum attr_enum;

	static_assert(std::tuple_size<data_type>::value == std::tuple_size<data_attrs>::value, "invalid data_attrs array size");
	static_assert(std::is_enum<data_enum>::value, "invalid data_enum type");
	static_assert(std::is_enum<attr_enum>::value, "invalid attr_enum type");

public:
	JsonEx() = default;
	explicit JsonEx(const data_type& other): data_(other) {}
	explicit JsonEx(data_type&& other): data_(std::forward<data_type>(other)) {}
	~JsonEx() override = default;

	// static object types validation for json object
	static bool JsonValidate(const Json::Value &root, std::ostream& err)
	{
		std::ostringstream ss;
		size_t iInvalidField = utils::find_if(*static_cast<data_type*>(nullptr), FnTypeValidate<data_type>(root, data_traits::attributes(), ss));
		bool bValid = iInvalidField >= std::tuple_size<data_type>::value;
		if (!bValid)
		{
			err << "." << std::get<attr_enum::AttrIndexName>(data_traits::attributes()[iInvalidField]) << ss.str();
		}
		return bValid;
	}

	// parse input json object into output JsonEx specialized object
	static bool JsonParse(const Json::Value &root, JsonEx& obj, std::ostream& err)
	{
		std::ostringstream ss;
		bool bValid = JsonValidate(root, ss);
		if (!bValid)
		{
			err << ss.str();
			return false;
		}
		ss = std::ostringstream();
		size_t iInvalidField = utils::find_if(obj.data_, FnValueParse<data_type>(root, data_traits::attributes(), ss));
		bValid = iInvalidField >= std::tuple_size<data_type>::value;
		if (!bValid)
		{
			err << "." << std::get<attr_enum::AttrIndexName>(data_traits::attributes()[iInvalidField]) << ss.str();
			return false;
		}
		return bValid;
	}

	// creates json object from input JsonEx specialized object
	static bool JsonCreate(Json::Value &root, const JsonEx& obj, std::ostream& err)
	{
		std::ostringstream ss;
		Json::Value jsonObj(Json::objectValue);
		size_t iInvalidField = utils::find_if(obj.data_, FnValueCreate<data_type>(jsonObj, data_traits::attributes(), ss));
		bool bValid = iInvalidField >= std::tuple_size<data_type>::value;
		if (bValid)
		{
			root = jsonObj;
		}
		else
		{
			err << "." << std::get<attr_enum::AttrIndexName>(data_traits::attributes()[iInvalidField]) << ss.str();
		}
		return bValid;
	}

	// access to data object
	data_type& data() { return data_; }
	const data_type& data() const { return data_; }

	// contains error json path and a message, can be used to identify invalid entries
	const std::string& errorInfo() const
	{
		return errorInfo_;
	}

protected:
	// main data storage
	data_type data_;
	mutable std::string errorInfo_;

protected:
	bool validate(const Json::Value &root) const override
	{
		std::ostringstream err;
		bool bValid = JsonValidate(root, err);
		if (!bValid)
		{
			errorInfo_ = std::string("$") + err.str();
		}
		return bValid;
	}
	bool parse(const Json::Value &root) override
	{
		std::ostringstream err;
		bool bValid = JsonParse(root, *this, err);
		if (!bValid)
		{
			errorInfo_ = std::string("$") + err.str();
		}
		return bValid;
	}
	bool create(Json::Value &root) const override
	{
		std::ostringstream err;
		bool bValid = JsonCreate(root, *this, err);
		if (!bValid)
		{
			errorInfo_ = std::string("$") + err.str();
		}
		return bValid;
	}

protected:
	// store type and value in template	arguments
	template<typename _Ty, _Ty _Val> struct value_constant {};

	template<typename _Rt, typename _Ot>
	using MethodTypePointer = _Rt(_Ot::*)() const;

	template<typename _Rt, MethodTypePointer<_Rt, Json::Value> P>
	// pair for storing type and Json::Value method returns the type from json value
	using JsonMethodTypePointer = std::pair<_Rt, value_constant<MethodTypePointer<_Rt, Json::Value>, P>>;

	template<typename _Rt, MethodTypePointer<bool, Json::Value> P>
	using JsonValidateMethodTypePointer = std::pair<_Rt, value_constant<MethodTypePointer<bool, Json::Value>, P>>;

protected:
	template<typename T, typename std::enable_if< !(std::is_base_of<JsonExBase, T>::value || std::is_arithmetic<T>::value || std::is_same<T, std::string>::value) >::type * = nullptr>
	// main json type validation template method
	static bool JsonTypeValidate(const Json::Value& json, const attr_type& attr, std::ostream& err, const T&)
	{
		static_assert(false, "Json validation for this type not implemented");
	}

	template<typename T, typename std::enable_if< std::is_base_of<JsonExBase, T>::value>::type * = nullptr>
	// main json type validation template method
	static bool JsonTypeValidate(const Json::Value& json, const attr_type& attr, std::ostream& err, const T&)
	{
		return T::JsonValidate(json, err);
	}

	// utils::Nullable<T> overload json type validation
	template<typename T> static bool JsonTypeValidate(const Json::Value& json, const attr_type& attr, std::ostream& err, const utils::Nullable<T>&)
	{
		if (json.isNull()) return true;
		return JsonTypeValidate(json, attr, err, *static_cast<T*>(nullptr));
	}

	template<typename _Tt, typename _Need_t>
	// search the needed type in the tuple and update the value with called function
	struct FnValidateBasicTypes
	{
		FnValidateBasicTypes(const Json::Value &json, const attr_type& attr): json_(json), attr_(attr) {}

		template<typename T, MethodTypePointer<bool, Json::Value> P, typename std::enable_if<!std::is_same<T, _Need_t>::value>::type * = nullptr>
		bool operator() (const size_t, const JsonValidateMethodTypePointer<T, P>&) const { return false; };

		template<typename T, MethodTypePointer<bool, Json::Value> P, typename std::enable_if<std::is_same<T, _Need_t>::value>::type * = nullptr>
		bool operator() (const size_t, const JsonValidateMethodTypePointer<T, P>&) const
		{
			return std::mem_fn(P)(json_);
		};

		const Json::Value& json_;
		const attr_type& attr_;
	};

	// This method is instead of set of specialized functions like this:
	//template<> static bool JsonTypeValidate(const Json::Value& json, const attr_type& attr, const bool&)
	//{	return json.isBool(); }
	template<typename T, typename std::enable_if< std::is_arithmetic<T>::value || std::is_same<T, std::string>::value>::type * = nullptr>
	static bool JsonTypeValidate(const Json::Value& json, const attr_type& attr, std::ostream& err, const T&)
	{
		using TupleValidateJsonTypes = std::tuple<
			JsonValidateMethodTypePointer<bool, &Json::Value::isBool>,
			JsonValidateMethodTypePointer<int, &Json::Value::isInt>,
			JsonValidateMethodTypePointer<unsigned int, &Json::Value::isUInt>,
			JsonValidateMethodTypePointer<long long, &Json::Value::isInt64>,
			JsonValidateMethodTypePointer<unsigned long long, &Json::Value::isUInt64>,
			JsonValidateMethodTypePointer<double, &Json::Value::isDouble>,
			JsonValidateMethodTypePointer<std::string, &Json::Value::isString>
		>;

		// null is ok here as we do not use tuple's fields in the functor, only types
		size_t iFoundField = utils::find_if(*((TupleValidateJsonTypes*)nullptr), FnValidateBasicTypes<TupleValidateJsonTypes, T>(json, attr));
		bool bValid = iFoundField < std::tuple_size<TupleValidateJsonTypes>::value;
		if (!bValid) err << " -> invalid value type.";
		return bValid;
	}

	// vector<T> overload json type validation
	template<typename T> static bool JsonTypeValidate(const Json::Value& json, const attr_type& attr, std::ostream& err, const std::vector<T>&)
	{
		if (!json.isArray())
		{
			err << " -> invalid type, must be array.";
			return false;
		}
		// to call JsonTypeValidate we need only type, not actual value
		for (Json::ArrayIndex i = 0; i < json.size(); i++)
		{
			std::ostringstream ss;
			bool bValid = JsonTypeValidate(json[i], attr, ss, *static_cast<T*>(nullptr));
			if (!bValid)
			{
				err << "[" << i << "]" << ss.str();
				return false;
			}
		}
		return true;
	}

	// array overload json type validation
	template<typename T, size_t Size> static bool JsonTypeValidate(const Json::Value& json, const attr_type& attr, std::ostream& err, const std::array<T, Size>&)
	{
		if (!json.isArray())
		{
			err << " -> invalid type, must be fixed size (" << Size << ") array.";
			return false;
		}
		if (json.size() != Size)
		{
			err << " -> invalid fixed size array " << json.size() << " != " << Size << ".";
			return false;
		}
		// to call JsonTypeValidate we need only type, not actual value
		for (size_t i = 0; i < Size; i++)
		{
			std::ostringstream ss;
			bool bValid = JsonTypeValidate(json[static_cast<Json::ArrayIndex>(i)], attr, ss, *static_cast<T*>(nullptr));
			if (!bValid)
			{
				err << "[" << i << "]" << ss.str();
				return false;
			}
		}
		return true;
	}

	template<typename TupleType>
	// functor to call tuple validation
	struct FnTypeValidate
	{
		FnTypeValidate(const Json::Value &root, const data_attrs& names, std::ostream& err): root_(root), names_(names), err_(err) {}

		template<typename T, size_t _Index> bool operator() (std::integral_constant<size_t, _Index>, const T& value) const
		{
			const attr_type& attr = std::get<_Index>(names_);
			bool bValid = JsonTypeValidate(root_[std::get<attr_enum::AttrIndexName>(attr)], attr, err_, value);
			return !bValid;
		};

		const Json::Value& root_;
		const data_attrs& names_;
		std::ostream& err_;
	};

protected:
	template<typename T, typename std::enable_if< !(std::is_base_of<JsonExBase, T>::value ||
		std::is_arithmetic<T>::value || std::is_same<T, std::string>::value)>::type * = nullptr
	>
	// main json type validation template method
	static bool JsonValueParse(const Json::Value& json, const attr_type& attr, std::ostream& err, T&)
	{
		static_assert(false, "Json parsing for this type not implemented");
	}

	template<typename T, typename std::enable_if< std::is_base_of<JsonExBase, T>::value>::type* = nullptr>
	// main json type validation template method
	static bool JsonValueParse(const Json::Value& json, const attr_type& attr, std::ostream& err, T& obj)
	{
		return T::JsonParse(json, obj, err);
	}

	// utils::Nullable<T> overload json parsing
	template<typename T> static bool JsonValueParse(const Json::Value& json, const attr_type& attr, std::ostream& err, utils::Nullable<T>& obj)
	{
		if (json.isNull())
		{
			obj = nullptr;
			return true;
		}

		T v;
		bool bValid = JsonValueParse(json, attr, err, v);
		if (bValid) obj = std::move(v);
		return bValid;
	}

	template<typename _Tt, typename _Nt>
	// search the needed type in the tuple and update the value with called function
	struct FnParseBasicTypes
	{
		FnParseBasicTypes(const Json::Value &json, const attr_type& attr, _Nt& v): json_(json), attr_(attr), v_(v) {}

		template<typename T, MethodTypePointer<T, Json::Value> P, typename std::enable_if<!std::is_same<T, _Nt>::value>::type * = nullptr>
		bool operator() (const size_t, const JsonMethodTypePointer<T, P>&) const { return false; };

		template<typename T, MethodTypePointer<T, Json::Value> P, typename std::enable_if<std::is_same<T, _Nt>::value>::type * = nullptr>
		bool operator() (const size_t, const JsonMethodTypePointer<T, P>&) const
		{
			v_ = std::mem_fn(P)(json_);
			return true;
		};

		const Json::Value& json_;
		const attr_type& attr_;
		_Nt& v_;
	};

	template<typename T, typename std::enable_if< std::is_arithmetic<T>::value || std::is_same<T, std::string>::value>::type * = nullptr>
	static bool JsonValueParse(const Json::Value& json, const attr_type& attr, std::ostream& err, T& v)
	{
		using TupleParseJsonTypes = std::tuple<
			JsonMethodTypePointer<bool, &Json::Value::asBool>,
			JsonMethodTypePointer<int, &Json::Value::asInt>,
			JsonMethodTypePointer<unsigned int, &Json::Value::asUInt>,
			JsonMethodTypePointer<long long, &Json::Value::asInt64>,
			JsonMethodTypePointer<unsigned long long, &Json::Value::asUInt64>,
			JsonMethodTypePointer<double, &Json::Value::asDouble>,
			JsonMethodTypePointer<std::string, &Json::Value::asString>
		>;

		// null is ok here as we do not use tuple's fields in the functor, only types
		size_t iFoundField = utils::find_if(*((TupleParseJsonTypes*)nullptr), FnParseBasicTypes<TupleParseJsonTypes, T>(json, attr, v));
		bool bValid = iFoundField < std::tuple_size<TupleParseJsonTypes>::value;
		if (!bValid) err << " -> invalid value.";
		return bValid;
	}

	// vector<T> overload json value parse
	template<typename T> static bool JsonValueParse(const Json::Value& json, const attr_type& attr, std::ostream& err, std::vector<T>& value)
	{
		if (!json.isArray())
		{
			err << " -> invalid type, must be array.";
			return false;
		}
		value.clear();
		value.reserve(json.size());

		for (Json::ArrayIndex i = 0; i < json.size(); i++)
		{
			std::ostringstream ss;
			value.push_back(T());
			bool bValid = JsonValueParse(json[i], attr, ss, value.back());
			if (!bValid)
			{
				err << "[" << i << "]" << ss.str();
				return false;
			}
		}
		return true;
	}
	// fixed size array overload json value parse
	template<typename T, size_t Size> static bool JsonValueParse(const Json::Value& json, const attr_type& attr, std::ostream& err, std::array<T, Size>& value)
	{
		if (!json.isArray())
		{
			err << " -> invalid type, must be fixed size (" << Size << ") array.";
			return false;
		}
		if (json.size() != Size)
		{
			err << " -> invalid fixed size array " << json.size() << " != " << Size << ".";
			return false;
		}

		for (size_t i = 0; i < Size; i++)
		{
			std::ostringstream ss;
			bool bValid = JsonValueParse(json[static_cast<Json::ArrayIndex>(i)], attr, ss, value[i]);
			if (!bValid)
			{
				err << "[" << i << "]" << ss.str();
				return false;
			}
		}
		return true;
	}

	template<typename _Tt>
	// functor to call tuple parsing
	struct FnValueParse
	{
		FnValueParse(const Json::Value &root, const data_attrs& names, std::ostream& err): root_(root), names_(names), err_(err) {}

		template<typename T, size_t _Index> bool operator() (std::integral_constant<size_t, _Index>, T& value) const
		{
			const attr_type& attr = std::get<_Index>(names_);
			bool bValid = JsonValueParse(root_[std::get<attr_enum::AttrIndexName>(attr)], attr, err_, value);
			return !bValid;
		};

		const Json::Value& root_;
		const data_attrs& names_;
		std::ostream& err_;
	};

protected:
	template<typename T, typename std::enable_if<!(std::is_base_of<JsonExBase, T>::value || std::is_arithmetic<T>::value || std::is_same<T, std::string>::value)>::type * = nullptr>
	// main json creation template method
	static bool JsonValueCreate(Json::Value& json, const attr_type& attr, std::ostream& err, const T&)
	{
		static_assert(false, "Json creation for this type not implemented");
	}

	template<typename T, typename std::enable_if< std::is_base_of<JsonExBase, T>::value>::type * = nullptr>
	// json creation for JsonExBase based types/subtypes template method
	static bool JsonValueCreate(Json::Value& json, const attr_type& attr, std::ostream& err, const T& obj)
	{
		return T::JsonCreate(json, obj, err);
	}

	// utils::Nullable<T> overload json value create
	template<typename T> static bool JsonValueCreate(Json::Value& json, const attr_type& attr, std::ostream& err, const utils::Nullable<T>& obj)
	{
		if (!obj)
		{
			json = Json::Value::nullSingleton();
			return true;
		}
		return JsonValueCreate(json, attr, err, obj.value());
	}

	template<typename T, typename std::enable_if< std::is_arithmetic<T>::value || std::is_same<T, std::string>::value>::type * = nullptr>
	// json creation for arithmetic and string types template method
	static bool JsonValueCreate(Json::Value& json, const attr_type& attr, std::ostream& err, const T& value)
	{
		json = Json::Value(value);
		return true;
	}

	// vector<T> overload json value create
	template<typename T> static bool JsonValueCreate(Json::Value& json, const attr_type& attr, std::ostream& err, const std::vector<T>& value)
	{
		Json::Value jsonV(Json::arrayValue);
		for (size_t i = 0; i < value.size(); i++)
		{
			std::ostringstream ss;
			Json::Value v;
			if (JsonValueCreate(v, attr, ss, value[i]))
			{
				jsonV.append(std::move(v));
			}
			else
			{
				err << "[" << i << "]" << ss.str();
				return false;
			}
		}
		json.swap(jsonV);
		return true;
	}

	// fixed size array overload json value create
	template<typename T, size_t Size> static bool JsonValueCreate(Json::Value& json, const attr_type& attr, std::ostream& err, const std::array<T, Size>& value)
	{
		Json::Value jsonV(Json::arrayValue);
		//for (const T& item : value)
		for (size_t i = 0; i < Size; i++)
		{
			std::ostringstream ss;
			Json::Value v;
			if (JsonValueCreate(v, attr, ss, value[i]))
			{
				jsonV.append(std::move(v));
			}
			else
			{
				err << "[" << i << "]" << ss.str();
				return false;
			}
		}
		json.swap(jsonV);
		return true;
	}

	template<typename _Tt>
	// functor to call tuple creation
	struct FnValueCreate
	{
		FnValueCreate(Json::Value &root, const data_attrs& names, std::ostream& err): root_(root), names_(names), err_(err) {}

		template<typename T, size_t _Index> bool operator() (std::integral_constant<size_t, _Index>, const T& value) const
		{
			const attr_type& attr = std::get<_Index>(names_);
			bool bValid = JsonValueCreate(root_[std::get<attr_enum::AttrIndexName>(attr)], attr, err_, value);
			return !bValid;
		};

		Json::Value& root_;
		const data_attrs& names_;
		std::ostream& err_;
	};

};

}

#pragma pack(pop)

//#pragma warning(pop)
