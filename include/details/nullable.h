// nullable.h
#pragma once

#include <cstddef>
#include <utility>
#include <ostream>

#pragma pack(push, 8)

namespace utils
{

template <typename T>
class Nullable final
{
public:
	Nullable(): m_hasValue(false), m_value(T()) {}
	Nullable(nullptr_t): Nullable() {}
	Nullable(const Nullable& value): m_hasValue(value.m_hasValue), m_value(value.m_value) {}
	Nullable(Nullable&& value): m_hasValue(std::forward<bool>(value.m_hasValue)), m_value(std::forward<T>(value.m_value)) {}
	Nullable(const T &value): m_hasValue(true), m_value(value) {};
	Nullable(T&& value): m_hasValue(true), m_value(std::forward<T>(value)) {};

	bool operator!() const
	{
		return !m_hasValue;
	}
	explicit operator bool() const
	{
		return m_hasValue;
	}

	const Nullable& operator=(const Nullable& value)
	{
		m_hasValue = value.m_hasValue;
		m_value = value.m_value;
		return *this;
	}
	const Nullable& operator=(Nullable&& value)
	{
		m_hasValue = std::forward<bool>(value.m_hasValue);
		m_value = std::forward<T>(value.m_value);
		return *this;
	}

	const Nullable& operator=(const T& value)
	{
		m_hasValue = true;
		m_value = value;
		return *this;
	}
	const Nullable& operator=(T&& value)
	{
		m_hasValue = true;
		m_value = std::forward<T>(value);
		return *this;
	}

	const Nullable& operator=(nullptr_t)
	{
		m_hasValue = false;
		m_value = T();
		return *this;
	}

	friend bool operator==(const Nullable& op1, const Nullable& op2)
	{
		if (op1.m_hasValue != op2.m_hasValue) return false;
		return op1.m_hasValue ? op1.m_value == op2.m_value : true;
	}
	friend bool operator==(const Nullable& op, const T& value)
	{
		if (!op) return false;
		return op.m_value == value;
	}
	friend bool operator==(const T& value, const Nullable& op)
	{
		if (!op) return false;
		return op.m_value == value;
	}
	friend bool operator==(const Nullable& op, nullptr_t)
	{
		return !op;
	}
	friend bool operator==(nullptr_t, const Nullable& op)
	{
		return !op;
	}

	bool has_value() const { return m_hasValue; }
	const T& value() const { return m_value; }

private:
	T m_value;
	bool m_hasValue;
};

/////////////////////////////////////////////////////////////////////

template <typename T> inline
std::ostream& operator<<(std::ostream& os, const Nullable<T>& obj)
{
	if (obj) os << obj.value();
	return os;
}

/////////////////////////////////////////////////////////////////////

}

#pragma pack(pop)
