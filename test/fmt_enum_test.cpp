// #include "efp.hpp"
// #include "fmt/core.h"

// int main(int argc, char const *argv[])
// {
//     // ? Can I take arg num from runtime format string?
//     fmt::print("fmt & efp::Enum test");

//     using IorF = efp::Enum<int, float>;
//     const IorF x = 42;

//     fmt::print("this is {}", x);

//     // ? Could Enum get formated correctly?

//     return 0;
// }

#include <fmt/core.h>
#include <fmt/args.h>
#include <variant>
#include <string>
#include "efp.hpp"

using namespace efp;

// Assume we have a variant type that can hold int, float, and std::string

struct FormatString
{
    const char *fmt_str;
    size_t arg_num;
};

using LogData = efp::Enum<FormatString, int, double>;

Vcq<LogData, 32> log_datas{};

template <typename A>
Unit enqueue_log_impl(A a)
{
    log_datas.push_back(a);
    return unit;
}

template <typename... As>
void enqueue_log(const char *fmt_str, As... as)
{
    const size_t arg_num = sizeof...(as);

    log_datas.push_back(FormatString{fmt_str, arg_num});

    efp::execute_pack(enqueue_log_impl(as)...);
}

// todo Should make Enum default constructable like std::variant

void dequeue_log()
{
    log_datas.pop_front()
        .match(
            [](const FormatString &fstr)
            {
                fmt::dynamic_format_arg_store<fmt::format_context> store;

                const auto store_arg = [&](size_t)
                {
                    log_datas.pop_front().match(
                        [&](int arg)
                        { store.push_back(arg); },
                        [&](double arg)
                        { store.push_back(arg); },
                        // ! Number of arguments are decided by number of argument, not parsing result.
                        [&]()
                        { fmt::println("number of arguments are smaller than required"); });
                };

                for_index(store_arg, fstr.arg_num);

                fmt::vprint(fstr.fmt_str, store);
                fmt::print("\n");
            },
            []()
            { fmt::println("invalid log data. first data has to be format string"); });
}

int main()
{
    enqueue_log("int: {}, float: {:.3f}", 42, 3.14);
    log_datas.push_back(-1);
    enqueue_log("int: {:f}, float: {:.5f}", 42., M_PI);
    enqueue_log("This is log line: {}", 3);

    while (!log_datas.empty())
    {
        dequeue_log();
    }
}
