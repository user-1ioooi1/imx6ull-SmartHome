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
#include <linux/i2c.h>
#include <linux/delay.h>
#include "ap3216c.h"



#define DEV_CNT 1
#define DEV_NAME "ap3216c"
#define ap3216c_read_reg(dev,reg,data) ap3216c_read_regs(dev,reg,data,1)
#define ap3216c_write_reg(dev, reg, data) \
    do { \
        u8 __temp = (data); \
        ap3216c_write_regs((dev), (reg), &__temp, 1); \
    } while(0)


struct ap3216c_Dev{
    dev_t devid;
    int major;
    int minor;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *nd;
    struct i2c_client *client;
    unsigned short ir,als,ps;
};

static struct ap3216c_Dev ap3216c;

/*读取ap3216c的N个寄存器*/
static int ap3216c_read_regs(struct ap3216c_Dev* dev, u8 reg, void *val,int len){
    struct i2c_client *client = dev->client;
    struct i2c_msg msg[2];
    int err = 0;
    msg[0].addr = client->addr;
    msg[0].flags = 0; /*标志位，写数据为0*/
    msg[0].buf = &reg; /*发送的数据（寄存器地址）的指针*/
    msg[0].len = 1;

    msg[1].addr = client->addr;
    msg[1].flags = I2C_M_RD; /*标志位，读数据为1*/
    msg[1].buf = val; /*接收数据地址*/
    msg[1].len = len;

    err = i2c_transfer(client->adapter, msg,2); /*返回传输成功msg数量*/ /*一次通信过程*/

    if(err != 2){
        err = -EINVAL;
        printk("read regs from ap3216c has been failed\n");
    }
    return err;
}

/*写ap3216c的N个寄存器*/
static int ap3216c_write_regs(struct ap3216c_Dev* dev, u8 reg, void *buf,int len){
    struct i2c_client *client = dev->client;
    struct i2c_msg msg;
    u8 b[256] = { 0 }; /*从机地址+发送数据*/
    int err = 0;
    b[0] = reg;
    memcpy(&b[1],buf,len);
    
    msg.addr = client->addr;
    msg.flags = 0; /*标志位，写数据为0*/
    msg.buf = b; /*要发送的数据的地址*/
    msg.len = len + 1;

    err = i2c_transfer(client->adapter, &msg,1); /*返回传输成功msg数量*/

    if(err != 1){
        err = -EINVAL;
        printk("read regs from ap3216c has been failed\n");
    }
    return err;
}


static int ap3216c_readData(struct ap3216c_Dev *dev){
    unsigned char buf[6] = { 0 };
    unsigned char i = 0;

    for(i = 0; i < 6; i++){
        ap3216c_read_reg(dev,AP3216C_IRDATALOW + i,buf + i);
    }
    if(buf[0] & 0x80){ /*ir ps 无效*/
        dev->ir = 0;
        dev->ps = 0;
    }else{
        dev->ir = ((unsigned short)buf[1] << 2) | (buf[0] & 0x03);
        dev->ps = ((unsigned short)(buf[5] & 0x3f) << 4) | (buf[4] & 0x0f);
    }
    dev->als = ((unsigned short)buf[3] << 8) | buf[2];

    return 0;
}



static int ap3216c_open(struct inode * inode, struct file * filp){
    unsigned char val = 0;
    filp->private_data = &ap3216c;
    printk("open !\n");
    ap3216c_write_reg(&ap3216c,AP3216C_SYSTEMCONG,0x4); /*复位*/
    mdelay(50);
    ap3216c_write_reg(&ap3216c,AP3216C_SYSTEMCONG,0x3);

    ap3216c_read_reg(&ap3216c,AP3216C_SYSTEMCONG,&val);
    printk("AP3216C_SYSTEMCONG = %d \n",val);

    return 0;
}

static int ap3216c_close(struct inode *inode, struct file *filp){
    struct ap3216c_Dev *dev = (struct ap3216c_Dev *)filp->private_data;

    
    return 0;
}

