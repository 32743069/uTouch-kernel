����������CT69X��ϵ��оƬ������ʵ����ʹ��оƬ�������޸ģ�
1.IC�ͺ�ΪCT690��CHIP_IDΪB8����CT69x_ts_probe�������ж�Ϊif(reg_value != 0xB8)��
  ICΪ�͵�ƽ��λ���޸İ弶�ļ�:
      #define TOUCH_RST_VALUE GPIO_HIGH
  ct69x_ts_init_platform_hw ������ ���õ�reset_pin��50ms�����ø�reset_pin��
2.��CT690������IC�ͺţ�CHIP_IDΪA8��tpd_probe�������ж�Ϊif(reg_value != 0xA8)��
  ICΪ�ߵ�ƽ��λ���޸İ弶�ļ�:
      #define TOUCH_RST_VALUE GPIO_LOW
  ct69x_ts_init_platform_hw �����У����ø�reset_pin��50ms�����õ�reset_pin��