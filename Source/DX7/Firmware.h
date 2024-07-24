//------------------------------------------------------------------------------
// The data in this file is a derivative of data reverse engineered from the
// DX7 firmware ROM published on AJXS github project. That publicly
// available project does not contain a copyright notice or any mention of
// permissions or restrictions that apply to the use of the data. However,
// according to law that does not necessarily mean that the data is free from
// restrictions on its use. At the time of publication, the copyright holder
// is probably Yamaha. If the copyright holder wishes to retrospectively declare
// reasonable and legal restrictions on the data, then either those restrictions
// must be obeyed or this file shouled be deleted.
//
// The statements above shall be included in all copies or substantial portions
// of the data.
//
// THE DATA IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE DATA OR THE USE OR OTHER DEALINGS IN THE
// DATA.
//------------------------------------------------------------------------------
//
// \brief DX7 firmware simulation
//
// Credit to AJXS for his excellent disassembly of the DX7 firmware ROM

#pragma once

#include "Egs.h"
#include "Lfo.h"

//! Model of Yamaha DX7 firmware
class Firmware
{
public:
   Firmware(Egs& hw_)
      : hw(hw_)
   {
   }

   //! Implement PATCH_LOAD_DATA
   void loadData(const SysEx::Voice* patch_)
   {
      patch = *patch_;

      for(unsigned i = 0; i < SysEx::NUM_OP; i++)
      {
         const SysEx::Op& op = patch.op[i];

         loadOperatorEgRates(i, op);
         loadOperatorEgLevels(i, op);
         loadOperatorKbdScaling(i, op);
         loadOperatorKbdVelSens(i, op);
         loadOperatorPitch(i, op);
         loadOperatorKbdRateScaling(i, op);
         loadOperatorDetune(i, op);
      }

      loadPitchEgValues();
      loadAlgMode();
      loadLfo();

      // TODO remove
      hw.prog(&patch);
   }

   //! Implement HANDLER_OCF should be called 375 Hz
   void tick()
   {
      lfo.tick();
   }

   //! Implement VOICE_ADD called for a note on event
   void voiceAdd(uint8_t note_, uint8_t velocity_)
   {
      uint8_t note = note_ + patch.transpose - 24;
      if (note > 127)
         note = 127;

      voiceConvertNoteToPitch(note);

      lfo.keyOn();

      voiceAddLoadOperatorDataToEgs(key_pitch, velocity_);

      hw.keyOn();
   }

   //! Implement VOICE_REMOVE called for a note off event
   void voiceRemove()
   {
      hw.keyOff();
   }

   //! Implement PITCH_BEND_PARSE
   void setPitchBend(int16_t raw14_)
   {
   }

   //! Set raw modulation input
   void setModWheel(        uint8_t raw_) { mod_wheel      = raw_; }
   void setModFootControl(  uint8_t raw_) { foot_control   = raw_; }
   void setModBreathControl(uint8_t raw_) { breath_control = raw_; }
   void setModAfterTouch(   uint8_t raw_) { after_touch    = raw_; }

private:
   //! Implement PATCH_LOAD_OPERATOR_EG_RATES
   void loadOperatorEgRates(unsigned op_index_, const SysEx::Op& op_)
   {
      for(unsigned i = 0; i < 4; ++i)
      {
         uint8_t rate6 = (op_.eg_amp.rate[i] * 164) >> 8;

         hw.setEgsOpEgRate(op_index_, i, rate6);
      }
   }

   //! Implement PATCH_LOAD_OPERATOR_EG_LEVELS
   void loadOperatorEgLevels(unsigned op_index_, const SysEx::Op& op_)
   {
      for(unsigned i = 0; i < 4; ++i)
      {
         uint8_t level6 = table_log[op_.eg_amp.level[i]] >> 1;

         hw.setEgsOpEgLevel(op_index_, i, level6);
      }
   }

   //! Implement PATCH_LOAD_OPERATOR_KBD_SCALING
   void loadOperatorKbdScaling(unsigned index, const SysEx::Op& op)
   {
      // TODO
   }

