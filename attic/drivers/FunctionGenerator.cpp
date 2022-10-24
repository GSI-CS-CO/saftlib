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

#include <iostream>

#include <assert.h>

#include "RegisteredObject.h"
#include "FunctionGenerator.h"
#include "TimingReceiver.h"
#include "fg_regs.h"
#include "clog.h"

namespace saftlib {

FunctionGenerator::FunctionGenerator(const ConstructorType& args)
 : Owned(args.objectPath),
   dev(args.dev),
   fgImpl(args.functionGeneratorImpl)      
{
	fgImpl->signal_running.connect(sigc::mem_fun(*this, &FunctionGenerator::on_fg_running));
	fgImpl->signal_armed.connect(sigc::mem_fun(*this, &FunctionGenerator::on_fg_armed));
	fgImpl->signal_enabled.connect(sigc::mem_fun(*this, &FunctionGenerator::on_fg_enabled));
	fgImpl->signal_refill.connect(sigc::mem_fun(*this, &FunctionGenerator::on_fg_refill));
	fgImpl->signal_started.connect(sigc::mem_fun(*this, &FunctionGenerator::on_fg_started));
	fgImpl->signal_stopped.connect(sigc::mem_fun(*this, &FunctionGenerator::on_fg_stopped));

}

FunctionGenerator::~FunctionGenerator()
{
}

// pass sigc signals from impl class to dbus
// to reduce traffic only generate signals if we have an owner
void FunctionGenerator::on_fg_running(bool b)
{
  if (!getOwner().empty())
  {
    SigRunning(b);
  }
}

void FunctionGenerator::on_fg_armed(bool b)
{
  if (!getOwner().empty())
  {  
 	  SigArmed(b);
  }
}

void FunctionGenerator::on_fg_enabled(bool b)
{
  if (!getOwner().empty())
  {
  	SigEnabled(b);
  }
}

void FunctionGenerator::on_fg_refill()
{
  if (!getOwner().empty())
  {
	  Refill();
  }
}


void FunctionGenerator::on_fg_started(uint64_t time)
{
  if (!getOwner().empty())
  {
    SigStarted(saftlib::makeTimeTAI(time));
  }
}

void FunctionGenerator::on_fg_stopped(uint64_t time, bool abort, bool hardwareUnderflow, bool microcontrollerUnderflow)
{
  if (!getOwner().empty())
  {
    SigStopped(saftlib::makeTimeTAI(time), abort, hardwareUnderflow, microcontrollerUnderflow);
  }
}


std::shared_ptr<FunctionGenerator> FunctionGenerator::create(const ConstructorType& args)
{
  return RegisteredObject<FunctionGenerator>::create(args.objectPath, args);
}


bool FunctionGenerator::AppendParameterSet(
  const std::vector< int16_t >& coeff_a,
  const std::vector< int16_t >& coeff_b,
  const std::vector< int32_t >& coeff_c,
  const std::vector< unsigned char >& step,
  const std::vector< unsigned char >& freq,
  const std::vector< unsigned char >& shift_a,
  const std::vector< unsigned char >& shift_b)
{
  ownerOnly();
  return fgImpl->appendParameterSet(coeff_a, coeff_b, coeff_c, step, freq, shift_a, shift_b);  
}

void FunctionGenerator::Flush()
{
  ownerOnly();
 	fgImpl->Flush();
}

uint32_t FunctionGenerator::getVersion() const
{
  return fgImpl->getVersion();
}

unsigned char FunctionGenerator::getSCUbusSlot() const
{
  return fgImpl->getSCUbusSlot();
}

unsigned char FunctionGenerator::getDeviceNumber() const
{
  return fgImpl->getDeviceNumber();
}

unsigned char FunctionGenerator::getOutputWindowSize() const
{
  return fgImpl->getOutputWindowSize();
}

bool FunctionGenerator::getEnabled() const
{
  return fgImpl->getEnabled();
}

bool FunctionGenerator::getArmed() const
{
  return fgImpl->getArmed();
}

bool FunctionGenerator::getRunning() const
{
  return fgImpl->getRunning();
}

uint32_t FunctionGenerator::getStartTag() const
{
  return fgImpl->getStartTag();
}

uint64_t FunctionGenerator::ReadFillLevel()
{
  return fgImpl->ReadFillLevel();
}

uint32_t FunctionGenerator::ReadExecutedParameterCount()
{
	return fgImpl->ReadExecutedParameterCount();
}


void FunctionGenerator::Arm()
{
  ownerOnly();
  fgImpl->arm();
}


void FunctionGenerator::Reset()
{
	fgImpl->Reset();
}

void FunctionGenerator::Abort()
{
  ownerOnly();
  Reset();
}

void FunctionGenerator::ownerQuit()
{
  // owner quit without Disown? probably a crash => turn off the function generator
  Reset();
}

void FunctionGenerator::setStartTag(uint32_t val)
{
  ownerOnly();
  fgImpl->setStartTag(val);
}


}
