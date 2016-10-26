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
#include <linux/sched.h> // Required for task states (TASK_INTERRUPTIBLE etc )
#include <linux/ioctl.h> // Used for ioctl command

#define READWRITE_BUFSIZE 16
#define DEFAULT_BUFSIZE 256

/* attribution des numero de commande ioctl */
#define CHARDRIVER_IOC_MAGIC 'j'

#define CHARDRIVER_IOC_GETNUMDATA _IOR(CHARDRIVER_IOC_MAGIC,0,unsigned short)
#define CHARDRIVER_IOC_GETNUMREADER _IOR(CHARDRIVER_IOC_MAGIC,1,unsigned short)
#define CHARDRIVER_IOC_GETBUFSIZE _IOR(CHARDRIVER_IOC_MAGIC,2,unsigned short)
#define CHARDRIVER_IOC_SETBUFSIZE _IOW(CHARDRIVER_IOC_MAGIC,3,unsigned short)

#define CHARDRIVER_IOC_MAXNR 3


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
    unsigned short BufSize;
    unsigned short *Buffer;
} Buffer;

struct Buf_Dev
{
    wait_queue_head_t inq, outq;
    unsigned short *ReadBuf;
    unsigned short *WriteBuf;
    struct rw_semaphore rw_semBuf;
    unsigned short numWriter;
    unsigned short numReader;
    unsigned short numData;
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
    unsigned short ReadBuf[READWRITE_BUFSIZE]; //Tableau -> Read Buffer
    unsigned short WriteBuf[READWRITE_BUFSIZE]; //Tableau -> Write Buffer
    int result = 0;
    printk(KERN_ALERT"Buffer_init (%s:%u) => charDriverDev is Alive !!\n", __FUNCTION__, __LINE__);

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
    BDev.numData = 0;

    //Init des files d'attentes
    init_waitqueue_head (&BDev.inq);
    init_waitqueue_head (&BDev.outq);

    //Allocation dynamique de l'unité-matériel
    result = alloc_chrdev_region (&BDev.dev, 0, 1, "charDriverDev" );
    if (result< 0)
        printk(KERN_WARNING"buf_init ERROR IN alloc_chrdev_region (%s:%s:%u)\n", __FILE__, __FUNCTION__, __LINE__);
    else
        printk(KERN_WARNING"buf_init : MAJOR = %u MINOR = %u \n", MAJOR(BDev.dev), MINOR(BDev.dev));

    //Création de notre noeud d'unité-matériel
    BDev.BufferDev_class =  class_create(THIS_MODULE, "charDriverDev");
    device_create(BDev.BufferDev_class, NULL, BDev.dev, NULL, "charDriverDev_node");
    cdev_init(&BDev.cdev, &Buf_fops);
    BDev.cdev.owner = THIS_MODULE;
    if(cdev_add(&BDev.cdev, BDev.dev, 1) < 0)
        printk(KERN_WARNING"BDev ERROR IN cdev_add (%s:%s:%u)\n", __FILE__, __FUNCTION__, __LINE__);

    return 0;
}

//Fermeture du pilote
void buf_exit(void) {
    kfree(Buffer.Buffer); //Free cicular buffer memory
    cdev_del(&BDev.cdev);   //remove cdev from the system
    unregister_chrdev_region(BDev.dev, 1);   //free major number allocation
    device_destroy(BDev.BufferDev_class, BDev.dev);
    class_destroy(BDev.BufferDev_class);

    printk(KERN_ALERT"Buffer_exit (%s:%u) => charDriverDev is dead !!\n", __FUNCTION__, __LINE__);
}

