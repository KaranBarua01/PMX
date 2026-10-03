#include <regex>
#include <string>
#include <string_view>
#include "pmx/AppVersion.h"

int main()
{
    const std::string version { pmx::AppVersion::current() };
    if (version.empty()) return 1;
    const std::regex semver { R"(^[0-9]+\.[0-9]+\.[0-9]+(?:-[0-9A-Za-z.-]+)?$)" };
    return std::regex_match(version, semver) ? 0 : 2;
}
