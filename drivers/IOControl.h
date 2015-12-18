#ifndef IOCONTROL_H
#define IOCONTROL_H

#include <stdio.h>

#include "Owned.h"
#include "io_control_regs.h"
#include "interfaces/IOControl.h"

#define IO_DEBUG_MODE 0 /* Additional klog messages 0/1 */

namespace saftlib 
{
  
  class TimingReceiver;
  
  class IOControl : public Owned, public iIOControl
  {
    /* Public */
    /* ==================================================================================================== */
    public:
      typedef IOControl_Service ServiceType;
      
      struct ConstructorType {
        TimingReceiver* dev;
        guint32 DeviceAddress;
      };
      
      static Glib::RefPtr<IOControl> create(const Glib::ustring& objectPath, ConstructorType args);
      
      void SetParameter(guint32 param, guint32 value);
      guint32 GetParameter(guint32 param);
      
      guint32 CheckIoName(const Glib::ustring& name);
      guint32 GetIoTableParameterByNumber(guint32 number, guint32 param);
      guint32 GetIoTableParameterByName(const Glib::ustring& name, guint32 param);
      guint32 GetIoTableNumber(const Glib::ustring& name);
      Glib::ustring GetIoTableName(guint32 number);
      guint32 GetInternalId(const Glib::ustring& name);
      guint32 GetDirection(const Glib::ustring& name);
      guint32 GetChannel(const Glib::ustring& name);
      
      guint32 SetOe(const Glib::ustring& name);
      guint32 ResetOe(const Glib::ustring& name);
      guint32 GetOe(const Glib::ustring& name);
      guint32 SetTerm(const Glib::ustring& name);
      guint32 ResetTerm(const Glib::ustring& name);
      guint32 GetTerm(const Glib::ustring& name);
      guint32 SetSpecIn(const Glib::ustring& name);
      guint32 ResetSpecIn(const Glib::ustring& name);
      guint32 GetSpecIn(const Glib::ustring& name);
      guint32 SetSpecOut(const Glib::ustring& name);
      guint32 ResetSpecOut(const Glib::ustring& name);
      guint32 GetSpecOut(const Glib::ustring& name);
      guint32 SetMux(const Glib::ustring& name);
      guint32 ResetMux(const Glib::ustring& name);
      guint32 GetMux(const Glib::ustring& name);
      guint32 DriveLow(const Glib::ustring& name);
      guint32 DriveHigh(const Glib::ustring& name);
      guint32 GetOutput(const Glib::ustring& name);
      guint32 GetOutputCombined(const Glib::ustring& name);
      guint32 GetInput(const Glib::ustring& name);
      
      guint32 getGPIOInputs() const;
      guint32 getGPIOInoutputs() const;
      guint32 getGPIOOutputs() const;
      guint32 getGPIOTotal() const;
      guint32 getLVDSInputs() const;
      guint32 getLVDSInoutputs() const;
      guint32 getLVDSOutputs() const;
      guint32 getLVDSTotal() const;
      guint32 getFIXEDTotal() const;
      guint32 getIOVersion() const;
      Glib::ustring getDeviceName() const;
      Glib::ustring getBuildName() const;
      
      s_IOCONTROL_SetupField s_aIOCONTROL_SetupField[IO_GPIO_MAX+IO_LVDS_MAX+IO_FIXED_MAX];
      
    /* Private */
    /* ==================================================================================================== */
    private:
      guint32 CommandWrapper(const Glib::ustring& name, guint32 operation, guint32 type, guint32 * value);
      guint32 DriveWrapper(const Glib::ustring& name, guint32 value);
      guint32 SetSel(const Glib::ustring& name);
      guint32 ResetSel(const Glib::ustring& name);
      guint32 GetSel(const Glib::ustring& name);
    
    /* Protected */
    /* ==================================================================================================== */
    protected:
      IOControl(ConstructorType args);
      ~IOControl();
      
      TimingReceiver* dev;
      guint32 DeviceAddress;
      guint32 GPIOInputs, GPIOInoutputs, GPIOOutputs, GPIOTotal;
      guint32 LVDSInputs, LVDSInoutputs, LVDSOutputs, LVDSTotal;
      guint32 FIXEDTotal;
      guint32 IOVersion;
      Glib::ustring DeviceName;
      char c_aDeviceName[IO_DEVICE_NAME_BYTES];
      
  };

} /* namespace saftlib  */

#endif
