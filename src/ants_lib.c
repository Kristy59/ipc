
#include "ants_lib.h"
#include "AntsServerSDK.h"
#include "AntsMidLayerSDK.h"
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "app_debug.h"
#include "ifconf.h"
#include "generic.h"
#include "media_buf.h"
//#include "sdk/sdk_api.h"
#include "sdk/sdk_enc.h"
#include "sysconf.h"


typedef struct ANTS_LIB_SERVER
{
	char server_eth[16];
	in_port_t server_port;
	
	pthread_t streamer_main_tid;
	pthread_t streamer_sub_tid;
	bool streamer_trigger;
}ANTS_LIB_SERVER_t;
static ANTS_LIB_SERVER_t _ants_server =
{
	.streamer_main_tid = (pthread_t)NULL,
	.streamer_sub_tid = (pthread_t)NULL,
	.streamer_trigger = false,
};

typedef const char* const MAPTBL_t;

static const char* unistruct_map(int val, const char** map, ssize_t map_size)
{
	if(val < map_size){
		return map[val];
	}
	return SYS_NULL;
}


static MAPTBL_t ants_type_map[] = {
	[ANTS_MID_CFG] = "ANTS_MID_CFG",
	[ANTS_MID_GET_DEVICECFG] = "ANTS_MID_GET_DEVICECFG",						//!��ȡ�豸���� +
	[ANTS_MID_SET_DEVICECFG] = "ANTS_MID_SET_DEVICECFG",						//!�����豸����
	
	[ANTS_MID_GET_NETCFG] = "ANTS_MID_GET_NETCFG",							//!��ȡ������� +
	[ANTS_MID_SET_NETCFG] = "ANTS_MID_SET_NETCFG",							//!�����������
	
	[ANTS_MID_GET_PICCFG] = "ANTS_MID_GET_PICCFG",							//!��ȡͼ����� +
	[ANTS_MID_SET_PICCFG] = "ANTS_MID_SET_PICCFG",							//!����ͼ�����
	
	[ANTS_MID_GET_PICCFG_V2] = "ANTS_MID_GET_PICCFG_V2",						//!��ȡͼ�����(64·��չ) +
	[ANTS_MID_SET_PICCFG_V2] = "ANTS_MID_SET_PICCFG_V2",						//!����ͼ�����(64·��չ)
	
	[ANTS_MID_GET_COMPRESSCFG] = "ANTS_MID_GET_COMPRESSCFG",					//!��ȡѹ������ +
	[ANTS_MID_SET_COMPRESSCFG] = "ANTS_MID_SET_COMPRESSCFG",					//!����ѹ������
	
	[ANTS_MID_GET_RECORDCFG] = "ANTS_MID_GET_RECORDCFG",						//!��ȡ¼��ʱ����� +
	[ANTS_MID_SET_RECORDCFG] = "ANTS_MID_SET_RECORDCFG",						//!����¼��ʱ�����
	
	[ANTS_MID_GET_DECODERCFG] = "ANTS_MID_GET_DECODERCFG",					//!��ȡ���������� +
	[ANTS_MID_SET_DECODERCFG] = "ANTS_MID_SET_DECODERCFG",					//!���ý���������
	
	[ANTS_MID_GET_RS232CFG] = "ANTS_MID_GET_RS232CFG",						//!��ȡ232���ڲ��� +
	[ANTS_MID_SET_RS232CFG] = "ANTS_MID_SET_RS232CFG",						//!����232���ڲ���
	
	[ANTS_MID_GET_ALARMINCFG] = "ANTS_MID_GET_ALARMINCFG",					//!��ȡ����������� +
	[ANTS_MID_SET_ALARMINCFG] = "ANTS_MID_SET_ALARMINCFG",					//!���ñ����������
	
	[ANTS_MID_GET_ALARMINCFG_V2] = "ANTS_MID_GET_ALARMINCFG_V2",				//!��ȡ�����������(64·��չ) +
	[ANTS_MID_SET_ALARMINCFG_V2] = "ANTS_MID_SET_ALARMINCFG_V2",				//!���ñ����������(64·��չ)
	
	[ANTS_MID_GET_ALARMOUTCFG] = "ANTS_MID_GET_ALARMOUTCFG",					//!��ȡ����������� +
	[ANTS_MID_SET_ALARMOUTCFG] = "ANTS_MID_SET_ALARMOUTCFG",					//!���ñ����������
	
	[ANTS_MID_GET_TIMECFG] = "ANTS_MID_GET_TIMECFG",							//!��ȡDVRʱ�� +
	[ANTS_MID_SET_TIMECFG] = "ANTS_MID_SET_TIMECFG",							//!����DVRʱ��
	
	[ANTS_MID_GET_USERCFG] = "ANTS_MID_GET_USERCFG",							//!��ȡ�û����� +
	[ANTS_MID_SET_USERCFG] = "ANTS_MID_SET_USERCFG",							//!�����û�����
	
	[ANTS_MID_GET_USERCFG_V2] = "ANTS_MID_GET_USERCFG_V2",					//!��ȡ�û�����(64·��չ) +
	[ANTS_MID_SET_USERCFG_V2] = "ANTS_MID_SET_USERCFG_V2",					//!�����û�����(64·��չ)
	
	[ANTS_MID_GET_EXCEPTIONCFG] = "ANTS_MID_GET_EXCEPTIONCFG",				//!��ȡ�쳣���� +
	[ANTS_MID_SET_EXCEPTIONCFG] = "ANTS_MID_SET_EXCEPTIONCFG",				//!�����쳣����
	
	[ANTS_MID_GET_ZONEANDDST] = "ANTS_MID_GET_ZONEANDDST",					//!��ȡʱ������ʱ�Ʋ��� +
	[ANTS_MID_SET_ZONEANDDST] = "ANTS_MID_SET_ZONEANDDST",					//!����ʱ������ʱ�Ʋ���
	
	[ANTS_MID_GET_SHOWSTRING] = "ANTS_MID_GET_SHOWSTRING",					//!��ȡ�����ַ����� +
	[ANTS_MID_SET_SHOWSTRING] = "ANTS_MID_SET_SHOWSTRING",					//!���õ����ַ�����
	
	[ANTS_MID_GET_EVENTCOMPCFG] = "ANTS_MID_GET_EVENTCOMPCFG",				//!��ȡ�¼�����¼����� +
	[ANTS_MID_SET_EVENTCOMPCFG] = "ANTS_MID_SET_EVENTCOMPCFG",				//!�����¼�����¼�����
	
	[ANTS_MID_GET_AUTOREBOOT] = "ANTS_MID_GET_AUTOREBOOT",					//!��ȡ�Զ�ά������ +
	[ANTS_MID_SET_AUTOREBOOT] = "ANTS_MID_SET_AUTOREBOOT",					//!�����Զ�ά������
	
	[ANTS_MID_GET_NETAPPCFG] = "ANTS_MID_GET_NETAPPCFG",						//!��ȡ����Ӧ�ò��� NTP/DDNS/EMAIL +
	[ANTS_MID_SET_NETAPPCFG] = "ANTS_MID_SET_NETAPPCFG",						//!��������Ӧ�ò��� NTP/DDNS/EMAIL
	
	[ANTS_MID_GET_NTPCFG] = "ANTS_MID_GET_NTPCFG",							//!��ȡ����Ӧ�ò��� NTP +
	[ANTS_MID_SET_NTPCFG] = "ANTS_MID_SET_NTPCFG",							//!��������Ӧ�ò��� NTP
	
	[ANTS_MID_GET_DDNSCFG] = "ANTS_MID_GET_DDNSCFG",							//!��ȡ����Ӧ�ò��� DDNS +
	[ANTS_MID_SET_DDNSCFG] = "ANTS_MID_SET_DDNSCFG",							//!��������Ӧ�ò��� DDNS
	
	[ANTS_MID_GET_EMAILCFG] = "ANTS_MID_GET_EMAILCFG",						//!��ȡ����Ӧ�ò��� EMAIL +
	[ANTS_MID_SET_EMAILCFG] = "ANTS_MID_SET_EMAILCFG",						//!��������Ӧ�ò��� EMAIL
	
	[ANTS_MID_GET_HDCFG] = "ANTS_MID_GET_HDCFG",								//!��ȡӲ�̹������ò��� +
	[ANTS_MID_SET_HDCFG] = "ANTS_MID_SET_HDCFG",								//!����Ӳ�̹������ò���
	
	[ANTS_MID_GET_HDGROUP_CFG] = "ANTS_MID_GET_HDGROUP_CFG",					//!��ȡ����������ò��� +
	[ANTS_MID_SET_HDGROUP_CFG] = "ANTS_MID_SET_HDGROUP_CFG",					//!��������������ò���
	
	[ANTS_MID_GET_HDGROUP_CFG_V2] = "ANTS_MID_GET_HDGROUP_CFG_V2",			//!��ȡ����������ò���(64·��չ)+
	[ANTS_MID_SET_HDGROUP_CFG_V2] = "ANTS_MID_SET_HDGROUP_CFG_V2",			//!��������������ò���(64·��չ)
	
	[ANTS_MID_GET_COMPRESSCFG_AUD] = "ANTS_MID_GET_COMPRESSCFG_AUD",			//!��ȡ�豸�����Խ�������� +
	[ANTS_MID_SET_COMPRESSCFG_AUD] = "ANTS_MID_SET_COMPRESSCFG_AUD",			//!�����豸�����Խ��������
	
	[ANTS_MID_GET_SNMPCFG] = "ANTS_MID_GET_SNMPCFG",							//!��ȡ�豸SNMP���ò��� +
	[ANTS_MID_SET_SNMPCFG] = "ANTS_MID_SET_SNMPCFG",							//!�����豸SNMP���ò���
	
	[ANTS_MID_GET_NETCFG_MULTI] = "ANTS_MID_GET_NETCFG_MULTI",				//!��ȡ�豸���������ò��� +
	[ANTS_MID_SET_NETCFG_MULTI] = "ANTS_MID_SET_NETCFG_MULTI",				//!�����豸���������ò���
	
	[ANTS_MID_GET_NFSCFG] = "ANTS_MID_GET_NFSCFG",							//!��ȡ�豸NFS���ò��� +
	[ANTS_MID_SET_NFSCFG] = "ANTS_MID_SET_NFSCFG",							//!�����豸NFS���ò���
	
	[ANTS_MID_GET_NET_DISKCFG] = "ANTS_MID_GET_NET_DISKCFG",					//!��ȡ�豸����Ӳ�����ò��� +
	[ANTS_MID_SET_NET_DISKCFG] = "ANTS_MID_SET_NET_DISKCFG",					//!�����豸����Ӳ����������
	
	[ANTS_MID_GET_IPCCFG] = "ANTS_MID_GET_IPCCFG",							//��ȡIPC���ò��� +
	[ANTS_MID_SET_IPCCFG] = "ANTS_MID_SET_IPCCFG",							//����IPC���ò��� 
	
	[ANTS_MID_GET_IPCCFG_V2] = "ANTS_MID_GET_IPCCFG_V2",						//����IPC���ò���(64·��չ)  
	[ANTS_MID_SET_IPCCFG_V2] = "ANTS_MID_SET_IPCCFG_V2",						//��ȡIPC���ò���(64·��չ)+

	[ANTS_MID_GET_WIFI_CFG] = "ANTS_MID_GET_WIFI_CFG",						//!��ȡIP����豸���߲���
	[ANTS_MID_SET_WIFI_CFG] = "ANTS_MID_SET_WIFI_CFG",						//!����IP����豸���߲��� +
	
	[ANTS_MID_GET_WIFI_WORKMODE] = "ANTS_MID_GET_WIFI_WORKMODE",				//!��ȡIP����豸���ڹ���ģʽ����
	[ANTS_MID_SET_WIFI_WORKMODE] = "ANTS_MID_SET_WIFI_WORKMODE",				//!����IP����豸���ڹ���ģʽ���� +
	
	[ANTS_MID_GET_3G_CFG] = "ANTS_MID_GET_3G_CFG",							//!��ȡ3G���ò���
	[ANTS_MID_SET_3G_CFG] = "ANTS_MID_SET_3G_CFG",							//!����3G���ò��� +
	
	[ANTS_MID_GET_MANAGERHOST_CFG] = "ANTS_MID_GET_MANAGERHOST_CFG", 		//!��ȡ����ע������������ò��� +
	[ANTS_MID_SET_MANAGERHOST_CFG] = "ANTS_MID_SET_MANAGERHOST_CFG",			//!��������ע������������ò���
	
	[ANTS_MID_GET_RTSPCFG] = "ANTS_MID_GET_RTSPCFG",							//!��ȡRTSP���ò��� +
	[ANTS_MID_SET_RTSPCFG] = "ANTS_MID_SET_RTSPCFG",							//!����RTSP���ò���
	
	[ANTS_MID_GET_VIDEOEFFECT] = "ANTS_MID_GET_VIDEOEFFECT",
	[ANTS_MID_SET_VIDEOEFFECT] = "ANTS_MID_SET_VIDEOEFFECT",
	
	[ANTS_MID_GET_MOTIONCFG] = "ANTS_MID_GET_MOTIONCFG",
	[ANTS_MID_SET_MOTIONCFG] = "ANTS_MID_SET_MOTIONCFG",
	
	[ANTS_MID_GET_MOTIONCFG_V2] = "ANTS_MID_GET_MOTIONCFG_V2",
	[ANTS_MID_SET_MOTIONCFG_V2] = "ANTS_MID_SET_MOTIONCFG_V2",
	
	[ANTS_MID_GET_SHELTERCFG] = "ANTS_MID_GET_SHELTERCFG",
	[ANTS_MID_SET_SHELTERCFG] = "ANTS_MID_SET_SHELTERCFG",
	
	[ANTS_MID_GET_HIDEALARMCFG] = "ANTS_MID_GET_HIDEALARMCFG",
	[ANTS_MID_SET_HIDEALARMCFG] = "ANTS_MID_SET_HIDEALARMCFG",
	
	[ANTS_MID_GET_VIDEOLOSTCFG] = "ANTS_MID_GET_VIDEOLOSTCFG",
	[ANTS_MID_SET_VIDEOLOSTCFG] = "ANTS_MID_SET_VIDEOLOSTCFG",
	
	[ANTS_MID_GET_OSDCFG] = "ANTS_MID_GET_OSDCFG",
	[ANTS_MID_SET_OSDCFG] = "ANTS_MID_SET_OSDCFG",
	
	[ANTS_MID_GET_VIDEOFORMAT] = "ANTS_MID_GET_VIDEOFORMAT",
	[ANTS_MID_SET_VIDEOFORMAT] = "ANTS_MID_SET_VIDEOFORMAT",

	[ANTS_MID_GET_NVRWORKMODE] = "ANTS_MID_GET_NVRWORKMODE",
	[ANTS_MID_SET_NVRWORKMODE] = "ANTS_MID_SET_NVRWORKMODE",
	
	[ANTS_MID_GET_NETDEVCONNETCTCFG] = "ANTS_MID_GET_NETDEVCONNETCTCFG",
	[ANTS_MID_SET_NETDEVCONNETCTCFG] = "ANTS_MID_SET_NETDEVCONNETCTCFG",
	
	[ANTS_MID_GET_DEVCHANNAME_CFG] = "ANTS_MID_GET_DEVCHANNAME_CFG",
	[ANTS_MID_SET_DEVCHANNAME_CFG] = "ANTS_MID_SET_DEVCHANNAME_CFG",

	[ANTS_MID_GET_DEVCHANNAME_CFG_V2] = "ANTS_MID_GET_DEVCHANNAME_CFG_V2",
	[ANTS_MID_SET_DEVCHANNAME_CFG_V2] = "ANTS_MID_SET_DEVCHANNAME_CFG_V2",

	[ANTS_MID_GET_HDCFG_V2] = "ANTS_MID_GET_HDCFG_V2",
	[ANTS_MID_SET_HDCFG_V2] = "ANTS_MID_SET_HDCFG_V2",

	[ANTS_MID_GET_SENSOR_CFG] = "ANTS_MID_GET_SENSOR_CFG",
	[ANTS_MID_SET_SENSOR_CFG] = "ANTS_MID_SET_SENSOR_CFG",

	[ANTS_MID_GET_3GDEVICE_CFG] = "ANTS_MID_GET_3GDEVICE_CFG",	//!��ȡ�豸֧��3G�����豸����	
	[ANTS_MID_GET_AP_INFO_LIST] = "ANTS_MID_GET_AP_INFO_LIST",				//!��ȡ����������Դ����
	[ANTS_MID_GET_USERINFO] = "ANTS_MID_GET_USERINFO",						//!��ȡ��ǰ�û���Ϣ
	[ANTS_MID_GET_USERINFO_V2] = "ANTS_MID_GET_USERINFO_V2",
	[ANTS_MID_GET_PTZCFG] = "ANTS_MID_GET_PTZCFG",							//!��ȡ�豸֧��PTZЭ�鼯��
	[ANTS_MID_GET_WORKSTATUS] = "ANTS_MID_GET_WORKSTATUS",					//!��ȡ�豸��ǰ����״̬
	[ANTS_MID_GET_WORKSTATUS_V2] = "ANTS_MID_GET_WORKSTATUS_V2",
	[ANTS_MID_GET_COMPRESS_ABILITY] = "ANTS_MID_GET_COMPRESS_ABILITY",		//!��ȡ�豸ѹ����������
	[ANTS_MID_GET_WORKSTATUS_V3] = "ANTS_MID_GET_WORKSTATUS_V3",
};


