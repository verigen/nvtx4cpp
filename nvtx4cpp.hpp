/* MIT License
 *
 * Copyright (c) 2020 VERIGEN Mateusz Maciąg
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

//
// NVIDIA Tool Extension for C++ (NVTX4CPP)
//

#ifndef NVTX4CPP_HPP_INCLUDED__
#define NVTX4CPP_HPP_INCLUDED__

#include <nvToolsExt.h>

namespace nvtx4cpp
{

enum NvRangeMode
{
    thread,
    process
};

enum NvDomainMode
{
    none,
    custom
};

class NvDomain
{
    nvtxDomainHandle_t m_handle;

public:
    NvDomain(const char* domainName) : m_handle{nvtxDomainCreate(domainName)} {}
    ~NvDomain() { nvtxDomainDestroy(m_handle); }

    nvtxDomainHandle_t handle() const { return m_handle; }
};

class NvRegisteredString
{
    nvtxStringHandle_t m_handle;

public:
    NvRegisteredString(const NvDomain& domain, const char* string) :
        m_handle{nvtxDomainRegisterString(domain.handle(), string)}
    {
    }

    NvRegisteredString(const NvDomain& domain, const std::string& string) :
        NvRegisteredString{domain, string.c_str()}
    {
    }

    nvtxStringHandle_t handle() const { return m_handle; }
};

enum class NvColor
{
    unknown,
    dark_blue,
    blue,
    dark_green,
    green,
    dark_red,
    red,
    dark_yellow,
    yellow,
    dark_magenta,
    magenta,
    dark_cyan,
    cyan,
    dark_gray,
    gray,
    light_gray
};

struct NvAttribute
{
    static constexpr std::array<uint32_t, 16> predefined_colors = {
        0x00000000,
        0xFF00007F,
        0xFF0000FF,
        0xFF007F00,
        0xFF00FF00,
        0xFF7F0000,
        0xFFFF0000,
        0xFF7F7F00,
        0xFFFFFF00,
        0xFF7F007F,
        0xFFFF00FF,
        0xFF007F7F,
        0xFF00FFFF,
        0xFF444444,
        0xFF888888,
        0xFFAAAAAA,
    };

    nvtxEventAttributes_t attr;

    NvAttribute(const char* msg, NvColor color = NvColor::unknown, uint32_t category = 0) :
        NvAttribute{color, category}
    {
        attr.messageType   = NVTX_MESSAGE_TYPE_ASCII;
        attr.message.ascii = msg;
    }

    NvAttribute(
        const NvRegisteredString& msg,
        NvColor color     = NvColor::unknown,
        uint32_t category = 0) :
        NvAttribute{color, category}
    {
        attr.messageType        = NVTX_MESSAGE_TYPE_REGISTERED;
        attr.message.registered = msg.handle();
    }

    NvAttribute(
        const std::string& msg,
        NvColor color     = NvColor::unknown,
        uint32_t category = 0) :
        NvAttribute{msg.c_str(), color, category}
    {
    }

private:
    NvAttribute(NvColor color = NvColor::unknown, uint32_t category = 0)
    {
        attr.version   = NVTX_VERSION;
        attr.size      = NVTX_EVENT_ATTRIB_STRUCT_SIZE;
        attr.category  = category;
        attr.color     = predefined_colors[static_cast<size_t>(color)];
        attr.colorType = (color != NvColor::unknown) ? //
                             NVTX_COLOR_ARGB :
                             NVTX_COLOR_UNKNOWN;
    }
};

namespace details
{

template <NvDomainMode DOMAIN>
struct mark_ops
{
};

template <>
struct mark_ops<NvDomainMode::none>
{
    explicit mark_ops(const char* name) { nvtxMark(name); }
    explicit mark_ops(const NvAttribute& attr) { nvtxMarkEx(&attr.attr); }
};

template <>
struct mark_ops<NvDomainMode::custom>
{
    mark_ops(const NvDomain& domain, const NvAttribute& attr)
    {
        nvtxDomainMarkEx(domain.handle(), &attr.attr);
    }
};

template <NvRangeMode MODE, NvDomainMode DOMAIN>
struct range_ops
{
};

template <>
struct range_ops<NvRangeMode::thread, NvDomainMode::none>
{
    explicit range_ops(const char* name) { nvtxRangePush(name); }
    explicit range_ops(const NvAttribute& attr) { nvtxRangePushEx(&attr.attr); }
    void end() { nvtxRangePop(); }
};

template <>
struct range_ops<NvRangeMode::thread, NvDomainMode::custom>
{
    range_ops(const NvDomain& domain, const NvAttribute& attr) : m_domain{domain}
    {
        nvtxDomainRangePushEx(domain.handle(), &attr.attr);
    }

    void end() { nvtxDomainRangePop(m_domain.handle()); }

private:
    const NvDomain& m_domain;
};

template <>
struct range_ops<NvRangeMode::process, NvDomainMode::none>
{
    explicit range_ops(const char* name) : m_h{nvtxRangeStart(name)} {}
    explicit range_ops(const NvAttribute& attr) : m_h{nvtxRangeStartEx(&attr.attr)} {}
    void end() { nvtxRangeEnd(m_h); }

private:
    nvtxRangeId_t m_h;
};

template <>
struct range_ops<NvRangeMode::process, NvDomainMode::custom>
{
    range_ops(const NvDomain& domain, const NvAttribute& attr) :
        m_domain{domain}, m_h{nvtxDomainRangeStartEx(domain.handle(), &attr.attr)}
    {
    }

    void end() { nvtxDomainRangeEnd(m_domain.handle(), m_h); }

private:
    const NvDomain& m_domain;
    nvtxRangeId_t m_h;
};

} // namespace details

template <NvRangeMode MODE = NvRangeMode::process, NvDomainMode DOMAIN = NvDomainMode::none>
class NvRange : public details::range_ops<MODE, DOMAIN>
{
public:
    // inherit all c'tors
    using details::range_ops<MODE, DOMAIN>::range_ops;

    ~NvRange() { this->end(); }

    NvRange(const NvRange&) = delete;
    NvRange& operator=(const NvRange&) = delete;
};

using NvThreadRange        = NvRange<NvRangeMode::thread>;
using NvDomainThreadRange  = NvRange<NvRangeMode::thread, NvDomainMode::custom>;
using NvProcessRange       = NvRange<NvRangeMode::process>;
using NvDomainProcessRange = NvRange<NvRangeMode::process, NvDomainMode::custom>;

template <NvDomainMode DOMAIN = NvDomainMode::none>
class NvMark : public details::mark_ops<DOMAIN>
{
public:
    // inherit all c'tors
    using details::mark_ops<DOMAIN>::mark_ops;

    NvMark(const NvMark&) = delete;
    NvMark& operator=(const NvMark&) = delete;
};

using NvSimpleMark = NvMark<>;
using NvDomainMark = NvMark<NvDomainMode::custom>;

} // namespace nvtx4cpp

#endif // NVTX4CPP_HPP_INCLUDED__
