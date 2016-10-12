///Includes d'origine
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/device.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>

///Nouveaux includes :
#include <linux/rwsem.h>  ///semaphore lecteur/ecrivain

#define READWRITE_BUFSIZE 16
#define DEFAULT_BUFSIZE 256

MODULE_LICENSE("Dual BSD/GPL");

//----Function prototypes-----
int buf_init(void);
void buf_exit(void);
int buf_open (struct inode *inode, struct file *flip);
int buf_release (struct inode *inode, struct file *flip);
ssize_t buf_read (struct file *flip, char __user *ubuf, size_t count,
                  loff_t *f_ops);
ssize_t buf_write (struct file *flip, const char __user *ubuf, size_t count,
                   loff_t *f_ops);
long buf_ioctl (struct file *flip, unsigned int cmd, unsigned long arg);

//----Data structures-----
struct BufStruct
{
    unsigned int InIdx;
    unsigned int OutIdx;
    unsigned short BufFull;
    unsigned short BufEmpty;
    unsigned int BufSize;
    unsigned short *Buffer;
} Buffer;

struct Buf_Dev
{
    unsigned short *ReadBuf;
    unsigned short *WriteBuf;
    struct semaphore SemBuf;
    unsigned short numWriter;
    unsigned short numReader;
    dev_t dev;  //voir cdev_init   cdev_add
    struct cdev cdev;
    struct class *BufferDev_class;
} BDev;

//Structure à peupler, fonctions accessibles par l'utilisateur
struct file_operations Buf_fops =   
{
    .owner = THIS_MODULE,
    .open = buf_open,
    .release = buf_release,
    .read = buf_read,
    .write = buf_write,
    .unlocked_ioctl = buf_ioctl,
};


//----Functions-----

//Initialisation du pilote
int buf_init(void) {

    unsigned short BufTab[DEFAULT_BUFSIZE]; //Tableau -> Buffer circulaire
    unsigned short ReadBuf[READWRITE_BUFSIZE]; ///Tableau -> Read Buffer
    unsigned short WriteBuf[READWRITE_BUFSIZE]; ///Tableau -> Write Buffer
    int result = 0;
    printk(KERN_ALERT"Buffer_init (%s:%u) => CharPilote is Alive !!\n", __FUNCTION__, __LINE__);

    //Init du Buffer
    Buffer.InIdx = 0;
    Buffer.OutIdx = 0;
    Buffer.BufFull = 0;
    Buffer.BufEmpty = 1;
    Buffer.BufSize = DEFAULT_BUFSIZE;
    Buffer.Buffer = BufTab;

	 //Creer semaphore pour BDev

	 //Init du BDev
	 BDev.ReadBuf = ReadBuf;
	 BDev.WriteBuf = WriteBuf;
	 BDev.numWriter = 0;
    BDev.numReader = 0;

    //Allocation dynamique de l'unité-matériel
    result = alloc_chrdev_region (&BDev.dev, 0, 1, "MyBufferDev" );
    if (result< 0)
        printk(KERN_WARNING"buf_init ERROR IN alloc_chrdev_region (%s:%s:%u)\n", __FILE__, __FUNCTION__, __LINE__);
    else
        printk(KERN_WARNING"buf_init : MAJOR = %u MINOR = %u \n", MAJOR(BDev.dev), MINOR(BDev.dev));

    //Création de notre noeud d'unité-matériel
    BDev.BufferDev_class =  class_create(THIS_MODULE, "MyBufferDev");
    device_create(BDev.BufferDev_class, NULL, BDev.dev, NULL, "MyBufferDev_node");
    cdev_init(&BDev.cdev, &Buf_fops);
    BDev.cdev.owner = THIS_MODULE;
    if(cdev_add(&BDev.cdev, BDev.dev, 1) < 0) 
        printk(KERN_WARNING"BDev ERROR IN cdev_add (%s:%s:%u)\n", __FILE__, __FUNCTION__, __LINE__);

    return 0;
}

///Fermeture du pilote
void buf_exit(void) {
    cdev_del(&BDev.cdev);   //remove cdev from the system
    unregister_chrdev_region(BDev.dev, 1);   //free major number allocation
    device_destroy(BDev.BufferDev_class, BDev.dev);	
    class_destroy(BDev.BufferDev_class);

    printk(KERN_ALERT"Buffer_exit (%s:%u) => CharPilote is dead !!\n", __FUNCTION__, __LINE__);
}

//Accès au pilote par l'usager
int buf_open (struct inode *inode, struct file *flip){
    printk(KERN_WARNING"buf_open (%s:%u)\n", __FUNCTION__, __LINE__);
	return 0;
}

//Libération du pilote par l'usager
int buf_release (struct inode *inode, struct file *flip) {
    printk(KERN_WARNING"buf_release (%s:%u)\n", __FUNCTION__, __LINE__);
	return 0;
}

ssize_t buf_read (struct file *flip, char __user *ubuf, size_t count,
                  loff_t *f_ops){

    printk(KERN_WARNING"buf_read (%s:%u)\n", __FUNCTION__, __LINE__);
	return 0;

}

ssize_t buf_write (struct file *flip, const char __user *ubuf, size_t count,
                   loff_t *f_ops){

    printk(KERN_WARNING"buf_write (%s:%u)\n", __FUNCTION__, __LINE__);
	return 0;
}

long buf_ioctl (struct file *flip, unsigned int cmd, unsigned long arg){

    printk(KERN_WARNING"buf_ioctl (%s:%u)\n", __FUNCTION__, __LINE__);
	return 0;
}

///Point d'entree pilote
module_init(buf_init);
///Point de sortie pilote
module_exit(buf_exit);

int BufIn (struct BufStruct *Buf, unsigned short *Data)
{
    if (Buf->BufFull)
        return -1;
    Buf->BufEmpty = 0;
    Buf->Buffer[Buf->InIdx] = *Data;
    Buf->InIdx = (Buf->InIdx + 1) % Buf->BufSize;
    if (Buf->InIdx == Buf->OutIdx)
        Buf->BufFull = 1;
    return 0;
}

int BufOut (struct BufStruct *Buf, unsigned short *Data)
{
    if (Buf->BufEmpty)
        return -1;
    Buf->BufFull = 0;
    *Data = Buf->Buffer[Buf->OutIdx];
    Buf->OutIdx = (Buf->OutIdx + 1) % Buf->BufSize;
    if (Buf->OutIdx == Buf->InIdx)
        Buf->BufEmpty = 1;
    return 0;
}
