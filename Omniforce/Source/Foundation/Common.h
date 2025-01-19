#pragma once

#include "Platform.h"
#include "BasicTypes.h"
#include "Assert.h"

#include "Memory/Ptr.h"
#include "Memory/Ref.h"
#include "Memory/WeakPtr.h"
#include "Memory/Allocator.h"
#include "Memory/Allocators/DedicatedMemoryAllocator.h"
#include "Memory/Allocators/PersistentAllocator.h"
#include "Memory/Allocators/TransientAllocator.h"
#include "Memory/VirtualMemoryBlock.h"
#include "Log/Logger.h"

#include "Containers/Array.h"
#include "Containers/StaticArray.h"
#include "Containers/Stack.h"

#include "Timer.h"
#include "UUID.h"

#include <iostream>
#include <fstream>
#include <functional>
#include <algorithm>
#include <utility>
#include <vector>
#include <string>
#include <string_view>
#include <sstream>
#include <filesystem>
#include <array>
#include <stack>

#include <nlohmann/json.hpp>