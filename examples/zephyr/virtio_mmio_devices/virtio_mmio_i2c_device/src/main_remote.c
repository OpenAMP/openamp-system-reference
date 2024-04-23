/*
 * Copyright (c) 2023, STMICROLECTRONICS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <drivers/virtio/virtio_mmio_dev.h>

#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/ipm.h>
#include <zephyr/kernel.h>

/* Display / Framebuffer */
#include <zephyr/display/cfb.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(openamp_virtio_i2c, LOG_LEVEL_DBG);

#define I2C_REMOTE_DISP_ADDR 0x3D

/* constant derivated from linker symbols */

#define APP_TASK_STACK_SIZE (4096)
K_THREAD_STACK_DEFINE(thread_stack, APP_TASK_STACK_SIZE);
static struct k_thread thread_data;

static const struct device *const ipm_handle =
	DEVICE_DT_GET(DT_CHOSEN(zephyr_ipc));

static const struct device *const virtio_mmio_dev =
	DEVICE_DT_GET(DT_NODELABEL(virtio_mmio_dev));

static const struct device *const virtio_i2c_dev =
	DEVICE_DT_GET(DT_NODELABEL(i2c_virtio_dev));

static const struct device *const i2c_dev = DEVICE_DT_GET(DT_NODELABEL(arduino_i2c));

#ifdef CONFIG_SSD1306
static const struct device *const dev_display = DEVICE_DT_GET(DT_NODELABEL(ssd1306_zephyr));
#endif

#ifdef CONFIG_HTS221
static const struct device *const hts221 = DEVICE_DT_GET(DT_NODELABEL(hts221_x_nucleo_iks01a2));
#endif

#ifdef CONFIG_LPS22HB
static const struct device *const lps22hb =
			DEVICE_DT_GET(DT_NODELABEL(lps22hb_press_x_nucleo_iks01a2));
#endif

static uint16_t display_rows;
static uint8_t display_ppt;
static uint8_t display_font_width;
static uint8_t display_font_height;
static bool display_available;

static struct sensor_value temp1, temp2, hum, press;

static bool hts221_available = true;
static bool lps22hb_available = true;

void stop_display(void)
{
	display_blanking_on(dev_display);
}

static void platform_ipm_callback(const struct device *dev,
				  void *context, uint32_t id, volatile void *data)
{
	if (id == 2) {
		/* receive shutdown */
		stop_display();
		return;
	}

	if (id == 5) {
		virtio_mmio_interrupt(virtio_mmio_dev);
		return;
	}
}

static void send_virtio_it(void)
{
	ipm_send(ipm_handle, 0, 4, NULL, 0);
}

void init_sensors(void)
{
	/* search in devicetree if sensors are referenced */

#if CONFIG_HTS221
	if (!device_is_ready(hts221)) {
		LOG_ERR("Could not get HTS221 device\n");
		hts221_available = false;
	}
#endif
#if CONFIG_LPS22HB
	if (!device_is_ready(lps22hb)) {
		LOG_ERR("Could not get LPS22HB device\n");
		lps22hb_available = false;
	}
#endif

	/* set LSM6DSL accel/gyro sampling frequency to 104 Hz */
	struct sensor_value odr_attr;

	odr_attr.val1 = 104;
	odr_attr.val2 = 0;

}

int init_display(void)
{
	int ret;

	/* init display */
	if (!device_is_ready(dev_display)) {
		LOG_ERR("Display Device not found\n");
		return -ENODEV;
	}
	display_available = true;

	ret = display_set_pixel_format(dev_display, PIXEL_FORMAT_MONO10);
	if (ret) {
		LOG_ERR("Failed to set format PIXEL_FORMAT_MONO10\n");
		return ret;
	}

	ret = cfb_framebuffer_init(dev_display);
	if (ret) {
		LOG_ERR("Framebuffer initialization failed!\n");
		return ret;
	}
	cfb_framebuffer_clear(dev_display, true);

	display_blanking_off(dev_display);

	display_rows = cfb_get_display_parameter(dev_display,
						 CFB_DISPLAY_ROWS);
	display_ppt = cfb_get_display_parameter(dev_display,
						CFB_DISPLAY_PPT);
	for (int idx = 0; idx < 42; idx++) {
		if (cfb_get_font_size(dev_display, idx,
				      &display_font_width,
				      &display_font_height)) {
			break;
		}
		cfb_framebuffer_set_font(dev_display, idx);
		LOG_DBG("idx: %d font width %d, font height %d\n",
			idx, display_font_width, display_font_height);
	}
	/* for idx: 0 font width 10, font height 16*/
	cfb_framebuffer_set_font(dev_display, 0);
	cfb_get_font_size(dev_display, 0, &display_font_width,
			  &display_font_height);

	LOG_DBG("x_res %d, y_res %d, ppt %d, rows %d, cols %d\n",
		cfb_get_display_parameter(dev_display, CFB_DISPLAY_WIDTH),
		cfb_get_display_parameter(dev_display, CFB_DISPLAY_HEIGH),
		display_ppt,
		display_rows,
		cfb_get_display_parameter(dev_display, CFB_DISPLAY_COLS));

	return 0;
}

int init_remote_display(void)
{
	if (!device_is_ready(i2c_dev)) {
		LOG_ERR("failed to bind remote display\n");
		return -ENODEV;
	}

	return 0;
}