static unsigned int JaVersion2Ants(unsigned char* pVersion)
{
	unsigned int ret_val = 0, maj = 0, min = 0, rev = 0, ext = 0;

	sscanf(pVersion, "%d.%d.%d.%d", &maj, &min, &rev, &ext);

	ret_val = maj<<16 | (ext/1000)<<12 | ((ext%1000)/100)<<8 | ((ext%100)/10)<<4 | (ext%10);
	return ret_val;
}

static unsigned int AntsBitrate2Ja(unsigned int Bitrate)
{
	unsigned int ret_val = 0;
	if(Bitrate & (1<<31)){
		ret_val = Bitrate & (~(1<<31));
	}else{
		if(Bitrate == 1){
			ret_val = 16;
		}else if(Bitrate == 2){
			ret_val = 32;
		}else if(Bitrate == 3){
			ret_val = 48;
		}else if(Bitrate == 4){
			ret_val = 64;
		}else if(Bitrate == 5){
			ret_val = 80;
		}else if(Bitrate == 6){
			ret_val = 96;
		}else if(Bitrate == 7){
			ret_val = 128;
		}else if(Bitrate == 8){
			ret_val = 160;
		}else if(Bitrate == 9){
			ret_val = 192;
		}else if(Bitrate == 10){
			ret_val = 224;
		}else if(Bitrate == 11){
			ret_val = 256;
		}else if(Bitrate == 12){
			ret_val = 320;
		}else if(Bitrate == 13){
			ret_val = 384;
		}else if(Bitrate == 14){
			ret_val = 448;
		}else if(Bitrate == 15){
			ret_val = 512;
		}else if(Bitrate == 16){
			ret_val = 640;
		}else if(Bitrate == 17){
			ret_val = 768;
		}else if(Bitrate == 18){
			ret_val = 896;
		}else if(Bitrate == 19){
			ret_val = 1024;
		}else if(Bitrate == 20){
			ret_val = 1280;
		}else if(Bitrate == 21){
			ret_val = 1536;
		}else if(Bitrate == 22){
			ret_val = 1792;
		}else if(Bitrate == 23){
			ret_val = 2048;
		}else if(Bitrate == 24){
			ret_val = 2560;
		}else if(Bitrate == 25){
			ret_val = 3072;
		}else if(Bitrate == 26){
			ret_val = 4096;
		}else if(Bitrate == 27){
			ret_val = 5120;
		}else if(Bitrate == 28){
			ret_val = 6144;
		}else if(Bitrate == 29){
			ret_val = 7168;
		}else if(Bitrate == 30){
			ret_val = 8192;
		}else{
			ret_val = 2048;
		}
	}
	return ret_val;
}

static unsigned int AntsResolution2Ja(unsigned int resolution)
{
	unsigned int ret_val = 0;
	switch(resolution){
		case 16:
			ret_val = SYS_VIN_SIZE_360P;
			break;
		case 19:
			ret_val = SYS_VIN_SIZE_720P;
			break;
		case 6:
			ret_val = SYS_VIN_SIZE_QVGA;
			break;
		case 27:
			ret_val = SYS_VIN_SIZE_1080P;
			break;
		default:
			ret_val = SYS_VIN_SIZE_720P;
	}
	return ret_val;
}

static unsigned int JaResolution2Ants(unsigned int resolution)
{
	unsigned int ret_val = 0;
	switch(resolution){
		/*case SYS_VIN_SIZE_QCIF:
			ret_val = 2;
			break;
		case SYS_VIN_SIZE_CIF:
			ret_val = 4;
			break;
		case SYS_VIN_SIZE_HALFD1:
			ret_val = 
			break;
		case SYS_VIN_SIZE_D1:
			ret_val = 
			break;
		case SYS_VIN_SIZE_QQ960H:
			ret_val = 
			break;
		case SYS_VIN_SIZE_Q960H:
			ret_val = 
			break;
		case SYS_VIN_SIZE_HALF960H:
			ret_val = 
			break;
		case SYS_VIN_SIZE_960H:
			ret_val = 
			break;
		case SYS_VIN_SIZE_180P:
			ret_val = 
			break;*/
		case SYS_VIN_SIZE_360P:
			ret_val = 16;
			break;
		case SYS_VIN_SIZE_720P:
			ret_val = 19;
			break;
		case SYS_VIN_SIZE_QVGA:
			ret_val = 6;
			break;
		case SYS_VIN_SIZE_VGA:
			ret_val = 16;
			break;
		case SYS_VIN_SIZE_1080P:
			ret_val = 27;
			break;
		default:
			ret_val = 19;
	}
	return ret_val;
}

int JaFrameRate2Ants(int value)
{
	int ret_val = 0;
	if(value == 1){
		ret_val = 5;
	}else if(value == 2){
		ret_val = 6;
	}
	else if(value == 4){
		ret_val = 7;
	}
	else if(value == 6){ 
		ret_val = 8;
	}
	else if(value == 8){
		ret_val = 9;
	}
	else if(value == 10){
		ret_val = 10;
	}
	else if(value == 12){
		ret_val = 11;
	}
	else if(value == 16){
		ret_val = 12;
	}
	else if(value == 20){
		ret_val = 13;
	}
	else if(value == 15){
		ret_val = 14;
	}
	else if(value == 18){
		ret_val = 15;
	}
	else if(value == 22){
		ret_val = 16;
	}
	else if(value == 25){
		ret_val = 17;
	}
	else if(value == 3){
		ret_val = 18;
	}
	else if(value == 5){
		ret_val = 19;
	}
	else if(value == 7){
		ret_val = 20;
	}
	else if(value == 9){
		ret_val = 21;
	}
	else if(value == 11){
		ret_val = 22;
	}
	else if(value == 13){
		ret_val = 23;
	}
	else if(value == 14){
		ret_val = 24;
	}
	else if(value == 17){
		ret_val = 25;
	}
	else if(value == 19){
		ret_val = 26;
	}else if(value == 21){
		ret_val = 27;
	}else if(value == 24){
		ret_val = 28;
	}else if(value == 26){
		ret_val = 29;
	}else if(value == 27){
		ret_val = 30;
	}else if(value == 28){
		ret_val = 31;
	}else if(value == 29){
		ret_val = 32;
	}else{
		ret_val = 0;
	}
	return ret_val;
}

int AntsFrameRate2Ja(int value)
{
	int ret_val = 0;
	if(value == 5){
		ret_val = 1;
	}else if(value == 6){
		ret_val = 2;
	}
	else if(value == 7){
		ret_val = 4;
	}
	else if(value == 8){ 
		ret_val = 6;
	}
	else if(value == 9){
		ret_val = 8;
	}
	else if(value == 10){
		ret_val = 10;
	}
	else if(value == 11){
		ret_val = 12;
	}
	else if(value == 12){
		ret_val = 16;
	}
	else if(value == 13){
		ret_val = 20;
	}
	else if(value == 14){
		ret_val = 15;
	}
	else if(value == 15){
		ret_val = 18;
	}
	else if(value == 16){
		ret_val = 22;
	}
	else if(value == 17){
		ret_val = 25;
	}
	else if(value == 18){
		ret_val = 3;
	}
	else if(value == 19){
		ret_val = 5;
	}
	else if(value == 20){
		ret_val = 7;
	}
	else if(value == 21){
		ret_val = 9;
	}
	else if(value == 22){
		ret_val = 11;
	}
	else if(value == 23){
		ret_val = 13;
	}
	else if(value == 24){
		ret_val = 14;
	}
	else if(value == 25){
		ret_val = 17;
	}
	else if(value == 26){
		ret_val = 19;
	}else if(value == 27){
		ret_val = 21;
	}else if(value == 28){
		ret_val = 24;
	}else if(value == 29){
		ret_val = 26;
	}else if(value == 30){
		ret_val = 37;
	}else if(value == 31){
		ret_val = 28;
	}else if(value == 32){
		ret_val = 29;
	}else{
		ret_val = 25;
	}
	return ret_val;
}


#define JADATE2ANTS(Y, M, D) \
	((Y/1000)<<12 | ((Y%1000)/100)<<8 | ((Y%100)/10)<<4 | (Y%10))<<16 |\
	(((M%100)/10)<<4 | (M%10))<<8 |\
	(((D%100)/10)<<4 | (D%10))


void Area2MotionScope(IN int LeftTopX,IN int LeftTopY,IN int RightBottomX,IN int RightBottomY,IN int Width,IN int Height,OUT BYTE **lpMotionScope)
{
	int col = 0, row = 0;
	//!��352*288���ο�
	int WidthV1=352;
	int HeightV1=288;
	int LeftTopx=(WidthV1*LeftTopX)/Width;
	int LeftTopy=(HeightV1*LeftTopY)/Height;
	int RightBottomx=(WidthV1*RightBottomX)/Width;
	int RightBottomy=(HeightV1*RightBottomY)/Height;
	int MacroblockWidth=WidthV1/16;
	int MacroblockHeight=HeightV1/12;

	for(col=0;col<64;col++){
		for(row=0;row<96;row++){
			if(LeftTopx<=MacroblockWidth*row && LeftTopy<=MacroblockHeight*col && RightBottomx>=MacroblockWidth*row && RightBottomy>=MacroblockHeight*col)
				lpMotionScope[col][row]=1;
			else
				lpMotionScope[col][row]=0;
		}
	}
	
}

void MotionScope2Area(IN BYTE **lpMotionScope,IN int Width,IN int Height,OUT int *lpLeftTopX,OUT int *lpLeftTopY,OUT int *lpRightBottomX,OUT int *lpRightBottomY)
{
	//!��352*288������
	int WidthV1=352;
	int HeightV1=288;
	int LeftTopx=0;
	int LeftTopy=0;
	int RightBottomx=352;
	int RightBottomy=288;
	int MacroblockWidth=WidthV1/16;
	int MacroblockHeight=HeightV1/12;
	int col=0;
	int row=0;

	//!���ҵ�һ����1������Ԫ��
	for(col=0;col<64;col++){
		for(row=0;row<96;row++){
			if(lpMotionScope[col][row]==1){
				LeftTopx=MacroblockWidth*row;
				LeftTopy=MacroblockHeight*col;
				break;
			}
		}
	}
	
	for(col=63;col<=0;col--){
		for(row=95;row<=0;row--){
			if(lpMotionScope[col][row]==1){
				RightBottomx=MacroblockWidth*row;
				RightBottomy=MacroblockHeight*col;
				break;
			}
		}
	}

	*lpLeftTopX=(Width*LeftTopx)/WidthV1;
	*lpLeftTopY=(Height*LeftTopy)/HeightV1;
	*lpRightBottomX=(Width*RightBottomx)/WidthV1;
	*lpRightBottomY=(Height*RightBottomy)/HeightV1;	
	
}

