
#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include "servo_control.h"

#define DRIVER_NAME "vga_ball"

/* Device registers */
#define BG_S0(x) (x)
#define BG_S1(x) ((x)+1)
#define BG_S2(x) ((x)+2)
#define BG_OT(x) ((x)+3)

/*
 * Information about our device
 */
struct servos_dev {
	struct resource res; /* Resource: our registers */
	void __iomem *virtbase; /* Where registers can be accessed in memory */
        servos_t all_servos;
} dev;

/*
 * Write segments of a single digit
 * Assumes digit is in range and the device information has been set up
 */
static void write_background(servos_t *all_servos)
{
	iowrite8(all_servos->s0, BG_S0(dev.virtbase) );
	iowrite8(all_servos->s1, BG_S1(dev.virtbase) );
	iowrite8(all_servos->s2, BG_S2(dev.virtbase) );
	iowrite8(all_servos->ot, BG_OT(dev.virtbase) );
	dev.all_servos = *all_servos;
}

/*
 * Handle ioctl() calls from userspace:
 * Read or write the segments on single digits.
 * Note extensive error checking of arguments
 */
static long servo_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
	control_servo_t vla;

	switch (cmd) {
	case WRITE_to_SERVO:
		if (copy_from_user(&vla, (control_servo_t *) arg,
				   sizeof(control_servo_t)))
			return -EACCES;
		write_background(&vla.all_servos);
		break;

	case READ_SERVO:
	  	vla.all_servos = dev.all_servos;
		if (copy_to_user((control_servo_t *) arg, &vla,
				 sizeof(control_servo_t)))
			return -EACCES;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

/* The operations our device knows how to do */
static const struct file_operations servo_fops = {
	.owner		= THIS_MODULE,
	.unlocked_ioctl = servo_ioctl,
};

/* Information about our device for the "misc" framework -- like a char dev */
static struct miscdevice servo_misc_device = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= DRIVER_NAME,
	.fops		= &servo_fops,
};

/*
 * Initialization code: get resources (registers) and display
 * a welcome message
 */
static int __init servo_probe(struct platform_device *pdev)
{
        servos_t beige = { 0x87, 0x87, 0x87, 0x14 };
	int ret;

	ret = misc_register(&servo_misc_device);

	/* Get the address of our registers from the device tree */
	ret = of_address_to_resource(pdev->dev.of_node, 0, &dev.res);
	if (ret) {
		ret = -ENOENT;
		goto out_deregister;
	}

	/* Make sure we can use these registers */
	if (request_mem_region(dev.res.start, resource_size(&dev.res),
			       DRIVER_NAME) == NULL) {
		ret = -EBUSY;
		goto out_deregister;
	}

	/* Arrange access to our registers */
	dev.virtbase = of_iomap(pdev->dev.of_node, 0);
	if (dev.virtbase == NULL) {
		ret = -ENOMEM;
		goto out_release_mem_region;
	}
        
	/* Set an initial color */
        write_background(&beige);

	return 0;

out_release_mem_region:
	release_mem_region(dev.res.start, resource_size(&dev.res));
out_deregister:
	misc_deregister(&servo_misc_device);
	return ret;
}

/* Clean-up code: release resources */
static int servo_remove(struct platform_device *pdev)
{
	iounmap(dev.virtbase);
	release_mem_region(dev.res.start, resource_size(&dev.res));
	misc_deregister(&servo_misc_device);
	return 0;
}

/* Which "compatible" string(s) to search for in the Device Tree */
#ifdef CONFIG_OF
static const struct of_device_id servo_of_match[] = {
	{ .compatible = "csee4840,vga_ball-1.0" },
	{},
};
MODULE_DEVICE_TABLE(of, servo_of_match);
#endif

/* Information for registering ourselves as a "platform" driver */
static struct platform_driver servo_driver = {
	.driver	= {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(servo_of_match),
	},
	.remove	= __exit_p(servo_remove),
};

/* Called when the module is loaded: set things up */
static int __init servo_init(void)
{
	pr_info(DRIVER_NAME ": init\n");
	return platform_driver_probe(&servo_driver, servo_probe);
}

/* Calball when the module is unloaded: release resources */
static void __exit servo_exit(void)
{
	platform_driver_unregister(&servo_driver);
	pr_info(DRIVER_NAME ": exit\n");
}

module_init(servo_init);
module_exit(servo_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Robot ARM");
MODULE_DESCRIPTION("servos driver");
