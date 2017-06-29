#include "effect_gen.hpp"
#include "SoundRex.hpp"

#include "IPlug_include_in_plug_src.h"
#include "IControl.h"
#include "resource.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

typedef uint8_t sample_t;
constexpr sample_t maxval =  255;

constexpr int num_samples = 384;
constexpr int num_channels = 3;

std::array<effect_t, num_channels> effects;
uint32_t packet_num = 0;
sockaddr_in scaClient, scaServer;
socklen_t addrlen = sizeof(scaServer);
int sockfd = 0;

template <typename T, int N>
struct valarray_t {
  std::array<T,N> data;
  valarray_t(T val=0) {
    for (T &t: data)
      t = val;
  }

  valarray_t<T,N> operator+(valarray_t<T,N> const &other) const {
    valarray_t<T,N> res;
    for (int i=0; i<N; ++i) {
      res.data[i] = data[i] + other.data[i];
    }
    return res;
  }

  valarray_t<T,N> operator-(valarray_t<T,N> const &other) const {
    valarray_t<T,N> res;
    for (int i=0; i<N; ++i) {
      res.data[i] = data[i] - other.data[i];
    }
    return res;
  }

  valarray_t<T,N> operator*(T const &mul) const {
    valarray_t<T,N> res;
    for (int i=0; i<N; ++i) {
      res.data[i] = mul * data[i];
    }
    return res;
  }
};

typedef valarray_t<double, num_channels> value_t;

void send_to_router(void const *data, size_t size) {
  if (sockfd == 0) {
    throw std::runtime_error("Socket not present for sending data.");
  }
  sendto(sockfd, data, size, 0, (sockaddr const *)&scaServer, sizeof(scaServer));
}

void transmit_data() {
  std::vector<uint8_t> to_send;
  to_send.push_back(0xff);
  to_send.push_back(num_channels);

  std::stringstream effect_stream;
  for (effect_t &effect: effects)
    effect_stream << effect << ' ';

  std::string effect_str = effect_stream.str();
  write_to_vec(effect_str.c_str(), to_send);
  send_to_router(to_send.data(), to_send.size());
}

static constexpr int kNumPrograms = 1;

enum EParams {
  kNumRows = 0,
  kNumColumns,
  kType1,
  kPeriod1,
  kType2,
  kPeriod2,
  kType3,
  kPeriod3,
  kNumParams
};

void setEffectNames(IParam *param) {
  param->SetDisplayText(effect_t::Immersive, "Immersive");
  param->SetDisplayText(effect_t::LeftHalf, "Left Half");
  param->SetDisplayText(effect_t::RightHalf, "Right Half");
  param->SetDisplayText(effect_t::LeftToRight, "Left to Right");
  param->SetDisplayText(effect_t::Circular, "Circular");
  param->SetDisplayText(effect_t::Randomized, "Randomized");
  param->SetDisplayText(effect_t::SpreadingOut, "Spreading Out");
  param->SetDisplayText(effect_t::Diagonal, "Diagonal");
}

SoundRex::SoundRex(IPlugInstanceInfo instanceInfo)
:	IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo) {
  TRACE;

  //arguments are: name, defaultVal, minVal, maxVal
  GetParam(kNumRows)->InitInt("Number of Rows", 10, 1, 50);
  GetParam(kNumColumns)->InitInt("Number of Columns", 10, 1, 50);

  //arguments are: name, defaultVal, numEnums
  GetParam(kType1)->InitEnum("Effect 1", effect_t::Immersive, effect_t::typeNum);
  GetParam(kType2)->InitEnum("Effect 2", effect_t::Immersive, effect_t::typeNum);
  GetParam(kType3)->InitEnum("Effect 3", effect_t::Immersive, effect_t::typeNum);
  setEffectNames(GetParam(kType1));
  setEffectNames(GetParam(kType2));
  setEffectNames(GetParam(kType3));

  //arguments are: name, defaultVal, minVal, maxVal, step, label
  GetParam(kPeriod1)->InitDouble("Time Period 1", 1., 0.01, 20.0, 0.01, "s");
  GetParam(kPeriod2)->InitDouble("Time Period 2", 1., 0.01, 20.0, 0.01, "s");
  GetParam(kPeriod3)->InitDouble("Time Period 3", 1., 0.01, 20.0, 0.01, "s");

  //MakePreset("preset 1", ... );
  MakeDefaultPreset((char *) "-", kNumPrograms);

  scaClient.sin_family = AF_INET;
  scaClient.sin_addr.s_addr = inet_addr("192.167.1.106");
  scaClient.sin_port = htons(9428);

  scaServer.sin_family = AF_INET;
  scaServer.sin_addr.s_addr = inet_addr("192.167.1.1");
  scaServer.sin_port = htons(9430);
}

