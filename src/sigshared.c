/*Copyright 2026  Universidade Federal do Mato Grosso do Sul
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
* 
*    http://www.apache.org/licenses/LICENSE-2.0 
* 
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>  
#include <sys/types.h> 
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/prctl.h>
#include <bpf/libbpf.h>
#include <bpf/bpf.h>
#include <assert.h>
#include <stddef.h>
#include <sched.h>

#include "sigshared.h"
#include "include/spright.h"
#include "include/http.h"
#include "log/log.h"

void *sigshared_ptr;
struct spright_cfg_s *sigshared_cfg;
int fd_sigshared_mem;
int fd_sigshared_mempool;
int fd_cfg_file;

char temp[400];
char dir_temp[256];
int map_fd = -1;

struct sigshared_ringbuffer *ringbuff;
void *sigshared_mempool;
uint64_t rb[N_ELEMENTS];

int fd_shm = -1;

int mapa_sinal_fd = -1;

 void *sigshared_create_mem(){

    fd_sigshared_mem = shm_open(SIGSHARED_NAME, O_CREAT | O_RDWR, 0777);
    if (fd_sigshared_mem < 0){ 
        perror("Error: shm_open()");
        exit(1);
    }

    int ret_ftruncate = ftruncate(fd_sigshared_mem, SIGSHARED_TAM); 
    if ( ret_ftruncate == -1 ){
        perror("Error: ftruncate()");
        exit(1);  
    }

    return ( void *) mmap(0, SIGSHARED_TAM, PROT_WRITE, MAP_SHARED, fd_sigshared_mem, 0);
}

void *sigshared_ptr_mem(){

    fd_sigshared_mem = shm_open(SIGSHARED_NAME, O_CREAT | O_RDWR, 0777);
    if ( fd_sigshared_mem < 0){ 
        perror("Error: shm_open()");
        exit(1);
    }

    return ( void *) mmap(0, SIGSHARED_TAM, PROT_WRITE, MAP_SHARED, fd_sigshared_mem, 0);
}

struct spright_cfg_s *sigshared_cfg_mem(){

    fd_cfg_file = shm_open("CFG_MEM", O_CREAT | O_RDWR, 0777);
    if (fd_cfg_file < 0){ 
        perror("Error: shm_open()");
        exit(1);
    }

    int ret_ftruncate = ftruncate(fd_cfg_file, sizeof(struct spright_cfg_s)); 
    if ( ret_ftruncate == -1 ){
        perror("Error: ftruncate()");
        exit(1);  
    }

    return ( struct spright_cfg_s *) mmap(0, sizeof(struct spright_cfg_s), PROT_WRITE, MAP_SHARED, fd_cfg_file, 0);
}

struct spright_cfg_s *sigshared_cfg_ptr(){

    fd_cfg_file = shm_open("CFG_MEM", O_CREAT | O_RDWR, 0777);
    if (fd_cfg_file < 0){ 
        perror("Error: shm_open()");
        exit(1);
    }

    return ( struct spright_cfg_s *) mmap(0, sizeof(struct spright_cfg_s), PROT_WRITE, MAP_SHARED, fd_cfg_file, 0);
}

int sigshared_update_map(char *map_name, int fn_id, int pid, int *map_fd){

    char temp[256];
    char *dir_temp = getenv("SIGSHARED");

    sprintf(temp, "%s/dados/%s", dir_temp, map_name);
    *map_fd = bpf_obj_get(temp);

    if(bpf_map_update_elem(*map_fd, &fn_id, &pid, BPF_ANY) < 0){
        perror("Error to update eBPF map");
        return -1;
    }

    log_info("(update_map(%d)) eBPF map updated...\n", getpid());
    return 0;
}

pid_t sigshared_lookup_map(char *map_name, int key, int *map_sig_fd){

	char temp[256];
	char *dir_temp = getenv("SIGSHARED");
	pid_t pid_ret;

	if(map_sig_fd <= 0){
		sprintf(temp, "%s/dados/%s", dir_temp, map_name);
		*map_sig_fd = bpf_obj_get(temp);
	}

	if( bpf_map_lookup_elem(*map_sig_fd, &key, &pid_ret) < 0 ){
		printf("Error to lookup at eBPF map | map_sinal_fd:%d  key:%d  pid_ret:%d\n", *map_sig_fd, key, pid_ret);
		return -1;
	}

	return pid_ret;
}

struct sigshared_ringbuffer *sigshared_mempool_create(){

    int fd_ringbuff = shm_open(RINGBUF_REGION, O_RDWR | O_CREAT, 0777);
    if (fd_ringbuff < 0){
        perror("Error to create ringbuff's shared mem");
        return NULL;
    }

    int ret_truncate = ftruncate( fd_ringbuff, RINGBUF_TAM);
    if(ret_truncate < 0){
        perror("Error to set ringbuff's size");
        return NULL;
    }

    ringbuff = (struct sigshared_ringbuffer *) mmap(0, RINGBUF_TAM, PROT_WRITE, MAP_SHARED, fd_ringbuff, 0);
    for(uint64_t i=0; i < N_ELEMENTS; i++){
        ringbuff->ringbuffer[i] = i;
        ringbuff->rb[i] = i;
	rb[i] = i;
    }

    ringbuff->head = 0;
    ringbuff->tail = N_ELEMENTS - 1;

    return (struct sigshared_ringbuffer *) mmap(0, RINGBUF_TAM, PROT_WRITE, MAP_SHARED, fd_ringbuff, 0);
}

struct sigshared_ringbuffer *sigshared_mempool_ptr(){
    int fd_ringbuff = shm_open(RINGBUF_REGION, O_CREAT | O_RDWR, 0777);
    if (fd_ringbuff < 0){ 
        perror("Error: shm_open()");
        exit(1);
    }

    return ( struct sigshared_ringbuffer *) mmap(0, RINGBUF_TAM, PROT_WRITE, MAP_SHARED, fd_ringbuff, 0);
}

uint64_t sigshared_mempool_get(){

    uint64_t temp;
    if (ringbuff->head+1 == N_ELEMENTS ){

        ringbuff->head = 0;

        if (ringbuff->head != ringbuff->tail){
            temp = ringbuff->rb[ ringbuff->ringbuffer[ringbuff->head] ];
            ringbuff->head++;
            return temp;
        
	}
        else{
            while(ringbuff->head == ringbuff->tail){
                printf("Error: <sigshared_mempool_get()> HEAD == TAIL | head:%ld tail:%ld\n", ringbuff->head, ringbuff->tail);
            }

            temp =  ringbuff->rb[ringbuff->ringbuffer[ringbuff->head]];
            return temp;
        }
    }

    else if (ringbuff->head+1 != ringbuff->tail){
        temp = ringbuff->rb[ringbuff->ringbuffer[ringbuff->head]];
        ringbuff->head++;
        return temp;
    }

    else{
        while(ringbuff->head == ringbuff->tail){
                printf("<sigshared_mempool_get()> HEAD == TAIL \n");
        }

        temp =  ringbuff->rb[ringbuff->ringbuffer[ringbuff->head]];
        ringbuff->head++;
        return temp;
    }
    return -1; 
}

int sigshared_mempool_put( uint64_t addr){

    if(ringbuff->tail+1 == N_ELEMENTS){
        ringbuff->tail = 0;

        if ( ringbuff->tail != ringbuff->head ){
            ringbuff->ringbuffer[ringbuff->tail] = addr;
            ringbuff->tail++;
            return 0;
        }
        else{
            printf("Error: <sigshared_mempool_put> HEAD == TAIL \n");
            ringbuff->ringbuffer[ringbuff->tail] = addr;
            ringbuff->head = ringbuff->tail+1; 
            return 0;
        }
    }
    else if( ringbuff->tail+1 != ringbuff->head ){
        ringbuff->ringbuffer[ringbuff->tail] = addr;
        ringbuff->tail++;
        return 0;
    }

    else if (ringbuff->tail+1 == ringbuff->head){
        ringbuff->head = ringbuff->tail; 
        ringbuff->tail++;
        return 0;
    }

    return 0;
}

struct http_transaction *sigshared_mempool_access(void **temp, uint64_t addr){

	*temp = (void *)sigshared_ptr;
	
	if (unlikely(*temp == NULL)){
		printf("Error: sigshared_mempool_access: mempool == NULL\n");
		return NULL;
	}

	else if(likely(addr >= 0 && addr <= N_ELEMENTS-1)){
		
		*temp += sizeof(struct http_transaction) * addr;
		return *temp;
	}
	else{
		printf("Error: sigshared_mempool_access invalid ADDR\n");
		return NULL;
	}
}
