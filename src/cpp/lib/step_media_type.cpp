#include "step_media_type.h"

#include <cstddef>
#include <string_view>

namespace geometer
{
namespace step_media_type_detail
{
namespace
{
constexpr std::size_t kMaxBytes = 128;

bool token_character(char value)
{
    return value > ' ' && value < 127 &&
           std::string_view("()<>@,;:\\\"/[]?=").find(value) == std::string_view::npos;
}

bool equal_token(std::string_view actual, std::string_view expected)
{
    if (actual.size() != expected.size())
        return false;
    for (std::size_t index = 0; index < actual.size(); ++index)
    {
        const char value = actual[index];
        const char lower = value >= 'A' && value <= 'Z' ? value + ('a' - 'A') : value;
        if (lower != expected[index])
            return false;
    }
    return true;
}

class Parser
{
  public:
    explicit Parser(std::string_view value) : value_(value) {}

    bool step()
    {
        if (!cfws())
            return false;
        const auto type = token();
        if (!cfws() || !take('/') || !cfws())
            return false;
        const auto subtype = token();
        if ((!equal_token(type, "model") && !equal_token(type, "application")) ||
            !equal_token(subtype, "step"))
            return false;
        while (cfws())
        {
            if (position_ == value_.size())
                return true;
            if (!take(';') || !cfws() || token().empty() || !cfws() || !take('=') || !cfws())
                return false;
            if (position_ < value_.size() && value_[position_] == '"')
            {
                if (!quoted())
                    return false;
            }
            else if (token().empty())
                return false;
        }
        return false;
    }

  private:
    bool take(char character)
    {
        if (position_ == value_.size() || value_[position_] != character)
            return false;
        ++position_;
        return true;
    }

    std::string_view token()
    {
        const auto start = position_;
        while (position_ < value_.size() && token_character(value_[position_]))
            ++position_;
        return value_.substr(start, position_ - start);
    }

    bool escaped()
    {
        if (position_ == value_.size())
            return false;
        ++position_;
        return true;
    }

    bool quoted()
    {
        ++position_;
        while (position_ < value_.size())
        {
            const char character = value_[position_++];
            if (character == '"')
                return true;
            if (character == '\\' && !escaped())
                return false;
        }
        return false;
    }

    bool comment()
    {
        ++position_;
        std::size_t depth = 1;
        while (position_ < value_.size())
        {
            const char character = value_[position_++];
            if (character == '\\')
            {
                if (!escaped())
                    return false;
            }
            else if (character == '(')
                ++depth;
            else if (character == ')' && --depth == 0)
                return true;
        }
        return false;
    }

    bool cfws()
    {
        while (position_ < value_.size())
        {
            if (value_[position_] == ' ' || value_[position_] == '\t')
                ++position_;
            else if (value_[position_] == '(')
            {
                if (!comment())
                    return false;
            }
            else
                break;
        }
        return true;
    }

    std::string_view value_;
    std::size_t position_ = 0;
};
} // namespace

bool accepts(std::string_view value)
{
    if (value.empty() || value.size() > kMaxBytes)
        return false;
    for (unsigned char character : value)
        if ((character < 32 && character != '\t') || character >= 127)
            return false;
    return Parser(value).step();
}
} // namespace step_media_type_detail
} // namespace geometer
