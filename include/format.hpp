#ifndef EFP_FORMAT_HPP_
#define EFP_FORMAT_HPP_

#include "sfinae.hpp"
#include "sequence.hpp"
#include "string.hpp"

namespace efp
{
    namespace detail
    {
        // struct Customchar
        // {
        //     int value;
        //     Customchar() = default;

        //     template <typename T>
        //     constexpr Customchar(T val) : value(static_cast<int>(val)) {}

        //     constexpr operator char() const
        //     {
        //         return value <= 0xff ? static_cast<char>(value) : '\0';
        //     }
        //     constexpr bool operator<(Customchar c) const { return value < c.value; }
        // };

        // template <class _charT>
        // struct char_traits;

        // template <>
        struct char_traits
        // <custom_char>
        {
            // using char = custom_char;
            using int_type = int;
            using off_type = streamoff;
            using pos_type = streampos;
            using state_type = mbstate_t;

            static constexpr void assign(char &r, const char &a) { r = a; }
            static constexpr bool eq(char a, char b) { return a == b; }
            static constexpr bool lt(char a, char b) { return a < b; }
            static constexpr int compare(const char *s1, const char *s2, size_t count)
            {
                for (; count; count--, s1++, s2++)
                {
                    if (lt(*s1, *s2))
                        return -1;
                    if (lt(*s2, *s1))
                        return 1;
                }
                return 0;
            }

            static constexpr size_t length(const char *s)
            {
                size_t count = 0;
                while (!eq(*s++, '\0'))
                    count++;
                return count;
            }

            // static const char *find(const char *, size_t, const char &);

            static constexpr char *move(char *dest, const char *src, size_t count)
            {
                if (count == 0)
                    return dest;
                char *ret = dest;
                if (src < dest)
                {
                    dest += count;
                    src += count;
                    for (; count; count--)
                        assign(*--dest, *--src);
                }
                else if (src > dest)
                    copy(dest, src, count);
                return ret;
            }

            static constexpr char *copy(char *dest, const char *src, size_t count)
            {
                char *ret = dest;
                for (; count; count--)
                    assign(*dest++, *src++);
                return ret;
            }

            static constexpr char *assign(char *dest, std::size_t count, char a)
            {
                char *ret = dest;
                for (; count; count--)
                    assign(*dest++, a);
                return ret;
            }

            // ?
            // not_eof(int_type);
            // static char to_char_type(int_type);
            // static int_type to_int_type(char);
            // static bool eq_int_type(int_type, int_type);
            // static int_type eof();
        };

        // * StringView could be made from const char * with compile time length check.
        // * What about the compile time parsing? How does it works?

        template <typename Char, typename... Args>
        class format_string_checker
        {
        private:
            using parse_context_type = compile_parse_context<Char>;
            static constexpr int num_args = sizeof...(Args);

            // Format specifier parsing function.
            // In the future basic_format_parse_context will replace compile_parse_context
            // here and will use is_constant_evaluated and downcasting to access the data
            // needed for compile-time checks: https://godbolt.org/z/GvWzcTjh1.
            using parse_func = const Char *(*)(parse_context_type &);

            type types_[num_args > 0 ? static_cast<size_t>(num_args) : 1];
            parse_context_type context_;
            parse_func parse_funcs_[num_args > 0 ? static_cast<size_t>(num_args) : 1];

        public:
            explicit constexpr format_string_checker(basic_string_view<Char> fmt)
                : types_{mapped_type_constant<Args, buffer_context<Char>>::value...},
                  context_(fmt, num_args, types_),
                  parse_funcs_{&parse_format_specs<Args, parse_context_type>...} {}

            constexpr void on_text(const Char *, const Char *) {}

            constexpr auto on_arg_id() -> int { return context_.next_arg_id(); }

            constexpr auto on_arg_id(int id) -> int
            {
                return context_.check_arg_id(id), id;
            }

            constexpr auto on_arg_id(basic_string_view<Char> id) -> int
            {
                // #if FMT_USE_NONTYPE_TEMPLATE_ARGS
                //                 auto index = get_arg_index_by_name<Args...>(id);
                //                 if (index < 0)
                //                     on_error("named argument is not found");
                //                 return index;
                // #else
                (void)id;
                on_error("compile-time checks for named arguments require C++20 support");
                return 0;
                // #endif
            }

