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
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>
#include <linux/input/mt.h>
#include "gt1158.h"


#define DEV_CNT 1
#define DEV_NAME "gt1158"
#define X_MAX 800
#define Y_MAX 480

#define gt1158_read_reg(dev,reg,data) gt1158_read_regs(dev,reg,data,1)
#define gt1158_write_reg(dev, reg, data) \
    do { \
        u8 __temp = (data); \
        gt1158_write_regs((dev), (reg), &__temp, 1); \
    } while(0)

enum Touch
{
    TOUCH_EVENT_RELEASE = 0,
    TOUCH_EVENT_PRESS,
    
};




struct gt1158_Dev{
    struct device_node *nd;
    struct i2c_client *client;
    int irq_pin;
    int reset_pin;
    int irq_num;
    struct input_dev* input;
};



static struct gt1158_Dev gt1158;

/*读取gt1158的N个寄存器,reg为起始地址*/
static int gt1158_read_regs(struct gt1158_Dev* dev, u16 reg, void *val,int len){
    struct i2c_client *client = dev->client;
    struct i2c_msg msg[2];
    int err = 0;
    u8 reg_byte[2];
    reg_byte[0] = reg >> 8;
    reg_byte[1] = reg & 0xff;

    msg[0].addr = client->addr;
    msg[0].flags = 0; /*标志位，写数据为0*/
    msg[0].buf = reg_byte; /*发送的数据（寄存器地址）的指针*/
    msg[0].len = 2;

    msg[1].addr = client->addr;
    msg[1].flags = I2C_M_RD; /*标志位，读数据*/
    msg[1].buf = val; /*接收数据地址*/
    msg[1].len = len;

    err = i2c_transfer(client->adapter, msg,2); /*返回传输成功msg数量*/ /*一次通信过程*/

    if(err != 2){
        err = -EINVAL;
        printk("read regs from gt1158 has been failed\n");
    }
    return err;
}

/*写gt1158的N个寄存器，reg为起始地址*/
static int gt1158_write_regs(struct gt1158_Dev* dev, u16 reg, void *buf,int len){
    struct i2c_client *client = dev->client;
    struct i2c_msg msg;  
    int err = 0;
    u8 *b = kzalloc(len + 2,GFP_KERNEL); /*从机地址+发送数据*/
    if(!b){
 	kfree(b);
        return -EFAULT;
    }

    b[0] = reg >> 8;
    b[1] = reg & 0xff;

    memcpy(&b[2],buf,len);
    msg.addr = client->addr;
    msg.flags = 0; /*标志位，写数据为0*/
    msg.buf = b; /*要发送的数据的地址*/
    msg.len = len + 2;

    err = i2c_transfer(client->adapter, &msg,1); /*返回传输成功msg数量*/

    if(err != 1){
        err = -EINVAL;
        printk("write regs from gt1158 has been failed\n");
    }

    kfree(b);

    return err;
}


static int gt1158_readConfig(struct gt1158_Dev *dev){

    u8 data[7] = { 0 };
    u16 maxX, maxY;
    u8 trigger_state;
    gt1158_read_regs(dev,GT_1xx_CFGS_REG,data,7);
    maxX = (data[2] << 8 ) | data[1];
    maxY = (data[4] << 8 ) | data[3];

    trigger_state = data[6] & 0x3;


    printk("maxX = %u, maxY = %u,triggger = %u \n",maxX,maxY,trigger_state);

    return 0;
}



static int gt1158_open(struct inode * inode, struct file * filp){
    filp->private_data = &gt1158;
    printk("open !\n");
    



    return 0;
}

static int gt1158_close(struct inode *inode, struct file *filp){
    struct gt1158_Dev *dev = (struct gt1158_Dev *)filp->private_data;

    
    return 0;
}

static ssize_t gt1158_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos){
   struct gt1158_Dev *dev = (struct gt1158_Dev *)filp->private_data;
 

   return 0;
  
}

static ssize_t  gt1158_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos){
    struct gt1158_Dev *dev = (struct gt1158_Dev *)filp->private_data;
    
    return 0;
}

static const struct file_operations gt1158_fops = {
    .owner = THIS_MODULE,
    .open = gt1158_open,
    .write = gt1158_write,
    .read = gt1158_read,
    .release =  gt1158_close,
};


static struct i2c_device_id gt1158_id[]= {
    {"gt1158"},
    {}
};


static struct of_device_id of_match_gt1158[]= {
    {.compatible = "my,gt1158"},
    {} 
};



static irqreturn_t gt1158_irq_handler(int irq, void *dev_id){
    struct gt1158_Dev* dev = (struct gt1158_Dev*)dev_id;
    int i = 0;
    int ret = 0 ;
    u8 touch_num = 0;
    u8 data[40] = { 0 };
    u16 x,y;  
    u8 id = 0;
    u16 touch_index = 0; /*位图*/
    static u16 last_index = 0;

    ret = gt1158_read_regs(dev, GT_GSTID_REG, &touch_num , 1);

    if(touch_num == 0){
        return IRQ_HANDLED;
    }else{
        touch_num &= 0x0f;
    }
	

    if(touch_num > 5)
	    touch_num = 5;

    if(touch_num){
	    gt1158_read_regs(dev,GT_TP1_REG,data,40);

	    for(i = 0; i < touch_num; i++){			
	        id =  data[i * 8] & 0x0f;
            	x = (data[i * 8 + 2] << 8) | data[i * 8 + 1];
            	y = (data[i * 8 + 4] << 8) | data[i * 8 + 3];

            	//printk("x = %d  y = %d \n",x,y);
            
            	touch_index |= (1 << id);

            	/*按下的坐标上报*/
                input_mt_slot(dev->input,id);
            	input_mt_report_slot_state(dev->input,MT_TOOL_FINGER,TOUCH_EVENT_PRESS);
            	input_report_abs(dev->input,ABS_MT_POSITION_X,x);
            	input_report_abs(dev->input,ABS_MT_POSITION_Y,y);
            
        }
   }

    for(i = 0; i < 5; i++){
        if((last_index & (1 << i)) && !(touch_index & (1 << i))){ /*last有 current 没有*/
            input_mt_slot(dev->input, i);	
            input_mt_report_slot_state(dev->input,MT_TOOL_FINGER,TOUCH_EVENT_RELEASE);			
        }
    }

    last_index = touch_index;
   

    //input_mt_report_pointer_emulation(dev->input,false);
    input_sync(dev->input);

    gt1158_write_reg(dev,GT_GSTID_REG,0);

    return IRQ_HANDLED;

}