//!������ȡ������
/*
---------------------�������ò���˵��------------------------
@dwCommand		:�豸��������
@dwChannel			:�豸ͨ����,��0��ʼ
@lpBuffer				:�豸�������ݵĻ���ָ�� 
@dwSize				:�������ݵĻ��峤��(���ֽ�Ϊ��λ) 
@����ֵ			:TRUE��ʾ�ɹ�,FALSE��ʾʧ��
---------------------------------------------------------
*/
BOOL gfxSetParameter(IN DWORD dwCommand,IN DWORD dwChannel,IN LPVOID lpBuffer,IN DWORD dwSize)
{
	unsigned char type_str[128] = {0};
		printf("Set para type:%s\r\n",unistruct_map(dwCommand, (const char**)ants_type_map, sizeof(ants_type_map)/sizeof(ants_type_map[0])));

	if(lpBuffer==NULL||dwSize<=0)
		return FALSE;
	SYSCONF_t *sysconf = SYSCONF_dup();
	switch(dwCommand){
		case ANTS_MID_SET_DEVICECFG:
		{
			//!TODO: Add your message handler code here and/or call default
			/* �����豸����,������ͨ����*/
			if(dwSize!=sizeof(ANTS_MID_DEVICECFG))
				return FALSE;
			
			/* ��lpDevCfg�г�Ա������Ϣ���ݵ��ײ�IPC��*/			
			LPANTS_MID_DEVICECFG lpDevCfg=(LPANTS_MID_DEVICECFG)lpBuffer;
		}		
		break;
		case ANTS_MID_SET_NETCFG:
		{
			//!TODO: Add your message handler code here and/or call default
			/* �����������,������ͨ����*/
			if(dwSize!=sizeof(ANTS_MID_NETCFG))
				return FALSE;
			
			/* ��lpNetCfg�г�Ա������Ϣ���ݵ��ײ�IPC��*/
			LPANTS_MID_NETCFG lpNetCfg=(LPANTS_MID_NETCFG)lpBuffer;
			LPANTS_MID_ETHERNET lpEthrNet = (LPANTS_MID_ETHERNET)(lpNetCfg->struEtherNet + 0); // ֻʹ�õ�һ��ETH��
			ifconf_interface_t ifr;
			ifconf_ipv4_addr_t ip4_addr;
			int i = 0;
			
			if(lpNetCfg->dwSize == sizeof(ANTS_MID_NETCFG)){		
				ip4_addr = ifconf_ipv4_aton(lpEthrNet->struDVRIP.sIpV4);
				sysconf->ipcam.network.lan.static_ip.in_addr= ip4_addr.in_addr;
				ip4_addr = ifconf_ipv4_aton(lpEthrNet->struDVRIPMask.sIpV4);
				sysconf->ipcam.network.lan.static_netmask.in_addr = ip4_addr.in_addr;
				_ants_server.server_port = lpEthrNet->wDVRPort;//�˿ں�
				ifr.mtu = lpEthrNet->wMTU;//����MTU����,Ĭ��1500
				//�����ַ
				for(i = 0; i < (sizeof(lpEthrNet->byMACAddr) / sizeof(lpEthrNet->byMACAddr[0]))
					&& i < (sizeof(sysconf->ipcam.network.mac.s) / sizeof(sysconf->ipcam.network.mac.s[0])); ++i){
					 sysconf->ipcam.network.mac.s[i]= lpEthrNet->byMACAddr[i];
				}

				ip4_addr = ifconf_ipv4_aton(lpNetCfg->struDnsServer1IpAddr.sIpV4);
				sysconf->ipcam.network.lan.static_preferred_dns.in_addr = ip4_addr.in_addr;
				ip4_addr = ifconf_ipv4_aton(lpNetCfg->struDnsServer2IpAddr.sIpV4);			
				sysconf->ipcam.network.lan.static_alternate_dns.in_addr = ip4_addr.in_addr;
				sysconf->ipcam.network.lan.dhcp = lpNetCfg->byUseDhcp ? 1 : 0;// �Ƿ�����DHCP 0xff-��Ч 0-������ 1-����
				sysconf->ipcam.network.lan.port[0].value = lpNetCfg->wHttpPortNo;/* HTTP�˿ں� */
				ip4_addr = ifconf_ipv4_aton(lpNetCfg->struGatewayIpAddr.sIpV4);
				sysconf->ipcam.network.lan.static_gateway.in_addr = ip4_addr.in_addr;
				sysconf->ipcam.network.lan.port[1].value =lpEthrNet->wDVRPort;
				sysconf->ipcam.network.pppoe.enable = lpNetCfg->struPPPoE.dwPPPOE;
				
				SYSCONF_save(sysconf);
				exit(0);
				}
		}		
		break;
		case ANTS_MID_SET_PICCFG:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ����ͼ�����,����ͨ����*/
			if(dwSize!=sizeof(ANTS_MID_PICCFG))
				return FALSE;
			
			/* ��lpPicCfg�г�Ա������Ϣ���ݵ��ײ�IPC��*/
			LPANTS_MID_PICCFG lpPicCfg=(LPANTS_MID_PICCFG)lpBuffer;
			if(lpPicCfg->dwSize == sizeof(ANTS_MID_PICCFG)){
				sysconf->ipcam.isp.image_attr.saturation.val = lpPicCfg->struColor.bySaturation * 
					sysconf->ipcam.isp.image_attr.saturation.max / 255;					
				SENSOR_saturation_set(sysconf->ipcam.isp.image_attr.saturation.val);
				sysconf->ipcam.isp.image_attr.contrast.val = lpPicCfg->struColor.byContrast *
					sysconf->ipcam.isp.image_attr.contrast.max / 255;
				SENSOR_contrast_set(sysconf->ipcam.isp.image_attr.contrast.val);
				sysconf->ipcam.isp.image_attr.brightness.val = lpPicCfg->struColor.byBrightness *
					sysconf->ipcam.isp.image_attr.brightness.max / 255;
				SENSOR_brightness_set(sysconf->ipcam.isp.image_attr.brightness.val);
				sysconf->ipcam.isp.image_attr.hue.val = lpPicCfg->struColor.byHue *
					sysconf->ipcam.isp.image_attr.hue.max / 255;
				SENSOR_hue_set(sysconf->ipcam.isp.image_attr.hue.val);
					
			}
		}		
		break;
		case ANTS_MID_SET_PICCFG_V2:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ����ͼ�����,����ͨ����,�ýṹ�����64·��չ��*/
			if(dwSize!=sizeof(ANTS_MID_PICCFG_V2))
				return FALSE;

			/* ��lpPicCfg�г�Ա������Ϣ���ݵ��ײ�IPC��*/
			LPANTS_MID_PICCFG_V2 lpPicCfg=(LPANTS_MID_PICCFG_V2)lpBuffer;
		}		
		break;
		case ANTS_MID_SET_COMPRESSCFG:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ����ѹ������,����ͨ����*/
			if(dwSize!=sizeof(ANTS_MID_COMPRESSIONCFG))
				return FALSE;

			/* ��lpEncCfg�г�Ա������Ϣ���ݵ��ײ�IPC��*/
			LPANTS_MID_COMPRESSIONCFG lpEncCfg=(LPANTS_MID_COMPRESSIONCFG)lpBuffer;

			
			if(lpEncCfg->dwSize == sizeof(ANTS_MID_COMPRESSIONCFG)){
				//main stream
				//sysconf->ipcam.vin[0].enc_h264[0].stream[0].size.val = AntsResolution2Ja(lpEncCfg->struNormHighRecordPara.byResolution);
				sysconf->ipcam.vin[0].enc_h264[0].stream[0].mode.val = 1 << lpEncCfg->struNormHighRecordPara.byBitrateType;
				sysconf->ipcam.vin[0].enc_h264[0].stream[0].quality.val = lpEncCfg->struNormHighRecordPara.byPicQuality;
				sysconf->ipcam.vin[0].enc_h264[0].stream[0].bps = AntsBitrate2Ja(lpEncCfg->struNormHighRecordPara.dwVideoBitrate);
				sysconf->ipcam.vin[0].enc_h264[0].stream[0].fps = AntsFrameRate2Ja(lpEncCfg->struNormHighRecordPara.dwVideoFrameRate);
				sysconf->ipcam.vin[0].enc_h264[0].stream[0].gop = lpEncCfg->struNormHighRecordPara.wIntervalFrameI;
				
				//sub stream
				//sysconf->ipcam.vin[0].enc_h264[0].stream[1].size.val = AntsResolution2Ja(lpEncCfg->struNetPara.byResolution);
				sysconf->ipcam.vin[0].enc_h264[1].stream[0].mode.val = 1 << lpEncCfg->struNetPara.byBitrateType;
				sysconf->ipcam.vin[0].enc_h264[1].stream[0].size.val = AntsResolution2Ja(lpEncCfg->struNetPara.byResolution);
				sysconf->ipcam.vin[0].enc_h264[1].stream[0].quality.val = lpEncCfg->struNetPara.byPicQuality;
				sysconf->ipcam.vin[0].enc_h264[1].stream[0].bps = lpEncCfg->struNetPara.dwVideoBitrate & (~(1<<31));
				sysconf->ipcam.vin[0].enc_h264[1].stream[0].fps = AntsFrameRate2Ja(lpEncCfg->struNetPara.dwVideoFrameRate);
				sysconf->ipcam.vin[0].enc_h264[1].stream[0].gop = lpEncCfg->struNetPara.wIntervalFrameI;

				SYSCONF_save(sysconf);
				exit(0);
			}
		}		
		break;
		case ANTS_MID_SET_RECORDCFG:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ����¼�����,����ͨ����*/
			if(dwSize!=sizeof(ANTS_MID_RECORD))
				return FALSE;
			
			/* ��lpRecCfg�г�Ա������Ϣ���ݵ��ײ�IPC��*/
			LPANTS_MID_RECORD lpRecCfg=(LPANTS_MID_RECORD)lpBuffer;
		}		
		break;
		case ANTS_MID_SET_DECODERCFG:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ������̨����������,����ͨ����*/
			if(dwSize!=sizeof(ANTS_MID_DECODERCFG))
				return FALSE;
			
			/* ��lpDecCfg�г�Ա������Ϣ���ݵ��ײ�IPC��*/
			LPANTS_MID_DECODERCFG lpDecCfg=(LPANTS_MID_DECODERCFG)lpBuffer;
		}		
		break;
		case ANTS_MID_SET_RS232CFG:
		break;
		case ANTS_MID_SET_ALARMINCFG:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ���ñ����������,���Ǳ�������ͨ����*/
			if(dwSize!=sizeof(ANTS_MID_ALARMINCFG))
				return FALSE;

			/* ��lpAICfg�г�Ա������Ϣ���ݵ��ײ�IPC��*/
			LPANTS_MID_ALARMINCFG lpAICfg=(LPANTS_MID_ALARMINCFG)lpBuffer;
		}		
		break;
		case ANTS_MID_SET_ALARMINCFG_V2:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ���ñ����������,���Ǳ�������ͨ����,�ýṹ���64·��չ��*/
			if(dwSize!=sizeof(ANTS_MID_ALARMINCFG_V2))
				return FALSE;

			/* ��lpAICfg�г�Ա������Ϣ���ݵ��ײ�IPC��*/
			LPANTS_MID_ALARMINCFG_V2 lpAICfg=(LPANTS_MID_ALARMINCFG_V2)lpBuffer;
		}		
		break;
		case ANTS_MID_SET_ALARMOUTCFG:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ���ñ����������,���Ǳ������ͨ����*/
			if(dwSize!=sizeof(ANTS_MID_ALARMOUTCFG))
				return FALSE;

			/* ��lpAOCfg�г�Ա������Ϣ���ݵ��ײ�IPC��*/
			LPANTS_MID_ALARMOUTCFG lpAOCfg=(LPANTS_MID_ALARMOUTCFG)lpBuffer;			
		}		
		break;
		case ANTS_MID_SET_TIMECFG:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ����ʱ�����,������ͨ����*/
			if(dwSize!=sizeof(ANTS_MID_TIME))
				return FALSE;

			/* ��lpTmCfg�г�Ա������Ϣ���ݵ��ײ�IPC��*/
			LPANTS_MID_TIME lpTmCfg=(LPANTS_MID_TIME)lpBuffer;
		}		
		break;
		case ANTS_MID_SET_USERCFG:
		{
			//!TODO: Add your message handler code here and/or call default
			/* �����û�����,������ͨ����*/
			if(dwSize!=sizeof(ANTS_MID_USER))
				return FALSE;

			/* ��lpUserCfg�г�Ա������Ϣ���ݵ��ײ�IPC��*/
			LPANTS_MID_USER lpUserCfg=(LPANTS_MID_USER)lpBuffer;
		}		
		break;
		case ANTS_MID_SET_USERCFG_V2:
		{
			//!TODO: Add your message handler code here and/or call default
			/* �����û�����,������ͨ����,���64·��չ�Ľṹ*/
			if(dwSize!=sizeof(ANTS_MID_USER_V2))
				return FALSE;

			/* ��lpUserCfg�г�Ա������Ϣ���ݵ��ײ�IPC��*/
			LPANTS_MID_USER_V2 lpUserCfg=(LPANTS_MID_USER_V2)lpBuffer;
		}		
		break;
		case ANTS_MID_SET_EXCEPTIONCFG:
		break;
		case ANTS_MID_SET_ZONEANDDST:
		break;
		case ANTS_MID_SET_SHOWSTRING:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ���õ����ַ�����,����ͨ����*/
			if(dwSize!=sizeof(ANTS_MID_SHOWSTRING))
				return FALSE;

			/* ��lpShowString�г�Ա������Ϣ���ݵ��ײ�IPC��*/
			LPANTS_MID_SHOWSTRING lpShowString=(LPANTS_MID_SHOWSTRING)lpBuffer;	
		}		
		break;
		case ANTS_MID_SET_EVENTCOMPCFG:
		break;
		case ANTS_MID_SET_AUTOREBOOT:
		break;
		case ANTS_MID_SET_NETAPPCFG:
		break;
		case ANTS_MID_SET_NTPCFG:
		break;
		case ANTS_MID_SET_DDNSCFG:
		break;
		case ANTS_MID_SET_EMAILCFG:
		break;
		case ANTS_MID_SET_HDCFG:
		break;
		case ANTS_MID_SET_HDGROUP_CFG:
		break;
		case ANTS_MID_SET_HDGROUP_CFG_V2:
		break;
		case ANTS_MID_SET_COMPRESSCFG_AUD:
		break;
		case ANTS_MID_SET_SNMPCFG:
		break;
		case ANTS_MID_SET_NETCFG_MULTI:
		break;
		case ANTS_MID_SET_NFSCFG:
		break;
		case ANTS_MID_SET_NET_DISKCFG:
		break;
		case ANTS_MID_SET_IPCCFG:
		break;
		case ANTS_MID_SET_IPCCFG_V2:
		break;
		case ANTS_MID_SET_WIFI_CFG:
		break;
		case ANTS_MID_SET_WIFI_WORKMODE:
		break;
		case ANTS_MID_SET_3G_CFG:
		break;
		case ANTS_MID_SET_MANAGERHOST_CFG:
		break;
		case ANTS_MID_SET_RTSPCFG:
		break;
		case ANTS_MID_SET_VIDEOEFFECT:
		{
			//!TODO: Add your message handler code here and/or call default
			/* �������ȶԱȶ�ɫ�Ȳ���,����ͨ����*/
			if(dwSize!=sizeof(ANTS_MID_COLOR))
				return FALSE;

			/* ��lpColorCfg�г�Ա������Ϣ���ݵ��ײ�IPC��*/
			LPANTS_MID_COLOR lpColorCfg=(LPANTS_MID_COLOR)lpBuffer;
		}		
		break;
		case ANTS_MID_SET_MOTIONCFG:
		{
			//!TODO: Add your message handler code here and/or call default
			/* �����ƶ�������,����ͨ����*/
			if(dwSize!=sizeof(ANTS_MID_MOTION))
				return FALSE;

			/* ��lpMotionCfg�г�Ա������Ϣ���ݵ��ײ�IPC��*/
			LPANTS_MID_MOTION lpMotionCfg=(LPANTS_MID_MOTION)lpBuffer;
		}		
		break;
		case ANTS_MID_SET_MOTIONCFG_V2:
		{
			//!TODO: Add your message handler code here and/or call default
			/* �����ƶ�������,����ͨ����,���64·��չ�Ľṹ*/
			if(dwSize!=sizeof(ANTS_MID_MOTION_V2))
				return FALSE;

			/* ��lpMotionCfg�г�Ա������Ϣ���ݵ��ײ�IPC��*/
			LPANTS_MID_MOTION_V2 lpMotionCfg=(LPANTS_MID_MOTION_V2)lpBuffer;
		}		
		break;
		case ANTS_MID_SET_SHELTERCFG:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ������Ƶ�ڵ��������,����ͨ����*/
			if(dwSize!=sizeof(ANTS_MID_SHELTERCFG))
				return FALSE;
			
			/* ��lpShelterCfg�г�Ա������Ϣ���ݵ��ײ�IPC��*/
			LPANTS_MID_SHELTERCFG lpShelterCfg=(LPANTS_MID_SHELTERCFG)lpBuffer;
		}		
		break;
		case ANTS_MID_SET_HIDEALARMCFG:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ������Ƶ�ڵ���������,����ͨ����*/
			if(dwSize!=sizeof(ANTS_MID_HIDEALARM))
				return FALSE;
			
			/* ��lpHideAlarmCfg�г�Ա������Ϣ���ݵ��ײ�IPC��*/
			LPANTS_MID_HIDEALARM lpHideAlarmCfg=(LPANTS_MID_HIDEALARM)lpBuffer;
		}		
		break;
		case ANTS_MID_SET_VIDEOLOSTCFG:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ������Ƶ��ʧ����,����ͨ����*/
			if(dwSize!=sizeof(ANTS_MID_VILOST))
				return FALSE;

			/* ��lpVILostCfg�г�Ա������Ϣ���ݵ��ײ�IPC��*/
			LPANTS_MID_VILOST lpVILostCfg=(LPANTS_MID_VILOST)lpBuffer;	
		}		
		break;
		case ANTS_MID_SET_OSDCFG:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ����OSD���Ӳ���,����ͨ����*/
			if(dwSize!=sizeof(ANTS_MID_OSDCFG))
				return FALSE;

			/* ��lpOsdCfg�г�Ա������Ϣ���ݵ��ײ�IPC��*/
			LPANTS_MID_OSDCFG lpOsdCfg=(LPANTS_MID_OSDCFG)lpBuffer;
			memset(sysconf->ipcam.vin[0].channel_name, 0, sizeof(sysconf->ipcam.vin[0].channel_name));
			strcpy(sysconf->ipcam.vin[0].channel_name, lpOsdCfg->sChanName);
			APP_OVERLAY_set_title(dwChannel, sysconf->ipcam.vin[0].channel_name);
			sysconf->ipcam.date_time.date_format.val = lpOsdCfg->byOSDType != 4 ? lpOsdCfg->byOSDType:2;
			sysconf->ipcam.date_time.time_format.val = lpOsdCfg->byHourOSDType ? 0 : 1;
		}		
		break;
		case ANTS_MID_SET_VIDEOFORMAT:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ������Ƶ��ʽ����,����ͨ����*/
			if(dwSize!=sizeof(DWORD))
				return FALSE;

			/* ��lpVideoFormat�г�Ա������Ϣ���ݵ��ײ�IPC��*/
			LPDWORD lpVideoFormat=(LPDWORD)lpBuffer;
		}		
		break;
	}
		SYSCONF_save(sysconf);
	return TRUE;
	
}

