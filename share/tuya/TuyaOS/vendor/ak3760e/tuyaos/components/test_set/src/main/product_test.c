/*
 *  @copyright Copyright (c) tuya.inc 2021
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "list.h"
#include "tb_log.h"
#include "interface_tcp.h"
#include "interface_uart.h"
#include "product_test.h"
#include "product_test_utils.h"
#include "product_test_protocol.h"
#include "product_test_items.h"

#define LOG_LEVEL_KEY "product_test_debug"
#define DEF_WAIT_TIMEOUT (3)

enum {
    E_FD_RECV,
    E_FD_LISTEN,
};

typedef struct com_parm_s {
    struct list_head list;
    int type; 
    int *fd;
    COM_INIT_T init;
    COM_UINIT_T uinit;
    COM_SEND_T send;
    COM_RECV_T recv;
}COM_PARM_T;

typedef struct pt_parm_s
{
    struct list_head list;

    PT_COMPLETE_T complete;
    PT_ENCODE_T encode;
    PT_DECODE_T decode;
}PT_PARM_T;

typedef struct list_fd_s {
    struct list_head list;
    int fd;
    int type;
    COM_PARM_T *com;
}LIST_FD_T;


typedef struct product_test_param_s
{
    int type;
    int list_init;
    int bsize;
    int pt_cnts;
    int com_cnts;
    uint8_t *buf;
    PT_HBUF_T hbuf;
    PT_FBUF_T fbuf;  
    PT_FBUF_T sbuf;  

    int exit_prod;
    pthread_t thread;
    struct list_head fd_head;
    struct list_head pt_head;
    struct list_head com_head;
}PRODUCT_TEST_PARAM_T;

static PRODUCT_TEST_PARAM_T sg_product_test_parm = {0};
static int sg_product_test_init_flag = 0;
static int sg_wait_seconds = DEF_WAIT_TIMEOUT;
static int sg_wait_exit = 0;

int global_log_level = LEVEL_OVER_LOGW;

static  ENTER_HOOK sg_enter_hook = NULL;

int product_test_register_enter_hook(ENTER_HOOK hook)
{
    if (hook) {
        sg_enter_hook = hook;
    }
    return 0;
}

int product_test_set_wait_times(int seconds)
{
    sg_wait_seconds = seconds;
    sg_wait_exit  = 1;

    return 0;
}

int product_test_get_log_level(int *log) 
{
    int value=0;

    if(product_test_utils_read_int(LOG_LEVEL_KEY,&value)==0) {
        global_log_level = value;
    }

    if(log)
       *log = global_log_level;
    
    printf("current log level:%d\n",global_log_level);

    return 0;
}

int product_test_set_log_level(int log)
{
    if(product_test_utils_write_int(LOG_LEVEL_KEY,log,0)==0) {
        global_log_level = log;
    }

    printf("current log level:%d\n",global_log_level);

    return 0;

}

static void __dump_data(unsigned char *buf,int len)
{
    int i;
    printf("dump %d bytes\n",len);
    printf("###########################################\n");
    for(i=0;i<len;i++) {
        printf("%02x ",buf[i]); 
    }
    printf("\n");
    printf("###########################################\n");

}

static int  __fd_add_in_fds(struct list_head*head,fd_set *fdset) {

    int fd = 0;
    LIST_FD_T *flist;

    list_for_each_entry(flist,head,list) {
        FD_SET(flist->fd,fdset);
        if(fd < flist->fd) {
            fd = flist->fd; 
        }
    }

    return fd;
}

static void __fd_del_in_list(LIST_FD_T *lfd)
{
    product_test_list_del(&lfd->list);
    free(lfd);
}

int __attribute__((weak)) product_test_is_entered(void)
{
    TB_LOGD();
    return 0;
}

static void* product_test_thread(void *arg)
{
    int iret = 0;
    LIST_FD_T *flist,*_flist;
    PT_PARM_T *pt,*_pt;
    PRODUCT_TEST_PARAM_T *fparm = (PRODUCT_TEST_PARAM_T *) arg;

    if(fparm == NULL) {
        TB_LOGE("invalid parm");
        return NULL;    
    }

    fparm->exit_prod = 0;

    while(!fparm->exit_prod) {
        int fd;
        fd_set fds; 
        struct timeval timeout; 
        FD_ZERO(&fds);
        if((fd = __fd_add_in_fds(&fparm->fd_head,&fds)) <= 0) {
            TB_LOGE("fd_set fail"); 
            return NULL;    
        }

        timeout.tv_sec  = sg_wait_seconds;
        timeout.tv_usec = 0;
        iret = select(fd + 1, &fds, NULL, NULL, &timeout);
        if(iret < 0) {
            TB_LOGE("select error : %s\n",strerror(errno)); 
            continue;
        }else if(iret == 0) {
            if(!sg_wait_exit) {
                continue;
            }

            if(product_test_is_entered()) {
                sg_wait_exit = 0;
                sg_wait_seconds = DEF_WAIT_TIMEOUT;
                TB_LOGD("== product test entered ==");
                continue;
            } else {
                break; 
            }
        }

        list_for_each_entry_safe(flist,_flist,&fparm->fd_head,list) {
            LIST_FD_T *fd;
            if(!FD_ISSET(flist->fd,&fds)) 
                continue;
            if(flist->type == E_FD_RECV) {
                PT_FBUF_T *fbuf = &fparm->fbuf;
                PT_HBUF_T *hbuf = &fparm->hbuf;
                PT_FBUF_T *sbuf = &fparm->sbuf;
                memset(sbuf->buf,0,fparm->bsize);
                memset(hbuf->data,0,fparm->bsize);
		TB_LOGD("bsize:%x",fparm->bsize);
                if((iret = flist->com->recv(flist->fd,(char *)fparm->sbuf.buf,fparm->bsize)) > 0) {
                    sbuf->len = iret;
                    TB_LOGD("recv data:%d bytes",iret);

		    memcpy(fbuf->buf+fbuf->len,sbuf->buf,sbuf->len);
                    fbuf->len += sbuf->len;

                    if (global_log_level >= LEVEL_OVER_LOGV) {
                        __dump_data(sbuf->buf,sbuf->len);
		    }
		    

                    list_for_each_entry_safe(pt,_pt,&fparm->pt_head,list) {
                        ASYNC_PARM_T async_parm = {0};
                        TB_LOGD();

                        if(!pt->complete(fbuf->buf,fbuf->len)) {
		            continue;    	
			}

                        if (global_log_level >= LEVEL_OVER_LOGV) {
                            __dump_data(fbuf->buf,fbuf->len);
		        }

                        if(pt->decode(fbuf,hbuf)){
                            TB_LOGE("decode fail");
                            goto err;
                        }

                        async_parm.fd = flist->fd;
                        async_parm.send = flist->com->send;
                        async_parm.encode = pt->encode;
                        async_parm.type = (int)(flist->com->type);

                        if(product_test_run_ops(&async_parm,hbuf,fbuf)) {
                            TB_LOGD("fail hwl run ops");
                            goto err;
                        }

                        if(pt->encode(hbuf,fbuf)) {
                            TB_LOGE("encode fail");
                            goto err;
                        }
                        TB_LOGD("send data");

                        if (global_log_level >= LEVEL_OVER_LOGV) {
                            __dump_data(fbuf->buf,fbuf->len);
			}

                        if(flist->com->send(flist->fd,(char *)fbuf->buf,fbuf->len) !=fbuf->len) {
                            TB_LOGE("send fail");   
                        }
err:
                        fbuf->len  = 0;
                    }

                }else if (iret == 0 && flist->type == E_FD_RECV) {
                    // socket close
                    TB_LOGW("close fd %d",flist->fd);
                    close(flist->fd);
                    __fd_del_in_list(flist);
                }else {
                    TB_LOGE("recv err");    
                }
            }else if(flist->type == E_FD_LISTEN) {
                int afd;
                struct sockaddr_in client;
                socklen_t addrlen;
                addrlen = sizeof(client);
                afd = accept(flist->fd,(struct sockaddr *)&client, &addrlen);
                if(afd < 0) {
                    TB_LOGE("fail to create socket %d %s\n",errno,strerror(errno));
                    usleep(30*1000);
                    continue;
                }

                if((fd = (LIST_FD_T*)malloc(sizeof(LIST_FD_T))) == NULL) {
                    TB_LOGE("malloc fd node fail");     
                    return NULL;
                }

                memset(fd,0,sizeof(LIST_FD_T));
                fd->com = flist->com;
                fd->type = E_FD_RECV;
                fd->fd = afd;
                product_test_list_add_tail(&fd->list,&fparm->fd_head);
                TB_LOGD("add fd %d\n",afd);
            }
                    
        }
    }
        
    TB_LOGI("***************TUYA TCP PRODUCT THREAD EXIT****************");
    return (void*)0;
}


int product_test_zalloc_hbuf(size_t size,PT_HBUF_T **hbuf)
{
    uint8_t *buf;
    PT_HBUF_T *_hbuf; 

    buf = (uint8_t*)malloc(sizeof(PT_HBUF_T) + size);

    if(buf == NULL) {
        TB_LOGE("malloc fail\n");
        return -1;  
    }

    memset(buf,0,sizeof(PT_HBUF_T) + size);

    _hbuf = (PT_HBUF_T*)buf;

    _hbuf->data = buf + sizeof(PT_HBUF_T);

    *hbuf = _hbuf;

    return  0;
}

int product_test_free_hbuf(PT_HBUF_T *hbuf) 
{
    if(hbuf) {
        free(hbuf);
    }
    return 0;
}

int product_test_zalloc_fbuf(size_t size,PT_FBUF_T **fbuf) 
{
    uint8_t *buf;
    PT_FBUF_T *_fbuf; 

    buf = (uint8_t*)malloc(sizeof(PT_FBUF_T) + size);

    if(buf == NULL) {
        TB_LOGE("malloc fail\n");
        return -1;  
    }

    memset(buf,0,sizeof(PT_FBUF_T) + size);

    _fbuf = (PT_FBUF_T*)buf;

    _fbuf->buf = buf + sizeof(PT_FBUF_T);

    *fbuf = _fbuf;

    return  0;
}

int product_test_free_fbuf(PT_FBUF_T *fbuf)
{
    if(fbuf) {
        free(fbuf); 
    }
    return 0;
}

int product_test_register_protocol(int bsize,PT_ENCODE_T encode,PT_DECODE_T decode,PT_COMPLETE_T complete) 
{
    PT_PARM_T *pt;
    PRODUCT_TEST_PARAM_T *parm = &sg_product_test_parm;

    if(!parm->list_init) {
        memset(parm,0,sizeof(PRODUCT_TEST_PARAM_T));
        PRODUCT_TEST_INIT_LIST_HEAD(&parm->fd_head);
        PRODUCT_TEST_INIT_LIST_HEAD(&parm->pt_head);
        PRODUCT_TEST_INIT_LIST_HEAD(&parm->com_head);
        parm->list_init = 1;
    }

    pt = (PT_PARM_T*)malloc(sizeof(PT_PARM_T));

    if(pt == NULL) {
        TB_LOGE("malloc fail\n");
        return -1;  
    }
        
    memset(pt,0,sizeof(PT_PARM_T));

    pt->encode = encode;
    pt->decode = decode;
    pt->complete = complete;

    if(parm->bsize < bsize) {
        parm->bsize = bsize;
    }

    product_test_list_add_tail(&pt->list,&parm->pt_head);

    parm->pt_cnts ++;

    return 0;
}

int product_test_register_communication_interface(COM_INTFACE_T *intface) 
{
    int fd;
    COM_PARM_T *com;
    LIST_FD_T *flist;
    PRODUCT_TEST_PARAM_T *parm = &sg_product_test_parm;

    if(intface == NULL) {
        TB_LOGE("invalid parm");
        return -1;  
    }

    if(!parm->list_init) {
        memset(parm,0,sizeof(PRODUCT_TEST_PARAM_T));
        PRODUCT_TEST_INIT_LIST_HEAD(&parm->fd_head);
        PRODUCT_TEST_INIT_LIST_HEAD(&parm->pt_head);
        PRODUCT_TEST_INIT_LIST_HEAD(&parm->com_head);
        parm->list_init = 1;
    }

    list_for_each_entry(com,&parm->com_head,list) {
        if(com->type == intface->type) {
            TB_LOGW("type:%d has register",com->type);
            return 0;
        }
    }

    if(intface->init(intface->parm,&fd)) {
        TB_LOGE("init init fail");
        return -1;  
    }

    com = (COM_PARM_T*)malloc(sizeof(COM_PARM_T));

    if(com == NULL) {
        TB_LOGE("malloc fail\n");
        intface->uinit(fd);
        return -1;  
    }

    flist = (LIST_FD_T*)malloc(sizeof(LIST_FD_T));

    if(flist == NULL) {
        TB_LOGE("malloc fail\n");
        intface->uinit(fd);
        free(com);
        return -1;  
    }
        
    memset(com,0,sizeof(COM_PARM_T));
    memset(flist,0,sizeof(LIST_FD_T));

    com->type = intface->type;
//  com->init = intface->init;
    com->uinit = intface->uinit;
    com->send = intface->send;
    com->recv = intface->recv;

    flist->fd = fd;
    com->fd = &fd;

    if(com->type == E_COM_TCP) {
        flist->type = E_FD_LISTEN;
    }else {
        flist->type = E_FD_RECV;
    }

    flist->com = com;

    product_test_list_add_tail(&com->list,&parm->com_head);
    product_test_list_add_tail(&flist->list,&parm->fd_head);

    parm->com_cnts++;

    TB_LOGD(" type %d,fd %d add\n",com->type,*(com->fd));

    return 0;
}

static int __product_test_init_parm(PRODUCT_TEST_PARAM_T *parm)
{
    uint8_t *buf;
    COM_PARM_T *com,*_com;
    PT_PARM_T *pt,*_pt;
    LIST_FD_T *fd,*_fd; 
    PT_HBUF_T *hbuf;
    PT_FBUF_T *fbuf;
    PT_FBUF_T *sbuf;

    if(!parm->pt_cnts) {
        TB_LOGE("need register protocal");
        return -1;  
    }

    if(!parm->com_cnts) {
        TB_LOGE("need register interface");
        return -1;
    }


    if((buf= (uint8_t*)malloc(parm->bsize*3))== NULL) {
        TB_LOGE("malloc fail\n");
        goto MALLOC_FAIL_EXIT;  
    }
    parm->buf = buf;

    memset(buf,0,parm->bsize*3);

    hbuf = &parm->hbuf;

    memset(hbuf,0,sizeof(PT_HBUF_T));
    hbuf->data=buf;

    fbuf = &parm->fbuf;
    memset(fbuf,0,sizeof(PT_FBUF_T));
    fbuf->buf = hbuf->data + parm->bsize;   

    sbuf = &parm->sbuf;
    memset(sbuf,0,sizeof(PT_FBUF_T));
    sbuf->buf = fbuf->buf + parm->bsize;

    TB_LOGD("malloc%p(%d),fbuf:%p,hbuf:%p,sbuf:%p",buf,parm->bsize*3,fbuf->buf,hbuf->data,sbuf->buf);

    return 0;

MALLOC_FAIL_EXIT:

    list_for_each_entry_safe(com,_com,&parm->com_head,list) {
        com->uinit(*(com->fd));
        *(com->fd) = -1;
        product_test_list_del(&com->list);
        free(com);
    }

    list_for_each_entry_safe(fd,_fd,&parm->fd_head,list) {
        if(fd->fd) {
            close(fd->fd);
        }
        product_test_list_del(&fd->list);
        free(fd);
    }

    list_for_each_entry_safe(pt,_pt,&parm->pt_head,list) {
        product_test_list_del(&pt->list);
        free(pt);
    }
        
    return -1;
}

static int __product_test_uinit_parm(PRODUCT_TEST_PARAM_T *parm)
{
    COM_PARM_T *com,*_com;
    PT_PARM_T *pt,*_pt;
    LIST_FD_T *fd,*_fd; 

    if(parm == NULL) {
        TB_LOGE("invalid parm");
        return -1;
    }

    list_for_each_entry_safe(com,_com,&parm->com_head,list) {
        com->uinit(*(com->fd));
        *(com->fd) = -1;
        product_test_list_del(&com->list);
        free(com);
    }

    list_for_each_entry_safe(fd,_fd,&parm->fd_head,list) {
        if(fd->fd) {
            close(fd->fd);
        }
        product_test_list_del(&fd->list);
        free(fd);
    }

    list_for_each_entry_safe(pt,_pt,&parm->pt_head,list) {
        product_test_list_del(&pt->list);
        free(pt);
    }

    free(parm->buf);
        
    return 0;
}

static int __product_test_start(PRODUCT_TEST_PARAM_T *fparm )
{

    int iret;
    pthread_attr_t attr;
    
    if(fparm == NULL) {
        TB_LOGE("invalid parm");
        return -1;
    }
 
    pthread_attr_init(&attr);
    iret = pthread_create(&fparm->thread,&attr,product_test_thread,fparm);
    pthread_attr_destroy(&attr);

    if(iret != 0) {
            TB_LOGE("create thread fails %d", iret);
            return -1;
        }  
    
    return 0;
}


static int __product_test_stop(PRODUCT_TEST_PARAM_T *fparm )
{

    if(fparm == NULL) {
        TB_LOGE("invalid parm");
        return -1;
    }
    
    fparm->exit_prod = 1;
    pthread_cancel(fparm->thread);
    pthread_join(fparm->thread,NULL);

    return 0;
}


int product_test_enter(void)
{
    static PRODUCT_TEST_PARAM_T *fparm = &sg_product_test_parm;

    if(sg_product_test_init_flag == 1) {
        TB_LOGE( "ty product is started");
        return 0;
    }
    
    if(__product_test_init_parm(fparm)) {
        TB_LOGE("invalid parm");    
        return -1;
    }

    if(__product_test_start(fparm)) {
        TB_LOGE("ty start recv server fail");
        return -1;
    }
    sg_product_test_init_flag = 1;

    return 0;
}

void product_test_wait_exit(void)
{
    PRODUCT_TEST_PARAM_T *fparm= &sg_product_test_parm;

    if(sg_product_test_init_flag) {
        if(fparm == NULL) {
            TB_LOGE("invalid parm");
            return ;
    }

        TB_LOGD("== wait for exit == ");

        pthread_join(fparm->thread,NULL);

        __product_test_uinit_parm(fparm);

        sg_product_test_init_flag = 0;
    }

    if (sg_enter_hook != NULL) {
        sg_enter_hook("on");
    }

    return ;
}

void product_test_exit(void)
{
    PRODUCT_TEST_PARAM_T *fparm= &sg_product_test_parm;

    if(sg_product_test_init_flag) {
        if(fparm == NULL) {
            TB_LOGE("invalid parm");
            return ;
        }

        __product_test_stop(fparm);
        __product_test_uinit_parm(fparm);

        sg_product_test_init_flag = 0;
    }

    return ;
}