void SoundRex::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames) {
  static std::array<value_t, 4> buffer;
  static int bufdex = 0;
  static double pos = 1;
  static std::array<uint8_t, sizeof(sample_t) * num_channels * num_samples + 1> packet;
  static auto itr = packet.begin() + 1;

  double const incr = GetSampleRate()/24000;
  // Mutex is already locked for us.
  for (int s = 0; s < nFrames; ++s) {
    for (int t=0; t<num_channels; ++t) {
      buffer[bufdex].data[t] = inputs[t][s];
    }
    bufdex = (bufdex+1)&3;

    for (pos-=1; pos<1; pos+=incr) {
      double const vec = 2*pos-1;
      value_t const &m3_2 = buffer[bufdex&3];
      value_t const &m1_2 = buffer[(bufdex+1)&3];
      value_t const &p1_2 = buffer[(bufdex+2)&3];
      value_t const &p3_2 = buffer[(bufdex+3)&3];
      value_t const g1 = p1_2 + m1_2;
      value_t const g3 = p3_2 + m3_2;
      value_t const h1 = p1_2 - m1_2;
      value_t const h3 = (p3_2 - m3_2)*(1./3);

      value_t val =  ((g1*9 - g3) + ((h1*9 - h3) + ((g3-g1) + (h3-h1)*vec)*vec)*vec) * (1.0/16);
      val = (val + value_t(1))*(maxval/2.);
      for (double v: val.data)
        *itr++ = static_cast<uint8_t>(v);

      if (itr == packet.end()) {
        itr = packet.begin();
        *itr++ = packet_num % 252;
        if (packet_num % num_skip == 0)
          transmit_data();

        send_to_router(packet.data(), packet.size());
        ++packet_num;
      }
    }
  }
}

void SoundRex::Reset() {
  TRACE;
  IMutexLock lock(this);
}

void SoundRex::OnActivate(bool active) {
  TRACE;
  IMutexLock lock(this);
  if (active && sockfd == 0) {
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
      throw std::runtime_error("Can't create socket.");
  } else {
    close(sockfd);
    sockfd = 0;
  }
}

void SoundRex::OnParamChange(int paramIdx) {
  IMutexLock lock(this);
  switch (paramIdx) {
    case kNumRows:
      set_rows(GetParam(paramIdx)->Value());
      break;
    case kNumColumns:
      set_cols(GetParam(paramIdx)->Value());
      break;
    case kType1:
      effects[0].type = (effect_t::type_t)(GetParam(paramIdx)->Int());
      break;
    case kPeriod1:
      effects[0].timePeriod_s = GetParam(paramIdx)->Value();
      break;
    case kType2:
      effects[1].type = (effect_t::type_t)(GetParam(paramIdx)->Int());
      break;
    case kPeriod2:
      effects[1].timePeriod_s = GetParam(paramIdx)->Value();
      break;
    case kType3:
      effects[2].type = (effect_t::type_t)(GetParam(paramIdx)->Int());
      break;
    case kPeriod3:
      effects[2].timePeriod_s = GetParam(paramIdx)->Value();
      break;
    default:
      break;
  }
}
