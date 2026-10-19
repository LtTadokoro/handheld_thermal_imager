#ifndef _CONFIG_H_
#define _CONFIG_H_

#define VL53L1X     // 不装激光测距模块就取消注释

#define IPS_screen  // 若买的TFT屏幕就取消注释

#ifndef IPS_screen
#define TFT_screen
#endif

#endif