#include "cci.h"
#include "log.h"
#include <unistd.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

static int cci_last_read_count = 0;

/**
 * Initialise the CCI.
 */
int cci_init(int fd)
{
  if (ioctl(fd, I2C_SLAVE, CCI_ADDRESS) < 0) {
      log_error("CCI: failed to initialise the CCI (I2C setup failed)");
      return -1;
  }

  return 1;
}

/**
 * Write a CCI register.
 */
int cci_write_register(int fd, uint16_t reg, uint16_t value)
{
  // Write the register address and value
  uint8_t write_buf[4] = {
    reg >> 8 & 0xff,
    reg & 0xff,
    value >> 8 & 0xff,
    value & 0xff
  };

  if (write(fd, write_buf, sizeof(write_buf)) != sizeof(write_buf)) {
    log_error("CCI: failed to write CCI register %02x with value %02x", reg, value);
    return -1;
  };

  return 1;
}

/**
 * Read a CCI register.
 * Updates cci_last_read_count to indicate how many bytes were read from the CCI.
 * This should be checked by calling code after calling cci_read_register().
 */
uint16_t cci_read_register(int fd, uint16_t reg)
{
  uint8_t buf[2] = {0, 0};

  // Write the register address
  buf[0] = reg >> 8;
  buf[1] = reg & 0xff;
  if (write(fd, buf, sizeof(buf)) != sizeof(buf)) {
    log_error("CCI: failed to write CCI register %02x", reg);
    return -1;
  }

  // Read
  if ((cci_last_read_count = read(fd, buf, sizeof(buf))) != sizeof(buf)) {
    log_error("CCI: failed to read from CCI register %02x (read %d)", reg, cci_last_read_count);
    cci_last_read_count = 0;
  }

  return buf[0] << 8 | buf[1];
}

/**
 * Wait for busy to be clear in the status register
 */
void cci_wait_busy_clear(int fd)
{
  uint8_t buf[2] = {0x00, 0x07};

  // Wait for booted, not busy
  while ((buf[1] & 0x07) != 0x06) {
    // Write STATUS register address
    buf[0] = 0x00;
    buf[1] = 0x02;
    if (write(fd, buf, sizeof(buf)) != sizeof(buf)) {
      log_error("CCI: failed to set STATUS register");
    };

    // Read register - low bits in buf[1]
    if ((cci_last_read_count = read(fd, buf, sizeof(buf))) != sizeof(buf)) {
      log_error("CCI: failed to read STATUS register");
    }
  }
}


/**
 * Request that a flat field correction occur immediately.
 */
void cci_run_ffc(int fd)
{
  WAIT_FOR_BUSY_DEASSERT()
  cci_write_register(fd, CCI_REG_COMMAND, CCI_CMD_SYS_RUN_FFC);
  WAIT_FOR_BUSY_DEASSERT()
}

/**
 * Get the system uptime.
 */
uint32_t cci_get_uptime(int fd)
{
  WAIT_FOR_BUSY_DEASSERT()
  cci_write_register(fd, CCI_REG_DATA_LENGTH, 2);
  cci_write_register(fd, CCI_REG_COMMAND, CCI_CMD_SYS_GET_UPTIME);
  WAIT_FOR_BUSY_DEASSERT()
  uint16_t ls_word = cci_read_register(fd, CCI_REG_DATA_0);
  uint16_t ms_word = cci_read_register(fd, CCI_REG_DATA_0 + CCI_WORD_LENGTH);
  return ms_word << 16 | ls_word;
}


/**
 * Change the telemetry enable state.
 */
void cci_set_telemetry_enable_state(int fd, cci_telemetry_enable_state_t state)
{
  uint32_t value = state;
  WAIT_FOR_BUSY_DEASSERT()
  cci_write_register(fd, CCI_REG_DATA_0, value & 0xffff);
  cci_write_register(fd, CCI_REG_DATA_0 + CCI_WORD_LENGTH, value >> 16 & 0xffff);
  cci_write_register(fd, CCI_REG_COMMAND, CCI_CMD_SYS_SET_TELEMETRY_ENABLE_STATE);
  cci_write_register(fd, CCI_REG_DATA_LENGTH, 2);
  WAIT_FOR_BUSY_DEASSERT()
}

/**
 * Get the telemetry enable state.
 */
uint32_t cci_get_telemetry_enable_state(int fd)
{
  WAIT_FOR_BUSY_DEASSERT()
  cci_write_register(fd, CCI_REG_DATA_LENGTH, 2);
  cci_write_register(fd, CCI_REG_COMMAND, CCI_CMD_SYS_GET_TELEMETRY_ENABLE_STATE);
  WAIT_FOR_BUSY_DEASSERT()
  uint16_t ls_word = cci_read_register(fd, CCI_REG_DATA_0);
  uint16_t ms_word = cci_read_register(fd, CCI_REG_DATA_0 + CCI_WORD_LENGTH);
  return ms_word << 16 | ls_word;
}

