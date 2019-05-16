/*
This file is part of Telegram Desktop,
the official desktop application for the Telegram messaging service.

For license and copyright information please follow this link:
https://github.com/telegramdesktop/tdesktop/blob/master/LEGAL
*/
#pragma once

#include <QByteArray>
#include <rapidjson/document.h>

namespace Lottie {
namespace details {

inline const rapidjson::Document::ValueType *JsonEmptyValue() {
	static const auto kEmpty = rapidjson::Document::ValueType();
	return &kEmpty;
}

inline const rapidjson::Document::ValueType *JsonEmptyObject() {
	static const auto kEmpty = rapidjson::Document::ValueType(
		rapidjson::kObjectType);
	return &kEmpty;
}

inline const rapidjson::Document::ValueType *JsonEmptyArray() {
	static const auto kEmpty = rapidjson::Document::ValueType(
		rapidjson::kArrayType);
	return &kEmpty;
}

} // namespace details

class JsonArray;
class JsonObject;

class JsonValue {
public:
	JsonValue(const rapidjson::Document::ValueType *value = nullptr)
		: _value(value ? value : details::JsonEmptyValue()) {
	}

	bool isInt() const;
	bool isBool() const;
	bool isDouble() const;
	bool isArray() const;
	bool isObject() const;
	bool isString() const;

	int toInt(int defaultValue = 0) const;
	bool toBool(bool defaultValue = false) const;
	double toDouble(double defaultValue = 0.) const;
	JsonArray toArray() const;
	JsonObject toObject() const;
	QByteArray toString() const;

private:
	const rapidjson::Document::ValueType *_value = nullptr;

};

class JsonObject {
public:
	// For empty object pass nullptr as value.
	JsonObject(const rapidjson::Document::ValueType *value = nullptr)
		: _value(value ? value : details::JsonEmptyObject()) {
	}

	class value_type {
	public:
		value_type(const rapidjson::Document::Member *value) : _value(value) {
		}

		JsonValue name() const {
			return _value ? &_value->name : nullptr;
		}
		JsonValue value() const {
			return _value ? &_value->value : nullptr;
		}

	private:
		const rapidjson::Document::Member *_value = nullptr;

	};
	class iterator {
	public:
		using impl = rapidjson::Document::ConstMemberIterator;

		iterator(impl data) : _data(data) {
		}
		using difference_type = impl::difference_type;
		using iterator_category = impl::iterator_category;

		iterator &operator++() { ++_data; return *this; }
		iterator &operator--() { --_data; return *this; }
		iterator operator++(int) { return _data++; }
		iterator operator--(int) { return _data--; }

		iterator operator+(difference_type n) const { return _data + n; }
		iterator operator-(difference_type n) const { return _data - n; }

		iterator &operator+=(difference_type n) { _data += n; return *this; }
		iterator &operator-=(difference_type n) { _data -= n; return *this; }

		bool operator==(iterator that) const { return _data == that._data; }
		bool operator!=(iterator that) const { return _data != that._data; }
		bool operator<=(iterator that) const { return _data <= that._data; }
		bool operator>=(iterator that) const { return _data >= that._data; }
		bool operator<(iterator that) const { return _data < that._data; }
		bool operator>(iterator that) const { return _data > that._data; }

		difference_type operator-(iterator that) const { return _data - that._data; }

		value_type operator*() const { return &*_data; }
		value_type operator[](difference_type n) const { return &_data[n]; }

	private:
		impl _data;

	};
	using const_iterator = iterator;

	auto empty() const {
		return _value->ObjectEmpty();
	}
	auto size() const {
		return _value->MemberCount();
	}
	iterator begin() const {
		return _value->MemberBegin();
	}
	iterator end() const {
		return _value->MemberEnd();
	}

