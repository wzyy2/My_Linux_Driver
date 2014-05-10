#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <asm/uaccess.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <asm/atomic.h>
#include <linux/mutex.h>
#include <linux/spi/spi.h>
#include "rc522_api.h"
#include <linux/workqueue.h>


#define  N_1  1
#define  N_2  2
#define  N_3  3
#define  N_4  4
#define  N_NOP  255



#define  RELOAD_COUNT  0xfb    //18.432M  9600  0xfb
//         4800  0xf6 
#define CHANGE_PASSWD 1
#define CHANGE_BLOCK  2
#define READ_CARD     3  
#define WRITE_CARD    4
#define CHANGE_KEY    5
#define GET_ID 	      6

typedef unsigned char uchar;
uchar NewKey[16]={0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x07,0x80,0x69,0x00,0x00,0x00,0x00,0x00,0x00};

/*static DECLARE_WAIT_QUEUE_HEAD(rc522_wait);*/
static unsigned char Read_Data[16]={0x00};
static unsigned char read_data_buff[16];

static uchar PassWd[6]={0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
static uchar WriteData[16];
static unsigned char RevBuffer[30];
static unsigned char MLastSelectedSnr[4];

uint KuaiN;
uchar operationcard;

struct workqueue_struct *rc522_wq;
struct work_struct rc522_work;

struct spi_device *rc522_spi;

void delay_ms(uint tms)
{
	mdelay(tms);
}
void InitRc522(void)
{
	unsigned char a;
	PcdReset();
	a = ReadRawRC(TReloadRegL);
	if(a != 30)
		printk(KERN_DEBUG"FUCK ,NO RC522%d\n",a);
	else
		printk(KERN_DEBUG"RC522 exist\n");
	PcdAntennaOff();  
	PcdAntennaOn();
	M500PcdConfigISOType( 'A' );
}

static char rc522_loop_work(uchar opnd)
{
	char *pdata = read_data_buff;
	char status;

	PcdReset();
	status=PcdRequest(PICC_REQIDL,&RevBuffer[0]);//Ñ°ÌìÏßÇøÄÚÎ´½øÈëÐÝÃß×´Ì¬µÄ¿¨£¬·µ»Ø¿¨Æ¬ÀàÐÍ 2×Ö½Ú
	if(status!=MI_OK)
	{
		printk(KERN_DEBUG"search card: no card\n");
		return -EFAULT;
	}
	status=PcdAnticoll(&RevBuffer[2]);//·À³å×²£¬·µ»Ø¿¨µÄÐòÁÐºÅ 4×Ö½Ú
	if(status!=MI_OK)
	{
		printk(KERN_DEBUG"get card nu: no number\n");
		return -EFAULT;
	} 
	memcpy(MLastSelectedSnr,&RevBuffer[2],4);

	status=PcdSelect(MLastSelectedSnr);//Ñ¡¿¨
	if(status!=MI_OK)
	{
		printk(KERN_DEBUG"select card: no card\n");
		return -EFAULT;            
	}
	if (opnd == GET_ID) {
		PcdHalt();	
		return 0;	
	}
	else if (opnd == READ_CARD) {//¶Á¿¨
		status=PcdAuthState(PICC_AUTHENT1A,KuaiN,PassWd,MLastSelectedSnr);
		if(status!=MI_OK)
		{
			printk(KERN_DEBUG"read authorize card err\n");
			return -EFAULT;               
		}
		status=PcdRead(KuaiN,Read_Data);
		if(status!=MI_OK)
		{
			printk(KERN_DEBUG"read card err\n");
			return -EFAULT;
		} else {
			int i;
			memcpy(pdata, Read_Data, sizeof(Read_Data));
			/*wake_up_interruptible(&rc522_wait);*/
			printk(KERN_DEBUG"read block %d info:", KuaiN);
			for(i = 0; i < 16; i++) {
				printk(KERN_DEBUG"%2.2X",pdata[i]);
			}
			printk(KERN_DEBUG"\n");
		}
	} else if (opnd == CHANGE_KEY) {//ÐÞ¸ÄÃÜÂë
		status=PcdAuthState(PICC_AUTHENT1A,KuaiN,PassWd,MLastSelectedSnr);
		if(status!=MI_OK)
		{
			printk(KERN_DEBUG"card authorize err");
			return -EFAULT;
		}
		status=PcdWrite(KuaiN,&NewKey[0]);
		if(status!=MI_OK)
		{
			printk(KERN_DEBUG"change password err");
			return -EFAULT;
		} else
			printk(KERN_DEBUG"set password success");
	} else if (opnd == WRITE_CARD) {//Ð´¿¨
		status=PcdAuthState(PICC_AUTHENT1A,KuaiN,PassWd,MLastSelectedSnr);
		if(status!=MI_OK)
		{
			printk(KERN_DEBUG"write authrioze err\n");
			return -EFAULT;               
		}
		status=PcdWrite(KuaiN,&WriteData[0]);
		if(status!=MI_OK)
		{
			printk(KERN_DEBUG"write data err\n");
			return -EFAULT;
		} else {
			printk(KERN_DEBUG"write data to block %d sucess\n", KuaiN);
		}
	}
	PcdHalt();	
	return 0;
}



static int rc522_open(struct inode *inode,struct file *filp)
{

	InitRc522();
	printk(KERN_DEBUG"rc522 start work!\n");
	return 0;
}

static ssize_t rc522_read (struct file *filp, char *buf, size_t count, loff_t *f_pos)
{
	/*PcdReset();*/
	operationcard = READ_CARD;
	if(rc522_loop_work(operationcard))
		return 0;
	printk(KERN_DEBUG"card info:%2.2X\n",Read_Data[0]);
	if (copy_to_user(buf, read_data_buff, sizeof(read_data_buff))) {
		printk(KERN_DEBUG"copy card number to userspace err\n");
		return 0;
	}
	return sizeof(read_data_buff);
}

static ssize_t rc522_write (struct file *filp, const char *buf, size_t count, loff_t *f_pos)
{
	if (KuaiN == 0) {
		printk(KERN_DEBUG"block[0] is reserveed, can't write\n");
		return 0;
	}
	if (KuaiN < 0 || KuaiN > 63) {
		printk(KERN_DEBUG"block[%d] unreachable, please set the write block first", KuaiN);
		return -0;
	} 
	if ((KuaiN % 4) == 3) {
		printk(KERN_DEBUG"block[%d] is key block, not data block\n", KuaiN);
		return -0;
	}
	memset(WriteData, 0, sizeof(WriteData));
	if (copy_from_user(WriteData, (char *)buf, count)) {
		printk(KERN_DEBUG"%s, [line %d] copy from user err.\n", __FILE__, __LINE__);
		return 0;
	}
	/*PcdReset();*/
	operationcard =  WRITE_CARD;
	if(rc522_loop_work(operationcard))
		return -EFAULT;
	return 0;
}

static int rc522_release(struct inode *inode,struct file *filp)
{
	printk(KERN_DEBUG"%s\n", __func__);
	return 0;
}


static unsigned int rc522_poll(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;

	//poll_wait(file, &rc522_wait, wait);

	/*if (have_card_number) {*/
	/*have_card_number = 0;*/
	/*mask |= POLLIN | POLLRDNORM;*/
	/*}   */

	return mask;
}

static long rc522_ioctl(struct file *file, unsigned int cmd, unsigned long arg) 
{
	switch(cmd) {
		case CHANGE_PASSWD:
			operationcard = CHANGE_PASSWD;
			if (copy_from_user(PassWd, (char *)arg, sizeof(PassWd))) {
				printk(KERN_DEBUG"%s:change pass word err", __func__);
				return -EFAULT;
			}
			break;
		case CHANGE_BLOCK:
			if (arg < 0 || arg > 63) {
				printk(KERN_DEBUG"block number err %lu", arg);
				return -EFAULT;
			}
			KuaiN = (int)arg;
			break;
		case READ_CARD:
			break;
		case WRITE_CARD:
			break;
		case CHANGE_KEY:
			operationcard = CHANGE_KEY;
			break;
		case GET_ID:
			operationcard =  GET_ID;
			if(!rc522_loop_work(operationcard)){
				if (copy_to_user((char *)arg, MLastSelectedSnr,4)) {
					printk(KERN_DEBUG"%s, [line %d] copy to user err.\n", __FILE__, __LINE__);
					return -EFAULT;
				}
			}
			else
				return -EFAULT;
			break;
		default:
			break;
	}
	return 0;
}


static int rc522_remove(struct spi_device *spi)
{

	return 0;
}
static int rc522_probe(struct spi_device *spi)
{
	//默认读第一块(可选0 ~ 63)
	KuaiN = 1; 
	printk(KERN_DEBUG"%s\n", __func__);
	rc522_spi = spi;
	return 0;
};

static struct spi_driver rc522_driver = {
	.probe = rc522_probe,
	.remove = rc522_remove,
	.driver = {
		.name = "rfid_rc522",
	},
};

static struct file_operations rc522_fops = {
	.owner = THIS_MODULE,
	.open = rc522_open,
	.release = rc522_release, 
	.read = rc522_read,
	.write = rc522_write,
	.poll = rc522_poll, 
	.unlocked_ioctl = rc522_ioctl,
};

static struct miscdevice rc522_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "rfid_rc522_dev",
	.fops = &rc522_fops,
};

