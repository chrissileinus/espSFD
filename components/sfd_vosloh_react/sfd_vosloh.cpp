#include "sfd_vosloh.h"

#include "esphome/core/log.h"

namespace esphome
{
  namespace sfd_vosloh_react
  {
    // protected
    // set functions
    bool sfdVosloh::set_string(std::string string, uint8_t position = 1)
    {
      ESP_LOGD(TAG, "[set_string] pos: %3d; string: \"%s\"", position, string.c_str());
      for (int i = 0; i < string.length(); i++)
      {
        if (!this->set_character(string[i], position + i))
          return false;
      }
      return true;
    }

    bool sfdVosloh::set_character(char character, uint8_t position = 1)
    {
      if (position <= this->last_module)
      {
        this->parent_->write_byte(_WRITE);
        this->parent_->write_byte(position);
        this->parent_->write_byte(character);
        // this->parent_->flush();

        // ESP_LOGD(TAG, "[set_character]: pos: %3d; char: %#x '%c'", position, character, character);
        this->current_position = position + 1;
        return true;
      }
      ESP_LOGD(TAG, "[set_character]: reached last_module");
      return false;
    }

    // request functions
    void sfdVosloh::request_content(uint8_t position)
    {
      this->parent_->write_byte(_READ);
      this->parent_->write_byte(position);
      this->parent_->flush();

      this->current_job = _JOB_CONTENT;
      this->respond_timeout = millis() + this->UART_TIMEOUT;
      this->respond_cached = false;
      this->respond_asked = true;  
      this->respond_position = position;
    }

    void sfdVosloh::request_state(uint8_t position)
    {
      this->parent_->write_byte(_STATE);
      this->parent_->write_byte(position);
      this->parent_->flush();

      this->current_job = _JOB_STATE;
      this->respond_timeout = millis() + this->UART_TIMEOUT;
      this->respond_cached = false;
      this->respond_asked = true;   
      this->respond_position = position;
    }

    void sfdVosloh::collect_respond()
    {
      if (!this->respond_asked) return;

      if (millis() > this->respond_timeout)
      {
        this->respond_cache = 0x00;
        this->respond_cached = true;
      }

      if (available())
      {
        this->respond_cache = read();
        this->respond_cached = true;
      }
    }

    // update
    void sfdVosloh::content_loop()
    {
      static std::string str = "";

      if (this->current_job != _JOB_CONTENT) return;
      if (this->respond_cached)
      {
        char character = this->get_respond();

        switch (character)
        {
        case 0x00: // no respond / timeout
          // ESP_LOGD(TAG, "[character] pos: %3d; timeout", pos);
          str.push_back('|');
          break;
        case 0x10: // char not valid
          // ESP_LOGD(TAG, "[character] pos: %3d; char not valid", pos);
          str.push_back('_');
          break;
        case 0x20 ... 0xFF: // char is valid
          str.push_back(character);
          break;

        default: // undefined respond
          // ESP_LOGD(TAG, "[character] pos: %3d; undefined respond: %#x", pos, character);
          str.push_back('-');
          break;
        }

        if (this->respond_position < this->last_module)
        {
          this->request_content(this->respond_position + 1);
          return;
        }

        if (this->current_content->get_state() != str)
        this->current_content->publish_state(str);
  
        str = "";
  
        this->request_state(1);
      }
    }

    void sfdVosloh::state_loop()
    {
      static bool rolling = false;
      static std::string c_str = "";
      static std::string m_str = "";

      if (this->current_job != _JOB_STATE) return;
      if (this->respond_cached)
      {
        uint8_t state = this->get_respond();
        uint8_t c_state = state | 0x0F;
        uint8_t m_state = state | 0xF0;
  
        switch (c_state)
        {
        case 0xAF: // First run
          // ESP_LOGD(TAG, "[c_state] pos: %3d; first run", pos);
          c_str.push_back('f');
          break;
        case 0xCF: // Char unknown
          // ESP_LOGD(TAG, "[c_state] pos: %3d; char unknown", pos);
          c_str.push_back('u');
          break;
        case 0x8F: // Char known
          // ESP_LOGD(TAG, "[c_state] pos: %3d; char known", pos);
          c_str.push_back('k');
          break;
  
        default:
          c_str.push_back('_');
          break;
        }
  
        switch (m_state)
        {
        case 0xF0: // at position
          // ESP_LOGD(TAG, "[m_state] pos: %3d; at position", pos);
          m_str.push_back('p');
          break;
        case 0xFF: // moving
          // ESP_LOGD(TAG, "[m_state] pos: %3d; moving", pos);
          m_str.push_back('m');
          rolling = true;
          break;
        case 0xF4: // motor power supply failure
          // ESP_LOGD(TAG, "[m_state] pos: %3d; no supply", pos);
          m_str.push_back('s');
          break;
        case 0xF2: // motor failure, not moving
          // ESP_LOGD(TAG, "[m_state] pos: %3d; no respond", pos);
          m_str.push_back('r');
          break;
  
        default:
          m_str.push_back('_');
          break;
        }

        if (this->respond_position < this->last_module)
        {
          this->request_state(this->respond_position + 1);
          return;
        }

        if (this->current_c_state != nullptr && this->current_c_state->get_state() != c_str)
          this->current_c_state->publish_state(c_str);
        if (this->current_m_state != nullptr && this->current_m_state->get_state() != m_str)
          this->current_m_state->publish_state(m_str);

        c_str = "";
        m_str = "";
  
        this->set_rolling(rolling);
        rolling = false;

        if (this->current_content != nullptr)
          this->request_content(1);
        else
          this->request_state(1);
      }
    }

