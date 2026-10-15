#include <Arduino.h>
#include <Wire.h>
#include "bme280.h"

#define I2C_SDA 8
#define I2C_SCL 9

static struct bme280_dev dev;
static uint8_t dev_addr = BME280_I2C_ADDR_PRIM;

/* Bosch API I2C read: register address, repeated start, then burst read. */
static int8_t bme280_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr) {
  uint8_t addr = *(uint8_t *)intf_ptr;

  Wire.beginTransmission(addr);
  Wire.write(reg_addr);
  if (Wire.endTransmission(false) != 0) {
    return -1;
  }

  if (Wire.requestFrom(addr, (uint8_t)len) != len) {
    return -1;
  }

  for (uint32_t i = 0; i < len; i++) {
    reg_data[i] = Wire.read();
  }

  return 0;
}

/* Bosch API I2C write: register address followed by the data bytes. */
static int8_t bme280_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr) {
  uint8_t addr = *(uint8_t *)intf_ptr;

  Wire.beginTransmission(addr);
  Wire.write(reg_addr);
  for (uint32_t i = 0; i < len; i++) {
    Wire.write(reg_data[i]);
  }

  return Wire.endTransmission() == 0 ? 0 : -1;
}

static void bme280_delay_us(uint32_t period, void *intf_ptr) {
  (void)intf_ptr;
  delayMicroseconds(period);
}

static void i2c_scan() {
  for (uint8_t i = 0; i < 128; i++) {
    Wire.beginTransmission(i);
    if (Wire.endTransmission() == 0) {
      Serial.print("Found I2C device at address 0x");
      if (i < 16) {
        Serial.print("0");
      }
      Serial.println(i, HEX);
    }
  }
}

static void bme280_setup() {
  dev.intf = BME280_I2C_INTF;
  dev.read = bme280_i2c_read;
  dev.write = bme280_i2c_write;
  dev.delay_us = bme280_delay_us;
  dev.intf_ptr = &dev_addr;

  int8_t rslt = bme280_init(&dev);
  if (rslt != BME280_OK) {
    Serial.printf("bme280_init failed (%d)\n", rslt);
    return;
  }

  struct bme280_settings settings;
  rslt = bme280_get_sensor_settings(&settings, &dev);
  if (rslt != BME280_OK) {
    Serial.printf("bme280_get_sensor_settings failed (%d)\n", rslt);
    return;
  }

  settings.osr_h = BME280_OVERSAMPLING_1X;
  settings.osr_p = BME280_OVERSAMPLING_1X;
  settings.osr_t = BME280_OVERSAMPLING_1X;
  settings.filter = BME280_FILTER_COEFF_16;
  settings.standby_time = BME280_STANDBY_TIME_1000_MS;
  bme280_set_sensor_settings(BME280_SEL_ALL_SETTINGS, &settings, &dev);

  bme280_set_sensor_mode(BME280_POWERMODE_NORMAL, &dev);

  Serial.println("BME280 ready");
}

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL, 100000);

  i2c_scan();
  bme280_setup();
}

void loop() {
  struct bme280_data data;

  int8_t rslt = bme280_get_sensor_data(BME280_ALL, &data, &dev);
  if (rslt == BME280_OK) {
    Serial.printf("T: %.2f C  P: %.2f hPa  H: %.2f %%\n",
                  data.temperature, data.pressure / 100.0, data.humidity);
  } else {
    Serial.printf("bme280_get_sensor_data failed (%d)\n", rslt);
  }

  delay(1000);
}
