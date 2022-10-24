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

#include "UDPProtocol.hpp"
#include "Data.hpp"

#if defined(WIN32)
#define NOMINMAX
#include <winsock.h>
#elif defined(__linux__)
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#define SOCKET int
#define closesocket(x) close(x)
#endif

#include <algorithm>

bool gelf::UDPProtocol::send( const struct sockaddr_in& pSockAddr, const ChunkedData & rData )
{
    SOCKET sock;

    if( ( sock = socket( AF_INET, SOCK_DGRAM, 0 ) ) == -1 )
    {
        m_strError = "Could not create socket";
        return false;
    }

    size_t uTotalBytesSent = 0;
    uint8_t* pData = rData.m_pData;

    while( uTotalBytesSent < rData.m_uSize )
    {
        int iBytesToSend = static_cast< int >( std::min( rData.m_uChunkSize, rData.m_uSize - uTotalBytesSent ) );
        int iBytesSent = sendto( sock, reinterpret_cast< const char* >( pData ), iBytesToSend, 0, ( struct sockaddr * )&pSockAddr, sizeof( struct sockaddr ) );

        if( iBytesSent != iBytesToSend )
        {
            m_strError = "Was not able to send data";
            return false;
        }

        uTotalBytesSent += iBytesSent;
    }

    closesocket( sock );

    return true;
}

const std::string gelf::UDPProtocol::getError() const
{
    return  m_strError;
}