	iterator find(const char *key) const {
		return _value->FindMember(key);
	}
	JsonValue value(const char *key) const {
		const auto i = find(key);
		return (i != end()) ? (*i).value() : nullptr;
	}
	bool contains(const char *key) const {
		return find(key) != end();
	}

private:
	const rapidjson::Document::ValueType *_value = nullptr;

};

class JsonArray {
public:
	// For empty array pass nullptr as value.
	JsonArray(const rapidjson::Document::ValueType *value = nullptr)
		: _value(value ? value : details::JsonEmptyArray()) {
	}

	class iterator {
	public:
		using impl = rapidjson::Document::ConstValueIterator;

		iterator(impl data) : _data(data) {
		}
		using difference_type = std::ptrdiff_t;
		using iterator_category = std::random_access_iterator_tag;

		iterator &operator++() { ++_data; return *this; }
		iterator &operator--() { --_data; return *this; }
		iterator operator++(int) { return _data++; }
		iterator operator--(int) { return _data--; }

		iterator operator+(difference_type n) const { return _data + n; }
		iterator operator-(difference_type n) const { return _data - n; }

		iterator &operator+=(difference_type n) { _data += n; return *this; }
		iterator &operator-=(difference_type n) { _data -= n; return *this; }

		bool operator==(iterator that) const { return _data == that._data; }
		bool operator!=(iterator that) const { return _data != that._data; }
		bool operator<=(iterator that) const { return _data <= that._data; }
		bool operator>=(iterator that) const { return _data >= that._data; }
		bool operator<(iterator that) const { return _data < that._data; }
		bool operator>(iterator that) const { return _data > that._data; }

		difference_type operator-(iterator that) const { return _data - that._data; }

		JsonValue operator*() const { return &*_data; }
		JsonValue operator[](difference_type n) const { return &_data[n]; }

	private:
		impl _data;

	};
	using const_iterator = iterator;

	auto empty() const {
		return _value->Empty();
	}
	auto size() const {
		return _value->Size();
	}
	iterator begin() const {
		return _value->Begin();
	}
	iterator end() const {
		return _value->End();
	}
	JsonValue at(int index) const {
		return (index >= 0 && index < size())
			? *(begin() + index)
			: JsonValue();
	}

private:
	const rapidjson::Document::ValueType *_value = nullptr;

};

class JsonDocument {
public:
	JsonDocument(QByteArray &&content) : _content(std::move(content)) {
		_data.ParseInsitu(_content.data());
	}

	rapidjson::ParseErrorCode error() const {
		return data().GetParseError();
	}

	JsonObject root() const {
		return data().IsObject() ? &data() : nullptr;
	}

private:
	const rapidjson::Document &data() const {
		return _data;
	}

	rapidjson::Document _data;
	QByteArray _content;

};

inline bool JsonValue::isInt() const {
	return _value->IsNumber();
}

inline bool JsonValue::isBool() const {
	return _value->IsBool();
}

inline bool JsonValue::isDouble() const {
	return _value->IsNumber();
}

inline bool JsonValue::isArray() const {
	return _value->IsArray();
}

inline bool JsonValue::isObject() const {
	return _value->IsObject();
}

inline bool JsonValue::isString() const {
	return _value->IsString();
}

inline int JsonValue::toInt(int defaultValue) const {
	return _value->IsInt()
		? _value->GetInt()
		: _value->IsDouble()
		? qRound(_value->GetDouble())
		: defaultValue;
}

inline bool JsonValue::toBool(bool defaultValue) const {
	return _value->IsBool() ? _value->GetBool() : defaultValue;
}

inline double JsonValue::toDouble(double defaultValue) const {
	return _value->IsNumber() ? _value->GetDouble() : defaultValue;
}

inline JsonArray JsonValue::toArray() const {
	return _value->IsArray() ? _value : nullptr;
}

inline JsonObject JsonValue::toObject() const {
	return _value->IsObject() ? _value : nullptr;
}

inline QByteArray JsonValue::toString() const {
	return _value->IsString()
		? QByteArray(_value->GetString(), _value->GetStringLength())
		: QByteArray();
}

} // namespace Lottie
