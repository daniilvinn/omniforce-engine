#pragma once

#include <type_traits>

namespace Omni {

	template <typename T, typename U>
	concept PtrCheckType = std::is_same_v<T, U> || std::is_base_of_v<T, U> || std::is_base_of_v<U, T>;

}