/*
---------------------��ȡ���ò���˵��------------------------
@dwCommand		:�豸��������
@dwChannel			:�豸ͨ����,��0��ʼ
@lpBuffer				:�豸�������ݵĻ���ָ�� 
@dwInSize			:�������ݵĻ��峤��(���ֽ�Ϊ��λ),����Ϊ0 
@lpOutSize			:ʵ���յ������ݳ���ָ��,����ΪNULL 
@����ֵ			:TRUE��ʾ�ɹ�,FALSE��ʾʧ��
---------------------------------------------------------
*/
BOOL gfxGetParameter(IN DWORD dwCommand,IN DWORD dwChannel,OUT LPVOID lpBuffer,IN DWORD dwInSize,OUT DWORD *lpOutSize)
{
	int i = 0;
	unsigned char type_str[128] = {0};
	printf("get para type:%s\r\n",unistruct_map(dwCommand, (const char**)ants_type_map, sizeof(ants_type_map)/sizeof(ants_type_map[0])));
	SYSCONF_t *sysconf = SYSCONF_dup();
	switch(dwCommand){
	if(lpBuffer==NULL||lpOutSize==NULL||dwInSize<=0)
		return FALSE;

		case ANTS_MID_GET_DEVICECFG:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ��ȡ�豸����,������ͨ����*/			
			/* ���ײ��ȡ�豸����������Ϣ���Ƶ�lpDevCfg�г�Ա������*/			
			LPANTS_MID_DEVICECFG lpDevCfg=(LPANTS_MID_DEVICECFG)lpBuffer;
			memset(lpDevCfg, 0, sizeof(ANTS_MID_DEVICECFG));

			lpDevCfg->dwSize = sizeof(ANTS_MID_DEVCHANNELNAME_CFG);
			strcpy(lpDevCfg->sDVRName, sysconf->ipcam.vin[0].channel_name);//DVR����
			lpDevCfg->dwDVRID = 0;//DVR ID,����ң����
			lpDevCfg->dwRecycleRecord = 0;//�Ƿ�ѭ��¼��,0:����; 1:��

			//!���²�������
			sprintf(lpDevCfg->sSerialNumber, "%02x%02x%02x%02x", 
				sysconf->ipcam.network.mac.s2, 
				sysconf->ipcam.network.mac.s3, 
				sysconf->ipcam.network.mac.s4, 
				sysconf->ipcam.network.mac.s5);//���к�
			lpDevCfg->dwSoftwareVersion = JaVersion2Ants(sysconf->ipcam.info.software_version);//����汾��,��16λ�����汾,��16λ�Ǵΰ汾
			lpDevCfg->dwSoftwareBuildDate = JADATE2ANTS(sysconf->ipcam.info.build_date.year, sysconf->ipcam.info.build_date.month, sysconf->ipcam.info.build_date.day);//�����������,0xYYYYMMDD
			lpDevCfg->dwDSPSoftwareVersion = 0x00010001;//DSP����汾,��16λ�����汾,��16λ�Ǵΰ汾
			lpDevCfg->dwDSPSoftwareBuildDate = 0x00010001;// DSP�����������,0xYYYYMMDD
			lpDevCfg->dwPanelVersion = 0x00010001;//ǰ���汾,��16λ�����汾,��16λ�Ǵΰ汾
			lpDevCfg->dwHardwareVersion = 0x00010001;//Ӳ���汾,��16λ�����汾,��16λ�Ǵΰ汾
			lpDevCfg->byAlarmInPortNum = 0;//DVR�����������
			lpDevCfg->byAlarmOutPortNum = 0;//DVR�����������
			lpDevCfg->byRS232Num = 0;//DVR 232���ڸ���
			lpDevCfg->byRS485Num = 0;//DVR 485���ڸ���
			lpDevCfg->byNetworkPortNum = 1;//����ڸ���
			lpDevCfg->byDiskCtrlNum = 0;//DVR Ӳ�̿���������
			lpDevCfg->byDiskNum = 0;//DVR Ӳ�̸���
			lpDevCfg->byDVRType = 0;//DVR����, 1:DVR 2:Result 3:DVS ......
			lpDevCfg->byChanNum = 1;//DVR ͨ������
			lpDevCfg->byStartChan = 1;//��ʼͨ����,����DVS-1,DVR - 1
			lpDevCfg->byDecodeChans = 0;//DVR ����·��
			lpDevCfg->byVGANum = 0;//VGA�ڵĸ���
			lpDevCfg->byUSBNum = 0;//USB�ڵĸ���
			lpDevCfg->byAuxoutNum = 0;//���ڵĸ���
			lpDevCfg->byAudioNum = 0;//�����ڵĸ���
			lpDevCfg->byIPChanNum = 1;//�������ͨ����

			*lpOutSize=sizeof(ANTS_MID_DEVICECFG);
		}		
		break;
		case ANTS_MID_GET_NETCFG:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ��ȡ�豸�������,������ͨ����*/			
			/* ���ײ��ȡ�豸���������Ϣ���Ƶ�lpNetCfg�г�Ա������*/

			ifconf_interface_t ifr;
			char lpAddress[32] = {""};
			LPANTS_MID_NETCFG lpNetCfg=(LPANTS_MID_NETCFG)lpBuffer;
			LPANTS_MID_ETHERNET lpEthrNet = (LPANTS_MID_ETHERNET)(lpNetCfg->struEtherNet + 0); // ֻʹ�õ�һ��ETH��
			memset(lpNetCfg, 0, sizeof(ANTS_MID_NETCFG));

			if(0 == ifconf_get_interface(_ants_server.server_eth, &ifr)){
				lpNetCfg->dwSize = sizeof(ANTS_MID_NETCFG);
				strcpy(lpEthrNet->struDVRIP.sIpV4, ifconf_ipv4_ntoa(ifr.ipaddr));
				strcpy(lpEthrNet->struDVRIPMask.sIpV4, ifconf_ipv4_ntoa(ifr.netmask));
				lpEthrNet->dwNetInterface = 5;//����ӿ�1-10MBase-T 2-10MBase-Tȫ˫�� 3-100MBase-TX 4-100Mȫ˫�� 5-10M/100M����Ӧ 6-100M/1000M����Ӧ
				lpEthrNet->wDVRPort = _ants_server.server_port;//�˿ں�
				lpEthrNet->wMTU = ifr.mtu;//����MTU����,Ĭ��1500
				//�����ַ
				for(i = 0; i < (sizeof(lpEthrNet->byMACAddr) / sizeof(lpEthrNet->byMACAddr[0]))
					&& i < (sizeof(ifr.hwaddr.s_b) / sizeof(ifr.hwaddr.s_b[0])); ++i){
					lpEthrNet->byMACAddr[i] = ifr.hwaddr.s_b[i];
				}

				// ifconf��ȱ����ӿڣ��貹��
				sprintf(lpAddress, "%d.%d.%d.%d",
					sysconf->ipcam.network.lan.static_preferred_dns.s1,
					sysconf->ipcam.network.lan.static_preferred_dns.s2,
					sysconf->ipcam.network.lan.static_preferred_dns.s3,
					sysconf->ipcam.network.lan.static_preferred_dns.s4);
				strcpy(lpNetCfg->struDnsServer1IpAddr.sIpV4, lpAddress);// ����������1��IP��ַ
				sprintf(lpAddress, "%d.%d.%d.%d",
					sysconf->ipcam.network.lan.static_alternate_dns.s1,
					sysconf->ipcam.network.lan.static_alternate_dns.s2,
					sysconf->ipcam.network.lan.static_alternate_dns.s3,
					sysconf->ipcam.network.lan.static_alternate_dns.s4);
				strcpy(lpNetCfg->struDnsServer2IpAddr.sIpV4, lpAddress);// ����������2��IP��ַ

				lpNetCfg->wAlarmHostIpPort = 0;// ���������˿ں�
				lpNetCfg->byUseDhcp = sysconf->ipcam.network.lan.dhcp ? 1 : 0;// �Ƿ�����DHCP 0xff-��Ч 0-������ 1-����

				//ANTS_MID_IPADDR struDnsServer1IpAddr;/* ����������1��IP��ַ */
				//ANTS_MID_IPADDR	struDnsServer2IpAddr;/* ����������2��IP��ַ */
				//BYTE byIpResolver[ANTS_MID_MAX_DOMAIN_NAME];	/* IP����������������IP��ַ */
				//WORD wIpResolverPort;/* IP�����������˿ں� */
				lpNetCfg->wHttpPortNo = sysconf->ipcam.network.lan.port[0].value;/* HTTP�˿ں� */
				//ANTS_MID_IPADDR struMulticastIpAddr;/* �ಥ���ַ */
				strcpy(lpNetCfg->struGatewayIpAddr.sIpV4, ifconf_ipv4_ntoa(ifr.gateway));/* ���ص�ַ */
				lpNetCfg->struPPPoE.dwPPPOE = 0;
				//char szManagerHostIpV4[32];/*����ע�������IP��ַ0-������1-����*/
				//WORD wManagerHostPort;/*����ע��������˿�*/
				//BYTE byUseManagerHost;/*�Ƿ���������ע�����0-������1-����*/

				*lpOutSize=sizeof(ANTS_MID_NETCFG);
			}
		}		
		break;
		case ANTS_MID_GET_PICCFG:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ��ȡ�豸ͼ�����,����ͨ����*/			
			/* ���ײ��ȡ�豸ͼ�������Ϣ���Ƶ�lpPicCfg�г�Ա������*/			
			LPANTS_MID_PICCFG lpPicCfg=(LPANTS_MID_PICCFG)lpBuffer;
			memset(lpPicCfg, 0, sizeof(ANTS_MID_PICCFG));
			
			lpPicCfg->dwSize = sizeof(ANTS_MID_PICCFG);
			strncpy(lpPicCfg->sChanName, sysconf->ipcam.vin[0].channel_name, ANTS_MID_NAME_LEN);
			lpPicCfg->dwVideoFormat = 2;
			lpPicCfg->struColor.byHue = (sysconf->ipcam.isp.image_attr.hue.val*255)/sysconf->ipcam.isp.image_attr.hue.max;
			lpPicCfg->struColor.byBrightness= (sysconf->ipcam.isp.image_attr.brightness.val*255)/sysconf->ipcam.isp.image_attr.brightness.max;
			lpPicCfg->struColor.byContrast= (sysconf->ipcam.isp.image_attr.contrast.val*255)/sysconf->ipcam.isp.image_attr.contrast.max;
			lpPicCfg->struColor.bySaturation= (sysconf->ipcam.isp.image_attr.saturation.val*255)/sysconf->ipcam.isp.image_attr.saturation.max;
			lpPicCfg->dwShowChanName = 1;
			
			*lpOutSize=sizeof(ANTS_MID_PICCFG);
		}		
		break;
		case ANTS_MID_GET_PICCFG_V2:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ��ȡ�豸ͼ�����,����ͨ����,���64·��չ�Ľṹ*/			
			/* ���ײ��ȡ�豸ͼ�������Ϣ���Ƶ�lpPicCfg�г�Ա������*/			
			LPANTS_MID_PICCFG_V2 lpPicCfg=(LPANTS_MID_PICCFG_V2)lpBuffer;

			*lpOutSize=sizeof(ANTS_MID_PICCFG_V2);
		}		
		break;
		case ANTS_MID_GET_COMPRESSCFG:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ��ȡ�豸����ѹ������,����ͨ����,���64·��չ�Ľṹ*/			
			/* ���ײ��ȡ�豸����ѹ��������Ϣ���Ƶ�lpEncCfg�г�Ա������*/			
			LPANTS_MID_COMPRESSIONCFG lpEncCfg=(LPANTS_MID_COMPRESSIONCFG)lpBuffer;
			memset(lpEncCfg, 0, sizeof(ANTS_MID_COMPRESSIONCFG));

			//main stream
			lpEncCfg->dwSize = sizeof(ANTS_MID_COMPRESSIONCFG);
			lpEncCfg->struNormHighRecordPara.byStreamType = 0;
			lpEncCfg->struNormHighRecordPara.byResolution = JaResolution2Ants(sysconf->ipcam.vin[0].enc_h264[0].stream[0].size.val);
			lpEncCfg->struNormHighRecordPara.byBitrateType = sysconf->ipcam.vin[0].enc_h264[0].stream[0].mode.val >> 1;
			lpEncCfg->struNormHighRecordPara.byPicQuality = sysconf->ipcam.vin[0].enc_h264[0].stream[0].quality.val;
			lpEncCfg->struNormHighRecordPara.dwVideoBitrate = sysconf->ipcam.vin[0].enc_h264[0].stream[0].bps | (1<<31);
			lpEncCfg->struNormHighRecordPara.dwVideoFrameRate = JaFrameRate2Ants(sysconf->ipcam.vin[0].enc_h264[0].stream[0].fps);
			lpEncCfg->struNormHighRecordPara.wIntervalFrameI = sysconf->ipcam.vin[0].enc_h264[0].stream[0].gop;
			lpEncCfg->struNormHighRecordPara.byIntervalBPFrame = 2;
			lpEncCfg->struNormHighRecordPara.byVideoEncType = 1;
			lpEncCfg->struNormHighRecordPara.byAudioEncType = 2;
			
			//sub stream
			lpEncCfg->struNetPara.byStreamType = 0;
			lpEncCfg->struNetPara.byResolution = JaResolution2Ants(sysconf->ipcam.vin[0].enc_h264[1].stream[0].size.val);
			lpEncCfg->struNetPara.byBitrateType = sysconf->ipcam.vin[0].enc_h264[1].stream[0].mode.val >> 1;
			lpEncCfg->struNetPara.byPicQuality = sysconf->ipcam.vin[0].enc_h264[1].stream[0].quality.val;
			lpEncCfg->struNetPara.dwVideoBitrate = sysconf->ipcam.vin[0].enc_h264[1].stream[0].bps | (1<<31);
			lpEncCfg->struNetPara.dwVideoFrameRate = JaFrameRate2Ants(sysconf->ipcam.vin[0].enc_h264[1].stream[0].fps);
			lpEncCfg->struNetPara.wIntervalFrameI = sysconf->ipcam.vin[0].enc_h264[1].stream[0].gop;
			lpEncCfg->struNetPara.byIntervalBPFrame = 2;
			lpEncCfg->struNetPara.byVideoEncType = 1;
			lpEncCfg->struNetPara.byAudioEncType = 2;
			
			*lpOutSize=sizeof(ANTS_MID_COMPRESSIONCFG);
		}		
		break;
		case ANTS_MID_GET_RECORDCFG:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ��ȡ�豸¼�����,����ͨ����*/			
			/* ���ײ��ȡ�豸¼�������Ϣ���Ƶ�lpRecCfg�г�Ա������*/
			LPANTS_MID_RECORD lpRecCfg=(LPANTS_MID_RECORD)lpBuffer;
			
			*lpOutSize=sizeof(ANTS_MID_RECORD);
		}		
		break;
		case ANTS_MID_GET_DECODERCFG:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ��ȡ�豸����������,����ͨ����*/			
			/* ���ײ��ȡ�豸������������Ϣ���Ƶ�lpDecCfg�г�Ա������*/
			LPANTS_MID_DECODERCFG lpDecCfg=(LPANTS_MID_DECODERCFG)lpBuffer;
			
			*lpOutSize=sizeof(ANTS_MID_DECODERCFG);
		}		
		break;
		case ANTS_MID_GET_RS232CFG:
		break;
		case ANTS_MID_GET_ALARMINCFG:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ��ȡ�豸�����������,���Ǳ�������ͨ����*/			
			/* ���ײ��ȡ�豸�������������Ϣ���Ƶ�lpAICfg�г�Ա������*/
			LPANTS_MID_ALARMINCFG lpAICfg=(LPANTS_MID_ALARMINCFG)lpBuffer;
			
			*lpOutSize=sizeof(ANTS_MID_ALARMINCFG);
		}		
		break;
		case ANTS_MID_GET_ALARMINCFG_V2:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ��ȡ�豸�����������,���Ǳ�������ͨ����,���64·��չ�Ľṹ*/			
			/* ���ײ��ȡ�豸�������������Ϣ���Ƶ�lpAICfg�г�Ա������*/
			LPANTS_MID_ALARMINCFG_V2 lpAICfg=(LPANTS_MID_ALARMINCFG_V2)lpBuffer;
			
			*lpOutSize=sizeof(ANTS_MID_ALARMINCFG_V2);
		}		
		break;
		case ANTS_MID_GET_ALARMOUTCFG:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ��ȡ�豸�����������,���Ǳ������ͨ����*/			
			/* ���ײ��ȡ�豸�������������Ϣ���Ƶ�lpAOCfg�г�Ա������*/
			LPANTS_MID_ALARMOUTCFG lpAICfg=(LPANTS_MID_ALARMOUTCFG)lpBuffer;
			
			*lpOutSize=sizeof(ANTS_MID_ALARMOUTCFG);
		}		
		break;
		case ANTS_MID_GET_TIMECFG:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ��ȡ�豸����ʱ�����,������ͨ����*/			
			/* ���ײ��ȡ�豸����ʱ�������Ϣ���Ƶ�lpTmCfg�г�Ա������*/
			LPANTS_MID_TIME lpTmCfg=(LPANTS_MID_TIME)lpBuffer;
				
			*lpOutSize=sizeof(ANTS_MID_TIME);
		}		
		break;
		case ANTS_MID_GET_USERCFG:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ��ȡ�豸�û�����,������ͨ����*/			
			/* ���ײ��ȡ�豸�û�������Ϣ���Ƶ�lpUserCfg�г�Ա������*/
			LPANTS_MID_USER lpUserCfg=(LPANTS_MID_USER)lpBuffer;
				
			*lpOutSize=sizeof(ANTS_MID_USER);
		}		
		break;
		case ANTS_MID_GET_USERCFG_V2:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ��ȡ�豸�û�����,������ͨ����,���64·��չ�Ľṹ*/			
			/* ���ײ��ȡ�豸�û�������Ϣ���Ƶ�lpUserCfg�г�Ա������*/
			LPANTS_MID_USER_V2 lpUserCfg=(LPANTS_MID_USER_V2)lpBuffer;
				
			*lpOutSize=sizeof(ANTS_MID_USER_V2);
		}		
		break;
		case ANTS_MID_GET_EXCEPTIONCFG:
		break;
		case ANTS_MID_GET_ZONEANDDST:
		break;
		case ANTS_MID_GET_SHOWSTRING:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ��ȡ�豸�ַ����Ӳ���,����ͨ����*/			
			/* ���ײ��ȡ�豸�ַ����Ӳ�����Ϣ���Ƶ�lpShowStringCfg�г�Ա������*/
			LPANTS_MID_SHOWSTRING lpShowString=(LPANTS_MID_SHOWSTRING)lpBuffer;

			*lpOutSize=sizeof(ANTS_MID_SHOWSTRING);
		}		
		break;
		case ANTS_MID_GET_EVENTCOMPCFG:
		break;
		case ANTS_MID_GET_AUTOREBOOT:
		break;
		case ANTS_MID_GET_NETAPPCFG:
		break;
		case ANTS_MID_GET_NTPCFG:
		break;
		case ANTS_MID_GET_DDNSCFG:
		break;
		case ANTS_MID_GET_EMAILCFG:
		break;
		case ANTS_MID_GET_HDCFG:
		break;
		case ANTS_MID_GET_HDGROUP_CFG:
		break;
		case ANTS_MID_GET_HDGROUP_CFG_V2:
		break;
		case ANTS_MID_GET_COMPRESSCFG_AUD:
		break;
		case ANTS_MID_GET_SNMPCFG:
		break;
		case ANTS_MID_GET_NETCFG_MULTI:
		break;
		case ANTS_MID_GET_NFSCFG:
		break;
		case ANTS_MID_GET_NET_DISKCFG:
		break;
		case ANTS_MID_GET_IPCCFG:
		break;
		case ANTS_MID_GET_IPCCFG_V2:
		break;
		case ANTS_MID_GET_WIFI_CFG:
		break;
		case ANTS_MID_GET_WIFI_WORKMODE:
		break;
		case ANTS_MID_GET_3G_CFG:
		break;
		case ANTS_MID_GET_MANAGERHOST_CFG:
		break;
		case ANTS_MID_GET_RTSPCFG:
		break;
		case ANTS_MID_GET_3GDEVICE_CFG:
		break;
		case ANTS_MID_GET_AP_INFO_LIST:
		break;
		case ANTS_MID_GET_USERINFO:
		break;
		case ANTS_MID_GET_USERINFO_V2:
		break;
		case ANTS_MID_GET_PTZCFG:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ��ȡ�豸PTZЭ�鼯��,������ͨ����*/			
			/* ���ײ��ȡ�豸PTZЭ�鼯����Ϣ���Ƶ�lpPtzCfg�г�Ա������*/
			LPANTS_MID_PTZCFG lpPtzCfg=(LPANTS_MID_PTZCFG)lpBuffer;
			lpPtzCfg->dwPtzNum=2;
			lpPtzCfg->dwSize=sizeof(ANTS_MID_PTZCFG);

			lpPtzCfg->struPtz[0].dwType=1;
			strcpy((char *)(lpPtzCfg->struPtz[0].byDescribe),"PELCO-D");

			lpPtzCfg->struPtz[1].dwType=2;
			strcpy((char *)(lpPtzCfg->struPtz[1].byDescribe),"PELCO-P");

			*lpOutSize=sizeof(ANTS_MID_PTZCFG);			
		}		
		break;
		case ANTS_MID_GET_WORKSTATUS:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ��ȡ�豸����״̬,������ͨ����*/			
			/* ���ײ��ȡ�豸����״̬��Ϣ���Ƶ�lpWorkStatus�г�Ա������*/
			LPANTS_MID_WORKSTATE lpWorkStatus=(LPANTS_MID_WORKSTATE)lpBuffer;
			
			*lpOutSize=sizeof(ANTS_MID_WORKSTATE);			
		}		
		break;
		case ANTS_MID_GET_WORKSTATUS_V2:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ��ȡ�豸����״̬,������ͨ����,���64·��չ�Ľṹ*/			
			/* ���ײ��ȡ�豸����״̬��Ϣ���Ƶ�lpWorkStatus�г�Ա������*/
			LPANTS_MID_WORKSTATE_V2 lpWorkStatus=(LPANTS_MID_WORKSTATE_V2)lpBuffer;

			*lpOutSize=sizeof(ANTS_MID_WORKSTATE_V2);			
		}		
		break;
		case ANTS_MID_GET_COMPRESS_ABILITY:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ��ȡ�豸������������,����ͨ����*/			
			/* ���ײ��ȡ�豸����������Ϣ���Ƶ�lpCompressAbility�г�Ա������*/
			LPANTS_MID_COMPRESSIONCFG_ABILITY lpCompressAbility=(LPANTS_MID_COMPRESSIONCFG_ABILITY)lpBuffer;
			//printf("ANTS_MID_GET_COMPRESS_ABILITY\r\n");
			//!StreamAbility
			lpCompressAbility->struAbilityNode[0].dwAbilityType=ANTS_MID_COMPRESSION_STREAM_ABILITY;
			lpCompressAbility->struAbilityNode[0].dwNodeNum=3;
			
			lpCompressAbility->struAbilityNode[0].struDescNode[0].iValue=0;
			strcpy((char*)lpCompressAbility->struAbilityNode[0].struDescNode[0].byDescribe, "Main Stream");
			
			lpCompressAbility->struAbilityNode[0].struDescNode[1].iValue=1;
			strcpy((char*)lpCompressAbility->struAbilityNode[0].struDescNode[1].byDescribe, "Aux Stream");
			
			lpCompressAbility->struAbilityNode[0].struDescNode[2].iValue=2;
			strcpy((char*)lpCompressAbility->struAbilityNode[0].struDescNode[2].byDescribe, "Event Stream");

			//!MainResolutionAbility
			lpCompressAbility->struAbilityNode[1].dwAbilityType=ANTS_MID_MAIN_RESOLUTION_ABILITY;
			lpCompressAbility->struAbilityNode[1].dwNodeNum=1;

			lpCompressAbility->struAbilityNode[1].struDescNode[0].iValue=19;
			strcpy((char*)lpCompressAbility->struAbilityNode[1].struDescNode[0].byDescribe, "HD720P(1280*720)");

			//!SubResolutionAbility
			lpCompressAbility->struAbilityNode[2].dwAbilityType=ANTS_MID_SUB_RESOLUTION_ABILITY;
			lpCompressAbility->struAbilityNode[2].dwNodeNum=2;

			lpCompressAbility->struAbilityNode[2].struDescNode[0].iValue=16;
			strcpy((char*)lpCompressAbility->struAbilityNode[2].struDescNode[0].byDescribe, "360P(640*360)");

			lpCompressAbility->struAbilityNode[2].struDescNode[1].iValue=6;
			strcpy((char*)lpCompressAbility->struAbilityNode[2].struDescNode[1].byDescribe, "QVGA(320*240)");

			//!EventResolutionAbility
			lpCompressAbility->struAbilityNode[3].dwAbilityType=ANTS_MID_MAIN_RESOLUTION_ABILITY;
			lpCompressAbility->struAbilityNode[3].dwNodeNum=1;

			lpCompressAbility->struAbilityNode[3].struDescNode[0].iValue=19;
			strcpy((char*)lpCompressAbility->struAbilityNode[3].struDescNode[0].byDescribe, "HD720P(1280*720)");

			//!FrameAbility
			lpCompressAbility->struAbilityNode[4].dwAbilityType=ANTS_MID_FRAME_ABILITY;
			lpCompressAbility->struAbilityNode[4].dwNodeNum=60;

			lpCompressAbility->struAbilityNode[4].struDescNode[0].iValue=0;
			strcpy((char*)lpCompressAbility->struAbilityNode[4].struDescNode[0].byDescribe, "All");

			lpCompressAbility->struAbilityNode[4].struDescNode[1].iValue=5;
			strcpy((char*)lpCompressAbility->struAbilityNode[4].struDescNode[1].byDescribe, "1");

			lpCompressAbility->struAbilityNode[4].struDescNode[2].iValue=6;
			strcpy((char*)lpCompressAbility->struAbilityNode[4].struDescNode[2].byDescribe, "2");

			lpCompressAbility->struAbilityNode[4].struDescNode[3].iValue=7;
			strcpy((char*)lpCompressAbility->struAbilityNode[4].struDescNode[3].byDescribe, "4");

			lpCompressAbility->struAbilityNode[4].struDescNode[4].iValue=8;
			strcpy((char*)lpCompressAbility->struAbilityNode[4].struDescNode[4].byDescribe, "6");

			lpCompressAbility->struAbilityNode[4].struDescNode[5].iValue=9;
			strcpy((char*)lpCompressAbility->struAbilityNode[4].struDescNode[5].byDescribe, "8");

			lpCompressAbility->struAbilityNode[4].struDescNode[6].iValue=10;
			strcpy((char*)lpCompressAbility->struAbilityNode[4].struDescNode[6].byDescribe, "10");

			lpCompressAbility->struAbilityNode[4].struDescNode[7].iValue=11;
			strcpy((char*)lpCompressAbility->struAbilityNode[4].struDescNode[7].byDescribe, "12");

			lpCompressAbility->struAbilityNode[4].struDescNode[8].iValue=12;
			strcpy((char*)lpCompressAbility->struAbilityNode[4].struDescNode[8].byDescribe, "16");

			lpCompressAbility->struAbilityNode[4].struDescNode[9].iValue=13;
			strcpy((char*)lpCompressAbility->struAbilityNode[4].struDescNode[9].byDescribe, "20");

			lpCompressAbility->struAbilityNode[4].struDescNode[10].iValue=14;
			strcpy((char*)lpCompressAbility->struAbilityNode[4].struDescNode[10].byDescribe, "15");

			lpCompressAbility->struAbilityNode[4].struDescNode[11].iValue=15;
			strcpy((char*)lpCompressAbility->struAbilityNode[4].struDescNode[11].byDescribe, "18");

			lpCompressAbility->struAbilityNode[4].struDescNode[12].iValue=16;
			strcpy((char*)lpCompressAbility->struAbilityNode[4].struDescNode[12].byDescribe, "22");

			lpCompressAbility->struAbilityNode[4].struDescNode[13].iValue=17;
			strcpy((char*)lpCompressAbility->struAbilityNode[4].struDescNode[13].byDescribe, "25");

			lpCompressAbility->struAbilityNode[4].struDescNode[14].iValue=18;
			strcpy((char*)lpCompressAbility->struAbilityNode[4].struDescNode[14].byDescribe, "3");

			lpCompressAbility->struAbilityNode[4].struDescNode[15].iValue=19;
			strcpy((char*)lpCompressAbility->struAbilityNode[4].struDescNode[15].byDescribe, "5");

			lpCompressAbility->struAbilityNode[4].struDescNode[16].iValue=20;
			strcpy((char*)lpCompressAbility->struAbilityNode[4].struDescNode[16].byDescribe, "7");

			lpCompressAbility->struAbilityNode[4].struDescNode[17].iValue=21;
			strcpy((char*)lpCompressAbility->struAbilityNode[4].struDescNode[17].byDescribe, "9");

			lpCompressAbility->struAbilityNode[4].struDescNode[18].iValue=22;
			strcpy((char*)lpCompressAbility->struAbilityNode[4].struDescNode[18].byDescribe, "11");

			lpCompressAbility->struAbilityNode[4].struDescNode[19].iValue=23;
			strcpy((char*)lpCompressAbility->struAbilityNode[4].struDescNode[19].byDescribe, "13");

			lpCompressAbility->struAbilityNode[4].struDescNode[20].iValue=24;
			strcpy((char*)lpCompressAbility->struAbilityNode[4].struDescNode[20].byDescribe, "14");

			lpCompressAbility->struAbilityNode[4].struDescNode[21].iValue=25;
			strcpy((char*)lpCompressAbility->struAbilityNode[4].struDescNode[21].byDescribe, "17");

			lpCompressAbility->struAbilityNode[4].struDescNode[22].iValue=26;
			strcpy((char*)lpCompressAbility->struAbilityNode[4].struDescNode[22].byDescribe, "19");

			lpCompressAbility->struAbilityNode[4].struDescNode[23].iValue=27;
			strcpy((char*)lpCompressAbility->struAbilityNode[4].struDescNode[23].byDescribe, "21");

			lpCompressAbility->struAbilityNode[4].struDescNode[24].iValue=28;
			strcpy((char*)lpCompressAbility->struAbilityNode[4].struDescNode[24].byDescribe, "24");

			lpCompressAbility->struAbilityNode[4].struDescNode[25].iValue=29;
			strcpy((char*)lpCompressAbility->struAbilityNode[4].struDescNode[25].byDescribe, "26");

			lpCompressAbility->struAbilityNode[4].struDescNode[26].iValue=30;
			strcpy((char*)lpCompressAbility->struAbilityNode[4].struDescNode[26].byDescribe, "27");

			lpCompressAbility->struAbilityNode[4].struDescNode[27].iValue=31;
			strcpy((char*)lpCompressAbility->struAbilityNode[4].struDescNode[27].byDescribe, "28");

			lpCompressAbility->struAbilityNode[4].struDescNode[28].iValue=32;
			strcpy((char*)lpCompressAbility->struAbilityNode[4].struDescNode[28].byDescribe, "29");

			for(i=1;i<=30;i++){
				lpCompressAbility->struAbilityNode[4].struDescNode[29+i-1].iValue=33+i-1;
				sprintf((char*)lpCompressAbility->struAbilityNode[4].struDescNode[29+i-1].byDescribe,"%d",i+30);
			}

			//!BitRateTypeAbility
			lpCompressAbility->struAbilityNode[5].dwAbilityType=ANTS_MID_BITRATE_TYPE_ABILITY;
			lpCompressAbility->struAbilityNode[5].dwNodeNum=2;
			
			lpCompressAbility->struAbilityNode[5].struDescNode[0].iValue=0;
			strcpy((char*)lpCompressAbility->struAbilityNode[5].struDescNode[0].byDescribe,"VBR");
			
			lpCompressAbility->struAbilityNode[5].struDescNode[5].iValue=1;
			strcpy((char*)lpCompressAbility->struAbilityNode[5].struDescNode[1].byDescribe,"CBR");

			//!BitrateAbility
			lpCompressAbility->struAbilityNode[6].dwAbilityType=ANTS_MID_BITRATE_ABILITY;
			lpCompressAbility->struAbilityNode[6].dwNodeNum=31;

			lpCompressAbility->struAbilityNode[6].struDescNode[0].iValue=1;
			strcpy((char*)lpCompressAbility->struAbilityNode[6].struDescNode[0].byDescribe,"16kps");

			lpCompressAbility->struAbilityNode[6].struDescNode[1].iValue=2;
			strcpy((char*)lpCompressAbility->struAbilityNode[6].struDescNode[1].byDescribe,"32kps");

			lpCompressAbility->struAbilityNode[6].struDescNode[2].iValue=3;
			strcpy((char*)lpCompressAbility->struAbilityNode[6].struDescNode[2].byDescribe,"48kps");

			lpCompressAbility->struAbilityNode[6].struDescNode[3].iValue=4;
			strcpy((char*)lpCompressAbility->struAbilityNode[6].struDescNode[3].byDescribe,"64kps");

			lpCompressAbility->struAbilityNode[6].struDescNode[4].iValue=5;
			strcpy((char*)lpCompressAbility->struAbilityNode[6].struDescNode[4].byDescribe,"80kps");

			lpCompressAbility->struAbilityNode[6].struDescNode[5].iValue=6;
			strcpy((char*)lpCompressAbility->struAbilityNode[6].struDescNode[5].byDescribe,"96kps");

			lpCompressAbility->struAbilityNode[6].struDescNode[6].iValue=7;
			strcpy((char*)lpCompressAbility->struAbilityNode[6].struDescNode[6].byDescribe,"128kps");

			lpCompressAbility->struAbilityNode[6].struDescNode[7].iValue=8;
			strcpy((char*)lpCompressAbility->struAbilityNode[6].struDescNode[7].byDescribe,"160kps");

			lpCompressAbility->struAbilityNode[6].struDescNode[8].iValue=9;
			strcpy((char*)lpCompressAbility->struAbilityNode[6].struDescNode[8].byDescribe,"192kps");

			lpCompressAbility->struAbilityNode[6].struDescNode[9].iValue=10;
			strcpy((char*)lpCompressAbility->struAbilityNode[6].struDescNode[9].byDescribe,"224kps");

			lpCompressAbility->struAbilityNode[6].struDescNode[10].iValue=11;
			strcpy((char*)lpCompressAbility->struAbilityNode[6].struDescNode[10].byDescribe,"256kps");

			lpCompressAbility->struAbilityNode[6].struDescNode[11].iValue=12;
			strcpy((char*)lpCompressAbility->struAbilityNode[6].struDescNode[11].byDescribe,"320kps");

			lpCompressAbility->struAbilityNode[6].struDescNode[12].iValue=13;
			strcpy((char*)lpCompressAbility->struAbilityNode[6].struDescNode[12].byDescribe,"384kps");

			lpCompressAbility->struAbilityNode[6].struDescNode[13].iValue=14;
			strcpy((char*)lpCompressAbility->struAbilityNode[6].struDescNode[13].byDescribe,"448kps");

			lpCompressAbility->struAbilityNode[6].struDescNode[14].iValue=15;
			strcpy((char*)lpCompressAbility->struAbilityNode[6].struDescNode[14].byDescribe,"512kps");

			lpCompressAbility->struAbilityNode[6].struDescNode[15].iValue=16;
			strcpy((char*)lpCompressAbility->struAbilityNode[6].struDescNode[15].byDescribe,"640kps");

			lpCompressAbility->struAbilityNode[6].struDescNode[16].iValue=17;
			strcpy((char*)lpCompressAbility->struAbilityNode[6].struDescNode[16].byDescribe,"768kps");

			lpCompressAbility->struAbilityNode[6].struDescNode[17].iValue=18;
			strcpy((char*)lpCompressAbility->struAbilityNode[6].struDescNode[17].byDescribe,"896kps");

			lpCompressAbility->struAbilityNode[6].struDescNode[18].iValue=19;
			strcpy((char*)lpCompressAbility->struAbilityNode[6].struDescNode[18].byDescribe,"1024kps");

			lpCompressAbility->struAbilityNode[6].struDescNode[19].iValue=20;
			strcpy((char*)lpCompressAbility->struAbilityNode[6].struDescNode[19].byDescribe,"1280kps");

			lpCompressAbility->struAbilityNode[6].struDescNode[20].iValue=21;
			strcpy((char*)lpCompressAbility->struAbilityNode[6].struDescNode[20].byDescribe,"1536kps");

			lpCompressAbility->struAbilityNode[6].struDescNode[21].iValue=22;
			strcpy((char*)lpCompressAbility->struAbilityNode[6].struDescNode[21].byDescribe,"1792kps");

			lpCompressAbility->struAbilityNode[6].struDescNode[22].iValue=23;
			strcpy((char*)lpCompressAbility->struAbilityNode[6].struDescNode[22].byDescribe,"2048kps");

			lpCompressAbility->struAbilityNode[6].struDescNode[23].iValue=24;
			strcpy((char*)lpCompressAbility->struAbilityNode[6].struDescNode[23].byDescribe,"2560kps");

			lpCompressAbility->struAbilityNode[6].struDescNode[24].iValue=25;
			strcpy((char*)lpCompressAbility->struAbilityNode[6].struDescNode[24].byDescribe,"3072kps");

			lpCompressAbility->struAbilityNode[6].struDescNode[25].iValue=26;
			strcpy((char*)lpCompressAbility->struAbilityNode[6].struDescNode[25].byDescribe,"4096kps");

			lpCompressAbility->struAbilityNode[6].struDescNode[26].iValue=27;
			strcpy((char*)lpCompressAbility->struAbilityNode[6].struDescNode[26].byDescribe,"5120kps");

			lpCompressAbility->struAbilityNode[6].struDescNode[27].iValue=28;
			strcpy((char*)lpCompressAbility->struAbilityNode[6].struDescNode[27].byDescribe,"6144kps");

			lpCompressAbility->struAbilityNode[6].struDescNode[28].iValue=29;
			strcpy((char*)lpCompressAbility->struAbilityNode[6].struDescNode[28].byDescribe,"7168kps");

			lpCompressAbility->struAbilityNode[6].struDescNode[29].iValue=30;
			strcpy((char*)lpCompressAbility->struAbilityNode[6].struDescNode[29].byDescribe,"8192kps");

			lpCompressAbility->struAbilityNode[6].struDescNode[30].iValue=-1;
			strcpy((char*)lpCompressAbility->struAbilityNode[6].struDescNode[30].byDescribe,"Self-Define(16-16000kbps)");

			lpCompressAbility->dwAbilityNum=7;
			lpCompressAbility->dwSize=sizeof(ANTS_MID_COMPRESSIONCFG_ABILITY);			
			*lpOutSize=sizeof(ANTS_MID_COMPRESSIONCFG_ABILITY);			
		}		
		break;
		case ANTS_MID_GET_VIDEOEFFECT:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ��ȡ�豸���ȶԱȶ�ɫ�� ,����ͨ����*/			
			/* ���ײ��ȡ�豸���ȶԱȶ�ɫ����Ϣ���Ƶ�lpColorCfg�г�Ա������*/
			LPANTS_MID_COLOR lpColorCfg=(LPANTS_MID_COLOR)lpBuffer;
			
			*lpOutSize=sizeof(ANTS_MID_COLOR);			
		}		
		break;
		case ANTS_MID_GET_MOTIONCFG:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ��ȡ�豸�ƶ������� ,����ͨ����*/			
			/* ���ײ��ȡ�豸�ƶ���������Ϣ���Ƶ�lpMotionCfg�г�Ա������*/
			LPANTS_MID_MOTION lpMotionCfg=(LPANTS_MID_MOTION)lpBuffer;
			
			*lpOutSize=sizeof(ANTS_MID_MOTION);			
		}		
		break;
		case ANTS_MID_GET_MOTIONCFG_V2:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ��ȡ�豸�ƶ������� ,����ͨ���ţ����64·��չ�Ľṹ*/			
			/* ���ײ��ȡ�豸�ƶ���������Ϣ���Ƶ�lpMotionCfg�г�Ա������*/
			LPANTS_MID_MOTION_V2 lpMotionCfg=(LPANTS_MID_MOTION_V2)lpBuffer;
			
			*lpOutSize=sizeof(ANTS_MID_MOTION_V2);			
		}		
		break;
		case ANTS_MID_GET_SHELTERCFG:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ��ȡ�豸��Ƶ�ڵ�������� ,����ͨ����*/			
			/* ���ײ��ȡ�豸��Ƶ�ڵ����������Ϣ���Ƶ�lpShelterCfg�г�Ա������*/
			LPANTS_MID_SHELTERCFG lpShelterCfg=(LPANTS_MID_SHELTERCFG)lpBuffer;
			
			*lpOutSize=sizeof(ANTS_MID_SHELTERCFG);			
		}		
		break;
		case ANTS_MID_GET_HIDEALARMCFG:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ��ȡ�豸��Ƶ�ڵ��������� ,����ͨ����*/			
			/* ���ײ��ȡ�豸��Ƶ�ڵ�����������Ϣ���Ƶ�lpHideAlarmCfg�г�Ա������*/
			LPANTS_MID_HIDEALARM lpHideAlarmCfg=(LPANTS_MID_HIDEALARM)lpBuffer;
			
			*lpOutSize=sizeof(ANTS_MID_HIDEALARM);			
		}		
		break;
		case ANTS_MID_GET_VIDEOLOSTCFG:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ��ȡ�豸��Ƶ��ʧ���� ,����ͨ����*/			
			/* ���ײ��ȡ�豸��Ƶ��ʧ������Ϣ���Ƶ�lpVILostCfg�г�Ա������*/
			LPANTS_MID_VILOST lpVILostCfg=(LPANTS_MID_VILOST)lpBuffer;
						
			*lpOutSize=sizeof(ANTS_MID_VILOST);			
		}		
		break;
		case ANTS_MID_GET_OSDCFG:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ��ȡ�豸��ƵOSD���� ,����ͨ����*/			
			/* ���ײ��ȡ�豸��ƵOSD������Ϣ���Ƶ�lpOsdCfg�г�Ա������*/
			LPANTS_MID_OSDCFG lpOsdCfg=(LPANTS_MID_OSDCFG)lpBuffer;
			memset(lpOsdCfg, 0, sizeof(ANTS_MID_OSDCFG));
			lpOsdCfg->dwSize = sizeof(ANTS_MID_OSDCFG);
			strcpy(lpOsdCfg->sChanName, sysconf->ipcam.vin[0].channel_name);
			lpOsdCfg->dwShowChanName = 1;
			lpOsdCfg->byOSDType = sysconf->ipcam.date_time.date_format.val != 2 ? 
				sysconf->ipcam.date_time.date_format.val : 4;
			lpOsdCfg->dwShowOsd = 1;
			lpOsdCfg->byHourOSDType = sysconf->ipcam.date_time.time_format.val ? 0: 1;
			*lpOutSize=sizeof(ANTS_MID_OSDCFG);			
		}		
		break;
		case ANTS_MID_GET_VIDEOFORMAT:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ��ȡ�豸��Ƶ��ʽ���� ,����ͨ����*/			
			/* ���ײ��ȡ�豸��Ƶ��ʽ������Ϣ���Ƶ�lpVideoFormat�г�Ա������*/
			LPDWORD lpVideoFormat=(LPDWORD)lpBuffer;
							
			*lpOutSize=sizeof(DWORD);			
		}		
		break;
	}
	
	return TRUE;

}
	
	
//!PTZ2������
/*
---------------------??����????2������2?��y?��?��------------------------
@dwChannel			:�����̨�o?,�䨮0?a��?
@dwPtzCommand		:??����?????����?
@dwStop				:??��������?1?������?����??a��??������:0-?a��?1-����?1
@dwSpeed			:??����?????��?��[1-16]
@����???��			:TRUE������?3��1|,FALSE������?����㨹
---------------------------------------------------------
*/
BOOL gfxPtzControl(IN DWORD dwChannel,IN DWORD dwPtzCommand,IN DWORD dwStop,IN DWORD dwSpeed)
{
	switch(dwPtzCommand){
		case ANTS_MID_LIGHT_PWRON:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ?��������?1a��??�� */
		}		
		break;
		case ANTS_MID_WIPER_PWRON:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ?����������?��?a1? */
		}		
		break;
		case ANTS_MID_FAN_PWRON:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ?��������?����?a1? */
		}		
		break;
		case ANTS_MID_HEATER_PWRON:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ?������?������?��?a1? */
		}		
		break;
		case ANTS_MID_AUX_PWRON1:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ?������?��?��������??a1? */
		}		
		break;
		case ANTS_MID_AUX_PWRON2:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ?������?��?��������??a1? */
		}		
		break;
		case ANTS_MID_ZOOM_IN:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ?1?����??��?��SS��?�䨮(��??����?�䨮) */
		}		
		break;
		case ANTS_MID_ZOOM_OUT:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ?1?����??��?��SS��?D?(��??����?D?) */
		}		
		break;
		case ANTS_MID_FOCUS_NEAR:
		{
			//!TODO: Add your message handler code here and/or call default
			 /* ?1��?��??��?��SS?��̡� */
		}		
		break;
		case ANTS_MID_FOCUS_FAR:
		{
			//!TODO: Add your message handler code here and/or call default
			 /* ?1��?��??��?��SSo���̡� */
		}		
		break;
		case ANTS_MID_IRIS_OPEN:
		{
			//!TODO: Add your message handler code here and/or call default
			 /* 1a��|��??��?��SS��?�䨮 */
		}		
		break;
		case ANTS_MID_IRIS_CLOSE:
		{
			//!TODO: Add your message handler code here and/or call default
			 /* 1a��|��??��?��SS??D? */
		}		
		break;
		case ANTS_MID_TILT_UP:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ??������?SS��??��?����??? */
		}		
		break;
		case ANTS_MID_TILT_DOWN:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ??������?SS��??��?��???? */
		}		
		break;
		case ANTS_MID_PAN_LEFT:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ??������?SS��??��?��������a */
		}		
		break;
		case ANTS_MID_PAN_RIGHT:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ??������?SS��??��?��������a */
		}		
		break;
		case ANTS_MID_UP_LEFT:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ??������?SS��??��?����???o��������a */
		}		
		break;
		case ANTS_MID_UP_RIGHT:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ??������?SS��??��?����???o��������a */
		}		
		break;
		case ANTS_MID_DOWN_LEFT:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ??������?SS��??��?��????o��������a */
		}		
		break;
		case ANTS_MID_DOWN_RIGHT:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ??������?SS��??��?��????o��������a */
		}		
		break;
		case ANTS_MID_PAN_AUTO:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ??������?SS��??��?������������??������?�� */
		}		
		break;
		default:
		{	
			//!TODO: Add your message handler code here and/or call default
		}
		break;
	}

	return TRUE;
	
}

