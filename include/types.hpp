#ifndef DUMB_REMINDER_BOT_TYPES_HPP
#define DUMB_REMINDER_BOT_TYPES_HPP

#include <string>

namespace bot::types {
    using i8 = signed char;
    using i16 = signed short;
    using i32 = signed int;
    using i64 = signed long long;

    using u8 = unsigned char;
    using u16 = unsigned short;
    using u32 = unsigned int;
    using u64 = unsigned long long;

    using f32 = float;
    using f64 = double;

    using isize = i64;
    using usize = u64;

    using snowflake = std::string;
}

#endif //DUMB_REMINDER_BOT_TYPES_HPP