//Accès au pilote par l'usager
int buf_open (struct inode *inode, struct file *filp){


	struct Buf_Dev *dev = NULL;
    dev = container_of (inode->i_cdev, struct Buf_Dev, cdev);
	 /* Container_of recupere un pointeur vers la struc Buf_dev qui contient cdev , "This macro takes a pointer to a field of type container_field,
    within a structure of type container_type, and returns a pointer to the containing structure" */
    filp->private_data = dev; //pointe vers la structure perso
    printk(KERN_ALERT"Buffer_open (%s:%u) \n => start open \n", __FUNCTION__, __LINE__);
	if (dev==NULL){
        printk(KERN_ALERT"Buffer_open (%s:%u) \n => Error : Buf_Dev dev = NULL \n", __FUNCTION__, __LINE__);
	}

    //Ouverture pour ecriture ou lecture/ecriture
    if ( ((filp->f_flags & O_ACCMODE) == O_WRONLY) || ((filp->f_flags & O_ACCMODE) == O_RDWR) ){

        if (dev->numWriter!=0){
			printk(KERN_ALERT"Buffer_open (%s:%u) \n => numWriter!=0 -> ENOTTY \n", __FUNCTION__, __LINE__);
			return ENOTTY;
        }
		down_write (&dev->rw_semBuf); //Capture le verrou d'ecriture
		printk(KERN_ALERT"Buffer_open (%s:%u) \n => Capture le verrou d'ecriture \n", __FUNCTION__, __LINE__);
		dev->numWriter++;
		up_write (&dev->rw_semBuf); //Relache le verrou d'ecriture

		if((filp->f_flags & O_ACCMODE) == O_RDWR)
            printk(KERN_WARNING"buf_open in read/write mode (%s:%u)\n", __FUNCTION__, __LINE__);
		if ((filp->f_flags & O_ACCMODE) == O_WRONLY)
            printk(KERN_WARNING"buf_open in write only mode (%s:%u)\n", __FUNCTION__, __LINE__);
    }

    //Ouveture en lecture seule
    else if ( (filp->f_flags & O_ACCMODE) == O_RDONLY){
        down_read (&dev->rw_semBuf); //Captmode (%s:%u)\n", __FUNCTION__, __LINE__);
        printk(KERN_WARNING"buf_open success (%s:%u)\n", __FUNCTION__, __LINE__); //capture un verrou de lecture
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
	    printk(KERN_ALERT"Buffer_release (%s:%u) \n => Capture le verrou de lecture (FMODE_READ)  \n", __FUNCTION__, __LINE__);
		(dev->numReader)--;
	    up_read (&dev->rw_semBuf);
	 }
	 if (filp->f_mode & FMODE_WRITE){
	 	down_write (&dev->rw_semBuf);
	    printk(KERN_ALERT"Buffer_release (%s:%u) \n => Capture le verrou d'ecriture (FMODE_READ))  \n", __FUNCTION__, __LINE__);
        (dev->numWriter)--;
		up_write (&dev->rw_semBuf);
	 }

	 printk(KERN_WARNING"buf_release (%s:%u)\n", __FUNCTION__, __LINE__);
	 return 0;
}

ssize_t buf_read (struct file *filp, char __user *ubuf, size_t count,
                  loff_t *f_ops){
     int nbRead = 0, i=0;
	 unsigned short readCh = 0; //caractere lu dans le buffer
	 struct Buf_Dev *dev = filp->private_data;

     printk(KERN_ALERT"buf_read : start (%s:%u) \n", __FUNCTION__, __LINE__);

     ///Début région critique
	 down_read (&dev->rw_semBuf);

     //Boucle : tant que le buffer circulaire est vide
     while(Buffer.BufEmpty){
        up_read (&dev->rw_semBuf); //libère le sémaphore avant de dormir
        printk(KERN_ALERT"buf_read : read sleep (%s:%u) \n", __FUNCTION__, __LINE__);
        if(filp->f_flags & O_NONBLOCK){
            return -EAGAIN;
        }
        if(wait_event_interruptible (dev->outq,Buffer.BufEmpty)){  //endort la tache
            return -ERESTARTSYS;
        }
        down_read (&dev->rw_semBuf);
     }

     printk(KERN_ALERT"buf_read : reading data from Buffer... (%s:%u) \n", __FUNCTION__, __LINE__);
	 for(i=0; i<count; i++){
		 if (BufOut(&Buffer, &readCh)){
			 break;
		 }
		 dev->ReadBuf[i] = readCh;  //le caractere lu est stocké dans le buffer temporaire
		 dev->numData--;
		 nbRead++;
	 }
	 printk(KERN_ALERT"buf_read : function reads %d caracter(s)(%s:%u) \n",nbRead, __FUNCTION__, __LINE__);

     printk(KERN_ALERT"buf_read : data copy to user... (%s:%u) \n", __FUNCTION__, __LINE__);
	 //Tentative d'envoie des caractères lu à l'utilisateur
	 if(copy_to_user(ubuf, &dev->ReadBuf, nbRead)){
        //Tous les bits n'ont pas été echangé
        up_read (&dev->rw_semBuf); //libère le sémaphore avant de renvoyer l'erreur
        return -EFAULT;
	 }
    ///Fin région critique
	 up_read (&dev->rw_semBuf);
	 printk(KERN_ALERT"buf_read : END (%s:%u) \n", __FUNCTION__, __LINE__);

	 return nbRead;

}

