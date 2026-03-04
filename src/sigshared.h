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

#ifndef SIGSHARED_H
#define SIGSHARED_H

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
#include <sys/signalfd.h>

#include "include/spright.h"
#include "include/http.h"
#include "../ebpf/xsk_kern.skel.h"

#define INVALID_POSITION UINT64_MAX
#ifndef unlikely
#define unlikely(x) __builtin_expect(!!(x), 0)
#endif
#ifndef likely
#define likely(x) __builtin_expect(!!(x), 1)
#endif


#define SIGSHARED_NAME     "SIGSHARED_MEM"
#define SIGSHARED_MEMPOOL  "SIGSHARED_MEMPOOL"
#define SIGSHARED_TAM (1U << 16) * sizeof(struct http_transaction)
//#define SIGSHARED_TAM (1U << 20) * sizeof(struct http_transaction)

#define N_ELEMENTS (1U << 16)
#define RINGBUF_REGION "RINGBUF_MEM"
#define RINGBUF_TAM sizeof(struct sigshared_ringbuffer)

extern struct xsk_kern *skel;
extern struct spright_cfg_s *sigshared_cfg;
extern void *sigshared_ptr;

extern int fd_sigshared_mem;
extern int fd_sigshared_mempool;
extern int fd_cfg_file;

struct sigshared_ringbuffer{
	uint64_t counter;
	uint64_t ringbuffer[N_ELEMENTS];
	uint64_t rb[N_ELEMENTS];

	uint64_t head;
	uint64_t tail;

    pid_t pids[10];

    void *sigshared_mem;
};

extern struct sigshared_ringbuffer *ringbuff;
extern uint64_t rb[N_ELEMENTS];

extern char temp[400];
extern char dir_temp[256];
extern int map_fd;
extern pid_t pid_alvo;

extern char *path_fixo;
extern int mapa_sinal_fd;

extern void *sigshared_mempool;

void *sigshared_create_mem();
void *sigshared_ptr_mem();

struct spright_cfg_s *sigshared_cfg_mem();
struct spright_cfg_s *sigshared_cfg_ptr();

int sigshared_update_map(char *map_name, int fn_id, int pid, int *map_fd);
pid_t sigshared_lookup_map(char *map_name, int key, int *mapa_sinal_fd);

struct sigshared_ringbuffer *sigshared_mempool_create();
struct sigshared_ringbuffer *sigshared_mempool_ptr();

uint64_t sigshared_mempool_get();
int sigshared_mempool_put( uint64_t addr);

struct http_transaction *sigshared_mempool_access(void **temp, uint64_t addr);

#endif 

