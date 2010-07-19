/****************************************************************************
**
** COPYRIGHT(C) : Samsung Electronics Co.Ltd, 2006-2010 ALL RIGHTS RESERVED
**
** AUTHOR       : Kim, Geun-Young <geunyoung.kim@samsung.com>			@LDK@
**                                                                      @LDK@
****************************************************************************/

#ifndef __DPRAM_H__
#define __DPRAM_H__


#define DPRAM_SIZE									0x50000

#define DPRAM_START_ADDRESS 						0x00000000
#define DPRAM_MAGIC_CODE_ADDRESS					(DPRAM_START_ADDRESS)								// 0x 0000 0000
#define DPRAM_ACCESS_ENABLE_ADDRESS					(DPRAM_START_ADDRESS + 0x0004)						// 0x 0000 0004
#define RESERVED1										(DPRAM_ACCESS_ENABLE_ADDRESS+4)					// 0x 0000 0008

#define DPRAM_PDA2PHONE_FORMATTED_HEAD_ADDRESS		(RESERVED1 + 8)			// 0x 0000 0010
#define DPRAM_PDA2PHONE_FORMATTED_TAIL_ADDRESS		(DPRAM_PDA2PHONE_FORMATTED_HEAD_ADDRESS + 4)	// 0x 0000 00014
#define DPRAM_PHONE2PDA_FORMATTED_HEAD_ADDRESS		(DPRAM_PDA2PHONE_FORMATTED_TAIL_ADDRESS+4)			// 0x 0000 0018
#define DPRAM_PHONE2PDA_FORMATTED_TAIL_ADDRESS		(DPRAM_PHONE2PDA_FORMATTED_HEAD_ADDRESS + 4)	// 0x 0000 001C


#define DPRAM_PDA2PHONE_RAW_HEAD_ADDRESS			(DPRAM_PHONE2PDA_FORMATTED_TAIL_ADDRESS+4)					// 0x 0000 0020
#define DPRAM_PDA2PHONE_RAW_TAIL_ADDRESS			(DPRAM_PDA2PHONE_RAW_HEAD_ADDRESS + 4)				// 0x 0000 0024
#define DPRAM_PHONE2PDA_RAW_HEAD_ADDRESS			(DPRAM_PDA2PHONE_RAW_TAIL_ADDRESS+ 4)					// 0x 0000 0028
#define DPRAM_PHONE2PDA_RAW_TAIL_ADDRESS			(DPRAM_PHONE2PDA_RAW_HEAD_ADDRESS + 4)		// 0x 0000 002C


#define DPRAM_PDA2PHONE_RFS_HEAD_ADDRESS			(DPRAM_PHONE2PDA_RAW_TAIL_ADDRESS+4)					// 0x 0000 0030
#define DPRAM_PDA2PHONE_RFS_TAIL_ADDRESS			(DPRAM_PDA2PHONE_RFS_HEAD_ADDRESS + 4)				// 0x 0000 0034
#define DPRAM_PHONE2PDA_RFS_HEAD_ADDRESS			(DPRAM_PDA2PHONE_RFS_TAIL_ADDRESS+ 4)					// 0x 0000 0038
#define DPRAM_PHONE2PDA_RFS_TAIL_ADDRESS			(DPRAM_PHONE2PDA_RFS_HEAD_ADDRESS + 4)		// 0x 0000 003C


#define RESERVED2										(DPRAM_PHONE2PDA_RFS_TAIL_ADDRESS + 4)		// 0x 0000 0040
#define DPRAM_FATAL_DISPLAY_ADDRESS					(RESERVED2 + 4032)		// 0x 0000 1000
#define RESERVED3										(DPRAM_FATAL_DISPLAY_ADDRESS + 160)		// 0x 000010A0


#define DPRAM_PDA2PHONE_FORMATTED_BUFFER_ADDRESS	(RESERVED3 + 1036128)	// 0x 000F E000
#define DPRAM_FORMATTED_BUFFER_SIZE		4096    											
#define DPRAM_PHONE2PDA_FORMATTED_BUFFER_ADDRESS	(DPRAM_PDA2PHONE_FORMATTED_BUFFER_ADDRESS + DPRAM_FORMATTED_BUFFER_SIZE)	// 0x 000F F000


#define DPRAM_PDA2PHONE_RAW_BUFFER_ADDRESS			(DPRAM_PHONE2PDA_FORMATTED_BUFFER_ADDRESS + DPRAM_FORMATTED_BUFFER_SIZE)	// 0x 0010 0000
#define DPRAM_RAW_BUFFER_SIZE		1048576    
#define DPRAM_PHONE2PDA_RAW_BUFFER_ADDRESS			(DPRAM_PDA2PHONE_RAW_BUFFER_ADDRESS + DPRAM_RAW_BUFFER_SIZE)		// 0x 0020 0000


