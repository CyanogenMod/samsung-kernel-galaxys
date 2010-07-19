/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __KVM_IODEV_H__
#define __KVM_IODEV_H__

#include <linux/kvm_types.h>

struct kvm_io_device {
	void (*read)(struct kvm_io_device *this,
		     gpa_t addr,
		     int len,
		     void *val);
	void (*write)(struct kvm_io_device *this,
		      gpa_t addr,
		      int len,
		      const void *val);
	int (*in_range)(struct kvm_io_device *this, gpa_t addr, int len,
			int is_write);
	void (*destructor)(struct kvm_io_device *this);

	void             *private;
};

static inline void kvm_iodevice_read(struct kvm_io_device *dev,
				     gpa_t addr,
				     int len,
				     void *val)
{
	dev->read(dev, addr, len, val);
}

static inline void kvm_iodevice_write(struct kvm_io_device *dev,
				      gpa_t addr,
				      int len,
				      const void *val)
{
	dev->write(dev, addr, len, val);
}

static inline int kvm_iodevice_inrange(struct kvm_io_device *dev,
				       gpa_t addr, int len, int is_write)
{
	return dev->in_range(dev, addr, len, is_write);
}

static inline void kvm_iodevice_destructor(struct kvm_io_device *dev)
{
	if (dev->destructor)
		dev->destructor(dev);
}

#endif /* __KVM_IODEV_H__ */
