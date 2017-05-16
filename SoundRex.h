#ifndef __SOUNDREX__
#define __SOUNDREX__

#include <cstdint>
typedef uint32_t uint32;

#include "IPlug_include_in_plug_hdr.h"

class SoundRex : public IPlug {
public:
  SoundRex(IPlugInstanceInfo instanceInfo);
  ~SoundRex();

  void Reset();
  void OnParamChange(int paramIdx);
  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);

private:
  double mGain;
};

#endif
