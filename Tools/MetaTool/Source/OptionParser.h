#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <filesystem>

namespace Omni {

	class OptionParser {
	public:
		OptionParser(int argc, char* argv[]) {
			if (argc != 2) {
				throw std::invalid_argument("Usage: program <path>");
			}
			ParsePath(argv[1]);
		}

		const std::string& GetPath() const {
			return m_Path;
		}

		const bool IsValid() const {
			return m_Valid;
		}

	private:
		void ParsePath(const std::string& input_path) {
			m_Valid = !input_path.empty() && std::filesystem::exists(input_path);
			m_Path = input_path;
		}

	private:
		std::string m_Path;
		bool m_Valid = true;
	};


}