static ssize_t ap3216c_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos){
   struct ap3216c_Dev *dev = (struct ap3216c_Dev *)filp->private_data;
   unsigned short data[3];
   int err = 0;
   ap3216c_readData(dev);
   data[0] = dev->ir;
   data[1] = dev->als;
   data[2] = dev->ps;
   err = copy_to_user(buf,data,sizeof(data));
   if(err){
      err = -EFAULT;
      return err;
   }
   return sizeof(data);
  
}

static ssize_t  ap3216c_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos){
    struct ap3216c_Dev *dev = (struct ap3216c_Dev *)filp->private_data;
    
    return 0;
}

static const struct file_operations ap3216c_fops = {
    .owner = THIS_MODULE,
    .open = ap3216c_open,
    .write = ap3216c_write,
    .read = ap3216c_read,
    .release =  ap3216c_close,
};


static struct i2c_device_id ap3216c_id[]= {
    {"ap3216c"},
    {}
};


static struct of_device_id of_match_ap3216c[]= {
    {.compatible = "my,my_ap3216c"},
    {} 
};

static int ap3216c_probe(struct i2c_client *client, const struct i2c_device_id * id){
    int ret = 0;
    ap3216c.major = 0;
    printk("probe!\n");
    if(ap3216c.major){
        ap3216c.devid = MKDEV(ap3216c.major,0);
        /* dev_t [/dev/]：主次设备号。
        unsigned count：需要分配的次设备号数量。
        *const char [/name/]：设备的名称。*/
        ret = register_chrdev_region(ap3216c.devid,DEV_CNT,DEV_NAME);
    }else{
        /* *dev_t [/dev/]：这是一个输出参数，用于接收分配的主次设备号。
        unsigned baseminor：次设备号的起始号。
        unsigned count：需要分配的次设备号数量。
        *const char [/name/]：设备的名称。*/
        ret =  alloc_chrdev_region(&ap3216c.devid,0,DEV_CNT,DEV_NAME);
    }
    
    if(ret < 0){
        goto failed_devid;
    }

    ap3216c.cdev.owner = THIS_MODULE;
    cdev_init(&ap3216c.cdev,&ap3216c_fops);
    ret = cdev_add(&ap3216c.cdev,ap3216c.devid,DEV_CNT);
    if(ret < 0)
        goto failed_cdev;

    ap3216c.class = class_create(THIS_MODULE,DEV_NAME);
    if(IS_ERR(ap3216c.class)){
        ret = PTR_ERR(ap3216c.class);
        goto failed_class;
    }

    ap3216c.device = device_create(ap3216c.class,NULL,ap3216c.devid,NULL,DEV_NAME);
    if(IS_ERR(ap3216c.device)){
        ret = PTR_ERR(ap3216c.device);
        goto failed_device;
    }

    ap3216c.client = client;
    ap3216c.nd = client->dev.of_node;

    if(ap3216c.nd == NULL){
        ret = -EINVAL;
        goto failed_findnd;
    }

    return 0;

failed_findnd:
    device_destroy(ap3216c.class,ap3216c.devid);
failed_device:
    class_destroy(ap3216c.class);
failed_class:
    cdev_del(&ap3216c.cdev);
failed_cdev:
    unregister_chrdev_region(ap3216c.devid,DEV_CNT);
failed_devid:
    return ret;

}

static int ap3216c_remove(struct i2c_client *client){
    printk("ap3216c_exit\n");
    device_destroy(ap3216c.class,ap3216c.devid);
    class_destroy(ap3216c.class);
    cdev_del(&ap3216c.cdev);
    unregister_chrdev_region(ap3216c.devid,DEV_CNT);
    return 0;
}

static struct i2c_driver ap3216c_driver = {
    .probe =  ap3216c_probe,
    .remove = ap3216c_remove,
    .driver = {
        .name = "ap3216c",
        .owner = THIS_MODULE,
        .of_match_table = of_match_ap3216c,
    },
    .id_table = ap3216c_id,
};


static int __init ap3216c_init(void){
    return i2c_add_driver(&ap3216c_driver);
}


static void __exit ap3216c_exit(void){
    i2c_del_driver(&ap3216c_driver);
}


module_init(ap3216c_init);
module_exit(ap3216c_exit);



MODULE_LICENSE("GPL");
MODULE_AUTHOR("lcj ");
