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


#define DEV_NAME "dht11"
#define DEV_CNT 1

struct dht11_Dev{
    dev_t devid;
    int major;
    int minor;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *nd;
    int gpio;
    u8 humidity_H,humidity_L,tempature_H,tempature_L;
};

static struct dht11_Dev dht11;


static void dht11_reset(void)
{
  gpio_direction_output(dht11.gpio, 1);
}

static u8 dht11_readBit(struct dht11_Dev* dev){
    u8 retry = 0;
    while(gpio_get_value(dev->gpio) && retry < 100) //低电平就继续
	{
		udelay(1);
		retry += 1;
	}

    if(retry >= 100) {
        printk("dht11: timeout waiting for low\n");
        return 0xFF; // 错误码
    }
    retry = 0;
    while(!gpio_get_value(dev->gpio) && retry < 100) //高电平就继续
    {
		udelay(1);
		retry += 1;
    }
    if(retry >= 100) {
        printk("dht11: timeout waiting for high\n");
        return 0xFF;
    }
    /*低到高的跳变*/
    udelay(40);
    if(gpio_get_value(dev->gpio))
    {
	return 1;
    }else{
	return 0;
    }
 
}

static void dht11_sendBegin(struct dht11_Dev* dev){
    dht11_reset();/*note */
    gpio_set_value(dev->gpio,0); /*拉低*/
    msleep(20); 
    gpio_set_value(dev->gpio,1);
    udelay(30);
    gpio_direction_input(dev->gpio);

}

static u8 dht11_readResponse(struct dht11_Dev* dev){
    u8 retry = 0;//判断超时
 
	/*低电平通过，否则将一直循环，直到retry==100*/
	while(gpio_get_value(dev->gpio) && retry < 100)
	{
		retry++;
		udelay(1);
	}
	
	/*retry>=100时，说明从机DHT11没有响应主机*/
	if(retry >= 100)
	{
		printk("The host receives no response signal!\n");
		return 1;
	}
 
	retry = 0;
    /*高电平通过*/
	while(!gpio_get_value(dev->gpio) && retry < 100)
	{
		retry++;
		udelay(1);
	}
 
	if(retry >= 100)
	{
		printk("DHT11 has not received a start signal!\n");
		return 1;
	}
 
	return 0;

}

static u8 dht11_readByte(struct dht11_Dev* dev){
    u8 value = 0;
    int i = 0;
    u8 bit;
    for(; i < 8; i++){ //大端
        value <<= 1;
        bit = dht11_readBit(dev);
        if(bit == 0xFF) {  // 读取错误
            return 0xFF;
        }
        value |= bit;
    }
    return value;
}

static int dht11_readData(struct dht11_Dev* dev){
    u8 check;
    unsigned long flags;
    
    dht11_sendBegin(dev);
    local_irq_save(flags);
    if(dht11_readResponse(dev)){
	local_irq_restore(flags);
        return -EINVAL;
    }
    dht11.humidity_H = dht11_readByte(dev);
    dht11.humidity_L = dht11_readByte(dev);
    dht11.tempature_H = dht11_readByte(dev);
    dht11.tempature_L = dht11_readByte(dev);
    check = dht11_readByte(dev);
	
    local_irq_restore(flags);

    if(dht11.humidity_H == 0xFF || dht11.humidity_L == 0xFF || dht11.tempature_H == 0xFF || dht11.tempature_L == 0xFF || check == 0xFF) {
        printk("dht11: read byte timeout\n");
        return -EINVAL;
    }
    if(dht11.humidity_H + dht11.humidity_L + dht11.tempature_H + dht11.tempature_L != check){
        printk("readData error\n");
        return -EINVAL;
    }

    return 0;
}



static int dht11_open(struct inode * inode, struct file * filp){
    filp->private_data = &dht11;
    return 0;
}

static int dht11_close(struct inode *inode, struct file *filp){
    struct dht11_Dev *dev = (struct dht11_Dev *)filp->private_data;
    return 0;
}

static ssize_t dht11_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos){
   struct dht11_Dev *dev = (struct dht11_Dev *)filp->private_data;
   u8 data[4];
   if(dht11_readData(dev) == -EINVAL)
	return -EINVAL;

   data[0] = dht11.humidity_H;
   data[1] = dht11.humidity_L;
   data[2] = dht11.tempature_H;
   data[3] = dht11.tempature_L;

   if(copy_to_user(buf,data,sizeof(data))){
        return -EINVAL;
   }
   return sizeof(data);
  
}