   //! Implement PATCH_LOAD_OPERATOR_VEL_SENS
   void loadOperatorKbdVelSens(unsigned index, const SysEx::Op& op)
   {
      // TODO
   }

   //! Implement PATCH_LOAD_OPERATOR_PITCH
   void loadOperatorPitch(unsigned index, const SysEx::Op& op)
   {
      if (op.osc_mode == SysEx::RATIO)
      {
         const uint16_t table_op_freq_coarse[32] =
         {
            0xF000, 0x0000, 0x1000, 0x195C, 0x2000, 0x2528, 0x295C, 0x2CEC,
            0x3000, 0x32B8, 0x3528, 0x375A, 0x395C, 0x3B34, 0x3CEC, 0x3E84,
            0x4000, 0x4168, 0x42B8, 0x43F8, 0x4528, 0x4648, 0x475A, 0x4860,
            0x495C, 0x4A4C, 0x4B34, 0x4C14, 0x4CEC, 0x4DBA, 0x4E84, 0x4F44
         };

         const uint16_t table_op_freq_fine[100] =
         {
            0x000, 0x03A, 0x075, 0x0AE, 0x0E7, 0x120, 0x158, 0x18F, 0x1C6, 0x1FD,
            0x233, 0x268, 0x29D, 0x2D2, 0x306, 0x339, 0x36D, 0x39F, 0x3D2, 0x403,
            0x435, 0x466, 0x497, 0x4C7, 0x4F7, 0x526, 0x555, 0x584, 0x5B2, 0x5E0,
            0x60E, 0x63B, 0x668, 0x695, 0x6C1, 0x6ED, 0x719, 0x744, 0x76F, 0x799,
            0x7C4, 0x7EE, 0x818, 0x841, 0x86A, 0x893, 0x8BC, 0x8E4, 0x90C, 0x934,
            0x95C, 0x983, 0x9AA, 0x9D1, 0x9F7, 0xA1D, 0xA43, 0xA69, 0xA8F, 0xAB4,
            0xAD9, 0xAFE, 0xB22, 0xB47, 0xB6B, 0xB8F, 0xBB2, 0xBD6, 0xBF9, 0xC1C,
            0xC3F, 0xC62, 0xC84, 0xCA7, 0xCC9, 0xCEA, 0xD0C, 0xD2E, 0xD4F, 0xD70,
            0xD91, 0xDB2, 0xDD2, 0xDF3, 0xE13, 0xE33, 0xE53, 0xE72, 0xE92, 0xEB1,
            0xED0, 0xEEF, 0xF0E, 0xF2D, 0xF4C, 0xF6A, 0xF88, 0xFA6, 0xFC4, 0xFE2
         };

         hw.setEgsOpPitchFixed(index, false);

         hw.setEgsOpPitch(index, table_op_freq_coarse[op.osc_freq_coarse] +
                                 table_op_freq_fine[op.osc_freq_fine] +
                                 0x232C);
      }
      else
      {
         const uint16_t table_op_freq_fixed[4] =
         {
            0x0000, 0x3526, 0x6A4C, 0x9F74
         };

         hw.setEgsOpPitchFixed(index, true);

         hw.setEgsOpPitch(index, table_op_freq_fixed[op.osc_freq_coarse & 0b11] +
                                 op.osc_freq_fine * 136 +
                                 0x16AC);
      }
   }

   void loadOperatorKbdRateScaling(unsigned index, const SysEx::Op& op)
   {
      hw.setEgsOpRateScaling(index, op.kbd_rate_scale);
      hw.setEgsOpAmpModSens(index, op.amp_mod_sense);
   }

   //! Implement PATCH_LOAD_OPERATOR_DETUNE
   void loadOperatorDetune(unsigned index, const SysEx::Op& op)
   {
      hw.setEgsOpDetune(index, op.osc_detune);
   }

   //! Implement PATCH_LOAD_PITCH_EG_VALUES
   void loadPitchEgValues()
   {
   }

