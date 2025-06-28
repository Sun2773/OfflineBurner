/******************/
/*****升级信息*****/
/******************/

/*
 * 版本号说明
 * [日期-年].[日期-月].[日期-日].[修订版本]+[测试版本]
 * 日期         当前版本发布日期，在功能性版本更新时更新此字段
 * 修订版本     功能不变，对参数或BUG等问题进行修改的情况更新此字段
 * 测试版本     对于一些版本的测试的序号，不对外发布，只在进行测试的情况更新此字段
 */

#define PROJECT_NAME   "Burner"   // 工程名称
#define SYSTEM_VERSION "0.01"     // 系统版本
#define DEVICE_TYPE    ""         // 设备型号
#ifdef DEBUG                      // 调试版本尾缀
#define SYSTEM_VERSION_SUFFIX "-Alpha"
#else
#define SYSTEM_VERSION_SUFFIX
#endif
