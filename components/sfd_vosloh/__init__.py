import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, text_sensor, sensor, binary_sensor
from esphome.const import (
  CONF_ID,
  CONF_VALUE,
)
from esphome import automation
from esphome.automation import maybe_simple_id

AUTO_LOAD = ["text_sensor", "sensor", "binary_sensor"]
DEPENDENCIES = ["uart"]

sfd_vosloh_ns = cg.esphome_ns.namespace("sfd_vosloh")
sfdVosloh = sfd_vosloh_ns.class_(
    "sfdVosloh", cg.Component, uart.UARTDevice
)

# Actions
RollAction = sfd_vosloh_ns.class_("RollAction", automation.Action)
ClearAction = sfd_vosloh_ns.class_("ClearAction", automation.Action)
ContentAction = sfd_vosloh_ns.class_("ContentAction", automation.Action)

# Config
CONF_ROW_LENGTH = "row_length"
CONF_LAST_MODULE = "last_module"
CONF_CURRENT_CONTENT = "current_content"
CONF_CURRENT_C_STATE = "current_c_state"
CONF_CURRENT_M_STATE = "current_m_state"
CONF_ROLLING = "rolling"

CONFIG_SCHEMA = (
    cv.Schema({
        cv.GenerateID(): cv.declare_id(sfdVosloh),
        cv.Required(CONF_ROW_LENGTH): cv.int_range(min=1, max=127),
        cv.Required(CONF_LAST_MODULE): cv.int_range(min=1, max=127),
        cv.Optional(CONF_CURRENT_CONTENT):
            text_sensor._TEXT_SENSOR_SCHEMA,
        cv.Optional(CONF_CURRENT_C_STATE):
            text_sensor._TEXT_SENSOR_SCHEMA,
        cv.Optional(CONF_CURRENT_M_STATE):
            text_sensor._TEXT_SENSOR_SCHEMA,
        cv.Optional(CONF_ROLLING):
            binary_sensor._BINARY_SENSOR_SCHEMA,
    })
    .extend(cv.COMPONENT_SCHEMA)
    .extend(uart.UART_DEVICE_SCHEMA)
)

def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield uart.register_uart_device(var, config)
    cg.add(var.setup_row_length(config[CONF_ROW_LENGTH]))
    cg.add(var.setup_last_module(config[CONF_LAST_MODULE]))
    if CONF_CURRENT_CONTENT in config:
        conf = config[CONF_CURRENT_CONTENT]
        sens = yield text_sensor.new_text_sensor(conf)
        cg.add(var.setup_current_content(sens))
    if CONF_CURRENT_C_STATE in config:
        conf = config[CONF_CURRENT_C_STATE]
        sens = yield text_sensor.new_text_sensor(conf)
        cg.add(var.setup_current_c_state(sens))
    if CONF_CURRENT_M_STATE in config:
        conf = config[CONF_CURRENT_M_STATE]
        sens = yield text_sensor.new_text_sensor(conf)
        cg.add(var.setup_current_m_state(sens))
    if CONF_ROLLING in config:
        conf = config[CONF_ROLLING]
        sens = yield binary_sensor.new_binary_sensor(conf)
        cg.add(var.setup_rolling(sens))


SFD_ACTION_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_ID): cv.use_id(sfdVosloh),
    }
)
@automation.register_action("sfdVosloh.roll", RollAction, SFD_ACTION_SCHEMA)
async def sfd_vosloh_roll_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)

@automation.register_action("sfdVosloh.clear", ClearAction, SFD_ACTION_SCHEMA)
async def sfd_vosloh_clear_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    return cg.new_Pvariable(action_id, template_arg, paren)

CONF_TEXT = "text"
CONF_WORD_WRAP = "word_wrap"
CONF_OVERWRITE = "overwrite"
CONF_ROW = "row"
CONF_ALIGN = "align"

@automation.register_action(
        "sfdVosloh.content", 
        ContentAction, 
        maybe_simple_id(
        {
            cv.Required(CONF_ID): cv.use_id(sfdVosloh),
            cv.Required(CONF_TEXT): cv.templatable(cv.string_strict),
            cv.Optional(CONF_WORD_WRAP): cv.templatable(cv.boolean),
            cv.Optional(CONF_ROW): cv.templatable(
                cv.int_range(min=1, max=255, max_included=True)
            ),
            cv.Optional(CONF_ALIGN): cv.templatable(
                cv.one_of("left", "center", "right", string=True)
            ),
        }
    ),
)
async def sfd_vosloh_content_to_code(config, action_id, template_arg, args):
    paren = await cg.get_variable(config[CONF_ID])
    var = cg.new_Pvariable(action_id, template_arg, paren)
    template_ = await cg.templatable(config[CONF_TEXT], args, cg.std_string)
    cg.add(var.set_text(template_))
    if (word_wrap := config.get(CONF_WORD_WRAP)) is not None:
        template_ = await cg.templatable(word_wrap, args, cg.bool_)
        cg.add(var.set_word_wrap(template_))
    if (overwrite := config.get(CONF_OVERWRITE)) is not None:
        template_ = await cg.templatable(overwrite, args, cg.bool_)
        cg.add(var.set_overwrite(template_))
    if (row := config.get(CONF_ROW)) is not None:
        template_ = await cg.templatable(row, args, cg.uint8)
        cg.add(var.set_row(template_))
    if (align := config.get(CONF_ALIGN)) is not None:
        template_ = await cg.templatable(align, args, cg.std_string)
        cg.add(var.set_align(template_))
    return var