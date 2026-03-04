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



#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <signal.h>
#include <sys/shm.h>

#include <time.h>
#include "sigshared.h"


#include "./include/io.h"
#include "./log/log.h"
#include "./include/spright.h"


pid_t pid_alvo = -1;
int sigrtmin1 = 35;//SIGRTMIN+1;

//void io_rx( void **obj, void *sigshared_ptr, sigset_t *set){
void io_rx( void **obj, void *sigshared_ptr, sigset_t *set, int matriz[][2]){

	int i =0;
	uint64_t addr;
	siginfo_t data_rcv;

	if( likely( sigwaitinfo(set, &data_rcv) > 0) ){
		
		addr = (uint64_t)data_rcv.si_value.sival_ptr;
		pid_t pid_emissor = data_rcv.si_pid;
		
		// signal from frontend
		//if (pid_emissor == matriz[1][1]){ 
		//
		//    if(addr >= 0 && addr < N_ELEMENTS){
		//	sigshared_mempool_access(obj, addr);
		//	return;
		//    }
		//    else{
		//	printf("Invalid addr\n");
		//	return;
	 	//    }
		//}
		//else{
			for(i=0 ; i < 11; i++){
				if (matriz[i][1] == pid_emissor){

					if(addr >= 0 && addr < N_ELEMENTS){
						sigshared_mempool_access(obj, addr);
						return;
					}
					else{
						printf("Invalid addr\n");
						return;
					}




				}
			}
			printf("Invalid addr\n");
		//}

		//if(addr >= 0 && addr < N_ELEMENTS){
		//	sigshared_mempool_access(obj, addr);
		//	return;
		//}
		//else{
		//	printf("Invalid addr\n");
		//	return;
		//}
	}
	return;
}

int io_tx(uint64_t addr, uint8_t next_fn, int *map_fd){
  
    return 0;
}


int io_tx_matriz(uint64_t addr, uint8_t next_fn, int *map_fd, int pid ,int matriz[][2], int next_fn_pid){

    sigval_t data_send;

    if( unlikely( matriz[next_fn][1] == 0) ){
	    next_fn_pid = sigshared_lookup_map( "mapa_sinal", next_fn, map_fd);
	    matriz[next_fn][1] = next_fn_pid;
    }

    data_send.sival_ptr = (void *)addr;
    
    if( unlikely(pid == next_fn_pid) ){
	log_error("==%d== Error: Sending signal to itself...", pid);
	return -1;
    }


    if( unlikely(sigqueue( next_fn_pid, sigrtmin1, data_send) < 0) ){
	log_error("==io_tx_matriz(%d)== Error to send signal: next_fn:%d pid:%d | sinal:%d | addr:%ld", pid, next_fn, matriz[next_fn][1], sigrtmin1, addr);
	exit(1);
    }

    return 0;
}