int platform_init(void)
{
	int status;

	virtio_mmio_configure_interrupt(virtio_mmio_dev, send_virtio_it);

	/* setup IPM */
	if (!device_is_ready(ipm_handle)) {
		LOG_ERR("IPM device is not ready\n");
		return -1;
	}

	ipm_register_callback(ipm_handle, platform_ipm_callback, NULL);

	status = ipm_set_enabled(ipm_handle, 1);
	if (status) {
		LOG_ERR("ipm_set_enabled failed\n");
		return -1;
	}

	return 0;
}

static void cleanup_system(void)
{
	ipm_set_enabled(ipm_handle, 0);
}

static int i2c_target_transfer(struct i2c_target_config *config, struct i2c_msg msg)
{
	return i2c_transfer(i2c_dev, &msg, 1, config->address);
}

static const struct i2c_target_callbacks i2c_virtio_callbacks = {
	.transfer = i2c_target_transfer,
};

void app_task(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	int len;
	int ret = 0;
	char message[128];
	char display_buf[16];
	uint8_t line = 0;

	struct i2c_target_config config;

	/* Allow full access to the bus, specify address to allow only one address */
	config.address = 0xFF;
	config.callbacks = &i2c_virtio_callbacks;

	ret = i2c_target_register(virtio_i2c_dev, &config);
	if (ret) {
		LOG_ERR("Failed to register I2C SLAVE %#x\n", config.address);
		goto task_end;
	}

	LOG_INF("\r\nOpenAMP[remote] I2C demo started\n");

	init_remote_display();
	init_sensors();
	init_display();

	if (display_available) {
		line = 0;
		LOG_ERR("--init display screen --\n");

		cfb_framebuffer_clear(dev_display, false);
		cfb_print(dev_display, "Zephyr", 0,
			  line++ * display_font_height);
		cfb_print(dev_display, "   Waiting ", 0,
			  line++ * display_font_height);
		cfb_print(dev_display, "     TTY   ", 0,
			  line++ * display_font_height);
		cfb_print(dev_display, "    data   ", 0,
			  line++ * display_font_height);
		cfb_framebuffer_finalize(dev_display);
	}

	while (1) {
		unsigned int line = 0;

		if (display_available)
			cfb_framebuffer_clear(dev_display, false);

		/* Get sensor samples */
		if (hts221_available && sensor_sample_fetch(hts221) < 0)
			printf("HTS221 Sensor sample update error\n");
		if (lps22hb_available && sensor_sample_fetch(lps22hb) < 0)
			printf("LPS22HB Sensor sample update error\n");

		/* Get sensor data */
		if (hts221_available) {
			sensor_channel_get(hts221, SENSOR_CHAN_AMBIENT_TEMP,
					   &temp1);
			sensor_channel_get(hts221, SENSOR_CHAN_HUMIDITY, &hum);
		}
		if (lps22hb_available) {
			sensor_channel_get(lps22hb, SENSOR_CHAN_PRESS, &press);
			sensor_channel_get(lps22hb, SENSOR_CHAN_AMBIENT_TEMP,
					   &temp2);
		}

		/* Erase previous */
		if (display_available) {
			cfb_print(dev_display, "Zephyr", 0,
				  line++ * display_font_height);

			if (hts221_available) {
				/* temperature */
				snprintf(display_buf, sizeof(display_buf), "T: %.2f*C",
					 sensor_value_to_float(&temp1));

				cfb_print(dev_display, display_buf, 0,
					  line++ * display_font_height);

				/* humidity */
				snprintf(display_buf, sizeof(display_buf), "H: %.1f%%",
					 sensor_value_to_float(&hum));

				cfb_print(dev_display, display_buf, 0,
					  line++ * display_font_height);
			} else {
				cfb_print(dev_display, "T: N/A", 0, line++ * display_font_height);
				cfb_print(dev_display, "H: N/A", 0, line++ * display_font_height);
			}

			if (lps22hb_available) {
				/* pressure */
				/* lps22hb temperature */
				len = snprintf(message, sizeof(message),
					       "LPS22HB: Temperature: %.1f C\n",
					       sensor_value_to_double(&temp2));

				snprintf(display_buf, sizeof(display_buf), "P: %.2fkpa",
					 (sensor_value_to_double(&press) * 10.0));

				cfb_print(dev_display, display_buf, 0,
					  line++ * display_font_height);
			} else {
				cfb_print(dev_display, "P: N/A", 0, line++ * display_font_height);
			}

			cfb_framebuffer_finalize(dev_display);
		}

		k_msleep(500);
	}

task_end:
	cleanup_system();

	LOG_DBG("sensor task ended\n");
}

static int openamp_init(const struct device *dev)
{
	int ret;

	/* Initialize platform */
	ret = platform_init();
	if (ret) {
		LOG_ERR("Failed to initialize platform\n");
		goto error_case;
	}

	return 0;

error_case:
	cleanup_system();

	return 1;
}

void main(void)
{
	k_thread_create(&thread_data, thread_stack, APP_TASK_STACK_SIZE,
			(k_thread_entry_t)app_task,
			NULL, NULL, NULL, K_PRIO_COOP(7), 0, K_NO_WAIT);
}

SYS_INIT(openamp_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
