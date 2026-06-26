#include "PrusaPrinterAgent.hpp"
#include "Http.hpp"

#include "libslic3r/Preset.hpp"
#include "libslic3r/PresetBundle.hpp"
#include "libslic3r/PrintConfig.hpp"
#include "slic3r/GUI/GUI_App.hpp"

#include "nlohmann/json.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/log/trivial.hpp>
#include <algorithm>

namespace Slic3r {

namespace {
constexpr const char* PRUSA_AGENT_VERSION = "0.0.1";
} // namespace

PrusaPrinterAgent::PrusaPrinterAgent(std::string log_dir) : MoonrakerPrinterAgent(std::move(log_dir)) {}

AgentInfo PrusaPrinterAgent::get_agent_info_static()
{
    return AgentInfo{"prusa", "Prusa", PRUSA_AGENT_VERSION, "Prusa printer agent"};
}

PrusaPrinterAgent::HostConfig PrusaPrinterAgent::read_host_config()
{
    HostConfig cfg;

    auto* preset_bundle = GUI::wxGetApp().preset_bundle;
    if (!preset_bundle)
        return cfg;

    const DynamicPrintConfig& config = preset_bundle->printers.get_edited_preset().config;

    cfg.host = config.opt_string("print_host");
    if (cfg.host.empty())
        return cfg; // No PrusaLink host configured -> nothing to sync against.

    if (auto* opt = config.option<ConfigOptionEnum<AuthorizationType>>("printhost_authorization_type"))
        cfg.auth_type = opt->value;
    cfg.apikey   = config.opt_string("printhost_apikey");
    cfg.user     = config.opt_string("printhost_user");
    cfg.password = config.opt_string("printhost_password");
    cfg.cafile   = config.opt_string("printhost_cafile");
    cfg.valid    = true;
    return cfg;
}

std::string PrusaPrinterAgent::make_url(const std::string& host, const std::string& path)
{
    // Mirror OctoPrint::make_url: keep an explicit scheme, default to http otherwise.
    if (boost::istarts_with(host, "http://") || boost::istarts_with(host, "https://")) {
        if (!host.empty() && host.back() == '/')
            return host + path;
        return host + "/" + path;
    }
    return "http://" + host + "/" + path;
}

int PrusaPrinterAgent::connect_printer(std::string dev_id, std::string dev_ip, std::string username, std::string password, bool use_ssl)
{
    // PrusaLink is queried on demand (pull mode), so there is no persistent connection or
    // status stream to establish. We only populate device_info so build_ams_payload() can
    // resolve the MachineObject and stamp its printer_type during fetch_filament_info().
    if (dev_id.empty())
        return BAMBU_NETWORK_ERR_INVALID_HANDLE;

    init_device_info(dev_id, dev_ip, username, password, use_ssl);
    return BAMBU_NETWORK_SUCCESS;
}

int PrusaPrinterAgent::disconnect_printer()
{
    // No persistent connection to tear down.
    return BAMBU_NETWORK_SUCCESS;
}

bool PrusaPrinterAgent::fetch_filament_info(std::string dev_id)
{
    const HostConfig cfg = read_host_config();
    if (!cfg.valid) {
        BOOST_LOG_TRIVIAL(info) << "PrusaPrinterAgent::fetch_filament_info: no PrusaLink host configured";
        return false;
    }

    const std::string url = make_url(cfg.host, "api/v1/filament");

    std::string body;
    bool        success = false;
    std::string http_error;

    auto http = Http::get(url);
    // Authenticate exactly like the PrusaLink print host (PrusaLink::set_auth).
    if (cfg.auth_type == atUserPassword) {
        // PrusaLink digest auth; the firmware default user is "maker".
        const std::string user = cfg.user.empty() ? std::string("maker") : cfg.user;
        http.auth_digest(user, cfg.password);
    } else if (!cfg.apikey.empty()) {
        http.header("X-Api-Key", cfg.apikey);
    }
    if (!cfg.cafile.empty())
        http.ca_file(cfg.cafile);

    http.timeout_connect(5)
        .timeout_max(10)
        .on_complete([&](std::string response, unsigned status) {
            if (status == 200) {
                body    = std::move(response);
                success = true;
            } else {
                http_error = "HTTP error: " + std::to_string(status);
            }
        })
        .on_error([&](std::string /*body*/, std::string err, unsigned status) {
            http_error = err;
            if (status > 0)
                http_error += " (HTTP " + std::to_string(status) + ")";
        })
        .perform_sync();

    if (!success) {
        BOOST_LOG_TRIVIAL(warning) << "PrusaPrinterAgent::fetch_filament_info: request to " << url
                                   << " failed: " << (http_error.empty() ? "unknown error" : http_error);
        return false;
    }

    nlohmann::json json = nlohmann::json::parse(body, nullptr, false, true);
    if (json.is_discarded() || !json.contains("tools") || !json["tools"].is_array()) {
        BOOST_LOG_TRIVIAL(warning) << "PrusaPrinterAgent::fetch_filament_info: invalid /api/v1/filament response";
        return false;
    }

    // One entry per physical tool: tool -> slot index. Empty tools are kept so the slot
    // count (and therefore the AMS layout) matches the printer's physical tool count.
    std::vector<AmsTrayData> trays;
    int                      max_lane_index = -1;
    for (const auto& tool_json : json["tools"]) {
        if (!tool_json.contains("tool") || !tool_json["tool"].is_number_integer())
            continue;
        const int tool = tool_json["tool"].get<int>();
        if (tool < 0)
            continue;
        max_lane_index = std::max(max_lane_index, tool);

        AmsTrayData tray;
        tray.slot_index = tool;

        std::string type;
        if (tool_json.contains("type") && tool_json["type"].is_string())
            type = tool_json["type"].get<std::string>();

        tray.has_filament = !type.empty();
        if (tray.has_filament) {
            tray.tray_type = type;
            // PrusaLink reports only a bare type (e.g. "PLA"), never a specific preset id.
            // Leave the setting id unresolved so PresetBundle::sync_ams_list() resolves the
            // preset by filament type (preferring the user's own preset, then a vendor
            // generic) instead of forcing an OrcaFilamentLibrary id that is not the right
            // target for Prusa profiles.
            tray.tray_info_idx = UNKNOWN_FILAMENT_ID;
            // color_rgb is the read-only "#rrggbb" convenience field; normalize_color_value
            // (called inside build_ams_payload) turns it into RRGGBBAA.
            if (tool_json.contains("color_rgb") && tool_json["color_rgb"].is_string())
                tray.tray_color = tool_json["color_rgb"].get<std::string>();
        }
        trays.push_back(std::move(tray));
    }

    if (max_lane_index < 0) {
        BOOST_LOG_TRIVIAL(info) << "PrusaPrinterAgent::fetch_filament_info: printer reported no tools";
        return false;
    }

    // build_ams_payload() resolves the MachineObject via device_info.dev_id and stamps
    // device_info.model_id onto it, so make sure both are current for this device.
    if (auto* preset_bundle = GUI::wxGetApp().preset_bundle) {
        auto& preset           = preset_bundle->printers.get_edited_preset();
        device_info.model_id   = preset.get_printer_type(preset_bundle);
        device_info.model_name = preset.config.opt_string("printer_model");
    }
    device_info.dev_id = dev_id;

    const int ams_count = (max_lane_index + 4) / 4; // 4 slots per AMS unit, like Moonraker lanes.
    build_ams_payload(ams_count, max_lane_index, trays);

    BOOST_LOG_TRIVIAL(info) << "PrusaPrinterAgent::fetch_filament_info: synced " << (max_lane_index + 1)
                            << " tool(s)";
    return true;
}

} // namespace Slic3r
