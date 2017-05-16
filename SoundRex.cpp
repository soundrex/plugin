#include "SoundRex.h"
#include "IPlug_include_in_plug_src.h"
#include "IControl.h"
#include "resource.h"

const int kNumPrograms = 1;

enum EParams {
  kGain = 0,
  kNumParams
};

int const kWidth = GUI_WIDTH;
int const kHeight = GUI_HEIGHT;

int const kGainX = 100;
int const kGainY = 100;
int const kKnobFrames = 60;

SoundRex::SoundRex(IPlugInstanceInfo instanceInfo)
:	IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo), mGain(1.) {
  TRACE;

  //arguments are: name, defaultVal, minVal, maxVal, step, label
  GetParam(kGain)->InitDouble("Gain", 50., 0., 100.0, 0.01, "%");
  GetParam(kGain)->SetShape(1.);

  IGraphics* pGraphics = MakeGraphics(this, kWidth, kHeight);
  pGraphics->AttachPanelBackground(&COLOR_RED);

  IBitmap knob = pGraphics->LoadIBitmap(KNOB_ID, KNOB_FN, kKnobFrames);

  pGraphics->AttachControl(new IKnobMultiControl(this, kGainX, kGainY, kGain, &knob));

  AttachGraphics(pGraphics);

  //MakePreset("preset 1", ... );
  MakeDefaultPreset((char *) "-", kNumPrograms);
}

SoundRex::~SoundRex() {}

void SoundRex::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames) {
  // Mutex is already locked for us.

  double* in1 = inputs[0];
  double* in2 = inputs[1];
  double* out1 = outputs[0];
  double* out2 = outputs[1];

  for (int s = 0; s < nFrames; ++s, ++in1, ++in2, ++out1, ++out2) {
    *out1 = *in1 * mGain;
    *out2 = *in2 * mGain;
  }
}

void SoundRex::Reset() {
  TRACE;
  IMutexLock lock(this);
}

void SoundRex::OnParamChange(int paramIdx) {
  IMutexLock lock(this);

  if (paramIdx == kGain)
    mGain = GetParam(kGain)->Value() / 100.;
}
