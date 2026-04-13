#ifndef GT1158_H
#define GT1158_H

#define GT_CTRL_REG 	        0X8040  /* GT9147控制寄存器         */
#define GT_MODSW_REG 	        0X804D  /* GT9147模式切换寄存器        */
#define GT_9xx_CFGS_REG         0X8047  /* GT9147配置起始地址寄存器    */
#define GT_1xx_CFGS_REG         0X8050  /* GT1151配置起始地址寄存器    */
#define GT_CHECK_REG 	        0X80FF  /* GT9147校验和寄存器       */
#define Config_Fresh_reg        0x8100  /* 配置更新标记*/
#define GT_PID_REG 		        0X8140  /* GT9147产品ID寄存器       */


#define GT_GSTID_REG 	        0X814E  /* GT9147当前检测到的触摸情况 */
#define GT_TP1_REG 		        0X814F  /* 第一个触摸点数据地址 */
#define GT_TP2_REG 		        0X8157	/* 第二个触摸点数据地址 */
#define GT_TP3_REG 		        0X815F  /* 第三个触摸点数据地址 */
#define GT_TP4_REG 		        0X8167  /* 第四个触摸点数据地址  */
#define GT_TP5_REG 		        0X816F	/* 第五个触摸点数据地址   */
#define MAX_SUPPORT_POINTS      5       /* 最多5点电容触摸 */


#define GT_REG_DATA_WIDTH             8    /* GT911寄存器的位宽 */
#define BUFFER_SIZE    (GT_REG_DATA_WIDTH * MAX_SUPPORT_POINTS)  /* 读取寄存器时的缓存大小 */


#endif // !GT1158_H


