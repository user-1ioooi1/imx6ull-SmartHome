#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/string.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/pwm.h>



#define DEV_CNT 1
#define DEV_NAME "sg90"

struct sg90_Dev{
    dev_t devid;
    int major;
    int minor;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *nd;
    struct pwm_device *ppwm;
};

static struct sg90_Dev sg90;


static int sg90_open(struct inode * inode, struct file * filp){
    filp->private_data = &sg90;
    return 0;
}

static int sg90_close(struct inode *inode, struct file *filp){
    struct sg90_Dev *dev = (struct sg90_Dev *)filp->private_data;

    
    return 0;
}

static ssize_t sg90_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos){
    return 0;
}

static ssize_t  sg90_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos){
    struct sg90_Dev *dev = (struct sg90_Dev *)filp->private_data;
    int res;
	unsigned char data[1];
	if(count != 1)
		return -EINVAL;

	res = copy_from_user(data, buf, count);
	/* 配置PWM：旋转任意角度(单位1度) */
	pwm_config(dev->ppwm, 500000 + data[0] * 100000 / 9, 20000000);   /*data[0] = 90  为1.5ms 范围：0-180*/

	return 1;
}

static const struct file_operations sg90_fops = {
    .owner = THIS_MODULE,
    .open = sg90_open,
    .write = sg90_write,
    .read = sg90_read,
    .release =  sg90_close,
};



static struct of_device_id of_match_sg90[]= {
    {.compatible = "my,my_sg90"},
    {} 
};

static int sg90_probe(struct platform_device* pdev){
    int ret = 0;
    sg90.major = 0;
    printk("probe!\n");
    if(sg90.major){
        sg90.devid = MKDEV(sg90.major,0);
        /* dev_t [/dev/]：主次设备号。
        unsigned count：需要分配的次设备号数量。
        *const char [/name/]：设备的名称。*/
        ret = register_chrdev_region(sg90.devid,DEV_CNT,DEV_NAME);
    }else{
        /* *dev_t [/dev/]：这是一个输出参数，用于接收分配的主次设备号。
        unsigned baseminor：次设备号的起始号。
        unsigned count：需要分配的次设备号数量。
        *const char [/name/]：设备的名称。*/
        ret =  alloc_chrdev_region(&sg90.devid,0,DEV_CNT,DEV_NAME);
    }
    
    if(ret < 0){
        goto failed_devid;
    }

    sg90.cdev.owner = THIS_MODULE;
    cdev_init(&sg90.cdev,&sg90_fops);
    ret = cdev_add(&sg90.cdev,sg90.devid,DEV_CNT);
    if(ret < 0)
        goto failed_cdev;

    sg90.class = class_create(THIS_MODULE,DEV_NAME);
    if(IS_ERR(sg90.class)){
        ret = PTR_ERR(sg90.class);
        goto failed_class;
    }

    sg90.device = device_create(sg90.class,NULL,sg90.devid,NULL,DEV_NAME);
    if(IS_ERR(sg90.device)){
        ret = PTR_ERR(sg90.device);
        goto failed_device;
    }

    sg90.nd = pdev->dev.of_node;

    if(sg90.nd == NULL){
        ret = -EINVAL;
        goto failed_findnd;
    }

    sg90.ppwm = devm_of_pwm_get(&pdev->dev, sg90.nd, NULL);
    if(IS_ERR(sg90.ppwm)){
        printk("can't get pwm3\n");
        return -1;
    }

    pwm_config(sg90.ppwm, 500000, 20000000); /*0.5 ms 0度    1ms 45度 ..... 2.5ms 180度  周期20ms*/

    pwm_set_polarity(sg90.ppwm, PWM_POLARITY_NORMAL);

    pwm_enable(sg90.ppwm);



    return 0;

failed_findnd:
    device_destroy(sg90.class,sg90.devid);
failed_device:
    class_destroy(sg90.class);
failed_class:
    cdev_del(&sg90.cdev);
failed_cdev:
    unregister_chrdev_region(sg90.devid,DEV_CNT);
failed_devid:
    return ret;

}

static int sg90_remove(struct platform_device* pdev){
    printk("sg90_exit\n");
    pwm_config(sg90.ppwm,500000, 20000000);
    pwm_disable(sg90.ppwm);
    /*devm自动调用free*/
    device_destroy(sg90.class,sg90.devid);
    class_destroy(sg90.class);
    cdev_del(&sg90.cdev);
    unregister_chrdev_region(sg90.devid,DEV_CNT);
    return 0;
}

static struct platform_driver sg90_driver = {
    .probe =  sg90_probe,
    .remove = sg90_remove,
    .driver = {
        .name = DEV_NAME,
        .owner = THIS_MODULE,
        .of_match_table = of_match_sg90,
    },
};


static int __init sg90_init(void){
    return platform_driver_register(&sg90_driver);
}


static void __exit sg90_exit(void){
    platform_driver_unregister(&sg90_driver);
}
   


module_init(sg90_init);
module_exit(sg90_exit);



MODULE_LICENSE("GPL");
MODULE_AUTHOR("lcj ");
