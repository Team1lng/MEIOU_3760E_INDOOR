#ifndef _PRODUCT_TEST_UTILS_H_
#define _PRODUCT_TEST_UTILS_H_
enum {
    ePRODUCT_WRITE_TEMP,
    ePRODUCT_WRITE_PERSIST,
};
extern int product_test_utils_write_int(char *key,int value,int mode);
extern int product_test_utils_read_int(char *key,int *value);
extern int product_test_utils_write_string(char *key,char *value,int mode);
extern int  product_test_utils_read_string(char *key,char *value);
extern int product_test_exec_cmd_with_return(const char *pCmd, char cmdOutput[], int cmdOutMaxLen);
extern int product_test_exec_cmd_no_return (const char *format, ...);

extern int product_test_file_read(char *path,char *buf,int size);
extern int product_test_file_write(char *path,char *buf,int size);

extern int product_test_file_is_exists(char *path);
extern int product_test_file_size(char *path, int *size);


#define  LESS_TARGET(a,b,c) ((a-b) < c) ? 1: 0
#define  MORE_TARGET(a,b,c) ((a-b) >= c) ? 1: 0

extern unsigned long product_test_get_curtime(void);

#endif
