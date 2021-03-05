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

#ifndef GLEF_LIB_DATA_HPP
#define GLEF_LIB_DATA_HPP

#include <cinttypes>
#include <memory>
#include <string>
#include <cstring>

namespace gelf
{
    struct Data
    {
        uint8_t* m_pData;
        size_t m_uSize;
        bool m_bOwnBuffer;

        Data() :
            m_pData( nullptr ),
            m_uSize( 0 ),
            m_bOwnBuffer( true )
        {
        }

        Data( uint8_t* pData, size_t uSize ) :
            m_pData( pData ),
            m_uSize( uSize ),
            m_bOwnBuffer( false )
        {
        }

        Data( const Data& other ) :
            m_pData( new uint8_t[ other.m_uSize ] ),
            m_uSize( 0 ),
            m_bOwnBuffer( true )
        {
            std::memcpy( m_pData, other.m_pData, other.m_uSize );
        }

        Data( Data&& other ) noexcept :
            m_pData( other.m_pData ),
            m_uSize( other.m_uSize ),
            m_bOwnBuffer( other.m_bOwnBuffer )
        {
            other.m_pData = nullptr;
            other.m_uSize = 0;
        }

        ~Data() noexcept
        {
            if( m_bOwnBuffer )
            {
                delete[] m_pData;
            }
            m_pData = nullptr;
            m_uSize = 0;
        }

        Data& operator= ( const Data& other )
        {
            Data tmp( other );
            *this = std::move( tmp );
            return *this;
        }

        Data& operator= ( Data&& other ) noexcept
        {
            if( m_bOwnBuffer )
            {
                delete[] m_pData;
            }
            m_pData = other.m_pData;
            other.m_pData = nullptr;
            return *this;
        }
    };

    struct ChunkedData
    {
        size_t m_uChunkSize;
        uint8_t* m_pData;
        size_t m_uSize;

        ChunkedData() :
            m_uChunkSize( 0 ),
            m_pData( nullptr ),
            m_uSize( 0 )
        {
        }

        ChunkedData( const std::string& strText ) :
            m_uChunkSize( strText.size() ),
            m_pData( new uint8_t[ strText.size() ] ),
            m_uSize( strText.size() )
        {
            std::memcpy( m_pData, strText.c_str(), strText.size() );
        }

        ChunkedData( const ChunkedData& other ) :
            m_uChunkSize( 0 ),
            m_pData( new uint8_t[ other.m_uSize ] ),
            m_uSize( 0 )
        {
            std::memcpy( m_pData, other.m_pData, other.m_uSize );
        }

        ChunkedData( ChunkedData&& other ) noexcept :
            m_uChunkSize( other.m_uChunkSize ),
            m_pData( other.m_pData ),
            m_uSize( other.m_uSize )
        {
            other.m_pData = nullptr;
            other.m_uSize = 0;
            other.m_uChunkSize = 0;
        }

        ~ChunkedData() noexcept
        {
            delete[] m_pData;
            m_uSize = 0;
            m_uChunkSize = 0;
        }

        ChunkedData& operator= ( const ChunkedData& other )
        {
            ChunkedData tmp( other );
            *this = std::move( tmp );
            return *this;
        }

        ChunkedData& operator= ( ChunkedData&& other ) noexcept
        {
            delete[] m_pData;
            m_pData = other.m_pData;
            other.m_pData = nullptr;
            return *this;
        }
    };
}

#endif //GLEF_LIB_DATA_HPP
