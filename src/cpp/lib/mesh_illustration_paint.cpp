#include "mesh_illustration_internal.h"

#include <regex>

namespace geometer::illustration_detail
{
std::string safe_illustration_color(const std::string& text)
{
    // Match ECMAScript trim and /iu regular-expression semantics independently
    // of the process locale, while retaining the original accepted CSS spelling.
    struct Character
    {
        std::size_t begin, end;
        unsigned code;
    };
    std::vector<Character> characters;
    for (std::size_t i = 0; i < text.size();)
    {
        const auto begin = i;
        const auto first = static_cast<unsigned char>(text[i++]);
        unsigned code = first, remaining = 0;
        if (first >= 0xc2 && first <= 0xdf)
        {
            code &= 0x1f;
            remaining = 1;
        }
        else if (first >= 0xe0 && first <= 0xef)
        {
            code &= 0xf;
            remaining = 2;
        }
        else if (first >= 0xf0 && first <= 0xf4)
        {
            code &= 7;
            remaining = 3;
        }
        else if (first >= 0x80)
            return "#000000";
        for (unsigned j = 0; j < remaining; ++j)
        {
            if (i == text.size() || (static_cast<unsigned char>(text[i]) & 0xc0) != 0x80)
                return "#000000";
            code = (code << 6) | (static_cast<unsigned char>(text[i++]) & 0x3f);
        }
        characters.push_back({begin, i, code});
    }
    const auto whitespace = [](unsigned code)
    {
        return (code >= 9 && code <= 13) || code == 0x20 || code == 0xa0 || code == 0x1680 ||
               (code >= 0x2000 && code <= 0x200a) || code == 0x2028 || code == 0x2029 ||
               code == 0x202f || code == 0x205f || code == 0x3000 || code == 0xfeff;
    };
    std::size_t first = 0, last = characters.size();
    while (first < last && whitespace(characters[first].code))
        ++first;
    while (last > first && whitespace(characters[last - 1].code))
        --last;
    if (first == last)
        return "#000000";
    const auto color =
        text.substr(characters[first].begin, characters[last - 1].end - characters[first].begin);
    std::string normalized;
    for (auto i = first; i < last; ++i)
    {
        unsigned code = characters[i].code;
        if (whitespace(code))
            code = ' ';
        // These are the only non-ASCII simple folds into ECMAScript [a-z].
        if (code == 0x212a)
            code = 'k';
        if (code == 0x17f)
            code = 's';
        if (code >= 'A' && code <= 'Z')
            code += 'a' - 'A';
        if (code >= 0x80)
            return "#000000";
        normalized += static_cast<char>(code);
    }
    static const std::regex allowed(
        "^(?:#[0-9a-f]{3,8}|[a-z]+|(?:rgb|rgba|hsl|hsla)\\([0-9.,%+\\- ]+\\))$",
        std::regex::ECMAScript);
    return std::regex_match(normalized, allowed) ? color : "#000000";
}
} // namespace geometer::illustration_detail
