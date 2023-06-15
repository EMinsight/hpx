//  Copyright (c) 2007-2023 Hartmut Kaiser
//
//  SPDX-License-Identifier: BSL-1.0
//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
//  This code was partially taken from:
//      http://msdn.microsoft.com/en-us/library/xcb2z8hs.aspx

#include <hpx/config.hpp>
#include <hpx/thread_support/set_thread_name.hpp>
#include <cstddef>
#include <string>

#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__)) &&               \
    !defined(HPX_MINGW) && defined(HPX_HAVE_NAMEABLE_THREADS)
#include <windows.h>

namespace hpx::util {

    constexpr DWORD MS_VC_EXCEPTION = 0x406D1388;

#pragma pack(push, 8)
    using THREADNAME_INFO = struct tagTHREADNAME_INFO
    {
        DWORD dwType;        // Must be 0x1000.
        LPCSTR szName;       // Pointer to name (in user addr space).
        DWORD dwThreadID;    // Thread ID (-1=caller thread).
        DWORD dwFlags;       // Reserved for future use, must be zero.
    };
#pragma pack(pop)

    namespace detail {

        std::wstring to_wide_string(char const* name) noexcept
        {
            try
            {
                std::string const str(name);
                if (str.empty())
                    return {};

                // determine required length of new string
                std::size_t const req_length = MultiByteToWideChar(CP_UTF8, 0,
                    str.c_str(), static_cast<int>(str.length()), nullptr, 0);

                // construct new string of required length
                std::wstring ret(req_length, L'\0');

                // convert old string to new string
                MultiByteToWideChar(CP_UTF8, 0, str.c_str(),
                    static_cast<int>(str.length()), ret.data(),
                    static_cast<int>(ret.length()));

                return ret;
            }
            catch (...)
            {
                return {};
            }
        }

        void set_thread_name(char const* thread_name) noexcept
        {
            // set thread name the 'old' way
            THREADNAME_INFO info;
            info.dwType = 0x1000;
            info.szName = thread_name;
            info.dwThreadID = -1;
            info.dwFlags = 0;

            __try
            {
                RaiseException(MS_VC_EXCEPTION, 0,
                    sizeof(info) / sizeof(ULONG_PTR),
                    reinterpret_cast<ULONG_PTR*>(&info));
            }
            __except (EXCEPTION_EXECUTE_HANDLER)
            {
            }
        }
    }    // namespace detail

}    // namespace hpx::util

#elif defined(__linux__) && defined(HPX_HAVE_NAMEABLE_THREADS)

#include <pthread.h>
namespace hpx::util { namespace detail {

    void set_thread_name(char const* thread_name)
    {
        pthread_setname_np(pthread_self(), thread_name);
    }

    int get_thread_name(char* buf, size_t len)
    {
        return pthread_getname_np(pthread_self(), buf, len);
    }
}}    // namespace hpx::util::detail

#elif defined(__NetBSD__) && defined(HPX_HAVE_NAMEABLE_THREADS)

#include <pthread.h>
namespace hpx::util { namespace detail {

    void set_thread_name(char const* thread_name)
    {
        pthread_set_name_np(pthread_self(), thread_name);
    }

}}    // namespace hpx::util::detail

#elif (defined(__FreeBSD__) || defined(__OpenBSD__))
#include <pthread.h>
namespace hpx::util { namespace detail {

    void set_thread_name(char const* thread_name)
    {
        pthread_set_name_np(pthread_self(), thread_name);
    }

}}    // namespace hpx::util::detail

#else
namespace hpx::util { namespace detail {
    void set_thread_name([[maybe_unused]] char const* thread_name)
    {
        return;
    }
}}    // namespace hpx::util::detail
#endif

namespace hpx::util {

    // Set the name of the thread shown in the Visual Studio debugger
    void set_thread_name(char const* thread_name) noexcept
    {
        detail::set_thread_name(thread_name);

#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__)) &&               \
    !defined(HPX_MINGW) && defined(HPX_HAVE_NAMEABLE_THREADS)
        // also set it the 'new' way in case the application is not running in
        // the debugger at this time
        SetThreadDescription(
            GetCurrentThread(), detail::to_wide_string(thread_name).c_str());
#endif
    }
}    // namespace hpx::util