#define DPRAM_PDA2PHONE_RFS_BUFFER_ADDRESS			(DPRAM_PHONE2PDA_RAW_BUFFER_ADDRESS + DPRAM_RAW_BUFFER_SIZE)	// 0x 0030 0000
#define DPRAM_RFS_BUFFER_SIZE		1048576 
#define DPRAM_PHONE2PDA_RFS_BUFFER_ADDRESS			(DPRAM_PDA2PHONE_RFS_BUFFER_ADDRESS + DPRAM_RFS_BUFFER_SIZE)		// 0x 0040 0000


#define DPRAM_DGS_INFO_BLOCK_OFFSET        			0xFFF000
#define DPRAM_DGS_INFO_BLOCK_SIZE             		0x100

/* NV packet large data */
#define DPRAM_NV_PACKET_DATA_ADDRESS				(DPRAM_START_ADDRESS + DPRAM_SIZE)

/* default NV data offset */
#define DPRAM_DEFAULT_NV_DATA_OFFSET				(DPRAM_START_ADDRESS + 0XD80000)


#define DPRAM_INTERRUPT_PORT_SIZE					2
#define DPRAM_START_ADDRESS_PHYS 					0x30000000
#define DPRAM_SHARED_BANK							0x05000000

#define DPRAM_SHARED_BANK_SIZE						0x01000000
#define MAX_MODEM_IMG_SIZE							0x00C00000	//10 * 1024 * 1024
#define MAX_DBL_IMG_SIZE							0x00005000		//20 * 1024 
#define MAX_DEFAULT_NV_SIZE							0x00200000	// 2 * 1024 * 1024

#define DPRAM_SFR									0xFFF800
#define	DPRAM_SMP									DPRAM_SFR				//semaphore
#define DPRAM_MBX_AB								DPRAM_SFR + 0x20		//mailbox a -> b
#define DPRAM_MBX_BA								DPRAM_SFR + 0x40		//mailbox b -> a
#define ONEDRAM_CHECK_BA				DPRAM_SFR + 0xC0		//check mailbox b -> a read
//#define DPRAM_CHECK_AB							DPRAM_SFR + 0xA0		//check mailbox a -> b read
//#define DPRAM_CHECK_BA								DPRAM_SFR + 0xC0		//check mailbox b -> a read

#define DPRAM_PDA2PHONE_INTERRUPT_ADDRESS			DPRAM_MBX_BA
#define DPRAM_PHONE2PDA_INTERRUPT_ADDRESS			DPRAM_MBX_AB

#define PARTITION_ID_MODEM_IMG			0x06
#define TRUE	1
#define FALSE	0

/*
 * interrupt masks.
 */
#define INT_MASK_VALID					0x0080
#define INT_MASK_COMMAND				0x0040

#define INT_MASK_REQ_ACK_RFS				0x0400
#define INT_MASK_RES_ACK_RFS			0x0200
#define INT_MASK_SEND_RFS					0x0100
#define INT_MASK_REQ_ACK_F				0x0020
#define INT_MASK_REQ_ACK_R				0x0010
#define INT_MASK_RES_ACK_F				0x0008
#define INT_MASK_RES_ACK_R				0x0004
#define INT_MASK_SEND_F					0x0002
#define INT_MASK_SEND_R					0x0001


#define INT_MASK_CMD_INIT_START			0x0001
#define INT_MASK_CMD_INIT_END			0x0002
#define INT_MASK_CMD_REQ_ACTIVE			0x0003
#define INT_MASK_CMD_RES_ACTIVE			0x0004
#define INT_MASK_CMD_REQ_TIME_SYNC		0x0005
#define INT_MASK_CMD_POWER_OFF			0x0006
#define INT_MASK_CMD_RESET		0x0007
#define INT_MASK_CMD_PHONE_START 		0x0008
#define INT_MASK_CMD_ERR_DISPLAY		0x0009
#define INT_MASK_CMD_PHONE_DEEP_SLEEP	0x000A
#define INT_MASK_CMD_NV_REBUILDING		0x000B
#define INT_MASK_CMD_EMER_DOWN			0x000C
#define INT_MASK_CMD_SMP_REQ			0x000D
#define INT_MASK_CMD_SMP_REP			0x000E

#define INT_MASK_CP_ONLINE_BOOT			0x0000
#define INT_MASK_CP_AIRPLANE_BOOT		0x1000

