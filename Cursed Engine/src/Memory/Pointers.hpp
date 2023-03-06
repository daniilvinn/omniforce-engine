#pragma once

#include <memory>

namespace Cursed {

	

	template <typename T>
	using Scope = std::unique_ptr<T>;

	template <typename T>
	using Shared = std::shared_ptr<T>;



}