static int gt1158_reset(struct gt1158_Dev* dev){
    int ret = 0;
    struct i2c_client *client = dev->client;


	if (gpio_is_valid(dev->reset_pin)) {  		
		//申请复位IO，并且默认输出LOW电平 
		ret = devm_gpio_request_one(&client->dev,	
					dev->reset_pin, GPIOF_OUT_INIT_LOW,
					"gt1158 reset");
		if (ret) {
                        ret = -EINVAL;
			return ret;
		}
	}
        //申请中断IO
	if (gpio_is_valid(dev->irq_pin)) {  		
		// 申请irq IO，并且默认输出HIGH电平
		ret = devm_gpio_request_one(&client->dev,	
					dev->irq_pin, GPIOF_IN,
					"gt1158 int");
		if (ret) {
                        ret = -EINVAL;
			return ret;
		}
	}

	//4、初始化gt1158，要严格按照gt1158时序要求  复位gt1158
	msleep(10);
	gpio_set_value(dev->reset_pin, 1); 
        msleep(100); //delay 100ms
	return 0;

}



static int gt1158_ts_irq(struct gt1158_Dev* dev){
    int ret = 0;
    struct i2c_client *client = dev->client;
    
    printk("irq = %d \n",client->irq);

    ret = devm_request_threaded_irq(&client->dev,client->irq,NULL,
                                    gt1158_irq_handler,IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
                                    client->name,dev);//中断是一次性触发的，不能嵌套
    if(ret){
        printk("can't request handler\n");
        return -EINVAL;
    }
    return 0;
}

static int gt1158_probe(struct i2c_client *client, const struct i2c_device_id * id){
    int ret = 0;
    int i = 0;
    u8 data[4];
    printk("probe!\n");
    gt1158.client = client;
    gt1158.nd = client->dev.of_node;
    printk("I2C Address: 0x%02x\n", client->addr);

    if(gt1158.nd == NULL){
        ret = -EINVAL;
        goto failed_findnd;
    }

    gt1158.irq_pin = of_get_named_gpio(gt1158.nd,"interrupt-gpios",0);
    gt1158.reset_pin = of_get_named_gpio(gt1158.nd,"reset-gpios",0);

  

    gt1158_reset(&gt1158);
    
    gt1158_write_reg(&gt1158, GT_CTRL_REG,0x02); //软复位
    mdelay(100);
    gt1158_write_reg(&gt1158, GT_CTRL_REG, 0x0); //停止软复位
    mdelay(100);


    gt1158.input = devm_input_allocate_device(&client->dev);

    gt1158.input->name = client->name;
    gt1158.input->id.bustype = BUS_I2C;
    gt1158.input->dev.parent = &client->dev;

    __set_bit(EV_SYN,gt1158.input->evbit);
    __set_bit(EV_KEY,gt1158.input->evbit);
    __set_bit(EV_ABS,gt1158.input->evbit);
    __set_bit(BTN_TOUCH,gt1158.input->keybit);


    input_mt_init_slots(gt1158.input,MAX_SUPPORT_POINTS,0);
    input_set_abs_params(gt1158.input,ABS_MT_POSITION_X,0,X_MAX,0,0);
    input_set_abs_params(gt1158.input,ABS_MT_POSITION_Y,0,Y_MAX,0,0);


    ret = input_register_device(gt1158.input);
    if(ret){
        printk("register_device err ! \n");
        ret =  -EINVAL;
        goto register_err;
    }

    for(i = 0; i < 4; i++){
	    gt1158_read_regs(&gt1158,GT_PID_REG,data,4);
        printk("%d \n",data[i]);
    }

    gt1158_readConfig(&gt1158);

    gt1158_ts_irq(&gt1158);

    return 0;
    
register_err:
failed_findnd:
    return ret;
}

static int gt1158_remove(struct i2c_client *client){
    printk("gt1158_exit\n");
    input_unregister_device(gt1158.input);
    
    return 0;
}

static struct i2c_driver gt1158_driver = {
    .probe =  gt1158_probe,
    .remove = gt1158_remove,
    .driver = {
        .name = "gt1158",
        .owner = THIS_MODULE,
        .of_match_table = of_match_gt1158,
    },
    .id_table = gt1158_id,
};


static int __init gt1158_init(void){
    return i2c_add_driver(&gt1158_driver);
}


static void __exit gt1158_exit(void){
    i2c_del_driver(&gt1158_driver);
}


module_init(gt1158_init);
module_exit(gt1158_exit);



MODULE_LICENSE("GPL");
MODULE_AUTHOR("lcj ");
