#pragma once

#include <stdint.h>
#include <linked.h>

typedef struct driver_interface driver_interface_t;
typedef struct driver_handle driver_handle_t;

typedef driver_handle_t * (* driver_open_t)(driver_interface_t * driver);
typedef ssize_t (* driver_write_t)(driver_handle_t * handle, void * p, size_t size);
typedef ssize_t (* driver_read_t)(driver_handle_t * handle, void * p, size_t size);
typedef int32_t (* driver_ioctl_t)(driver_handle_t * handle, int cmd, int op);

typedef struct driver_interface {
	uint16_t * name;
	driver_open_t open;
	driver_write_t write;
	driver_read_t read;
	driver_ioctl_t ioctl;
	void * priv;
} driver_interface_t;

typedef struct driver_handle {
	driver_open_t open;
	driver_write_t write;
	driver_read_t read;
	driver_ioctl_t ioctl;
	void * priv;
} driver_handle_t;

void driver_register(driver_interface_t * handle);
driver_handle_t * driver_create_handle(driver_interface_t * driver);
driver_interface_t * driver_create(uint16_t * name, driver_open_t open, driver_write_t write, driver_read_t read, driver_ioctl_t ioctl);
driver_handle_t * driver_open(uint16_t * name);
ssize_t driver_write(driver_handle_t * handle, void * p, size_t size);
ssize_t driver_read(driver_handle_t * handle, void * p, size_t size);
int32_t driver_ioctl(driver_handle_t * handle, int cmd, int op);