//------------------------------------------------------------------------------
// Copyright (c) 2024 John D. Haughton
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//------------------------------------------------------------------------------

#pragma once

#include "MTL/rp2040/Usb.h"

#include "MTL/UsbAudioControl.h"
#include "MTL/UsbMIDI.h"

//! Standard Audio Controller interface
class ACInterface : public USB::Interface
{
public:
   ACInterface(USB::Config& config_)
      : USB::Interface(config_, USB::CLASS_AUDIO, USB::AC::SUB_CLASS)
   {
   }

   USB::AC::Header header{descr_list};
};


//! MIDI Streaming interface
class MIDIStreamingInterface : public USB::Interface
{
public:
   MIDIStreamingInterface(USB::Config& config_)
      : USB::Interface(config_, USB::CLASS_AUDIO, USB::MS::SUB_CLASS)
   {
   }

   void configured()
   {
      midi_in.startRx(64);
      printf("Configured\n");
   }

   void buffRx(uint8_t ep_, const uint8_t* data_, unsigned length_) override
   {
      uint32_t word = *(uint32_t*)data_;
      printf("MIDI rx %08X %u\n", word, length_);
   }

   void buffTx(uint8_t ep_) override
   {
      midi_in.startRx(64);
      printf("tx done\n");
   }

   USB::MS::HeaderDescr        header{descr_list};
   USB::MS::JackInDescr        jack_in{descr_list,  USB::MS::EMBEDDED, 1};
   USB::MS::JackOutDescr<1>    jack_out{descr_list, USB::MS::EMBEDDED, 1};
   USB::MS::EndPointDescr      d_midi_in{descr_list, USB::EndPointDescr::OUT, USB::EndPointDescr::BULK};
   USB::MS::CSEndPointDescr<1> cs_midi_in{descr_list, /* jack */ 1};
   USB::MS::EndPointDescr      d_midi_out{descr_list, USB::EndPointDescr::IN, USB::EndPointDescr::BULK};
   USB::MS::CSEndPointDescr<1> cs_midi_out{descr_list, /* jack */ 2};

   MTL::Usb::EndPoint midi_in{d_midi_in};
   MTL::Usb::EndPoint midi_out{d_midi_out};
};


class PicoX7USBDevice : public USB::Device
{
public:
   PicoX7USBDevice()
      : USB::Device(/* vendor */ 0xa704, /* product */ 0x91c0, /* version */ 0x0007)
   {
      setVendor("https://github.com/AnotherJohnH");
      setProduct("picoX7");
      setSerialNumber(MTL_COMMIT);
   }

   USB::Config            config{*this};
   ACInterface            ac_interface{config};
   MIDIStreamingInterface midi_streaming_interface{config};
};

