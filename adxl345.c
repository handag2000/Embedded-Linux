#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/i2c.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/fs.h>


#define DEVICE_NAME "/dev/adxl345-0"



#define MAX_BYTES_TO_READ 16

const char DEVID[] = {0x00};
const char ADXL_BW[] = {0x2C};
const char ADXL_INT_ENABLE[] = {0x2E};
const char ADXL_FIFO_MODE[] = {0x38};
const char ADXL_DATA_FORMAT[] = {0x31};
const char ADXL_POWER_CTL[] = {0x2D};

// Déclaration de la structure adxl345_device
struct adxl345_device {
    struct miscdevice misc_dev;
    int read_address;
};
static void writeToRegister(struct i2c_client *client, char reg, char value) {
    char buf[2];
    buf[0] = reg;
    buf[1] = value;
    i2c_master_send(client, buf, 2);
}


static int adxl345_dev_number = 0;

//i2c_multread helper 
static int i2c_multiread(struct i2c_client *client, char device_address,
                         char *data_buffer, int data_count)
{
    int bytes_sent, bytes_received;
    char address_buffer[1] = {device_address};

    // Envoyer un message pour sélectionner une adresse de départ
    bytes_sent = i2c_master_send(client, address_buffer, 1);
    if (bytes_sent < 0)
        printk("Failed to send data to device at address 0x%x\n", device_address);

    // Attendre de recevoir les valeurs demandées
    bytes_received = i2c_master_recv(client, data_buffer, data_count);
    if (bytes_received < 0)
        printk("Failed to receive data from device at address 0x%x\n", device_address);

    return bytes_received;
}


// Fonction de lecture adxl345_read
ssize_t adxl345_read(struct file *filp, char __user *user_buffer ,size_t len, loff_t *offset) {
    struct miscdevice *misc_device;
    struct adxl345_device *adxl_device;
    struct i2c_client *i2c_client;

    int bytes_remaining, bytes_succssefly_read =0; 
    char data_buffer[MAX_BYTES_TO_READ];

    // Retrieve the I2C client and device structure from the file
    misc_device = (struct miscdevice*) filp->private_data;
    adxl_device = container_of(misc_device, struct adxl345_device, misc_dev);
    i2c_client = to_i2c_client(adxl_device->misc_dev.parent);
    
    // Adjust the number of bytes to read and read data from the device
    
    if(len>MAX_BYTES_TO_READ){
        len=MAX_BYTES_TO_READ;
    }
    else {len=len; }
     bytes_succssefly_read = i2c_multiread(i2c_client, adxl_device->read_address, data_buffer, len);

    // copy the data to the user buffer

    bytes_remaining=copy_to_user(user_buffer,data_buffer,bytes_succssefly_read);
    if (bytes_remaining>0){
        printk("Error writing to the user space \n ");
        return -EFAULT;
    }

    return bytes_succssefly_read;
  
}

// Déclaration de la structure de fichiers adxl345_fops
static const struct file_operations adxl345_fops = {
    .owner = THIS_MODULE,
    .read = adxl345_read, 
};




// Fonction de sondage adxl345_probe
static int adxl345_probe(struct i2c_client *client, const struct i2c_device_id *id) {
    
char devid, *name;

    pr_info("adxl345 peripheral device detected \n");
    
    /*int ret;
    unsigned char data = 0;
    ret = i2c_master_send(client, DEVID, 1);
    ret = i2c_master_recv(client, &data, 1);
    pr_info("DEVID data is = <0x%x> \n", data);*/

    // Dynamically allocate memory for an instance of the struct adxl345_device
    struct adxl345_device *adxl345dev;
    adxl345dev = (struct adxl345_device*)kmalloc(sizeof(struct adxl345_device), GFP_KERNEL);
    if (!adxl345dev) {
        pr_err("Memory allocation failed\n");
        return -ENOMEM; // Return error if memory allocation fails
    }

    // Associate this instance with the struct i2c_client
    adxl345dev->misc_dev.parent = &client->dev; // Associate the i2c_client's device field with the adxl345_device instance 
    i2c_set_clientdata(client, adxl345dev); // Store the pointer to the accelerometer(adxl345dev) instance for future use
    adxl345dev->misc_dev.minor = MISC_DYNAMIC_MINOR;
    adxl345dev->misc_dev.fops = &adxl345_fops;

    // Register with the misc framework
    name = kasprintf(GFP_KERNEL, "adxl345-%d", adxl345_dev_number); // Génération du nom unique du périphérique en utilisant kasprintf
    adxl345dev->misc_dev.name = name; // Nom unique du périphérique
    
    
    int rett = misc_register(&adxl345dev->misc_dev);
    if (rett) {
        pr_err("Failed to register misc devicinsmod /mnt/adxl345.koe \n");
        kfree(adxl345dev);
        return rett;
    }
  /*misc_register(&adxl345dev->misc_dev);
    printk("Registered as \"%s\"\n",name);*/
    

    /*// Configuration initiale
    writeToRegister(client, ADXL_BW, 0x0A);
    writeToRegister(client, ADXL_INT_ENABLE, 0x00);
    writeToRegister(client, ADXL_FIFO_MODE, 0x00);
    writeToRegister(client, ADXL_DATA_FORMAT, 0x00);
    writeToRegister(client, ADXL_POWER_CTL, 0x08);
     */
    adxl345_dev_number++;
    return 0;
}



 static int adxl345_remove(struct i2c_client *client){		 		
    	pr_info("ADXL345: Removing device\n");
    	unsigned char data = 0;
    	int ret;
    	writeToRegister(client, ADXL_POWER_CTL, 0x00);
    	ret = i2c_master_send(client, ADXL_POWER_CTL, 1);	//Send a message to get the value in the register
	ret = i2c_master_recv(client, &data, 1);		//Read the value
	pr_info("POWER_CTL config is = <0x%x> \n", data);	//Print the value 

    struct adxl345_device *adxl345dev =i2c_get_clientdata(client);
    // Unregister from the misc framework
    misc_deregister(&adxl345dev->misc_dev);

   // free the memory allocated
   kfree(adxl345dev);
	return 0;
}



 static struct i2c_device_id adxl345_idtable[] = { 
	{ "adxl345", 0 }, 
	{ }
};

MODULE_DEVICE_TABLE(i2c, adxl345_idtable);

#ifdef CONFIG_OF
static const struct of_device_id adxl345_of_match[] = { 
	{ .compatible = "qemu,adxl345", 
	.data = NULL }, 
	{}
};

MODULE_DEVICE_TABLE(of, adxl345_of_match);
#endif

static struct i2c_driver adxl345_driver = {
	.driver = {
		.name = "adxl345", 
		.of_match_table = of_match_ptr(adxl345_of_match),
		},
	.id_table = adxl345_idtable,
	.probe = adxl345_probe,
	.remove = adxl345_remove,
};

module_i2c_driver(adxl345_driver);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("adxl345 driver");
MODULE_AUTHOR("Me");