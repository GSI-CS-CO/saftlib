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

#ifndef GELF_LIB_GELF_HPP
#define GELF_LIB_GELF_HPP

#include <cinttypes>
#include <string>

#include "DllDefines.hpp"
#include "MessageBuilder.hpp"

namespace gelf
{
    /**
     * Initializes internal library resources. Only after calling that method, other methods can be used.
     * @see gelf::destroy()
     * @return Returns true if library were initialized successfully, otherwise false.
     */
    GELFLIB_EXPORT bool initialize();

    /**
     * Destroys internal library resources.
     * @see gelf::initialize()
     */
    GELFLIB_EXPORT void destroy();

    /**
     * Posts a message to configured server.
     * @param rMessage A message to be sent
     * @return Returns true if message were sent successfully, otherwise false.
     */
    GELFLIB_EXPORT bool post( const gelf::Message& rMessage );

    /**
     * Posts a message.
     * @param rMessage A message to be sent
     * @param strHost Host IP address
     * @param uPort A port
     * @return Returns true if message were sent successfully, otherwise false.
     */
    GELFLIB_EXPORT bool post( const gelf::Message& rMessage, const std::string & strHost, uint16_t uPort );

    /**
     * Configures default server
     * @param strHost Host IP address
     * @param uPort A port
     */
    GELFLIB_EXPORT void configure( const std::string& strHost, uint16_t uPort );

    /**
     * Gets configured host IP address
     * @return Returns string with configured IP address
     */
    GELFLIB_EXPORT const std::string& getConfiguredHost();

    /**
     * Gets configured port
     * @return Returns a port
     */
    GELFLIB_EXPORT const uint16_t getConfiguredPort();
}

#endif //GELF_LIB_GLEF_HPP
