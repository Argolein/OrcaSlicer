#include "SnapmakerPrinterAgent.hpp"
#include "Http.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "slic3r/GUI/GUI_App.hpp"
#include "slic3r/GUI/DeviceCore/DevManager.h"

#include "nlohmann/json.hpp"
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/log/trivial.hpp>
#include <cctype>

namespace Slic3r {

namespace {

constexpr const char* SNAPMAKER_AGENT_VERSION = "0.0.1";

// Safely access a parallel array by index, returning a fallback if out of bounds.
template<typename T>
T safe_at(const std::vector<T>& vec, int index, const T& fallback)
{
    return (index >= 0 && index < static_cast<int>(vec.size())) ? vec[index] : fallback;
}

std::string normalize_http_base_url(std::string raw)
{
    boost::trim(raw);
    if (raw.empty())
        return "";

    if (auto hash = raw.find('#'); hash != std::string::npos)
        raw = raw.substr(0, hash);
    if (auto query = raw.find('?'); query != std::string::npos)
        raw = raw.substr(0, query);
    boost::trim(raw);
    if (raw.empty())
        return "";

    if (!boost::istarts_with(raw, "http://") && !boost::istarts_with(raw, "https://"))
        raw = "http://" + raw;

    const auto scheme_pos = raw.find("://");
    if (scheme_pos == std::string::npos)
        return "";
    const auto host_begin = scheme_pos + 3;
    if (host_begin >= raw.size())
        return "";

    if (auto slash = raw.find('/', host_begin); slash != std::string::npos)
        raw = raw.substr(0, slash);

    std::string authority = raw.substr(host_begin);
    boost::trim(authority);
    if (authority.empty())
        return "";

    if (raw.size() > 1 && raw.back() == '/')
        raw.pop_back();
    return raw;
}

} // anonymous namespace

SnapmakerPrinterAgent::SnapmakerPrinterAgent(std::string log_dir) : MoonrakerPrinterAgent(std::move(log_dir)) {}

AgentInfo SnapmakerPrinterAgent::get_agent_info_static()
{
    return AgentInfo{"snapmaker", "Snapmaker", SNAPMAKER_AGENT_VERSION, "Snapmaker printer agent"};
}

std::string SnapmakerPrinterAgent::combine_filament_type(const std::string& type, const std::string& sub_type)
{
    const std::string base = trim_and_upper(type);
    const std::string sub  = trim_and_upper(sub_type);

    if (base.empty())
        return "PLA";

    if (sub.empty() || sub == "NONE")
        return base;

    if (sub == "CF")
        return base + "-CF";
    if (sub == "GF")
        return base + "-GF";
    if (sub == "SILK")
        return base + " SILK";
    if (sub == "SNAPSPEED" || sub == "HS")
        return base + " HIGH SPEED";

    // Preserve unknown sub-type as a hint for central vendor/type matching.
    return base + " " + sub;
}

bool SnapmakerPrinterAgent::fetch_filament_info(std::string dev_id)
{
    auto resolve_sync_base_url = [&](const std::string& target_dev_id) {
        std::vector<std::string> candidates;
        candidates.push_back(device_info.base_url);

        if (auto* dev = GUI::wxGetApp().getDeviceManager()) {
            if (!target_dev_id.empty()) {
                if (auto* obj = dev->get_my_machine(target_dev_id); obj)
                    candidates.push_back(obj->get_dev_ip());
                if (auto* obj = dev->get_local_machine(target_dev_id); obj)
                    candidates.push_back(obj->get_dev_ip());
            }
            if (auto* obj = dev->get_selected_machine())
                candidates.push_back(obj->get_dev_ip());
        }

        if (auto* app_cfg = GUI::wxGetApp().app_config) {
            if (!target_dev_id.empty()) {
                const auto ip = app_cfg->get("ip_address", target_dev_id);
                if (!ip.empty())
                    candidates.push_back(ip);
            }
        }

        if (auto* preset_bundle = GUI::wxGetApp().preset_bundle) {
            const auto& cfg = preset_bundle->printers.get_edited_preset().config;
            candidates.push_back(cfg.opt_string("print_host"));
            candidates.push_back(cfg.opt_string("print_host_webui"));
        }

        for (const auto& candidate : candidates) {
            auto normalized = normalize_http_base_url(candidate);
            if (!normalized.empty())
                return normalized;
        }
        return std::string{};
    };

    const std::string base_url = resolve_sync_base_url(dev_id);
    if (base_url.empty()) {
        BOOST_LOG_TRIVIAL(warning) << "SnapmakerPrinterAgent::fetch_filament_info: cannot resolve base_url"
                                   << ", device_info.base_url='" << device_info.base_url
                                   << "', dev_id='" << dev_id << "'";
        return false;
    }

    // Query only print_task_config for robust cross-firmware behavior.
    std::string url = join_url(base_url, "/printer/objects/query?print_task_config");

    std::string response_body;
    bool        success = false;
    std::string http_error;

    auto http = Http::get(url);
    if (!device_info.api_key.empty()) {
        http.header("X-Api-Key", device_info.api_key);
    }
    http.timeout_connect(5)
        .timeout_max(10)
        .on_complete([&](std::string body, unsigned status) {
            if (status == 200) {
                response_body = body;
                success       = true;
            } else {
                http_error = "HTTP error: " + std::to_string(status);
            }
        })
        .on_error([&](std::string body, std::string err, unsigned status) {
            http_error = err;
            if (status > 0) {
                http_error += " (HTTP " + std::to_string(status) + ")";
            }
        })
        .perform_sync();

    if (!success) {
        BOOST_LOG_TRIVIAL(warning) << "SnapmakerPrinterAgent::fetch_filament_info: HTTP request failed: " << http_error;
        return false;
    }

    auto json = nlohmann::json::parse(response_body, nullptr, false, true);
    if (json.is_discarded()) {
        BOOST_LOG_TRIVIAL(warning) << "SnapmakerPrinterAgent::fetch_filament_info: Invalid JSON response";
        return false;
    }

    // Navigate to result.status.print_task_config
    if (!json.contains("result") || !json["result"].contains("status") ||
        !json["result"]["status"].contains("print_task_config")) {
        BOOST_LOG_TRIVIAL(warning) << "SnapmakerPrinterAgent::fetch_filament_info: Missing print_task_config in response";
        return false;
    }

    auto& ptc = json["result"]["status"]["print_task_config"];

    // Read parallel arrays from print_task_config
    auto filament_exist    = ptc.value("filament_exist", std::vector<bool>{});
    auto filament_vendor   = ptc.value("filament_vendor", std::vector<std::string>{});
    auto filament_type     = ptc.value("filament_type", std::vector<std::string>{});
    auto filament_sub_type = ptc.value("filament_sub_type", std::vector<std::string>{});
    auto filament_color    = ptc.value("filament_color_rgba", std::vector<std::string>{});

    const int slot_count = static_cast<int>(filament_exist.size());
    if (slot_count == 0) {
        BOOST_LOG_TRIVIAL(info) << "SnapmakerPrinterAgent::fetch_filament_info: No filament slots reported";
        return false;
    }

    // Read NFC filament_detect data for temperature info (optional)
    nlohmann::json nfc_info;
    if (json["result"]["status"].contains("filament_detect") &&
        json["result"]["status"]["filament_detect"].contains("info")) {
        nfc_info = json["result"]["status"]["filament_detect"]["info"];
    }

    static const std::string empty_str;
    static const std::string default_color = "FFFFFFFF";

    std::vector<AmsTrayData> trays;
    trays.reserve(slot_count);

    for (int i = 0; i < slot_count; ++i) {
        AmsTrayData tray;
        tray.slot_index   = i;
        tray.has_filament = filament_exist[i];

        if (tray.has_filament) {
            tray.tray_type     = combine_filament_type(safe_at(filament_type, i, empty_str),
                                                       safe_at(filament_sub_type, i, empty_str));
            tray.tray_vendor   = safe_at(filament_vendor, i, empty_str);
            // Keep ID resolution centralized in PresetBundle::sync_ams_list (Snapmaker-only path).
            tray.tray_info_idx = "";
            tray.tray_color    = safe_at(filament_color, i, default_color);

            // Extract NFC temperature data if available
            if (nfc_info.is_array() && i < static_cast<int>(nfc_info.size()) && nfc_info[i].is_object()) {
                auto& nfc_slot = nfc_info[i];
                std::string nfc_vendor = nfc_slot.value("VENDOR", "NONE");
                if (nfc_vendor != "NONE" && !nfc_vendor.empty()) {
                    tray.bed_temp    = nfc_slot.value("BED_TEMP", 0);
                    tray.nozzle_temp = nfc_slot.value("FIRST_LAYER_TEMP", 0);
                }
            }
        }

        trays.emplace_back(std::move(tray));
    }

    build_ams_payload(1, slot_count - 1, trays);
    return true;
}

} // namespace Slic3r
