#include "mbus.h"

struct mbus_serial_func ser_func = {
	.chk_dest = ser_chk_dest,
	.qry_parser = ser_query_parser,
	.resp_parser = ser_resp_parser,
	.build_qry = ser_build_query,
	.build_excp = ser_build_resp_excp,
	.build_0102_resp = ser_build_resp_read_status,
	.build_0304_resp = ser_build_resp_read_regs,
	.build_0506_resp = ser_build_resp_set_single,
};

int ser_query_parser(unsigned char *rx_buf, struct frm_para *sfpara)
{
	unsigned char qfc;
	unsigned char rfc;
	unsigned int qstraddr;
	unsigned int rstraddr;
	unsigned int qact;
	unsigned int rlen;
	
	qfc = *(rx_buf+1);
	qstraddr = *(rx_buf+2)<<8 | *(rx_buf+3);
	qact = *(rx_buf+4)<<8 | *(rx_buf+5);
	rfc = sfpara->fc;
	rstraddr = sfpara->straddr;
	rlen = sfpara->len;

	if(rfc != qfc){
		printf("<Modbus Serial Slave> Function code improper");
		printf(" Respond fc = %x | Query fc = %x\n", rfc, qfc);
		return -2;
	}
	
	if(!(rfc ^ FORCESIGLEREGS)){				// FC = 0x05, get the status to write(on/off)
		if(!qact || qact == 0xff<<8){
			sfpara->act = qact;
		}else{
			printf("<Modbus Serial Slave> Query set write status FUCKIN WRONG (fc = 0x05)\n");
			return -3;							
		}
		if(qstraddr != rstraddr){
			printf("<Modbus Serial Slave> Query register address wrong (fc = 0x05)");
			printf(", query addr : %x | resp addr : %x\n", qstraddr, rstraddr);
			return -2;
		} 
	}else if(!(rfc ^ PRESETEXCPSTATUS)){		// FC = 0x06, get the value to write
		if(qstraddr != rstraddr){
			printf("<Modbus Serial Slave> Query register address wrong (fc = 0x06)");
			printf(", query addr : %x | resp addr : %x\n", qstraddr, rstraddr);
			return -2;
		}
		sfpara->act = qact;
	}else{
		// Query addr+shift len must smaller than the contain we set in addr+shift len
		if((qstraddr + qact <= rstraddr + rlen) && (qstraddr >= rstraddr)){	
			sfpara->straddr = qstraddr;
			sfpara->len = qact;
		}else{
			printf("<Modbus Serial Slave> The address have no contain\n");
			return -4;
		}
	 }
	
	return 0;
}
/*
 * Check query Slave ID correct or not
 * If wrong, return -1 then throw away it
 */
int ser_chk_dest(unsigned char *rx_buf, struct frm_para *fpara)
{
	unsigned int qslvID;
	unsigned int rslvID;
	
	qslvID = *(rx_buf);
	rslvID = fpara->slvID; 
	
	if(qslvID != rslvID){
		printf("<Modbus Serial> Slave ID improper, slaveID from query : %d | slaveID from setting : %d\n", qslvID, rslvID);
		return -1;
	}
	
	return 0;
}