/*
---------------------?��??��?2������2?��y?��?��------------------------
@dwChannel			:�����̨�o?,�䨮0?a��?
@dwPresetCommand	:??����?��??��??????����?
@dwPresetIndex		:??����?��??��?D��o?,�䨮1?a��?��??��?��3?255???��??��?
@����???��			:TRUE������?3��1|,FALSE������?����㨹
---------------------------------------------------------
*/
BOOL gfxPtzPreset(IN DWORD dwChannel,IN DWORD dwPresetCommand,IN DWORD dwPresetIndex)
{
	switch(dwPresetCommand){
		case ANTS_MID_SET_PRESET:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ����???��??��? */
		}		
		break;
		case ANTS_MID_CLE_PRESET:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ??3y?��??��? */
		}		
		break;
		case ANTS_MID_GOTO_PRESET:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ?��?����a��??��??��? */
		}		
		break;
	}

	return TRUE;
	
}

/*
---------------------1��?��2������2?��y?��?��------------------------
@dwChannel			:�����̨�o?,�䨮0?a��?
@dwTrackCommand	:??����1��?��?����?
@����???��			:TRUE������?3��1|,FALSE������?����㨹
---------------------------------------------------------
*/	
BOOL gfxPtzTrack(IN DWORD dwChannel,IN DWORD dwTrackCommand)
{
	switch(dwTrackCommand){
		case ANTS_MID_STA_MEM_CRUISE:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ?a��?????1��?�� */
		}		
		break;
		case ANTS_MID_STO_MEM_CRUISE:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ����?1????1��?�� */
		}		
		break;
		case ANTS_MID_RUN_CRUISE:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ?a��?1��?�� */
		}		
		break;
	}

	return TRUE;
	
}
	