#define INT_MASK_CP_AP_ANDROID			0x0100
#define INT_MASK_CP_AP_WINMOBILE		0x0200
#define INT_MASK_CP_AP_LINUX			0x0300
#define INT_MASK_CP_AP_SYMBIAN			0x0400

#define INT_MASK_CP_QUALCOMM			0x0100
#define INT_MASK_CP_INFINEON			0x0200
#define INT_MASK_CP_BROADCOM			0x0300

#define INT_COMMAND(x)					(INT_MASK_VALID | INT_MASK_COMMAND | x)
#define INT_NON_COMMAND(x)				(INT_MASK_VALID | x)

#define FORMATTED_INDEX					0
#define RAW_INDEX						1
#define RFS_INDEX						2
#define MAX_INDEX						3

/* ioctl command definitions. */
#define IOC_MZ_MAGIC					('o')
#define DPRAM_PHONE_POWON				_IO(IOC_MZ_MAGIC, 0xd0)
#define DPRAM_PHONEIMG_LOAD				_IO(IOC_MZ_MAGIC, 0xd1)
#define DPRAM_NVDATA_LOAD				_IO(IOC_MZ_MAGIC, 0xd2)
#define DPRAM_PHONE_BOOTSTART			_IO(IOC_MZ_MAGIC, 0xd3)

struct _param_nv {
	unsigned char *addr;
	unsigned int size;
};


#define IOC_SEC_MAGIC            (0xf0)
#define DPRAM_PHONE_ON           _IO(IOC_SEC_MAGIC, 0xc0)
#define DPRAM_PHONE_OFF          _IO(IOC_SEC_MAGIC, 0xc1)
#define DPRAM_PHONE_GETSTATUS    _IOR(IOC_SEC_MAGIC, 0xc2, unsigned int)
#define DPRAM_PHONE_MDUMP        _IO(IOC_SEC_MAGIC, 0xc3)
#define DPRAM_PHONE_BATTERY      _IO(IOC_SEC_MAGIC, 0xc4)
#define DPRAM_PHONE_RESET        _IO(IOC_SEC_MAGIC, 0xc5)
#define DPRAM_PHONE_RAMDUMP_ON    _IO(IOC_SEC_MAGIC, 0xc6)
#define DPRAM_PHONE_RAMDUMP_OFF   _IO(IOC_SEC_MAGIC, 0xc7)
#define DPRAM_GET_DGS_INFO       _IOR(IOC_SEC_MAGIC, 0xc8, unsigned char [DPRAM_DGS_INFO_BLOCK_SIZE])
#define DPRAM_NVPACKET_DATAREAD		_IOR(IOC_SEC_MAGIC, 0xc9, unsigned int)
#define DPRAM_PHONE_UNSET_UPLOAD          _IO(IOC_SEC_MAGIC, 0xca)
#define DPRAM_PHONE_SET_AUTOTEST          _IO(IOC_SEC_MAGIC, 0xcb)
#define DPRAM_PHONE_SET_DEBUGLEVEL          _IO(IOC_SEC_MAGIC, 0xcc)
#define DPRAM_PHONE_GET_DEBUGLEVEL          _IO(IOC_SEC_MAGIC, 0xcd)

/*
 * structure definitions.
 */
typedef struct dpram_serial {
	/* pointer to the tty for this device */
	struct tty_struct *tty;

	/* number of times this port has been opened */
	int open_count;

	/* locks this structure */
	struct semaphore sem;
} dpram_serial_t;

typedef struct dpram_device {
	/* DPRAM memory addresses */
	unsigned long in_head_addr;
	unsigned long in_tail_addr;
	unsigned long in_buff_addr;
	unsigned long in_buff_size;

	unsigned long out_head_addr;
	unsigned long out_tail_addr;
	unsigned long out_buff_addr;
	unsigned long out_buff_size;

	unsigned int out_head_saved;
	unsigned int out_tail_saved;

	u_int16_t mask_req_ack;
	u_int16_t mask_res_ack;
	u_int16_t mask_send;

	dpram_serial_t serial;
} dpram_device_t;

typedef struct dpram_tasklet_data {
	dpram_device_t *device;
	u_int16_t non_cmd;
} dpram_tasklet_data_t;

#ifdef CONFIG_EVENT_LOGGING
typedef struct event_header {
	struct timeval timeVal;
	__u16 class;
	__u16 repeat_count;
	__s32 payload_length;
} EVENT_HEADER;
#endif

struct _mem_param {
	unsigned short addr;
	unsigned long data;
	int dir;
};

typedef enum {
	FIRST = 1,
	SECOND = 2,
	THIRD = 3,
} dump_order;

/* TODO: add more definitions */

#endif	/* __DPRAM_H__ */