static ssize_t  dht11_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos){
    struct dht11_Dev *dev = (struct dht11_Dev *)filp->private_data;
    return 0;
}

static const struct file_operations dht11_fops = {
    .owner = THIS_MODULE,
    .open = dht11_open,
    .write = dht11_write,
    .read = dht11_read,
    .release =  dht11_close,
};

static int dht11_probe(struct platform_device* dev){
    int ret = 0;
    dht11.major = 0;
    printk("dht11 probe\n");
    if(dht11.major){
        dht11.devid = MKDEV(dht11.major,0);
        /* dev_t [/dev/]：主次设备号。
        unsigned count：需要分配的次设备号数量。
        *const char [/name/]：设备的名称。*/
        ret = register_chrdev_region(dht11.devid,DEV_CNT,DEV_NAME);
    }else{
        /* *dev_t [/dev/]：这是一个输出参数，用于接收分配的主次设备号。
        unsigned baseminor：次设备号的起始号。
        unsigned count：需要分配的次设备号数量。
        *const char [/name/]：设备的名称。*/
        ret =  alloc_chrdev_region(&dht11.devid,0,DEV_CNT,DEV_NAME);
    }
    
    if(ret < 0){
        goto fail_devid;
    }

    dht11.cdev.owner = THIS_MODULE;
    cdev_init(&dht11.cdev,&dht11_fops);
    ret = cdev_add(&dht11.cdev,dht11.devid,DEV_CNT);
    if(ret < 0)
        goto fail_cdev;

    dht11.class = class_create(THIS_MODULE,DEV_NAME);
    if(IS_ERR(dht11.class)){
        ret = PTR_ERR(dht11.class);
        goto fail_class;
    }

    dht11.device = device_create(dht11.class,NULL,dht11.devid,NULL,DEV_NAME);
    if(IS_ERR(dht11.device)){
        ret = PTR_ERR(dht11.device);
        goto fail_device;
    }
#if 0
    dht11.nd = of_find_node_by_path("/dht11");
    if(dht11.nd == NULL){
        ret = -EINVAL;
        goto fail_findnd;
    }
#endif
    dht11.nd = dev->dev.of_node;/*设备树节点转为device实例 ，包含设备树节点*/
    if(dht11.nd== NULL){
        ret = -EINVAL;
        goto fail_findnd;
    }
    dht11.gpio = of_get_named_gpio(dht11.nd,"dht11-gpios",0);
    if(dht11.gpio < 0){
        printk("can't find gpio");
        ret = -EINVAL;
        goto fail_rs;
    }else{
        printk("gpio: %d\n",dht11.gpio);
    }

    /*申请io*/
    ret = gpio_request(dht11.gpio,"dht11-gpios");
    if(ret){
        printk("can't request gpio");
        ret = -EINVAL;
        goto fail_rs;
    }

    ret = gpio_direction_output(dht11.gpio,1); /*高电平*/
    if(ret){
        printk("can't output");
        ret = -EINVAL;
        goto fail_setoutput;
    }
    

    return 0;

fail_setoutput:
    gpio_free(dht11.gpio);
fail_rs:
fail_findnd:
    device_destroy(dht11.class,dht11.devid);
fail_device:
    class_destroy(dht11.class);
fail_class:
    cdev_del(&dht11.cdev);
fail_cdev:
    unregister_chrdev_region(dht11.devid,DEV_CNT);
fail_devid:
    return ret;
    return 0;
}

static int dht11_remove(struct platform_device* dev){
    printk("dht11 remove\n");
    gpio_free(dht11.gpio);
    device_destroy(dht11.class,dht11.devid);
    class_destroy(dht11.class);
    cdev_del(&dht11.cdev);
    unregister_chrdev_region(dht11.devid,DEV_CNT);
    return 0;
}

static struct of_device_id dht11_of_match[] = {
    {.compatible = "my,dht11" },
    {/*Sentinel*/}
};

static struct platform_driver dht11_driver = {
    .driver = {
        .name = DEV_NAME,
        .of_match_table = dht11_of_match,
    },
    .probe = dht11_probe,
    .remove = dht11_remove,
    
};


static int __init dht11_init(void){
    return platform_driver_register(&dht11_driver);
}


static void __exit dht11_exit(void){
    platform_driver_unregister(&dht11_driver);
}



/*模块出入口*/
module_init(dht11_init);
module_exit(dht11_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("lcj ");