    // public
    // setup
    void sfdVosloh::setup()
    {
      // this->roll();
      // this->clear();
      // delay_microseconds_safe(10000);

      this->request_state(1);
    }
    void sfdVosloh::dump_config()
    {
      ESP_LOGCONFIG(TAG, "row length: %3d", this->row_length);
      ESP_LOGCONFIG(TAG, "last module: %3d", this->last_module);
    }

    void sfdVosloh::loop()
    {
      if (this->blocked > 0) return;
      this->collect_respond();

      this->state_loop();
      this->content_loop();
    }

    // methods
    void sfdVosloh::roll()
    {
      this->blocked++;

      ESP_LOGD(TAG, "[roll]:");
      this->set_rolling(true);
      delay_microseconds_safe(1000);
      this->parent_->write_byte(_ROLL);
      this->current_position = 1;

      this->blocked--;
    }
    void sfdVosloh::adapt()
    {
      this->set_rolling(true);
      delay_microseconds_safe(1000);
      this->parent_->write_byte(_ADAPT);
    }
    void sfdVosloh::clear(bool adapt)
    {
      this->blocked++;

      for (int pos = 1; pos <= this->last_module; pos++)
      {
        this->set_character(' ', pos);
      }
      this->current_position = 1;
      if (adapt)
        this->adapt();

      this->blocked--;
    }

    void sfdVosloh::set_content(std::string input, uint8_t mode, uint8_t row)
    {
      this->blocked++;

      if (row < 1)
        row = 1;

      ESP_LOGD(TA<G, "[set_content] row: %2d input: %s", row, input.c_str());

      if (!(mode & OVERWRITE))
      {
        this->clear(false);
      }

      this->current_position = (this->row_length * (row - 1)) + 1;

      if (mode == RAW)
      {
        _erase_all(input, '\n');
        _erase_all(input, '\r');

        this->set_string(input);
        this->adapt();
        return;
      }

      input += " ";
      std::string output = "";

      size_t pos = 0;
      std::string word;
      std::string delimiter = " ";
      while ((pos = input.find(delimiter)) != std::string::npos)
      {
        word = input.substr(0, pos);
        input.erase(0, pos + delimiter.length());

        if (mode & WORD_WRAP)
        {
          if ((this->row_length - output.length()) <= word.length())
          {
            this->set_row(output, mode);
            output = "";
          }
        }

        bool nl = false;
        if ((pos = word.find('\n')) != std::string::npos)
        {
          if (pos == 0)
          {
            this->set_row(output, mode);
            output = "";
            nl = false;
          }
          else
            nl = true;
          word.erase(pos, 1);
        }

        if (output.length() != 0)
          output += " ";
        output += word;

        if (nl)
        {
          this->set_row(output, mode);
          output = "";
        }
      }
      this->set_row(output, mode);
      this->adapt();

      this->blocked--;
    }

    void sfdVosloh::set_row(std::string input, uint8_t mode)
    {
      this->blocked++;

      ESP_LOGD(TAG, "[set_row] input: %s", input.c_str());

      if (this->row_length > input.length())
      {
        int rest = (this->row_length - input.length());
        if (mode & ALIGN_CENTER)
          input.insert(0, rest / 2, ' ');
        if (mode & ALIGN_RIGHT)
          input.insert(0, rest, ' ');

        input.insert(input.length(), this->row_length - input.length(), ' ');
      }

      this->set_string(input, this->current_position);

      this->blocked--;
    }
  }
}