/**
 * Change the telemetry location.
 */
void cci_set_telemetry_location(int fd, cci_telemetry_location_t location)
{
  uint32_t value = location;
  WAIT_FOR_BUSY_DEASSERT()
  cci_write_register(fd, CCI_REG_DATA_0, value & 0xffff);
  cci_write_register(fd, CCI_REG_DATA_0 + CCI_WORD_LENGTH, value >> 16 & 0xffff);
  cci_write_register(fd, CCI_REG_COMMAND, CCI_CMD_SYS_SET_TELEMETRY_LOCATION);
  cci_write_register(fd, CCI_REG_DATA_LENGTH, 2);
  WAIT_FOR_BUSY_DEASSERT()
}

/**
 * Get the telemetry location.
 */
uint32_t cci_get_telemetry_location(int fd)
{
  WAIT_FOR_BUSY_DEASSERT()
  cci_write_register(fd, CCI_REG_DATA_LENGTH, 2);
  cci_write_register(fd, CCI_REG_COMMAND, CCI_CMD_SYS_GET_TELEMETRY_LOCATION);
  WAIT_FOR_BUSY_DEASSERT()
  uint16_t ls_word = cci_read_register(fd, CCI_REG_DATA_0);
  uint16_t ms_word = cci_read_register(fd, CCI_REG_DATA_0 + CCI_WORD_LENGTH);
  return ms_word << 16 | ls_word;
}

/**
 * Change the radiometry enable state.
 */
void cci_set_radiometry_enable_state(int fd, cci_radiometry_enable_state_t state)
{
  uint32_t value = state;
  WAIT_FOR_BUSY_DEASSERT()
  cci_write_register(fd, CCI_REG_DATA_0, value & 0xffff);
  cci_write_register(fd, CCI_REG_DATA_0 + CCI_WORD_LENGTH, value >> 16 & 0xffff);
  cci_write_register(fd, CCI_REG_COMMAND, CCI_CMD_RAD_SET_RADIOMETRY_ENABLE_STATE);
  cci_write_register(fd, CCI_REG_DATA_LENGTH, 2);
  WAIT_FOR_BUSY_DEASSERT()
}

/**
 * Get the radiometry enable state.
 */
uint32_t cci_get_radiometry_enable_state(int fd)
{
  WAIT_FOR_BUSY_DEASSERT()
  cci_write_register(fd, CCI_REG_DATA_LENGTH, 2);
  cci_write_register(fd, CCI_REG_COMMAND, CCI_CMD_RAD_GET_RADIOMETRY_ENABLE_STATE);
  WAIT_FOR_BUSY_DEASSERT()
  uint16_t ls_word = cci_read_register(fd, CCI_REG_DATA_0);
  uint16_t ms_word = cci_read_register(fd, CCI_REG_DATA_0 + CCI_WORD_LENGTH);
  return ms_word << 16 | ls_word;
}

/**
 * Change the radiometry TLinear enable state.
 */
void cci_set_radiometry_tlinear_enable_state(int fd, cci_radiometry_tlinear_enable_state_t state)
{
  uint32_t value = state;
  WAIT_FOR_BUSY_DEASSERT()
  cci_write_register(fd, CCI_REG_DATA_0, value & 0xffff);
  cci_write_register(fd, CCI_REG_DATA_0 + CCI_WORD_LENGTH, value >> 16 & 0xffff);
  cci_write_register(fd, CCI_REG_COMMAND, CCI_CMD_RAD_SET_RADIOMETRY_TLINEAR_ENABLE_STATE);
  cci_write_register(fd, CCI_REG_DATA_LENGTH, 2);
  WAIT_FOR_BUSY_DEASSERT()
}

/**
 * Get the radiometry TLinear enable state.
 */
uint32_t cci_get_radiometry_tlinear_enable_state(int fd)
{
  WAIT_FOR_BUSY_DEASSERT()
  cci_write_register(fd, CCI_REG_DATA_LENGTH, 2);
  cci_write_register(fd, CCI_REG_COMMAND, CCI_CMD_RAD_GET_RADIOMETRY_TLINEAR_ENABLE_STATE);
  WAIT_FOR_BUSY_DEASSERT()
  uint16_t ls_word = cci_read_register(fd, CCI_REG_DATA_0);
  uint16_t ms_word = cci_read_register(fd, CCI_REG_DATA_0 + CCI_WORD_LENGTH);
  return ms_word << 16 | ls_word;
}