/*
---------------------?2o?2������2?��y?��?��------------------------
@dwChannel			:�����̨�o?,�䨮0?a��?
@dwCruiseCommand	:??����?2o??����?
@byCruiseRoute		:??����?2o??��??,��??��?��3?32?��??(D��o?�䨮1?a��?)
@byCruisePoint		:??����?2o?��?,��??��?��3?32??��?(D��o?�䨮1?a��?)
@wInput				:??����2?��??2o??����?������??��2?��?,?��??��?(��?�䨮255),����??(��?�䨮255),?��?��(��?�䨮40)
@����???��			:TRUE������?3��1|,FALSE������?����㨹
---------------------------------------------------------
*/	
BOOL gfxPtzCruise(IN DWORD dwChannel,IN DWORD dwCruiseCommand,IN BYTE byCruiseRoute,IN BYTE byCruisePoint,IN WORD wInput)
{
	switch(dwCruiseCommand){
		case ANTS_MID_FILL_PRE_SEQ:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ???��??��??����??2o?D����D */
		}		
		break;
		case ANTS_MID_SET_SEQ_DWELL:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ����???2o?��?����?������?? */
		}		
		break;
		case ANTS_MID_SET_SEQ_SPEED:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ����???2o??��?�� */
		}		
		break;
		case ANTS_MID_CLE_PRE_SEQ:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ???��??��?�䨮?2o?D����D?D��?3y */
		}		
		break;
		case ANTS_MID_RUN_SEQ:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ?a��??2o? */
		}		
		break;
		case ANTS_MID_STOP_SEQ:
		{
			//!TODO: Add your message handler code here and/or call default
			/* ����?1?2o? */
		}		
		break;
	}

	return TRUE;
	
}
	
