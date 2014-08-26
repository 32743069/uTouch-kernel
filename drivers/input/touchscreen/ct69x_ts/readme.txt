本驱动兼容CT69X的系列芯片，根据实际所使用芯片做以下修改：
1.IC型号为CT690，CHIP_ID为B8，在CT69x_ts_probe函数中判断为if(reg_value != 0xB8)。
  IC为低电平复位，修改板级文件:
      #define TOUCH_RST_VALUE GPIO_HIGH
  ct69x_ts_init_platform_hw 函数中 先置低reset_pin，50ms后，在置高reset_pin。
2.除CT690的其余IC型号，CHIP_ID为A8，tpd_probe函数中判断为if(reg_value != 0xA8)。
  IC为高电平复位，修改板级文件:
      #define TOUCH_RST_VALUE GPIO_LOW
  ct69x_ts_init_platform_hw 函数中，先置高reset_pin，50ms后，在置低reset_pin。