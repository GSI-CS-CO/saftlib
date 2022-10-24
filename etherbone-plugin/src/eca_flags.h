/** Copyright (C) 2011-2016 GSI Helmholtz Centre for Heavy Ion Research GmbH 
 *
 *  @author Wesley W. Terpstra <w.terpstra@gsi.de>
 *
 *******************************************************************************
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 3 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *  
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *******************************************************************************
 */
#ifndef ECA_FLAGS_H
#define ECA_FLAGS_H

#define ECA_LATE      0
#define ECA_EARLY     1
#define ECA_CONFLICT  2
#define ECA_DELAYED   3
#define ECA_VALID     4
#define ECA_OVERFLOW  5
#define ECA_MAX_FULL  6

#define ECA_LINUX         0
#define ECA_WBM           1
#define ECA_EMBEDDED_CPU  2
#define ECA_SCUBUS        3

#endif
