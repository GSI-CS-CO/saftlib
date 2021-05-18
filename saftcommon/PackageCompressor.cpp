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

#include "PackageCompressor.hpp"

#include <zlib.h>

gelf::Data gelf::PackageCompressor::compress( const gelf::Data & rData )
{
    gelf::Data compressedData;

    uLong uMaxSize = compressBound( static_cast< uLong >( rData.m_uSize ) );
    compressedData.m_pData = new uint8_t[ uMaxSize ];

    if( compress2( compressedData.m_pData, &uMaxSize, rData.m_pData, static_cast< uLong >( rData.m_uSize ), Z_BEST_COMPRESSION ) == Z_OK )
    {
        compressedData.m_uSize = static_cast< size_t >( uMaxSize );
        return compressedData;
    }

    return gelf::Data();
}