/*
---------------------��??�¨����̨�2������2?��y?��?��------------------------
@dwChannel			:�����̨�o?,�䨮0?a��?
@lpBuffer				:��??�¨����̨���?��??o3???????
@dwSize				:��??�¨����̨���?��??o3???3��?��
@����???��			:TRUE������?3��1|,FALSE������?����㨹
---------------------------------------------------------
*/	
BOOL gfxPtzTrans(IN DWORD dwChannel,IN BYTE *lpBuffer,IN DWORD dwSize)
{
	if(lpBuffer==NULL||dwSize<=0)
		return FALSE;

	//!TODO: Add your message handler code here and/or call default	
	return TRUE;
}
	
/*
---------------------3D?��??2?��y?��?��------------------------
@dwChannel			:�����̨�o?,�䨮0?a��?
@dwXPoint			:3D?��??????��??����?����?��x [1-1000]
@dwYPoint			:3D?��??��1?����??����?����?��y [1-1000]
@dwScale			:3D?��??��???����?��[-35,35]
@����???��			:TRUE������?3��1|,FALSE������?����㨹
---------------------------------------------------------
*/	
BOOL gfxPtz3D(IN DWORD dwChannel,IN DWORD dwXPoint,IN DWORD dwYPoint,IN DWORD dwScale)
{
	return TRUE;
}
	
/*
---------------------??��??2o??��???��o?2?��y?��?��------------------------
@dwChannel			:�����̨�o?,�䨮0?a��?
@byCruiseRoute		:??����?2o??��??,��??��?��3?32?��??(D��o?�䨮1?a��?)
@lpCruiseRet			:??����?2o?��??��o?
@����???��			:TRUE������?3��1|,FALSE������?����㨹
---------------------------------------------------------
*/	
BOOL gfxGetPtzCruise(IN DWORD dwChannel,IN DWORD dwCruiseRoute,OUT LPANTS_MID_CRUISE_RET lpCruiseRet)
{
	return FALSE;
}
	