//
//
// return : nbWrite = nombre de caractere ecrit dans le buffer circulaire
ssize_t buf_write (struct file *filp, const char __user *ubuf, size_t count,
                   loff_t *f_ops){
     unsigned short userCh = 0;
     int nbWrite = 0, i = 0;
     struct Buf_Dev *dev = filp->private_data;

	 printk(KERN_WARNING"buf_write start(%s:%u)\n", __FUNCTION__, __LINE__);

     //Début région critique
     down_write (&dev->rw_semBuf);

     //Boucle : tant que le buffer d'ecriture est plein
     while(Buffer.BufFull){
        up_write (&dev->rw_semBuf); //libère le sémaphore avant de dormir
        printk(KERN_WARNING"buf_write sleep(%s:%u)\n", __FUNCTION__, __LINE__);
        if(filp->f_flags & O_NONBLOCK){
            return -EAGAIN;
        }
        if (wait_event_interruptible (dev->inq,Buffer.BufFull)){ //endort la tâche
            return -ERESTARTSYS;
        }
        down_write (&dev->rw_semBuf);
    }

     printk(KERN_ALERT"buf_write : data copy from user... (%s:%u) \n", __FUNCTION__, __LINE__);
     //Tentative de récupération des caractères envoyé par l'utilisateur
	 if(copy_from_user(&dev->WriteBuf, ubuf, count)){
        //Tous les bits n'ont pas été echangé
        up_write (&dev->rw_semBuf); //libère le sémaphore avant de renvoyer l'erreur
        return -EFAULT;
	 }

     printk(KERN_ALERT"buf_write : writing data to Buffer... (%s:%u) \n", __FUNCTION__, __LINE__);
     //Copie du buffer temporaire WriteBuf vers le buffer circulaire Buffer
	 for(i = 0; i<count; i++){
        userCh = dev->WriteBuf[i];
        if(BufIn(&Buffer, &userCh)){
            break;
        }
        nbWrite++;
        dev->numData++;
	 }
	 printk(KERN_ALERT"buf_write : function wrotes %d caracter(s)(%s:%u) \n",nbWrite, __FUNCTION__, __LINE__);

	 //Fin de région critique
	 up_write (&dev->rw_semBuf);

	 printk(KERN_WARNING"buf_write END(%s:%u)\n", __FUNCTION__, __LINE__);
	 return nbWrite;
}

long buf_ioctl (struct file *filp, unsigned int cmd, unsigned long arg){

    int err = 0, i = 0;
    unsigned short retval = 0, tmp;
    struct Buf_Dev *dev = filp->private_data;
    unsigned short *newBuf;

    /* verification que la commande corespond à notre pilote */
    if(_IOC_TYPE(cmd) != CHARDRIVER_IOC_MAGIC) return -ENOTTY;
    if(_IOC_TYPE(cmd) > CHARDRIVER_IOC_MAXNR) return -ENOTTY;

    /* verification de la possibilité de lire ou ecrire */
    if(_IOC_DIR(cmd) & _IOC_READ){
        err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
    }
    else if(_IOC_DIR(cmd) & _IOC_READ){
        err = !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
    }
    if(err) return -EFAULT;

    switch(cmd){
        case CHARDRIVER_IOC_GETNUMDATA :   retval = __put_user(dev->numData,(int __user*)arg);
                                           break;
        case CHARDRIVER_IOC_GETNUMREADER : retval = __put_user(dev->numReader,(int __user *)arg);
                                           break;
        case CHARDRIVER_IOC_GETBUFSIZE :   retval = __put_user(Buffer.BufSize, (int __user *)arg);
                                           break;
        case CHARDRIVER_IOC_SETBUFSIZE :   if(!capable(CAP_SYS_ADMIN)){
                                                return -EPERM;
                                           }
                                           retval = __get_user(tmp, (int __user *)arg);

                                           ///Debut région critique
                                           down_write(&dev->rw_semBuf);
                                           //Vérifie que la taille du nouveau Buffer puisse contenir toute les données présentes
                                           if(tmp<(dev->numData)){
                                                up_write(&dev->rw_semBuf); //libère le sémaphore avant de retourner un code d'erreur
                                                return -EADV;
                                           }

                                           //allocation d'une nouvelle zone mémoire
                                           newBuf = kmalloc(tmp, GFP_KERNEL);
                                           //remplis la nouvelle zone mémoire avec les données de notre buffer
                                           for (i=0;i<(dev->numData);i++){
                                            newBuf[i] = Buffer.Buffer[i];
                                           }
                                           //libère l'ancienne zone mémoire de notre buffer
                                           kfree(Buffer.Buffer);
                                           //fait pointer notre buffer vers la nouvelle zone mémoire
                                           Buffer.Buffer = newBuf;
                                           Buffer.BufSize = tmp;

                                           ///Fin région critique
                                           up_write(&dev->rw_semBuf);
                                           break;
        default : return -ENOTTY;
    }

    printk(KERN_WARNING"buf_ioctl (%s:%u)\n", __FUNCTION__, __LINE__);
	return retval;
}

///Point d'entree pilote
module_init(buf_init);
///Point de sortie pilote
module_exit(buf_exit);


