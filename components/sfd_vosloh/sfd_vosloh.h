#pragma once

#include "esphome.h"
#include "esphome/core/automation.h"
#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/components/sensor/sensor.h"
#include "esphome/components/binary_sensor/binary_sensor.h"

namespace esphome::sfd_vosloh
{
  class sfdVosloh;

  static const char *TAG = "sfd_vosloh";

  static const uint8_t RAW = 0b00000000;
  static const uint8_t ALIGN_LEFT = 0b10000000;
  static const uint8_t ALIGN_CENTER = 0b01000000;
  static const uint8_t ALIGN_RIGHT = 0b00100000;
  static const uint8_t WORD_WRAP = 0b00010000;
  static const uint8_t OVERWRITE = 0b00001000;

  class sfdVosloh : public Component, public uart::UARTDevice, public text_sensor::TextSensor, public sensor::Sensor, public binary_sensor::BinarySensor
  {
    protected:
      const uint8_t POSITION_MAX = 127;
      const uint8_t UART_TIMEOUT = 10;

      const uint8_t _WRITE = 0x88;
      const uint8_t _READ = 0x85;
      const uint8_t _STATE = 0x84;
      const uint8_t _ADAPT = 0x81;
      const uint8_t _ROLL = 0x82;

      int row_length;
      int last_module;
      int current_position = 1;
      int blocked = 0;
      
      const uint8_t _JOB_NONE = 0;
      const uint8_t _JOB_STATE = 1;
      const uint8_t _JOB_CONTENT = 2;
      uint8_t current_job = _JOB_NONE;
      bool respond_asked = false;
      bool respond_cached = false;
      uint8_t respond_cache;
      uint32_t respond_timeout;
      uint8_t respond_position = 0;
      uint8_t get_respond()
      {
        this->respond_asked = false;
        this->respond_cached = false;
        return this->respond_cache;
      }

      void state_loop();
      void content_loop();

      text_sensor::TextSensor *current_content;
      text_sensor::TextSensor *current_c_state;
      text_sensor::TextSensor *current_m_state;
      
      bool rolling_ = false;
      binary_sensor::BinarySensor *rolling;
      void set_rolling(bool rolling)
      {
        this->rolling_ = rolling;
        if (this->rolling != nullptr)
          this->rolling->publish_state(rolling);
      }

      bool set_string(std::string string, uint8_t position);
      bool set_character(char character, uint8_t position);

      void request_state(uint8_t position);
      void request_content(uint8_t position);

      void collect_respond();

      // helper

      // _erase_all
      //  find character in string and erase them all.
      static void _erase_all(std::string &string, char character)
      {
        size_t pos = 0;
        while ((pos = string.find('\n')) != std::string::npos)
        {
          string.erase(pos, 1);
        }
      }

    public:
      void setup_row_length(uint8_t length) { this->row_length = length; }
      void setup_last_module(uint8_t length) { this->last_module = length; }
      void setup_current_content(text_sensor::TextSensor *sensor) { this->current_content = sensor; }
      void setup_current_c_state(text_sensor::TextSensor *sensor) { this->current_c_state = sensor; }
      void setup_current_m_state(text_sensor::TextSensor *sensor) { this->current_m_state = sensor; }
      void setup_rolling(binary_sensor::BinarySensor *sensor) { this->rolling = sensor; }

      void setup() override;
      void dump_config() override;
      void loop() override;

      void roll();
      void adapt();
      void clear(bool adapt = true);

      void set_content(std::string str, uint8_t mode = 0x00, uint8_t row = 1);

      void set_row(std::string str, uint8_t mode = 0x00);
  };


  template<typename... Ts> class RollAction : public Action<Ts...>  {
    public:
      explicit RollAction(sfdVosloh *sfd_vosloh) : sfd_vosloh_(sfd_vosloh) {}

      void play(const Ts &...x) override { this->sfd_vosloh_->roll(); }
    
    protected:
      sfdVosloh *sfd_vosloh_;
  };

  template<typename... Ts> class ClearAction : public Action<Ts...>  {
    public:
      explicit ClearAction(sfdVosloh *sfd_vosloh) : sfd_vosloh_(sfd_vosloh) {}

      void play(const Ts &...x) override { this->sfd_vosloh_->clear(); }
    
    protected:
      sfdVosloh *sfd_vosloh_;
  };

  template<typename... Ts> class ContentAction : public Action<Ts...>  {
    public:
      explicit ContentAction(sfdVosloh *sfd_vosloh) : sfd_vosloh_(sfd_vosloh) {}
      TEMPLATABLE_VALUE(std::string, text)
      TEMPLATABLE_VALUE(bool, word_wrap)
      TEMPLATABLE_VALUE(bool, overwrite)
      TEMPLATABLE_VALUE(uint8_t, row)
      TEMPLATABLE_VALUE(std::string, align)

      void play(const Ts &...x) override {
        bool word_wrap = this->word_wrap_.value(x...);
        bool overwrite = this->overwrite_.value(x...);

        uint8_t mode = sfd_vosloh::RAW;
        if(word_wrap) mode |= sfd_vosloh::WORD_WRAP;
        if(overwrite) mode |= sfd_vosloh::OVERWRITE;

        if (this->align_.has_value()) {
          if(this->align_.value(x...) == "left")
            mode |= sfd_vosloh::ALIGN_LEFT;
          if(this->align_.value(x...) == "center")
            mode |= sfd_vosloh::ALIGN_CENTER;
          if(this->align_.value(x...) == "right")
            mode |= sfd_vosloh::ALIGN_RIGHT;
        }

        this->sfd_vosloh_->set_content(this->text_.value(x...), mode, this->row_.value(x...));
      }
    
    protected:
      sfdVosloh *sfd_vosloh_;
  };
}