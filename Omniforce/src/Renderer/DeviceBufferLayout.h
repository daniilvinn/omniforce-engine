#pragma once

#include "RendererCommon.h"
#include "Shader.h"

namespace Omni {

	struct DeviceBufferLayoutElement
	{
		std::string_view name;
		DeviceDataType format;
		uint32 size;
		uint32 offset;

		DeviceBufferLayoutElement(std::string_view name, DeviceDataType format)
			: name(name), format(format) {}

		bool operator==(const DeviceBufferLayoutElement& other) const {
			bool result = true;
			result = name == other.name;
			result = format == other.format;
			result = size == other.size;
			result = offset == other.offset;

			return result;
		}
	};

	class DeviceBufferLayout
	{
	public:
		DeviceBufferLayout() {}
		DeviceBufferLayout(const std::initializer_list<DeviceBufferLayoutElement> list) : m_Elements(list)
		{
			for (auto& element : m_Elements)
			{
				uint32 datasize = DeviceDataTypeSize(element.format);
				element.offset = m_Stride;
				element.size = datasize;
				m_Stride += datasize;

			}
		};

		uint32 GetStride() const { return m_Stride; }
		const std::vector<DeviceBufferLayoutElement>& GetElements() const { return m_Elements; }
		const std::vector<DeviceBufferLayoutElement>::iterator& begin() { return m_Elements.begin(); }
		const std::vector<DeviceBufferLayoutElement>::iterator& end() { return m_Elements.end(); }

	private:

		std::vector<DeviceBufferLayoutElement> m_Elements;
		uint32 m_Stride = 0;
	};
}