#ifndef CONFIG_H
#define CONFIG_H

// TODO:保证目录存在
//  Soc平台，可读写目录
#define PLATFORM_RW_DIR "/mnt/UDISK/"
// 厂家目录，一般不可随意"写"操作
#define ALIEN_CELL_RW_DIR PLATFORM_RW_DIR "AlienCell/"

// OTA文件存放目录
#define ALIEN_CELL_OTA_RW_DIR ALIEN_CELL_RW_DIR "OTA/"
// 配置文件存放目录
#define ALIEN_CELL_CONFIG_RW_DIR ALIEN_CELL_RW_DIR "config/"

// 日志文件存放目录
#define ALIEN_CELL_LOG_RW_DIR PLATFORM_RW_DIR "log/aliend/"

// AlienCell APP分区目录
#define ALIEN_CELL_APP_DIR "/mnt/app/"
// AlienCell 相关可执行文件目录
#define ALIEN_CELL_APP_BIN_DIR ALIEN_CELL_APP_DIR "bin/"
// AlienCell MCU(bin文件)目录
#define ALIEN_CELL_MCU_DIR ALIEN_CELL_APP_DIR "mcu/"
// AlienCell 配置文件目录
#define ALIEN_CELL_CONFIG_DIR ALIEN_CELL_APP_DIR "config/"

constexpr int MQ_PORT = 6655; // aliend Port(publisher)端绑定的PORT,发送给alien_ui  TODO:alien-ui需要增加connect端口
constexpr int AS_PORT = 6677;     // aliend Port(publisher)端绑定的PORT,发送给alien_service
constexpr int ALIEND_PORT = 6666; // aliend(subscriber)端监听来自alien-service消息的端口

// 与klipper RPC通信接口
#define KLIPPER_SOCKET "/tmp/klipper.sock"
// 在文件系统中，标识当前是否在升级状态。
#define FLAG_FILE_UPGRADE ALIEN_CELL_OTA_RW_DIR "upgrade.flag"

// OTA包.upm文件
#define OTA_ORI_UPM_FILE ALIEN_CELL_OTA_RW_DIR "ota_ori.upm"

#define TEST_OTA

#endif // CONFIG_H