/**
 * Get the AGC enable state.
 */
uint32_t cci_get_agc_enable_state(int fd)
{
  WAIT_FOR_BUSY_DEASSERT()
  cci_write_register(fd, CCI_REG_DATA_LENGTH, 2);
  cci_write_register(fd, CCI_REG_COMMAND, CCI_CMD_AGC_GET_AGC_ENABLE_STATE);
  WAIT_FOR_BUSY_DEASSERT()
  uint16_t ls_word = cci_read_register(fd, CCI_REG_DATA_0);
  uint16_t ms_word = cci_read_register(fd, CCI_REG_DATA_0 + CCI_WORD_LENGTH);
  return ms_word << 16 | ls_word;
}

/**
 * Set the AGC enable state.
 */
void cci_set_agc_enable_state(int fd, cci_agc_enable_state_t state)
{
  uint32_t value = state;
  WAIT_FOR_BUSY_DEASSERT()
  cci_write_register(fd, CCI_REG_DATA_0, value & 0xffff);
  cci_write_register(fd, CCI_REG_DATA_0 + CCI_WORD_LENGTH, value >> 16 & 0xffff);
  cci_write_register(fd, CCI_REG_COMMAND, CCI_CMD_AGC_SET_AGC_ENABLE_STATE);
  cci_write_register(fd, CCI_REG_DATA_LENGTH, 2);
  WAIT_FOR_BUSY_DEASSERT()
}
/**
 * Get the AGC calc enable state.
 */
uint32_t cci_get_agc_calc_enable_state(int fd)
{
  WAIT_FOR_BUSY_DEASSERT()
  cci_write_register(fd, CCI_REG_DATA_LENGTH, 2);
  cci_write_register(fd, CCI_REG_COMMAND, CCI_CMD_AGC_GET_CALC_ENABLE_STATE);
  WAIT_FOR_BUSY_DEASSERT()
  uint16_t ls_word = cci_read_register(fd, CCI_REG_DATA_0);
  uint16_t ms_word = cci_read_register(fd, CCI_REG_DATA_0 + CCI_WORD_LENGTH);
  return ms_word << 16 | ls_word;
}

/**
 * Set the AGC calc enable state.
 */
void cci_set_agc_calc_enable_state(int fd, cci_agc_enable_state_t state)
{
  uint32_t value = state;
  WAIT_FOR_BUSY_DEASSERT()
  cci_write_register(fd, CCI_REG_DATA_0, value & 0xffff);
  cci_write_register(fd, CCI_REG_DATA_0 + CCI_WORD_LENGTH, value >> 16 & 0xffff);
  cci_write_register(fd, CCI_REG_COMMAND, CCI_CMD_AGC_SET_CALC_ENABLE_STATE);
  cci_write_register(fd, CCI_REG_DATA_LENGTH, 2);
  WAIT_FOR_BUSY_DEASSERT()
}

/*
 * Get/Set GPIO3 mode to VSYNC.
 */
uint32_t cci_get_gpio_mode(int fd)
{
  WAIT_FOR_BUSY_DEASSERT()
  cci_write_register(fd, CCI_REG_DATA_LENGTH, 2);
  cci_write_register(fd, CCI_REG_COMMAND, CCI_CMD_OEM_GET_GPIO_MODE);
  WAIT_FOR_BUSY_DEASSERT()
  uint16_t ls_word = cci_read_register(fd, CCI_REG_DATA_0);
  uint16_t ms_word = cci_read_register(fd, CCI_REG_DATA_0 + CCI_WORD_LENGTH);
  return ms_word << 16 | ls_word;
}

void cci_set_gpio_mode(int fd, cci_gpio_mode_e_t state)
{
  uint32_t value = state;
  WAIT_FOR_BUSY_DEASSERT()
  cci_write_register(fd, CCI_REG_DATA_0, value & 0xffff);
  cci_write_register(fd, CCI_REG_DATA_0 + CCI_WORD_LENGTH, value >> 16 & 0xffff);
  cci_write_register(fd, CCI_REG_COMMAND, CCI_CMD_OEM_SET_GPIO_MODE);
  cci_write_register(fd, CCI_REG_DATA_LENGTH, 2);
  WAIT_FOR_BUSY_DEASSERT()
}

/*
 * Run the Reboot command
 */
void cc_run_oem_reboot(int fd)
{
  WAIT_FOR_BUSY_DEASSERT()
  cci_write_register(fd, CCI_REG_COMMAND, CCI_CMD_OEM_RUN_REBOOT);
  // Sleep to allow camera to reboot and run FFC
  sleep(6);
  WAIT_FOR_BUSY_DEASSERT()
}
