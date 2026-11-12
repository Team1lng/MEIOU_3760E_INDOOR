#ifndef _PTAL_GDL_H_
#define _PTAL_GDL_H_

#ifdef __cplusplus__
extern "C" {
#endif

extern int ptal_gdl_file_anyalysis(char  *file);
extern int  ptal_read_acoustics_setting(char *acoustics_setting, char *acoustics_setting2);
extern int  ptal_write_acoustics_setting(char *acoustics_setting);

#ifdef __cplusplus__
}
#endif
#endif
