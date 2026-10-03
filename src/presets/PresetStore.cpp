#include "PresetStore.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace pmx::presets
{
namespace
{
std::string escapeJson(const std::string& input)
{
    std::string out;
    out.reserve(input.size() + 8);
    for (const char c : input)
    {
        switch (c)
        {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    return out;
}

std::optional<std::string> jsonString(const std::string& line, const std::string& key)
{
    const auto marker = "\"" + key + "\"";
    auto pos = line.find(marker);
    if (pos == std::string::npos) return std::nullopt;
    pos = line.find(':', pos + marker.size());
    if (pos == std::string::npos) return std::nullopt;
    pos = line.find('"', pos + 1);
    if (pos == std::string::npos) return std::nullopt;

    std::string out;
    bool escaped = false;
    for (++pos; pos < line.size(); ++pos)
    {
        const char c = line[pos];
        if (escaped)
        {
            switch (c)
            {
                case 'n': out += '\n'; break;
                case 'r': out += '\r'; break;
                case 't': out += '\t'; break;
                case '\\': out += '\\'; break;
                case '"': out += '"'; break;
                default: out += c; break;
            }
            escaped = false;
        }
        else if (c == '\\') escaped = true;
        else if (c == '"') return out;
        else out += c;
    }
    return std::nullopt;
}

std::optional<double> jsonNumber(const std::string& line, const std::string& key)
{
    const auto marker = "\"" + key + "\"";
    auto pos = line.find(marker);
    if (pos == std::string::npos) return std::nullopt;
    pos = line.find(':', pos + marker.size());
    if (pos == std::string::npos) return std::nullopt;
    try { return std::stod(line.substr(pos + 1)); }
    catch (...) { return std::nullopt; }
}

std::optional<bool> jsonBool(const std::string& line, const std::string& key)
{
    const auto marker = "\"" + key + "\"";
    auto pos = line.find(marker);
    if (pos == std::string::npos) return std::nullopt;
    pos = line.find(':', pos + marker.size());
    if (pos == std::string::npos) return std::nullopt;
    auto value = line.substr(pos + 1);
    value.erase(std::remove_if(value.begin(), value.end(), [](unsigned char c){ return std::isspace(c) || c == ','; }), value.end());
    if (value.rfind("true", 0) == 0) return true;
    if (value.rfind("false", 0) == 0) return false;
    return std::nullopt;
}

std::string modeName(PresetMode mode)
{
    switch (mode)
    {
        case PresetMode::guitar: return "guitar";
        case PresetMode::bass: return "bass";
        case PresetMode::synth: return "synth";
        case PresetMode::drums: return "drums";
        case PresetMode::piano: return "piano";
        case PresetMode::violin: return "violin";
    }
    return "guitar";
}

PresetMode parseMode(const std::string& value)
{
    if (value == "bass") return PresetMode::bass;
    if (value == "synth") return PresetMode::synth;
    if (value == "drums") return PresetMode::drums;
    if (value == "piano") return PresetMode::piano;
    if (value == "violin") return PresetMode::violin;
    return PresetMode::guitar;
}

std::string serialize(const Preset& p)
{
    std::ostringstream os;
    os << std::setprecision(15);
    os << "{\n";
    os << "  \"schemaVersion\": " << p.schemaVersion << ",\n";
    os << "  \"id\": \"" << escapeJson(p.id) << "\",\n";
    os << "  \"displayName\": \"" << escapeJson(p.displayName) << "\",\n";
    os << "  \"category\": \"" << escapeJson(p.category) << "\",\n";
    os << "  \"mode\": \"" << modeName(p.mode) << "\",\n";
    os << "  \"favorite\": " << (p.favorite ? "true" : "false") << ",\n";
    os << "  \"parameters\": [\n";
    std::size_t i = 0;
    for (const auto& [key, value] : p.parameters)
        os << "    {\"key\":\"" << escapeJson(key) << "\",\"value\":" << value << "}" << (++i < p.parameters.size() ? "," : "") << "\n";
    os << "  ],\n";
    os << "  \"modules\": [\n";
    i = 0;
    for (const auto& [key, value] : p.modules)
        os << "    {\"key\":\"" << escapeJson(key) << "\",\"enabled\":" << (value ? "true" : "false") << "}" << (++i < p.modules.size() ? "," : "") << "\n";
    os << "  ],\n";
    os << "  \"nam\": {\"id\":\"" << escapeJson(p.nam.id) << "\",\"path\":\"" << escapeJson(p.nam.path.generic_string()) << "\"},\n";
    os << "  \"ir\": {\"id\":\"" << escapeJson(p.ir.id) << "\",\"path\":\"" << escapeJson(p.ir.path.generic_string()) << "\"}\n";
    os << "}\n";
    return os.str();
}

PresetLoadResult parse(const std::filesystem::path& path)
{
    std::ifstream input(path);
    if (!input) return { false, "Preset file could not be opened.", {} };

    Preset p;
    enum class Section { root, parameters, modules } section = Section::root;
    std::string line;
    while (std::getline(input, line))
    {
        if (line.find("\"parameters\"") != std::string::npos) { section = Section::parameters; continue; }
        if (line.find("\"modules\"") != std::string::npos) { section = Section::modules; continue; }
        if ((section == Section::parameters || section == Section::modules) && line.find(']') != std::string::npos) { section = Section::root; continue; }

        if (section == Section::parameters && line.find("\"key\"") != std::string::npos)
        {
            const auto key = jsonString(line, "key");
            const auto value = jsonNumber(line, "value");
            if (key && value) p.parameters[*key] = *value;
            continue;
        }
        if (section == Section::modules && line.find("\"key\"") != std::string::npos)
        {
            const auto key = jsonString(line, "key");
            const auto enabled = jsonBool(line, "enabled");
            if (key && enabled) p.modules[*key] = *enabled;
            continue;
        }

        if (line.find("\"schemaVersion\"") != std::string::npos)
        {
            if (const auto n = jsonNumber(line, "schemaVersion")) p.schemaVersion = static_cast<int>(*n);
        }
        else if (line.find("\"displayName\"") != std::string::npos) { if (const auto v = jsonString(line, "displayName")) p.displayName = *v; }
        else if (line.find("\"category\"") != std::string::npos) { if (const auto v = jsonString(line, "category")) p.category = *v; }
        else if (line.find("\"mode\"") != std::string::npos) { if (const auto v = jsonString(line, "mode")) p.mode = parseMode(*v); }
        else if (line.find("\"favorite\"") != std::string::npos) { if (const auto v = jsonBool(line, "favorite")) p.favorite = *v; }
        else if (line.find("\"nam\"") != std::string::npos)
        {
            if (const auto v = jsonString(line, "id")) p.nam.id = *v;
            if (const auto v = jsonString(line, "path")) p.nam.path = std::filesystem::path(*v);
        }
        else if (line.find("\"ir\"") != std::string::npos)
        {
            if (const auto v = jsonString(line, "id")) p.ir.id = *v;
            if (const auto v = jsonString(line, "path")) p.ir.path = std::filesystem::path(*v);
        }
        else if (line.find("\"id\"") != std::string::npos) { if (const auto v = jsonString(line, "id")) p.id = *v; }
    }

    if (p.schemaVersion != 1) return { false, "Preset schema version is not supported.", {} };
    if (p.id.empty() || p.displayName.empty()) return { false, "Preset is missing required fields.", {} };
    return { true, {}, std::move(p) };
}
} // namespace

PresetStore::PresetStore(std::filesystem::path dir) : directory(std::move(dir))
{
    std::error_code ec;
    std::filesystem::create_directories(directory, ec);
}

std::filesystem::path PresetStore::presetPath(const std::string& id) const
{
    return directory / (id + ".json");
}

PresetSaveResult PresetStore::save(const Preset& preset)
{
    if (preset.id.empty() || preset.displayName.empty()) return { false, "Preset needs an id and name.", {} };
    if (preset.schemaVersion != 1) return { false, "Preset schema version is not supported.", {} };

    std::error_code ec;
    std::filesystem::create_directories(directory, ec);
    if (ec) return { false, "Preset folder could not be created.", {} };

    const auto target = presetPath(preset.id);
    const auto temp = std::filesystem::path(target.string() + ".tmp");
    const auto backup = std::filesystem::path(target.string() + ".bak");

    {
        std::ofstream output(temp, std::ios::binary | std::ios::trunc);
        if (!output) return { false, "Temporary preset file could not be created.", {} };
        output << serialize(preset);
        output.flush();
        if (!output) { std::filesystem::remove(temp, ec); return { false, "Preset could not be written.", {} }; }
    }

    const bool hadExisting = std::filesystem::exists(target);
    if (hadExisting)
    {
        std::filesystem::remove(backup, ec);
        ec.clear();
        std::filesystem::rename(target, backup, ec);
        if (ec) { std::filesystem::remove(temp, ec); return { false, "Existing preset could not be backed up.", {} }; }
    }

    ec.clear();
    std::filesystem::rename(temp, target, ec);
    if (ec)
    {
        std::error_code ignored;
        std::filesystem::remove(temp, ignored);
        if (hadExisting && std::filesystem::exists(backup)) std::filesystem::rename(backup, target, ignored);
        return { false, "Preset could not be replaced safely.", {} };
    }

    if (currentPreset && currentPreset->id == preset.id)
    {
        currentPreset = preset;
        dirty = false;
    }
    return { true, {}, target };
}

PresetLoadResult PresetStore::load(const std::string& id) const
{
    return parse(presetPath(id));
}

std::vector<Preset> PresetStore::list() const
{
    std::vector<Preset> result;
    std::error_code ec;
    if (!std::filesystem::exists(directory)) return result;
    for (const auto& entry : std::filesystem::directory_iterator(directory, ec))
    {
        if (ec) break;
        if (!entry.is_regular_file() || entry.path().extension() != ".json") continue;
        const auto loaded = parse(entry.path());
        if (loaded.ok) result.push_back(loaded.preset);
    }
    std::sort(result.begin(), result.end(), [](const Preset& a, const Preset& b){ return a.displayName < b.displayName; });
    return result;
}

std::vector<std::string> PresetStore::missingAssets(const Preset& preset) const
{
    std::vector<std::string> missing;
    if (!preset.nam.id.empty() && (preset.nam.path.empty() || !std::filesystem::exists(preset.nam.path))) missing.push_back("NAM");
    if (!preset.ir.id.empty() && (preset.ir.path.empty() || !std::filesystem::exists(preset.ir.path))) missing.push_back("IR");
    return missing;
}

void PresetStore::setCurrent(const Preset& preset)
{
    currentPreset = preset;
    dirty = false;
}

void PresetStore::setCurrentParameter(const std::string& key, double value)
{
    if (!currentPreset) return;
    const auto it = currentPreset->parameters.find(key);
    if (it == currentPreset->parameters.end() || it->second != value)
    {
        currentPreset->parameters[key] = value;
        dirty = true;
    }
}
} // namespace pmx::presets