int ser_resp_parser(unsigned char *rx_buf, struct frm_para *mfpara, int rlen)
{
//	int i;
	unsigned int act_byte;
	unsigned char qfc;
	unsigned int qact;
	unsigned int qlen;
//	unsigned int raddr;
	unsigned char rfc;
	unsigned int rrlen;
	unsigned int ract;
/*	char *s[EXCPMSGTOTAL] = {
		"<Modbus Serial Master> Read Coil Status (FC=01) exception !!",
		"<Modbus Serial Master> Read Input Status (FC=02) exception !!",
		"<Modbus Serial Master> Read Holding Registers (FC=03) exception !!",
		"<Modbus Serial Master> Read Input Registers (FC=04) exception !!",
		"<Modbus Serial Master> Force Single Coil (FC=05) exception !!",
		"<Modbus Serial Master> Preset Single Register (FC=06) exception !!"
	};
*/	
	qfc = mfpara->fc;
	qlen = mfpara->len;	
	rfc = *(rx_buf+1);
	rrlen = *(rx_buf+2);

	if(qfc ^ rfc){
		if(rfc > PRESETEXCPSTATUS_EXCP || rfc < READCOILSTATUS_EXCP){   
			printf("<Modbus Serial Master> Uknow respond function code : %x !!\n", rfc);
			return -1;
        }	
	//	printf("%s\n", s[rfc-READCOILSTATUS_EXCP]);
		return -1;
	}

	if(!(rfc ^ READCOILSTATUS) || !(rfc ^ READINPUTSTATUS)){	// fc = 0x01/0x02, get data len
		act_byte = carry((int)qlen, 8);

		if(rrlen != act_byte){
			printf("<Modbus Serial Master> length fault !!\n");
			return -1;
		}
/*		printf("<Modbus Serial Master> Data :");
		for(i = 3; i < rlen-2; i++){
			printf(" %x |", *(rx_buf+i));
		}
		printf("\n");
*/
	}else if(!(rfc ^ READHOLDINGREGS) || !(rfc ^ READINPUTREGS)){	//fc = 0x03/0x04, get data byte
		if(rrlen != qlen<<1){
			printf("<Modbus Serial Master> byte fault !!\n");
			return -1;
		}
/*
		printf("<Modbus Serial Master> Data : ");
		for(i = 3; i < rlen-2; i+=2){
			printf(" %x%x", *(rx_buf+i), *(rx_buf+i+1));
			printf("|");
		}
		printf("\n");
*/
	}else if(!(rfc ^ FORCESIGLEREGS)){		//fc = 0x05, get action
		ract = *(rx_buf+4);
//		raddr = *(rx_buf+2)<<8 | *(rx_buf+3);
		if(ract == 255){
//			printf("<Modbus Serial Master> addr : %x The status to wirte on (FC:0x05)\n", raddr);
		}else if(!ract){
//			printf("<Modbus Serial Master> addr : %x The status to wirte off (FC:0x05)\n", raddr);
		}else{
			printf("<Modbus Serial Master> Unknow action !!\n");
			return -1;
		}
	}else if(!(rfc ^ PRESETEXCPSTATUS)){	// fc = 0x06, get action
		qact = mfpara->act;
//		raddr = *(rx_buf+2)<<8 | *(rx_buf+3);
		ract = *(rx_buf+4)<<8 | *(rx_buf+5);
		if(qact != ract){
			printf("<Modbus Serial Master> Action wrong !!\n");
			return -1;
		}
//		printf("<Modbus Serial Master> addr : %x Action code = %x\n", raddr, ract);
	}else{
		printf("<Modbus Serial Master> Unknow respond function code = %x\n", rfc);
		return -1;
	}
	
	return 0;
}
/*
 * @fn		int _build_frame(unsigned char *src, const char *fmt, ...)
 * @brief 	To build modbus serial frame
 * @param	The frame array
 * @param	The data format
 * @param	The variable argument lists of data
 * @return 	The frame's byte
 *
 * To build both of query and respond, 
 * support the data format:
 * 'c': unsgiend char
 * 'S': unsigned short (Big Endian)
 * 's': unsigned short (Little Endian)
 * 'D': means the register data (for now, all is 0)
*/
int _build_frame(unsigned char *src, const char *fmt, ...)
{
	va_list ap;
	unsigned char *pbuf;
	const char *pfmt;
	int byte;
	
	pbuf = src;
	va_start(ap, fmt);

	for(pfmt = fmt, byte = 0; *pfmt != 0; ++pfmt, ++byte){
		switch(*pfmt){
			case 'c':	// unsigend char
				*pbuf++ = va_arg(ap, int);
				break;
			case 'S':	// unsigned short (big endian)
			{
				unsigned short tmp_S = va_arg(ap, int);
				*pbuf++ = tmp_S >> 8;	// data addr Hi
				*pbuf++ = tmp_S & 0xff;		// data addr Lo
				byte++;
			}
				break;
			case 's':	// unsigned short (little endian)
			{
				unsigned short tmp_s = va_arg(ap, int);
				// need to invert
				*pbuf++ = (unsigned char)tmp_s;	// data addr Hi
				*pbuf++ = tmp_s >> 8;			// data addr Lo
				byte++;
			}
				break;
			case 'D':	// the register data value
			{
				int i;
				for(i = 0; i < va_arg(ap, int); i++)
					*pbuf++ = 0;
				byte += (i-1);
			}
				break;
			default:
				printf("%s Failed !!\n", __func__);
				va_end(ap);
				return -1;
		}
	}

	va_end(ap);

	return byte;
}
/*
 * build modbus serial master mode Query
 */
