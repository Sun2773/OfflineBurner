/******************/
/*****������Ϣ*****/
/******************/

/*
 * �汾��˵��
 * [����-��].[����-��].[����-��].[�޶��汾]+[���԰汾]
 * ����         ��ǰ�汾�������ڣ��ڹ����԰汾����ʱ���´��ֶ�
 * �޶��汾     ���ܲ��䣬�Բ�����BUG����������޸ĵ�������´��ֶ�
 * ���԰汾     ����һЩ�汾�Ĳ��Ե���ţ������ⷢ����ֻ�ڽ��в��Ե�������´��ֶ�
 */

#define PROJECT_NAME   "Offline"   // ��������
#define SYSTEM_VERSION "0.01"      // ϵͳ�汾
#define DEVICE_TYPE    ""          // �豸�ͺ�
#ifdef DEBUG                       // ���԰汾β׺
#define SYSTEM_VERSION_SUFFIX "-Alpha"
#else
#define SYSTEM_VERSION_SUFFIX
#endif
