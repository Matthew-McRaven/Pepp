#include "listing.hpp"

std::string masm::utils::format_bytecode(const std::vector<uint8_t>& object_code,
		uint8_t bytes_per_line)
{
	std::string object_string = "";
    for (int i = 0; i < object_code.size(); i++) {
        object_string +=(fmt::format("{:02x}", object_code[i]));
        object_string += (((i % bytes_per_line) == bytes_per_line - 1) ? "\n" : " ");
    }
    object_string.append("zz");
	return object_string;
}