static int RC522_init(void)
{
	int res;

	/* Register the character device (atleast try) */
	printk(KERN_DEBUG"rfid_rc522 module init.\n");

	res =  misc_register(&rc522_misc_device);
	if(res < 0) {
		printk(KERN_DEBUG"device register failed with %d.\n",res);
		return res;
	}

	/*rc522_wq = create_singlethread_workqueue("rfid_rc522_work");*/
	/*INIT_WORK(&rc522_work, rc522_loop_work); */
	/*init_timer(&beep_timer);  */
	/*beep_timer.function = start_beep;*/
	/*poll_timer.function = poll_time;*/

#if 1
	res = spi_register_driver(&rc522_driver);   
	if(res < 0){
		printk(KERN_DEBUG"spi register %s fail\n", __FUNCTION__);
		return res;
	}
#endif
	return 0;
}

static void RC522_exit(void)
{
	printk(KERN_DEBUG"FUCK YOU\n");
	/*flags = 0;*/

	/*del_timer(&poll_timer);*/
	/*flush_workqueue(rc522_wq);*/
	/*destroy_workqueue(rc522_wq);*/
	/*ifopen = 0;*/
	spi_unregister_driver(&rc522_driver);
	misc_deregister(&rc522_misc_device);
}

module_init(RC522_init);
module_exit(RC522_exit);

MODULE_AUTHOR("Chen Hao");
MODULE_LICENSE("Dual BSD/GPL");	
