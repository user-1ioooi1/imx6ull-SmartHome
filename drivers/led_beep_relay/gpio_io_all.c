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
#define DEV_NAME "gpio_ioX"

struct ioX_Dev{
    dev_t devid;
    int major;
    int minor;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *nd;
    int gpio; /*引脚编号*/
};


static int ioX_open(struct inode * inode, struct file * filp){ 
    /*index node (索引节点)	     file pointer (文件指针)*/
    /*一个文件一个	              一次打开一个*/
    /*内存中永久存在	          动态分配，用完释放*/
    struct ioX_Dev* ioX = container_of(inode->i_cdev,struct ioX_Dev, cdev);
    filp->private_data = ioX;

    return 0;
}

static int ioX_close(struct inode *inode, struct file *filp){
    
    filp->private_data = NULL;

    return 0;
}

static ssize_t ioX_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos){
    return 0;
}

static ssize_t  ioX_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos){
    struct ioX_Dev *dev = (struct ioX_Dev *)filp->private_data;
    int res;
	unsigned char value[1] = { 0 };
	if(count != 1)
		return -EINVAL;

	res = copy_from_user(value, buf, count);

    if(res){
        return -EINVAL;
    }

    gpio_set_value(dev->gpio,value[0]); 
	
	return 1;
}

static const struct file_operations ioX_fops = {
    .owner = THIS_MODULE,
    .open = ioX_open,
    .write = ioX_write,
    .read = ioX_read,
    .release =  ioX_close,
};



static struct of_device_id of_match_ioX[]= {
    {.compatible = "my,gpioled"},
    {.compatible = "my,beep"},
    {.compatible = "my,jdq"},
    {/*Sentinel*/}
};

static int ioX_probe(struct platform_device* pdev){
    int ret = 0;
    struct ioX_Dev *ioX = NULL;
    printk("%s probe!\n",pdev->name);

    
    // 为这个设备分配私有数据
    ioX = devm_kzalloc(&pdev->dev, sizeof(*ioX), GFP_KERNEL);
    if (!ioX)
        return -ENOMEM;
    ioX->major = 0;

    platform_set_drvdata(pdev, ioX);

    if(ioX->major){
        ioX->devid = MKDEV(ioX->major,0);
        /* dev_t [/dev/]：主次设备号。
        unsigned count：需要分配的次设备号数量。
        *const char [/name/]：设备的名称。*/
        ret = register_chrdev_region(ioX->devid,DEV_CNT,pdev->name);
    }else{
        /* *dev_t [/dev/]：这是一个输出参数，用于接收分配的主次设备号。
        unsigned baseminor：次设备号的起始号。
        unsigned count：需要分配的次设备号数量。
        *const char [/name/]：设备的名称。*/
        ret =  alloc_chrdev_region(&ioX->devid,0,DEV_CNT,pdev->name);
    }
    
    if(ret < 0){
        goto failed_devid;
    }

    ioX->cdev.owner = THIS_MODULE;
    cdev_init(&ioX->cdev,&ioX_fops);
    ret = cdev_add(&ioX->cdev,ioX->devid,DEV_CNT);
    if(ret < 0)
        goto failed_cdev;

    ioX->class = class_create(THIS_MODULE,pdev->name);
    if(IS_ERR(ioX->class)){
        ret = PTR_ERR(ioX->class);
        goto failed_class;
    }

    ioX->device = device_create(ioX->class, &pdev->dev,ioX->devid,NULL,pdev->name); /*devices interface*/
    if(IS_ERR(ioX->device)){
        ret = PTR_ERR(ioX->device);
        goto failed_device;
    }


    ioX->nd = pdev->dev.of_node;/*设备树节点*/
    if(ioX->nd == NULL){
        ret = -EINVAL;
        goto failed_findnd;
    }
	
    

    ioX->gpio = of_get_named_gpio(ioX->nd,"ioX-gpios",0);
    if(ioX->gpio < 0){
        printk("can't find gpio\n");
        ret = -EINVAL;
        goto failed_rs;
    }else{
        printk("gpio: %d\n",ioX->gpio);
    }

    /*申请io*/
    ret = gpio_request(ioX->gpio,"ioX-gpios");
    if(ret){
        printk("can't request gpio\n");
        ret = -EINVAL;
        goto failed_rs;
    }
    ret = gpio_direction_output(ioX->gpio,1); /*设置为输出，且默认为高电平*/
    if(ret){
        printk("can't output\n");
        ret = -EINVAL;
        goto failed_setoutput;
    }
    
    return 0;

failed_setoutput:
    gpio_free(ioX->gpio);
failed_rs:
failed_findnd:
    device_destroy(ioX->class,ioX->devid);
failed_device:
    class_destroy(ioX->class);
failed_class:
    cdev_del(&ioX->cdev);
failed_cdev:
    unregister_chrdev_region(ioX->devid,DEV_CNT);
failed_devid:
    return ret;

}

static int ioX_remove(struct platform_device* pdev){
    struct ioX_Dev *ioX =  platform_get_drvdata(pdev);
    printk("ioX_exit\n");
    gpio_free(ioX->gpio);
    device_destroy(ioX->class,ioX->devid);
    class_destroy(ioX->class);
    cdev_del(&ioX->cdev);
    unregister_chrdev_region(ioX->devid,DEV_CNT);
    return 0;
}

static struct platform_driver ioX_driver = {
    .probe =  ioX_probe,
    .remove = ioX_remove,
    .driver = {
        .name = DEV_NAME,
        .owner = THIS_MODULE,
        .of_match_table = of_match_ioX,
    },
};


static int __init ioX_init(void){
    return platform_driver_register(&ioX_driver);
}


static void __exit ioX_exit(void){
    platform_driver_unregister(&ioX_driver);
}
   


module_init(ioX_init);
module_exit(ioX_exit);



MODULE_LICENSE("GPL");
MODULE_AUTHOR("lcj ");
