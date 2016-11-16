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
#define ETHERBONE_THROWS 1

#define __STDC_FORMAT_MACROS
#define __STDC_CONSTANT_MACROS

#include <assert.h>

#include "RegisteredObject.h"
#include "MasterFunctionGenerator.h"
#include "TimingReceiver.h"
#include "fg_regs.h"
#include "clog.h"

namespace saftlib {

MasterFunctionGenerator::MasterFunctionGenerator(const ConstructorType& args)
 : Owned(args.objectPath),
   dev(args.dev),
   functionGenerators(args.functionGenerators)      
{
}

MasterFunctionGenerator::~MasterFunctionGenerator()
{
}


Glib::RefPtr<MasterFunctionGenerator> MasterFunctionGenerator::create(const ConstructorType& args)
{
  return RegisteredObject<MasterFunctionGenerator>::create(args.objectPath, args);
}

bool MasterFunctionGenerator::AppendParameterSet(
  const std::vector< gint16 >& coeff_a,
  const std::vector< gint16 >& coeff_b,
  const std::vector< gint32 >& coeff_c,
  const std::vector< unsigned char >& step,
  const std::vector< unsigned char >& freq,
  const std::vector< unsigned char >& shift_a,
  const std::vector< unsigned char >& shift_b,
  bool arm)
{
  ownerOnly();
  /*
  // confirm lengths match
  unsigned len = coeff_a.size();
  if (coeff_b.size() != len) throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "coeff_b length mismatch");
  if (coeff_c.size() != len) throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "coeff_c length mismatch");
  if (step.size()    != len) throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "step length mismatch");
  if (freq.size()    != len) throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "freq length mismatch");
  if (shift_a.size() != len) throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "shift_a length mismatch");
  if (shift_b.size() != len) throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "shift_b length mismatch");
  
  // validate data
  for (unsigned i = 0; i < len; ++i) {
    if (step[i] >= 8) throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "step must be < 8");
    if (freq[i] >= 8) throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "freq must be < 8");
    if (shift_a[i] > 48) throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "shift_a must be <= 48");
    if (shift_b[i] > 48) throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "shift_b must be <= 48");
  }
  */
  // import the data
	// ...
	bool result=false;
	
	for (auto fg : functionGenerators)
	{
		result |= fg->appendParameterSet(coeff_a, coeff_b, coeff_c, step, freq, shift_a, shift_b);
	}

	return result; 
}


void MasterFunctionGenerator::Flush()
{
  ownerOnly();
	for (auto fg : functionGenerators)
	{		
		fg->flush();
	}

/*  
 	for (auto fg : functionGenerators)
	{
		if (fg->getEnabled())
		{
		    throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "SUb-FG Enabled, cannot Flush");
		}
	}
*/

}


guint32 MasterFunctionGenerator::getStartTag() const
{
  return startTag;
}


void MasterFunctionGenerator::Arm()
{
  ownerOnly();
	for (auto fg : functionGenerators)
	{
		fg->arm();
	}
	
	// wait for arm response ...
	bool all_armed=false;
	do
	{
		all_armed=true;
		for (auto fg : functionGenerators)
		{
			all_armed &= fg->armed;
		}
		sleep(1);
	} while (all_armed == false) ;
		
	
}


void MasterFunctionGenerator::Reset()
{
	for (auto fg : functionGenerators)
	{
		fg->Reset();
	}
}

void MasterFunctionGenerator::Abort()
{
  ownerOnly();
  Reset();
}

void MasterFunctionGenerator::ownerQuit()
{
  // owner quit without Disown? probably a crash => turn off the function generator
  Reset();
}

void MasterFunctionGenerator::setStartTag(guint32 val)
{
  ownerOnly();

 	for (auto fg : functionGenerators)  
 	{
    if (fg->enabled)
	    throw Gio::DBus::Error(Gio::DBus::Error::INVALID_ARGS, "FG Enabled, cannot set StartTag");
	}
  
  if (val != startTag) {
    startTag = val;
  	for (auto fg : functionGenerators)
		{
			fg->startTag=startTag;
		}

    StartTag(startTag);
  }
}

}
