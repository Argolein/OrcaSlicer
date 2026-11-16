#include "BinaryGCode.hpp"

#include <string>

#include <boost/filesystem/operations.hpp>
#include <boost/nowide/cstdio.hpp>
#include <boost/system/error_code.hpp>
#include <LibBGCode/binarize/binarize.hpp>
#include <LibBGCode/convert/convert.hpp>
#include <LibBGCode/core/core.hpp>

#include "libslic3r/Exception.hpp"
#include "libslic3r/I18N.hpp"
#include "libslic3r/Utils.hpp"
#include "libslic3r/format.hpp"

namespace Slic3r {

void convert_to_binary_gcode(const std::string &src_path, const std::string &dst_path)
{
    using namespace bgcode;

    // The block compression and encoding PrusaSlicer uses for its .bgcode files.
    binarize::BinarizerConfig config;
    config.compression.print_metadata  = core::ECompressionType::Deflate;
    config.compression.slicer_metadata = core::ECompressionType::Deflate;
    config.compression.gcode           = core::ECompressionType::Heatshrink_12_4;
    config.gcode_encoding              = core::EGCodeEncodingType::MeatPackComments;

    FilePtr src{boost::nowide::fopen(src_path.c_str(), "rb")};
    if (src.f == nullptr)
        throw ExportError(format(_u8L("Cannot open the G-code file %1% for the binary G-code conversion."), src_path));
    FilePtr dst{boost::nowide::fopen(dst_path.c_str(), "wb")};
    if (dst.f == nullptr)
        throw ExportError(format(_u8L("Cannot create the binary G-code file %1%."), dst_path));

    const core::EResult res = convert::from_ascii_to_binary(*src.f, *dst.f, config);
    if (res != core::EResult::Success) {
        dst.close();
        boost::system::error_code ec;
        boost::filesystem::remove(dst_path, ec);
        // TRN %1% is the reason reported by the binary G-code library, in English.
        throw ExportError(format(_u8L("Converting the G-code to binary G-code failed: %1%"), std::string(core::translate_result(res))));
    }
}

} // namespace Slic3r
