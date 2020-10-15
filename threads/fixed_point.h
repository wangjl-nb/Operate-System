#ifndef __THREAD_FIXED_POINT_H
  #define __THREAD_FIXED_POINT_H
  
 /* 题目要求2^14，移位数即为14 */
 #define FP_SHIFT_AMOUNT 14
 /* 整数转化为浮点数 */
 #define INT_TO_FP(A) ((A << FP_SHIFT_AMOUNT))
 /* 浮点数相加 */
 #define FP_ADD_FP(A,B) (A + B)
 /* 浮点数加整数（第二个数为整数） */
 #define FP_ADD_INT(A,B) (A + (B << FP_SHIFT_AMOUNT))
 /* 浮点数相减 */
 #define FP_SUB_FP(A,B) (A - B)
 /* 浮点数减整数（第二个数为整数） */
 #define FP_SUB_INT(A,B) (A - (B << FP_SHIFT_AMOUNT))
 /* 浮点数相乘 */
 #define FP_MULT_FP(A,B) ((((int64_t) A) * B >> FP_SHIFT_AMOUNT))
 /* 浮点数乘整数（第二个数为整数） */
 #define FP_MULT_INT(A,B) (A * B)
 /* 浮点数除以浮点数 */
 #define FP_DIV_FP(A,B) (((((int64_t) A) << FP_SHIFT_AMOUNT) / B))
 /* 浮点数除以整数（第二个数为整数） */
 #define FP_DIV_INT(A,B) (A / B)
 /* 截断，即获取浮点数整数部分 */
 #define FP_TRUNCATE(A) (A >> FP_SHIFT_AMOUNT)
 /* 四舍五入到最近的整数 */
 #define FP_ROUND(A) (A >= 0 ? ((A + (1 << (FP_SHIFT_AMOUNT - 1))) >> FP_SHIFT_AMOUNT) \
         : ((A - (1 << (FP_SHIFT_AMOUNT - 1))) >> FP_SHIFT_AMOUNT))
 
 #endif /* thread/fixed_point.h */