   //! Implement PATCH_LOAD_ALG_MODE
   void loadAlgMode()
   {
      hw.setOpsSync(patch.osc_sync);
      hw.setOpsAlg(patch.alg);
      hw.setOpsFdbk(patch.feedback);
   }

   //! Implement PATCH_LOAD_LFO
   void loadLfo()
   {
      lfo.prog(patch);
   }

   //! Implement VOICE_CONVERT_NOTE_TO_PITCH
   void voiceConvertNoteToPitch(uint8_t note)
   {
      uint8_t value   = table_key_pitch[note];
      uint8_t ls_bits = value & 0b11;
      key_pitch       = (value << 6) | (ls_bits << 4) | (ls_bits << 2);
   }

   //! Implement VOICE_ADDLOAD_OPERATOR_DATA_TO_EGS
   void voiceAddLoadOperatorDataToEgs(uint16_t pitch, uint8_t note_vel)
   {
      hw.setEgsVoicePitch(pitch + master_tune);

      for(unsigned op_index = 0; op_index < SysEx::NUM_OP; ++op_index)
      {
         hw.setOpsFreq(op_index, hw.getEgsFreq(op_index));
      }
   }

   //! Implement MOD_PROCESS_INPUT_SOURCES
   void modProcessInputSources()
   {
      eg_bias_total_range = 0;
      for(unsigned i = 0; i < NUM_MOD_SOURCE; ++i)
      {
         mod[i].computeScaledInput(eg_bias_total_range);
      }

      // Calculate amplitude modulation and EG bias
      amp_mod = 0;
      eg_bias = 0;

      for(unsigned i = 0; i < NUM_MOD_SOURCE; ++i)
      {
         if (mod[i].isAmplitude())
            amp_mod += mod[i]();

         if (mod[i].isEgBias())
            eg_bias += mod[i]();
      }

      if (amp_mod > 0xFF)
         amp_mod = 0xFF;

      if (eg_bias > 0xFF)
         eg_bias = 0xFF;
   }

   //! Implement MOD_PITCH_LOAD_TO_EGS
   void modPitchLoadToEgs()
   {
      unsigned pitch_mod = 0;

      for(unsigned i = 0; i < NUM_MOD_SOURCE; ++i)
      {
         if (mod[i].isPitch())
            pitch_mod += mod[i]();
      }

      if (pitch_mod > 0xFF)
         pitch_mod = 0xFF;

      pitch_mod += pitch_bend;

      hw.setEgsPitchMod(pitch_mod);
   }

   const uint8_t table_log[100] =
   {
      0x7F, 0x7A, 0x76, 0x72, 0x6E, 0x6B, 0x68, 0x66,
      0x64, 0x62, 0x60, 0x5E, 0x5C, 0x5A, 0x58, 0x56,
      0x55, 0x54, 0x52, 0x51, 0x4F, 0x4E, 0x4D, 0x4C,
      0x4B, 0x4A, 0x49, 0x48, 0x47, 0x46, 0x45, 0x44,
      0x43, 0x42, 0x41, 0x40, 0x3F, 0x3E, 0x3D, 0x3C,
      0x3B, 0x3A, 0x39, 0x38, 0x37, 0x36, 0x35, 0x34,
      0x33, 0x32, 0x31, 0x30, 0x2F, 0x2E, 0x2D, 0x2C,
      0x2B, 0x2A, 0x29, 0x28, 0x27, 0x26, 0x25, 0x24,
      0x23, 0x22, 0x21, 0x20, 0x1F, 0x1E, 0x1D, 0x1C,
      0x1B, 0x1A, 0x19, 0x18, 0x17, 0x16, 0x15, 0x14,
      0x13, 0x12, 0x11, 0x10, 0x0F, 0x0E, 0x0D, 0x0C,
      0x0B, 0x0A, 0x09, 0x08, 0x07, 0x06, 0x05, 0x04,
      0x03, 0x02, 0x01, 0x00
   };

