// MIT License
// 
// Copyright (c) 2016 serge-14
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef GELF_LIB_PACKAGE_BUILDER_HPP
#define GELF_LIB_PACKAGE_BUILDER_HPP

#include "GELFConfig.hpp"
#include "Data.hpp"

namespace gelf
{
    const int g_uChunkHeaderMagicBytes = 0x0F1E;

#pragma pack(push)
#pragma pack(1)
    struct ChunkHeader
    {
        uint16_t m_uMagicBytes;
        uint64_t m_uMessageId;
        uint8_t m_uSequenceNumber;
        uint8_t m_uSequenceCount;
    };
#pragma pack(pop)

    class PackageBuilder
    {
    public:
        PackageBuilder( const gelf::Data& rData );
        PackageBuilder( const uint8_t* const pData, size_t uSize );

        PackageBuilder& setMessageId( uint64_t uMessageId );
        PackageBuilder& setChunkSize( size_t uSize );

        ChunkedData build();

    private:
        const uint8_t* const m_pOriginalData;
        size_t m_uOriginalSize;

        size_t m_uChunkSize;
        uint64_t m_uMessageId;
    };
}

#endif //GELF_LIB_PACKAGE_BUILDER_HPP