//!JPEGץͼ����
/*
---------------------JPEGץͼ����˵��------------------------
@dwChannel			:ͨ����,��0��ʼ
@lpJpegParam			: JPEGͼ����� 
@lpBuffer				:����JPEG���ݵĻ����� 
@dwInSize			:���뻺������С 
@lpOutSize			:����ͼƬ���ݵĴ�С 
@����ֵ 		:TRUE��ʾ�ɹ�,FALSE��ʾʧ��
---------------------------------------------------------
*/	
static BOOL gfxCaptureJpeg(IN DWORD dwChannel,IN LPANTS_MID_JPEGPARA lpJpegParam,OUT BYTE *lpBuffer,IN DWORD dwInSize,OUT DWORD *lpOutSize)
{
	FILE *jpeg_fid = NULL;
	int jpeg_size = 0;
	char jpeg_filename[128] = {""};
	int jpeg_width = 0, jpeg_height = 0;
	BOOL capture_success = FALSE;
	
	
	switch(lpJpegParam->wPicSize){
		//!ͼƬ�ߴ磺0-CIF��1-QCIF��2-D1��3-UXGA(1600x1200)��
			//!4-SVGA(800x600)��5-HD720p(1280x720)��6-VGA��7-XVGA��8-HD900p��
			//!9-HD1080��10-2560*1920��11-1600*304��12-2048*1536��13-2448*2048��
			//!14-2448*1200��15-2448*800��16-XGA(1024*768)��17-SXGA(1280*1024)��18-WD1(960*576/960*480),19-1080i 
		case 0: jpeg_width = 352; jpeg_height = 240; break;
		case 1: jpeg_width = 176; jpeg_height = 120; break;
		case 2: jpeg_width = 720; jpeg_height = 480; break;
		case 3: jpeg_width = 1600; jpeg_height = 1200; break;
		case 4: jpeg_width = 800; jpeg_height = 600; break;
		case 5: jpeg_width = 1280; jpeg_height = 720; break;
		case 6: jpeg_width = 640; jpeg_height = 480; break;
		case 7: jpeg_width = 1024; jpeg_height = 768; break;
		case 8: jpeg_width = 1600; jpeg_height = 900; break;
		case 9: jpeg_width = 1920; jpeg_height = 1080; break;
		case 10: jpeg_width = 2560; jpeg_height = 1920; break;
		case 11: jpeg_width = 1600; jpeg_height = 304; break;
		case 12: jpeg_width = 2048; jpeg_height = 1536; break;
		case 13: jpeg_width = 2448; jpeg_height = 2048; break;
		case 14: jpeg_width = 2448; jpeg_height = 1200; break;
		case 15: jpeg_width = 2448; jpeg_height = 800; break;
		case 16: jpeg_width = 1024; jpeg_height = 768; break;
		case 17: jpeg_width = 1280; jpeg_height = 1024; break;
		case 18: jpeg_width = 960; jpeg_height = 480; break;
		case 19: jpeg_width = 1920; jpeg_height = 640; break;
	}
	
	snprintf(jpeg_filename, sizeof(jpeg_filename), "/tmp/ants_capture_%d_%d.jpg", time(NULL), dwChannel);
	*lpOutSize = 0; // out size 0 mean not success
	jpeg_fid = fopen(jpeg_filename, "w+b");
	if(jpeg_fid){
		//if(0 == SDK_VENC_snapshot(dwChannel, jpeg_width, jpeg_height, jpeg_fid)){
		if(0 == sdk_enc->snapshot(dwChannel, kSDK_ENC_SNAPSHOT_QUALITY_HIGHEST, jpeg_width, jpeg_height, jpeg_fid)){
			GET_FILE_SIZE(jpeg_filename, jpeg_size);
			if(jpeg_size < dwInSize){
				void *out_ptr = lpBuffer;
				int read_size = 0;
				
				fseek(jpeg_fid, 0, SEEK_SET);
				while((read_size = fread(out_ptr, 1, 1024, jpeg_fid)) > 0){
					out_ptr += read_size;
				}

				*lpOutSize = jpeg_size;
			}
		}
		fclose(jpeg_fid);
		jpeg_fid = NULL;

		unlink(jpeg_filename);
		remove(jpeg_filename);
	}
	return *lpOutSize > 0 ? TRUE : FALSE;
}


//!??��?�����̨����̨���?��??����?�����??����D??��
/*
---------------------JPEG������?2?��y?��?��------------------------
@dwChannel			:�����̨�o?,�䨮0?a��?
@lpMainStreamBitRate	: ?��??���¨��̨���???��D??��
@lpSubStreamBitRate	:����??���¨��̨���???��D??�� 
@����???��			:TRUE������?3��1|,FALSE������?����㨹
---------------------------------------------------------
*/	
BOOL gfxGetChannelBitRate(IN DWORD dwChannel,OUT DWORD *lpMainStreamBitRate,OUT DWORD *lpSubStreamBitRate)
{
	return FALSE;
}

//!����?����?3?���䨬???��?��?����??
/*
---------------------??��?������?����?����?3?2?��y?��?��------------------------
@lpAlarmOutStatus		:����?����?3?���䨬?
@����???��			:TRUE������?3��1|,FALSE������?����㨹
---------------------------------------------------------
*/	
BOOL gfxGetAlarmOut(OUT LPANTS_MID_ALARMOUTSTATUS lpAlarmOutStatus)
{
	return FALSE;
}

/*
---------------------??��?������?����?����?3?2?��y?��?��------------------------
@lAlarmOutPort		:����?����?3??��.3?��?��?3??���䨮0?a��?,0x00ff������?��?2??��?a��?3?,0xff00������?��?2?��y��?��?3?
@lAlarmOutStatic		:����?����?3?���䨬?:0��-����?1��?3?,1��-��?3? 
@����???��			:TRUE������?3��1|,FALSE������?����㨹
---------------------------------------------------------
*/	
BOOL gfxSetAlarmOut(IN LONG lAlarmOutPort,IN LONG lAlarmOutStatic)
{
	return FALSE;
}
	
BOOL gfxGetAlarmInfo(OUT DWORD *lpCommand,OUT BYTE *lpBuffer,OUT DWORD *lpBufLen)
{
	return FALSE;	
}
	
//!???22������
LONG gfxStartVoice(IN DWORD dwVoiceChannel,IN BOOL bNeedCBNOEncData,IN fVoiceStreamCallBack fVoiceStream,IN void *lpUser)
{
	return FALSE;
}

BOOL gfxStopVoice(IN LONG lVoiceHandle)
{
	return FALSE;
	
}
	
BOOL gfxSendVoiceData(IN LONG lVoiceHandle,IN BYTE *lpBuffer,IN DWORD dwSize)
{
	return FALSE;
}
	
//!�豸����/�ػ�/�ָ�Ĭ��ֵ/�����������
BOOL gfxReboot( )
{	
	return FALSE;
}


//!��ȡSDK�汾��������
static DWORD gfxGetSDKVersion( )
{
	return 0x20121205;
}


DWORD gfxGetLastError( )
{
	return 0;
}

static void* antslib_streamer(void* arg)
{
	int ret = 0;
	lpMEDIABUF_USER user = NULL;

	user = MEDIABUF_attach(0);

	if(NULL != user){
		MEDIABUF_sync(user);
		while(_ants_server.streamer_trigger){
			bool out_success = false;
			if(0 == MEDIABUF_out_lock(user)){
				const lpSDK_ENC_BUF_ATTR attr = NULL;
				size_t out_size = 0;
				// out a frame from media buf
				if(0 == MEDIABUF_out(user, &attr, NULL, &out_size)){
					void* const raw_data = (void*)(attr + 1);
					size_t const raw_size = attr->data_sz;
					BYTE const frame_type = attr->h264.keyframe ? AntsMidPktIFrames : AntsMidPktPFrames;
					DWORD const timestamp_second = attr->timestamp_us / 1000000;
					DWORD const timestamp_usecond = attr->timestamp_us % 1000000;

					out_success = true;
					ANTS_SERVER_InputDataV3(AntsMidMainStream, 0, raw_data, raw_size, frame_type,
						attr->h264.width, attr->h264.height, timestamp_second, timestamp_usecond);

					//APP_TRACE("ANTS buffering %d keyframe = %s", raw_size, AntsMidPktIFrames == frame_type ? "yes" : "no");
					
				}
				MEDIABUF_out_unlock(user);
			}

			if(!out_success){
				usleep(40000);
			}
		}
		MEDIABUF_detach(user);
	}
	pthread_exit(NULL);
}


static void* antslib_streamer_sub(void* arg)
{
	int ret = 0;
	lpMEDIABUF_USER* user = NULL;

	int mediabuf_ch = MEDIABUF_lookup_byname("360p.264");
	if(mediabuf_ch < 0){
		mediabuf_ch = MEDIABUF_lookup_byname("qvga.264");
	}
	if(mediabuf_ch >= 0){
		user = MEDIABUF_attach(mediabuf_ch);
	}
	
	if(NULL != user){
		MEDIABUF_sync(user);
		while(_ants_server.streamer_trigger){
			bool out_success = false;
			if(0 == MEDIABUF_out_lock(user)){
				const lpSDK_ENC_BUF_ATTR attr = NULL;
				size_t out_size = 0;
				// out a frame from media buf
				if(0 == MEDIABUF_out(user, &attr, NULL, &out_size)){
					void* const raw_data = (void*)(attr + 1);
					size_t const raw_size = attr->data_sz;
					BYTE const frame_type = attr->h264.keyframe ? AntsMidPktIFrames : AntsMidPktPFrames;
					DWORD const timestamp_second = attr->timestamp_us / 1000000;
					DWORD const timestamp_usecond = attr->timestamp_us % 1000000;

					out_success = true;
					ANTS_SERVER_InputDataV3(AntsMidSubStream, 0, raw_data, raw_size, frame_type,
						attr->h264.width, attr->h264.height, timestamp_second, timestamp_usecond);

					//APP_TRACE("ANTS buffering %d keyframe = %s", raw_size, AntsMidPktIFrames == frame_type ? "yes" : "no");
					
				}
				MEDIABUF_out_unlock(user);
			}

			if(!out_success){
				usleep(40000);
			}
		}
		MEDIABUF_detach(user);
	}
	pthread_exit(NULL);
}


static void antslib_streamer_trigger()
{
	int ret = 0;
	if(!_ants_server.streamer_main_tid){
		_ants_server.streamer_trigger = true;
		ret = pthread_create(&_ants_server.streamer_main_tid, NULL, antslib_streamer, NULL);
		APP_ASSERT(0 == ret, "ANTS create streamer failed!");
	}
	if(!_ants_server.streamer_sub_tid){
		_ants_server.streamer_trigger = true;
		ret = pthread_create(&_ants_server.streamer_sub_tid, NULL, antslib_streamer_sub, NULL);
		APP_ASSERT(0 == ret, "ANTS create streamer sub failed!");
	}
}

static void antslib_streamer_quit()
{
	int ret = 0;
	if(_ants_server.streamer_main_tid){
		_ants_server.streamer_trigger = false;
		pthread_join(_ants_server.streamer_main_tid, NULL);
		_ants_server.streamer_main_tid = (pthread_t)NULL;
	}
	if(_ants_server.streamer_sub_tid){
		_ants_server.streamer_trigger = false;
		pthread_join(_ants_server.streamer_sub_tid, NULL);
		_ants_server.streamer_sub_tid = (pthread_t)NULL;
	}
}

int ANTSLIB_init(const char* eth, in_port_t port)
{
	BOOL bExit=FALSE;
	ANTS_SERVER_PARAM struServerParam;
	ANTS_SERVER_FUNCTION_CONFIG struFunctionCfg;

	strcpy(_ants_server.server_eth, eth);
	_ants_server.server_port = port;

	memset(&struServerParam,0,sizeof(ANTS_SERVER_PARAM));
	memset(&struFunctionCfg,0,sizeof(ANTS_SERVER_FUNCTION_CONFIG));

	struServerParam.dwLogDest=AntsLog2Console;
	struServerParam.dwListenPort = port ? port : 5050;
	struServerParam.dwMainStreamBufferNum=64;	
	struServerParam.dwSubStreamBufferNum=64;	
	struServerParam.dwMainStreamBlockSize=512*1024;
	struServerParam.dwSubStreamBlockSize=512*1024;

	struFunctionCfg.fpSetParameter=gfxSetParameter;
	struFunctionCfg.fpGetParameter=gfxGetParameter;
	struFunctionCfg.fpPtzControl=gfxPtzControl;
	struFunctionCfg.fpPtzPreset=gfxPtzPreset;
	struFunctionCfg.fpPtzTrack=gfxPtzTrack;
	struFunctionCfg.fpPtzCruise=gfxPtzCruise;
	struFunctionCfg.fpPtzTrans=gfxPtzTrans;
	struFunctionCfg.fpPtz3D=gfxPtz3D;
	struFunctionCfg.fpGetPtzCruise=gfxGetPtzCruise;
	struFunctionCfg.fpCaptureJpeg=gfxCaptureJpeg;
	struFunctionCfg.fpGetChannelBitRate=gfxGetChannelBitRate;
	struFunctionCfg.fpGetAlarmInfo=gfxGetAlarmInfo;

	struFunctionCfg.fpGetAlarmOut=gfxGetAlarmOut;
	struFunctionCfg.fpSetAlarmOut=gfxSetAlarmOut;
	struFunctionCfg.fpStartVoice=gfxStartVoice;
	struFunctionCfg.fpStopVoice=gfxStopVoice;
	struFunctionCfg.fpSendVoiceData=gfxSendVoiceData;
	struFunctionCfg.fpReboot=gfxReboot;
	struFunctionCfg.fpGetSDKVersion=gfxGetSDKVersion;
	struFunctionCfg.fpGetLastError=gfxGetLastError;
	
	APP_TRACE("[IPC][%s:%08d:%s] Initialize Begin",__FILE__,__LINE__,__FUNCTION__);	
	ANTS_SERVER_Initialize(&struServerParam,&struFunctionCfg);
	APP_TRACE("[IPC][%s:%08d:%s] Initialize",__FILE__,__LINE__,__FUNCTION__);
	APP_TRACE("[IPC][%s:%08d:%s] Initialize End",__FILE__,__LINE__,__FUNCTION__);

	antslib_streamer_trigger();

	return 0;
}

void ANTSLIB_destroy()
{
	antslib_streamer_quit();
	ANTS_SERVER_Release();
	APP_TRACE("[IPC][%s:%08d:%s] Release End",__FILE__,__LINE__,__FUNCTION__);
}

