#pragma once

#include <Foundation/Types.h>
#include <Foundation/Macros.h>

namespace Omni {

	// Robin-Hood Unordered Map wrapper which allows adding callback for situations when operator [] was called but no key was found
	// This allows for custom "constructor" when key is not found and default value is constructed
	template<typename Key, typename Value>
	class OMNIFORCE_API CallbackRHUMap : public rhumap<Key, Value> {
	public:
		void add_callback(std::function<void(Value&)> callback) { m_Callback = callback; }

		Value& operator[](const Key& key) {
			auto it = this->find(key);
			if (it == this->end()) {
				Value value = Value();
				m_Callback(value);
				it = rhumap<Key, Value>::emplace(key, value).first;
			}

			return it->second;
		}

	private:
		std::function<void(Value&)> m_Callback;
	};

}