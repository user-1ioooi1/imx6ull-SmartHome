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


#define DEV_NAME "sr501"
#define DEV_CNT 1

struct sr501_Dev{
    dev_t devid;
    int major;
    int minor;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *nd;
    int gpio;
};

struct sr501_Dev sr501;

static int sr501_open(struct inode * inode, struct file * filp){
    filp->private_data = &sr501;
    return 0;
}

static int sr501_close(struct inode *inode, struct file *filp){
    struct sr501_Dev *dev = (struct sr501_Dev *)filp->private_data;
    return 0;
}

static ssize_t sr501_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos){
   struct sr501_Dev *dev = (struct sr501_Dev *)filp->private_data;
   u8 data;
   data = gpio_get_value(dev->gpio);
  
   if(copy_to_user(buf,&data,sizeof(data))){
        return -EINVAL;
   }

   return sizeof(data);
  
}

static ssize_t  sr501_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos){
    struct sr501_Dev *dev = (struct sr501_Dev *)filp->private_data;
    return 0;
}

static const struct file_operations sr501_fops = {
    .owner = THIS_MODULE,
    .open = sr501_open,
    .write = sr501_write,
    .read = sr501_read,
    .release =  sr501_close,
};

static int sr501_probe(struct platform_device* dev){
    int ret = 0;
    sr501.major = 0;
    printk("sr501 probe\n");
    if(sr501.major){
        sr501.devid = MKDEV(sr501.major,0);
        /* dev_t [/dev/]：主次设备号。
        unsigned count：需要分配的次设备号数量。
        *const char [/name/]：设备的名称。*/
        ret = register_chrdev_region(sr501.devid,DEV_CNT,DEV_NAME);
    }else{
        /* *dev_t [/dev/]：这是一个输出参数，用于接收分配的主次设备号。
        unsigned baseminor：次设备号的起始号。
        unsigned count：需要分配的次设备号数量。
        *const char [/name/]：设备的名称。*/
        ret =  alloc_chrdev_region(&sr501.devid,0,DEV_CNT,DEV_NAME);
    }
    
    if(ret < 0){
        goto fail_devid;
    }

    sr501.cdev.owner = THIS_MODULE;
    cdev_init(&sr501.cdev,&sr501_fops);
    ret = cdev_add(&sr501.cdev,sr501.devid,DEV_CNT);
    if(ret < 0)
        goto fail_cdev;

    sr501.class = class_create(THIS_MODULE,DEV_NAME);
    if(IS_ERR(sr501.class)){
        ret = PTR_ERR(sr501.class);
        goto fail_class;
    }

    sr501.device = device_create(sr501.class,NULL,sr501.devid,NULL,DEV_NAME);
    if(IS_ERR(sr501.device)){
        ret = PTR_ERR(sr501.device);
        goto fail_device;
    }
#if 0
    sr501.nd = of_find_node_by_path("/sr501");
    if(sr501.nd == NULL){
        ret = -EINVAL;
        goto fail_findnd;
    }
#endif
    sr501.nd = dev->dev.of_node;/*设备树节点转为device实例 ，包含设备树节点*/
    if(sr501.nd== NULL){
        ret = -EINVAL;
        goto fail_findnd;
    }
    sr501.gpio = of_get_named_gpio(sr501.nd,"sr501-gpios",0);
    if(sr501.gpio < 0){
        printk("can't find gpio");
        ret = -EINVAL;
        goto fail_rs;
    }else{
        printk("gpio: %d\n",sr501.gpio);
    }

    /*申请io*/
    ret = gpio_request(sr501.gpio,"sr501-gpios");
    if(ret){
        ret = -EINVAL;
        goto fail_rs;
    }

    ret = gpio_direction_input(sr501.gpio); 
    if(ret){
        ret = -EINVAL;
        goto fail_setoutput;
    }
    

    return 0;

fail_setoutput:
    gpio_free(sr501.gpio);
fail_rs:
fail_findnd:
    device_destroy(sr501.class,sr501.devid);
fail_device:
    class_destroy(sr501.class);
fail_class:
    cdev_del(&sr501.cdev);
fail_cdev:
    unregister_chrdev_region(sr501.devid,DEV_CNT);
fail_devid:
    return ret;
    return 0;
}

static int sr501_remove(struct platform_device* dev){
    printk("sr501 remove\n");
    gpio_free(sr501.gpio);
    device_destroy(sr501.class,sr501.devid);
    class_destroy(sr501.class);
    cdev_del(&sr501.cdev);
    unregister_chrdev_region(sr501.devid,DEV_CNT);
    return 0;
}

static struct of_device_id sr501_of_match[] = {
    {.compatible = "my,sr501" },
    {/*Sentinel*/}
};

static struct platform_driver sr501_driver = {
    .driver = {
        .name = DEV_NAME,
        .of_match_table = sr501_of_match,
    },
    .probe = sr501_probe,
    .remove = sr501_remove,
    
};


static int __init sr501_init(void){
    return platform_driver_register(&sr501_driver);
}


static void __exit sr501_exit(void){
    platform_driver_unregister(&sr501_driver);
}



/*模块出入口*/
module_init(sr501_init);
module_exit(sr501_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("lcj ");
