#pragma once

#include <Foundation/Common.h>
#include <RHI/RHICommon.h>

#include <RHI/Shader.h>

namespace Omni {

	struct DeviceBufferLayoutElement
	{
		std::string name;
		DeviceDataType format = DeviceDataType::FLOAT4;
		uint32 size = 0;
		uint32 offset = 0;

		DeviceBufferLayoutElement(const std::string& in_name, DeviceDataType format)
			: name(in_name), format(format) {}

		DeviceBufferLayoutElement(DeviceBufferLayoutElement& other)
		{
			name = other.name;
			format = other.format;
			size = other.size;
			offset = other.offset;
		}

		DeviceBufferLayoutElement(const DeviceBufferLayoutElement& other)
		{
			name = other.name;
			format = other.format;
			size = other.size;
			offset = other.offset;
		}

		bool operator==(const DeviceBufferLayoutElement& other) const {
			bool result = true;
			result &= format == other.format;
			result &= size == other.size;
			result &= offset == other.offset;

			return result;
		}
	};

	class DeviceBufferLayout
	{
	public:
		DeviceBufferLayout() {}
		DeviceBufferLayout(const std::vector<DeviceBufferLayoutElement>& list) : m_Elements(list)
		{
			for (auto& element : m_Elements)
			{
				uint32 datasize = DeviceDataTypeSize(element.format);
				element.offset = m_Stride;
				element.size = datasize;
				m_Stride += datasize;
			}
		};

		DeviceBufferLayout(DeviceBufferLayout& other) {
			m_Stride = other.m_Stride;
			m_Elements = other.m_Elements;
		}

		DeviceBufferLayout(const DeviceBufferLayout& other) {
			m_Stride = other.m_Stride;
			m_Elements = other.m_Elements;
		}

		uint32 GetStride() const { return m_Stride; }
		const std::vector<DeviceBufferLayoutElement>& GetElements() const { return m_Elements; }
		const std::vector<DeviceBufferLayoutElement>::iterator& begin() { return m_Elements.begin(); }
		const std::vector<DeviceBufferLayoutElement>::iterator& end() { return m_Elements.end(); }

	private:

		std::vector<DeviceBufferLayoutElement> m_Elements;
		uint32 m_Stride = 0;
	};
}