int ser_build_query(unsigned char *tx_buf, struct frm_para *mfpara)
{
	int src_len = 0;
	unsigned char src[FRMLEN];
	unsigned char slvID;
	unsigned short straddr;
	unsigned short act;
	unsigned short len;
	unsigned char fc;
	
	slvID = mfpara->slvID;
	straddr = mfpara->straddr;
	len = mfpara->len;
	act = mfpara->act;
	fc = mfpara->fc;
	
	switch(fc){
		case READCOILSTATUS:	// 0x01
//			printf("<Modbus Serial Master> build Read Coil Status query\n");
			src_len = _build_frame(src, "ccSS", slvID, READCOILSTATUS, straddr, len);
			break;
		case READINPUTSTATUS:	// 0x02
//			printf("<Modbus Serial Master> build Read Input Status query\n");
			src_len = _build_frame(src, "ccSS", slvID, READINPUTSTATUS, straddr, len);
			break;
		case READHOLDINGREGS:	// 0x03
//			printf("<Modbus Serial Master> build Read Holding Register query\n");
			src_len = _build_frame(src, "ccSS", slvID, READHOLDINGREGS, straddr, len);
			break;
		case READINPUTREGS:		// 0x04
//			printf("<Modbus Serial Master> build Read Input Register query\n");
			src_len = _build_frame(src, "ccSS", slvID, READINPUTREGS, straddr, len);
			break;
		case FORCESIGLEREGS:	// 0x05
//			printf("<Modbus Serial Master> build Force Sigle Coil query\n");
			src_len = _build_frame(src, "ccSS", slvID, FORCESIGLEREGS, straddr, act);
			break;
		case PRESETEXCPSTATUS:	// 0x06	
//			printf("<Modbus Serial Master> build Preset Single Register query\n");
			src_len = _build_frame(src, "ccSS", slvID, PRESETEXCPSTATUS, straddr, act);
			break;
		default:
			printf("<Modbus Serial Master> Unknow Function Code\n");
			break;
	}

	build_rtu_frm(tx_buf, src, src_len);
	src_len += 2;
	
	return src_len;
}
/* 
 * build modbus serial respond exception
 */
int ser_build_resp_excp(unsigned char *tx_buf, 
		struct frm_para *sfpara, unsigned char excp_code)
{
	int src_len;
	unsigned char slvID;
	unsigned char fc;
	unsigned char src[FRMLEN];
	
	slvID = (unsigned char)sfpara->slvID;
	fc = sfpara->fc;
//	printf("<Modbus Serial Slave> respond Excption Code\n");
	
	src_len = _build_frame(src, "ccc", slvID, fc | EXCPTIONCODE, excp_code);
	build_rtu_frm(tx_buf, src, src_len);
	src_len += 2;	// CRC 2 byte

	return src_len;
}

/* 
 * FC 0x01 Read Coil Status respond / FC 0x02 Read Input Status
 */
int ser_build_resp_read_status(unsigned char *tx_buf, struct frm_para *sfpara, unsigned char fc)
{
	int byte;
	int src_len;
	unsigned char slvID;
	unsigned char src[FRMLEN];
	
	slvID = (unsigned char)sfpara->slvID;
	byte = carry(sfpara->len, 8);
//	printf("<modbus serial slave> respond read %s status\n", fc==READCOILSTATUS?"coil":"input");
	
	src_len = _build_frame(src, "cccD", slvID, fc, (unsigned char)byte, byte);
	build_rtu_frm(tx_buf, src, src_len);
	src_len += 2;	// CRC 2 byte
		
	return src_len;
} 
/* 
 * FC 0x03 Read Holding Registers respond / FC 0x04 Read Input Registers respond
 */
int ser_build_resp_read_regs(unsigned char *tx_buf, struct frm_para *sfpara, unsigned char fc)
{
	int byte;
	int src_len;
	unsigned char slvID;
	unsigned char src[FRMLEN];

	slvID = (unsigned char)sfpara->slvID;
	byte = sfpara->len * 2;
//	printf("<Modbus Serial Slave> respond Read %s Registers\n", fc==READHOLDINGREGS?"Holding":"Input");

	src_len = _build_frame(src, "cccD", slvID, fc, (unsigned char)byte, byte);	
	build_rtu_frm(tx_buf, src, src_len);

	src_len += 2;	// CRC 2 byte
	
	return src_len;
}
/*
 * FC 0x05 Force Single Coli respond / FC 0x06 Preset Single Register respond
 */
int ser_build_resp_set_single(unsigned char *tx_buf, struct frm_para *sfpara, unsigned char fc)
{
	int src_len;
	unsigned char slvID;
	unsigned short straddr;
	unsigned short act;
	unsigned char src[FRMLEN];
	
	slvID = (unsigned char)sfpara->slvID;
	straddr = (unsigned short)sfpara->straddr;
	act = (unsigned short)sfpara->act;
//	printf("<Modbus Serial Slave> respond %s\n", fc==FORCESIGLEREGS?"Force Single Coli":"Preset Single Register");

	src_len = _build_frame(src, "ccSS", slvID, fc, straddr, act);	
	build_rtu_frm(tx_buf, src, src_len);

	src_len += 2;	// CRC 2 byte
	
	return src_len;
}
