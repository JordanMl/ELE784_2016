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
int buf_open (struct inode *inode, struct file *filp);
int buf_release (struct inode *inode, struct file *filp);
ssize_t buf_read (struct file *filp, char __user *ubuf, size_t count,
                  loff_t *f_ops);
ssize_t buf_write (struct file *filp, const char __user *ubuf, size_t count,
                   loff_t *f_ops);
long buf_ioctl (struct file *filp, unsigned int cmd, unsigned long arg);

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
    struct rw_semaphore rw_semBuf;
    unsigned short numWriter;
    unsigned short numReader;
    dev_t dev;  							/* Device number */
    struct cdev cdev;  					/* Char device structure */
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


//Initialisation du pilote
int buf_init(void) {

    //unsigned short BufTab[DEFAULT_BUFSIZE]; //Tableau -> Buffer circulaire
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
    Buffer.Buffer = kmalloc(Buffer.BufSize, GFP_KERNEL); //dynamic allocation to create the circular buffer

    //Init rw semaphore pour BDev
    init_rwsem (&BDev.rw_semBuf);

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
    kfree(Buffer.Buffer); //Free cicular buffer memory
	 cdev_del(&BDev.cdev);   //remove cdev from the system
    unregister_chrdev_region(BDev.dev, 1);   //free major number allocation
    device_destroy(BDev.BufferDev_class, BDev.dev);
    class_destroy(BDev.BufferDev_class);

    printk(KERN_ALERT"Buffer_exit (%s:%u) => CharPilote is dead !!\n", __FUNCTION__, __LINE__);
}

//Accès au pilote par l'usager
int buf_open (struct inode *inode, struct file *filp){

    
	 struct Buf_Dev *dev; 
    dev = container_of (inode->i_cdev, struct Buf_Dev, cdev); 
	 /*Container_of recupere un pointeur vers la struc Buf_dev qui contient cdev , "This macro takes a pointer to a field of type container_field,
    within a structure of type container_type, and returns a pointer to the containing structure" */
    filp->private_data = dev; //pointe vers la structure perso
    
    //Ouverture pour ecriture ou lecture/ecriture
    if ( ((filp->f_flags & O_ACCMODE) == O_WRONLY) || ((filp->f_flags & O_ACCMODE) == O_RDWR) ){

        if (dev->numWriter!=0){ 
			return ENOTTY;
		  }
		  down_write (&dev->rw_semBuf); //Capture le verrou d'ecriture
		  dev->numWriter++;
		  up_write (&dev->rw_semBuf); //Relache le verrou d'ecriture
			
		  if((filp->f_flags & O_ACCMODE) == O_RDWR)
		  		printk(KERN_WARNING"buf_open in read/write mode (%s:%u)\n", __FUNCTION__, __LINE__);
		  if ((filp->f_flags & O_ACCMODE) == O_WRONLY)
				printk(KERN_WARNING"buf_open in write only mode (%s:%u)\n", __FUNCTION__, __LINE__);
    }
    
    //Ouveture en lecture seule
    else if ( (filp->f_flags & O_ACCMODE) == O_RDONLY){
		  down_read (&dev->rw_semBuf); //Capture un verrou de lecture
        dev->numReader++;
		  up_read (&dev->rw_semBuf); //Relache le verrou de lecture			
		  printk(KERN_WARNING"buf_open in read only mode (%s:%u)\n", __FUNCTION__, __LINE__);
    }

    printk(KERN_WARNING"buf_open success (%s:%u)\n", __FUNCTION__, __LINE__);
	 return 0;
}

//Libération du pilote par l'usager
int buf_release (struct inode *inode, struct file *filp) {

	 struct Buf_Dev *dev = filp->private_data; 

	 if (filp->f_mode & FMODE_READ){
	 	 down_read (&dev->rw_semBuf); 
		 dev->numReader--;
	    up_read (&dev->rw_semBuf); 
	 }
	 if (filp->f_mode & FMODE_READ){
	 	 down_write (&dev->rw_semBuf); 
		 dev->numWriter--;
		 up_write (&dev->rw_semBuf);
	 }
	
	 printk(KERN_WARNING"buf_release (%s:%u)\n", __FUNCTION__, __LINE__);	
	 return 0;
}

ssize_t buf_read (struct file *filp, char __user *ubuf, size_t count,
                  loff_t *f_ops){

	 int i=0;
	 unsigned short readCh = 0; //caractere lu dans le buffer
	 struct Buf_Dev *dev = filp->private_data; 
	 
	 down_read (&dev->rw_semBuf);	 		
	 for(i=0; i<count; i++){
		 if (BufOut(&Buffer, &readCh)){
			 break;			
		 }
		 dev->ReadBuf[i] = readCh;  //le caractere lu est stocké dans le buffer temporaire	
	 }
	 copy_to_user(ubuf, &dev->ReadBuf, i); //copie des i caractères lus vers l'appli user
	 up_read (&dev->rw_semBuf);	 

	 printk(KERN_WARNING"buf_read : %c (%s:%u)\n", (char)readCh,  __FUNCTION__, __LINE__);
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