   const uint8_t table_key_pitch[128] =
   {
      0x00, 0x00, 0x01, 0x02, 0x04, 0x05, 0x06, 0x08,
      0x09, 0x0A, 0x0C, 0x0D, 0x0E, 0x10, 0x11, 0x12,
      0x14, 0x15, 0x16, 0x18, 0x19, 0x1A, 0x1C, 0x1D,
      0x1E, 0x20, 0x21, 0x22, 0x24, 0x25, 0x26, 0x28,
      0x29, 0x2A, 0x2C, 0x2D, 0x2E, 0x30, 0x31, 0x32,
      0x34, 0x35, 0x36, 0x38, 0x39, 0x3A, 0x3C, 0x3D,
      0x3E, 0x40, 0x41, 0x42, 0x44, 0x45, 0x46, 0x48,
      0x49, 0x4A, 0x4C, 0x4D, 0x4E, 0x50, 0x51, 0x52,
      0x54, 0x55, 0x56, 0x58, 0x59, 0x5A, 0x5C, 0x5D,
      0x5E, 0x60, 0x61, 0x62, 0x64, 0x65, 0x66, 0x68,
      0x69, 0x6A, 0x6C, 0x6D, 0x6E, 0x70, 0x71, 0x72,
      0x74, 0x75, 0x76, 0x78, 0x79, 0x7A, 0x7C, 0x7D,
      0x7E, 0x80, 0x81, 0x82, 0x84, 0x85, 0x86, 0x88,
      0x89, 0x8A, 0x8C, 0x8D, 0x8E, 0x90, 0x91, 0x92,
      0x94, 0x95, 0x96, 0x98, 0x99, 0x9A, 0x9C, 0x9D,
      0x9E, 0xA0, 0xA1, 0xA2, 0xA4, 0xA5, 0xA6, 0xA8
   };

   class Modulation
   {
   public:
      Modulation() = default;

      bool isPitch() const     { return pitch; }
      bool isAmplitude() const { return amplitude; }
      bool isEgBias() const    { return eg_bias; }

      uint8_t operator()() const { return scaled_input; }

      void prog(bool pitch_, bool amplitude_, bool eg_bias_, uint8_t range_)
      {
         pitch     = pitch_;
         amplitude = amplitude_;
         eg_bias   = eg_bias_;
         range     = range_;
      }

      //! Set the raw input for the modulation source
      void operator=(uint8_t raw_input_)
      {
         raw_input = raw_input_;
      }

      //! Implement MOD_CALCULATE_SOURCE_SCALED_INPUT
      void computeScaledInput(uint8_t& eg_bias_total_range)
      {
         // Quantise range
         uint8_t q_range = (range * 660) >> 8;

         // Accumulate EG bias
         if (eg_bias)
         {
            unsigned total_range = eg_bias_total_range + q_range;

            if (total_range > 0xFF)
               total_range = 0xFF;

            eg_bias_total_range = total_range;
         }

         scaled_input = (raw_input * q_range) >> 8;
      }

   private:
      bool    pitch {false};
      bool    amplitude {false};
      bool    eg_bias {false};
      uint8_t raw_input {0};
      uint8_t scaled_input {0};
      uint8_t range {50};
   };

   SysEx::Voice  patch;
   Egs&          hw;                   //! Access to simulated DX7 EGS and OPS hardware
   uint16_t      master_tune {0x959};  //  XXX too high should default to 0x100
   uint16_t      key_pitch;
   Lfo           lfo;                  //! Firmware LFO

   // Modulation sources
   static const unsigned NUM_MOD_SOURCE {4};

   Modulation    mod[NUM_MOD_SOURCE];
   Modulation&   mod_wheel      {mod[0]};
   Modulation&   foot_control   {mod[1]};
   Modulation&   breath_control {mod[2]};
   Modulation&   after_touch    {mod[3]};

   uint8_t       eg_bias_total_range{};
   uint8_t       amp_mod {};
   uint8_t       eg_bias{};

   int16_t       pitch_bend;
};