            constexpr void on_replacement_field(int id, const Char *begin)
            {
                on_format_specs(id, begin, begin); // Call parse() on empty specs.
            }

            constexpr auto on_format_specs(int id, const Char *begin, const Char *)
                -> const Char *
            {
                context_.advance_to(begin);
                // id >= 0 check is a workaround for gcc 10 bug (#2065).
                return id >= 0 && id < num_args ? parse_funcs_[id](context_) : begin;
            }

            constexpr void on_error(const char *message)
            {
                throw_format_error(message);
            }
        };

        template <typename Handler>
        constexpr auto parse_replacement_field(const char *begin, const char *end, Handler &&handler)
            -> const char *
        {
            struct id_adapter
            {
                Handler &handler;
                int arg_id;

                constexpr void on_auto() { arg_id = handler.on_arg_id(); }
                constexpr void on_index(int id) { arg_id = handler.on_arg_id(id); }
                constexpr void on_name(basic_string_view<char> id)
                {
                    arg_id = handler.on_arg_id(id);
                }
            };

            ++begin;
            if (begin == end)
                return handler.on_error("invalid format string"), end;

            if (*begin == '}')
            {
                handler.on_replacement_field(handler.on_arg_id(), begin);
            }
            else if (*begin == '{')
            {
                handler.on_text(begin, begin + 1);
            }
            else
            {
                auto adapter = id_adapter{handler, 0};
                begin = parse_arg_id(begin, end, adapter);
                char c = begin != end ? *begin : char();
                if (c == '}')
                {
                    handler.on_replacement_field(adapter.arg_id, begin);
                }
                else if (c == ':')
                {
                    begin = handler.on_format_specs(adapter.arg_id, begin + 1, end);
                    if (begin == end || *begin != '}')
                        return handler.on_error("unknown format specifier"), end;
                }
                else
                {
                    return handler.on_error("missing '}' in format string"), end;
                }
            }
            return begin + 1;
        }

        template <bool is_constexpr, typename Handler>
        constexpr inline void parse_format_string(
            // todo StringView
            basic_string_view<char> format_str, Handler &&handler)
        {
            auto begin = format_str.data();
            auto end = begin + format_str.size();

            // if (end - begin < 32)
            // {
            // Use a simple loop instead of memchr for small strings.
            const char *p = begin;
            while (p != end)
            {
                auto c = *p++;
                if (c == '{')
                {
                    handler.on_text(begin, p - 1);
                    begin = p = parse_replacement_field(p - 1, end, handler);
                }
                else if (c == '}')
                {
                    if (p == end || *p != '}')
                        return handler.on_error("unmatched '}' in format string");
                    handler.on_text(begin, p);
                    begin = ++p;
                }
            }
            handler.on_text(begin, end);
            return;
            // }
        }

        /** A compile-time format string. */
        template <typename Char, typename... Args>
        class basic_format_string
        {
        private:
            basic_string_view<Char> str_;

        public:
            template <typename S,
                      FMT_ENABLE_IF(
                          std::is_convertible<const S &, basic_string_view<Char>>::value)>
            FMT_CONSTEVAL FMT_INLINE basic_format_string(const S &s) : str_(s)
            {
                static_assert(
                    detail::count<
                        (std::is_base_of<detail::view, remove_reference_t<Args>>::value && std::is_reference<Args>::value)...>() == 0,
                    "passing views as lvalues is disallowed");
                // #ifdef FMT_HAS_CONSTEVAL
                //                 if constexpr (detail::count_named_args<Args...>() ==
                //                               detail::count_statically_named_args<Args...>())
                //                 {
                //                     using checker =
                //                         detail::format_string_checker<Char, remove_cvref_t<Args>...>;
                //                     detail::parse_format_string<true>(str_, checker(s));
                //                 }
                // #else
                detail::check_format_string<Args...>(s);
                // #endif
            }
            // basic_format_string(runtime_format_string<Char> fmt) : str_(fmt.str) {}

            FMT_INLINE operator basic_string_view<Char>() const { return str_; }
            FMT_INLINE auto get() const -> basic_string_view<Char> { return str_; }
        };
    }
}

#endif