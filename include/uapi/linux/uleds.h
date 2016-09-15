/*
 * Userspace driver support for the LED subsystem
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef _UAPI__ULEDS_H_
#define _UAPI__ULEDS_H_

#define ULEDS_MAX_NAME_SIZE	80

struct uleds_user_dev {
	char name[ULEDS_MAX_NAME_SIZE];
};

#endif /* _UAPI__ULEDS_H_ */
