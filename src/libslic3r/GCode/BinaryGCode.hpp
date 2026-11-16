#pragma once

#include <string>

namespace Slic3r {

// Writes the ASCII G-code at src_path as Prusa binary G-code (.bgcode) to dst_path.
// Throws ExportError when either file cannot be opened or the conversion fails.
void convert_to_binary_gcode(const std::string &src_path, const std::string &dst_path);

} // namespace Slic3r
