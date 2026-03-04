/*
 * This file has been simplified and modified from the original project SPRIGHT for the purposes of test and experiment the latency impact of real-time signals in microsservices.
 *
 */



/*
# Copyright 2022 University of California, Riverside
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0
*/

#include <errno.h>
#include <math.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <time.h>
#include <unistd.h>

#include "../include/http.h"
#include "../include/io.h"
#include "../include/shm_rpc.h"
#include "../include/spright.h"
#include "../include/utility.h"

#include <execinfo.h>
#include <sys/signalfd.h>
#include "../sigshared.h"

int mapa_fd;
int matriz[11][2] = {0};
sigset_t set;
sigset_t set2;

int pid; 



static int pipefd_rx[UINT8_MAX][2];
static int pipefd_tx[UINT8_MAX][2];

// char defaultCurrency[5] = "CAD";

__always_inline char *find_char(const char *s, char c) {
    if (!s) return NULL;
    while (*s && *s != c)
        s++;
    return *s ? (char*)s : NULL;
}

static void setCurrencyHandler(struct http_transaction *txn){

    log_debug("Call setCurrencyHandler");
    
    char *query = httpQueryParser(txn->request);
    if (!query) {
            log_error("httpQueryParser retornou NULL");
            exit(1);
    }

    char _defaultCurrency[5] = "CAD";
    strcpy(_defaultCurrency, strchr(query, '=') + 1);

    txn->hop_count += 100;
    txn->next_fn = GATEWAY; // Hack: force gateway to return a response

}

static void homeHandler(struct http_transaction *txn){

    log_debug("Call homeHandler ### Hop: %u", txn->hop_count);

    if (txn->hop_count == 0){
        getCurrencies(txn);
    }
    else if (txn->hop_count == 1){
        getProducts(txn);
        txn->productViewCntr = 0;
    }
    else if (txn->hop_count == 2){
        getCart(txn);
    }
    else if (txn->hop_count == 3){
        convertCurrencyOfProducts(txn);
        homeHandler(txn);
    }
    else if (txn->hop_count == 4){
        chooseAd(txn);
    }
    else if (txn->hop_count == 5){
        returnResponse(txn);
    }
    else{
        log_warn("homeHandler doesn't know what to do for HOP %u.", txn->hop_count);
        returnResponse(txn);
    }
    return;
}

static void productHandler(struct http_transaction *txn)
{
    log_debug("Call productHandler ### Hop: %u", txn->hop_count);

    if (txn->hop_count == 0)
    {
        getProduct(txn);
        txn->productViewCntr = 0;
    }
    else if (txn->hop_count == 1)
    {
        getCurrencies(txn);
    }
    else if (txn->hop_count == 2)
    {
        getCart(txn);
    }
    else if (txn->hop_count == 3)
    {
        convertCurrencyOfProduct(txn);
    }
    else if (txn->hop_count == 4)
    {
        chooseAd(txn);
    }
    else if (txn->hop_count == 5)
    {
        returnResponse(txn);
    }
    else
    {
        log_warn("productHandler doesn't know what to do for HOP %u.", txn->hop_count);
        returnResponse(txn);
    }
    return;
}

static void addToCartHandler(struct http_transaction *txn)
{
    log_debug("Call addToCartHandler ### Hop: %u", txn->hop_count);
    
    if (txn->hop_count == 0)
    {
        getProduct(txn);
        txn->productViewCntr = 0;
    }
    else if (txn->hop_count == 1)
    {
        insertCart(txn);
    }
    else if (txn->hop_count == 2)
    {
        returnResponse(txn);
    }
    else
    {
        log_debug("addToCartHandler doesn't know what to do for HOP %u.", txn->hop_count);
        returnResponse(txn);
    }
}

static void viewCartHandler(struct http_transaction *txn)
{
    log_debug("Call viewCartHandler ### Hop: %u", txn->hop_count);
    
    if (txn->hop_count == 0)
    {
        getCurrencies(txn);
    }
    else if (txn->hop_count == 1)
    {
        getCart(txn);
        txn->cartItemViewCntr = 0;
        strcpy(txn->total_price.CurrencyCode, defaultCurrency);
    }
    else if (txn->hop_count == 2)
    {
        getRecommendations(txn);
    }
    else if (txn->hop_count == 3)
    {
        getShippingQuote(txn);
    }
    else if (txn->hop_count == 4)
    {
        convertCurrencyOfShippingQuote(txn);
        if (txn->get_quote_response.conversion_flag == true)
        {
            getCartItemInfo(txn);
            txn->hop_count++;
        }
        else
        {
            log_debug("Set get_quote_response.conversion_flag as true");
            txn->get_quote_response.conversion_flag = true;
        }
    }
    else if (txn->hop_count == 5)
    {
        getCartItemInfo(txn);
    }
    else if (txn->hop_count == 6)
    {
        convertCurrencyOfCart(txn);
    }
    else
    {
        log_debug("viewCartHandler doesn't know what to do for HOP %u.", txn->hop_count);
        returnResponse(txn);
    }
}

static void PlaceOrder(struct http_transaction *txn)
{
    parsePlaceOrderRequest(txn);
    
    strcpy(txn->rpc_handler, "PlaceOrder");
    
    txn->caller_fn = FRONTEND;
    txn->next_fn = CHECKOUT_SVC;
    
    
    txn->hop_count++;
    txn->checkoutsvc_hop_cnt = 0;

}

static void placeOrderHandler(struct http_transaction *txn)
{
    log_debug("Call placeOrderHandler ### Hop: %u", txn->hop_count);

    if (txn->hop_count == 0)
    {
        PlaceOrder(txn);
    }
    else if (txn->hop_count == 1)
    {
        getRecommendations(txn);
    }
    else if (txn->hop_count == 2)
    {
        getCurrencies(txn);
    }
    else if (txn->hop_count == 3)
    {
        returnResponse(txn);
    }
    else
    {
        log_debug("placeOrderHandler doesn't know what to do for HOP %u.", txn->hop_count);
        returnResponse(txn);
    }
}

static void httpRequestDispatcher(struct http_transaction *txn)
{

    char *req = txn->request;
    
    if (strstr(req, "/1/cart/checkout") != NULL)
    {
        placeOrderHandler(txn);
    }
    else if (strstr(req, "/1/cart") != NULL)
    {
        if (strstr(req, "GET"))
        {
            viewCartHandler(txn);
        }
        else if (strstr(req, "POST"))
        {
            addToCartHandler(txn);
        }
        else
        {
            log_debug("No handler found in frontend: %s", req);
        }
    }
    else if (strstr(req, "/1/product") != NULL)
    {
        productHandler(txn);
    }
    else if (strstr(req, "/1/setCurrency") != NULL)
    {
        setCurrencyHandler(txn);
    }
    else if (strstr(req, "/1") != NULL)
    {
        homeHandler(txn);
    }
    else
    {
        log_debug("Unknown handler. Check your HTTP Query, human!: %s", req);
        returnResponse(txn);
    }

    return;
}

static void *nf_worker(void *arg){
    struct http_transaction *txn;
    ssize_t bytes_written;
    ssize_t bytes_read;
    uint8_t index;

    /* TODO: Careful with this pointer as it may point to a stack */
    index = (uint64_t)arg;

    while (1){

        bytes_read = read(pipefd_rx[index][0], &txn, sizeof(struct http_transaction *));
        if (unlikely(bytes_read == -1)){
            log_error("read() error: %s", strerror(errno));
            return NULL;
        }
        httpRequestDispatcher(txn);

        bytes_written = write(pipefd_tx[index][1], &txn, sizeof(struct http_transaction *));
        if (unlikely(bytes_written == -1)){
            log_error("write() error: %s", strerror(errno));
            return NULL;
        }
    }

    return NULL;
}


static void *nf_rx(void *arg)
{
    struct http_transaction *txn = NULL;
    ssize_t bytes_written;
    uint8_t i;

    for (i = 0;; i = (i + 1) % sigshared_cfg->nf[fn_id - 1].n_threads){
        //io_rx((void **)&txn, sigshared_ptr, &set);
        io_rx((void **)&txn, sigshared_ptr, &set, matriz);
        if (unlikely(txn == NULL)){
            log_error("io_rx() error");
            return NULL;
        }

	if(unlikely(txn == NULL)){
		printf("==email== ERRO mempool_access retornou NULL\n");
		return NULL;
	}

        bytes_written = write(pipefd_rx[i][1], &txn, sizeof(struct http_transaction *));
        if (unlikely(bytes_written == -1))
        {
            log_error("write() error: %s", strerror(errno));
            return NULL;
        }
    }

    return NULL;
}


// signalfd implementation
//static void *nf_rx(void *arg){
//    //struct http_transaction *txn = NULL;
//    struct http_transaction *txn;
//    ssize_t bytes_written;
//    uint8_t i;
//    //int ret;
//    //uint64_t addr;
//    //int pid = getpid();
//
//    sigset_t block_set;
//
//    sigemptyset(&block_set);       
//    sigaddset(&block_set, SIGRTMIN+1); 
//    pthread_sigmask(SIG_BLOCK, &block_set, NULL);
//
//    //txn = (struct http_transaction *) mmap(0, SIGSHARED_TAM, PROT_WRITE, MAP_SHARED, fd_sigshared_mem, 0);
//
//    struct epoll_event eventos[UINT8_MAX];
//    int n;
//    int sigfd = signalfd(-1, &block_set, SFD_NONBLOCK | SFD_CLOEXEC);
//
//    int epfd = epoll_create1(0);
//    if (unlikely(epfd == -1))
//    {
//        log_error("epoll_create1() error: %s", strerror(errno));
//        return NULL;
//    }
//
//    eventos[0].events = EPOLLIN; // The associated file is available for read(2) operations.
//    eventos[0].data.fd = sigfd;
//
//    if (epoll_ctl(epfd, EPOLL_CTL_ADD, sigfd, &eventos[0]) < 0) {
//        perror("epoll_ctl");
//        exit(1);
//    }
//
//    for (i = 0;; i = (i + 1) % sigshared_cfg->nf[fn_id - 1].n_threads){
//
//	//log_info("Recebendo sinal...");
//        //ret = io_rx((void **)&txn);
//        //addr = io_rx(txn, sigshared_ptr, &set);
//        //txn = io_rx(txn, sigshared_ptr, &set);
//        
//	//io_rx((void **)&txn, sigshared_ptr, &set);
//        
//	//if (unlikely(ret == -1))
//        //if (unlikely(addr == -1)){
//        //    log_error("io_rx() error");
//        //    return NULL;
//        //}
//	
//	//printf("Esperando sinal com epoll_wait()...\n");
//	n = epoll_wait(epfd, eventos, UINT8_MAX, -1);  // -1 = block indefinitely
//        
//        if (n < 0) {
//            if (errno == EINTR)
//                continue;
//            perror("epoll_wait");
//            break;
//        }
//
//        for (int j = 0; j < n; j++) {
//            if (eventos[j].data.fd == sigfd) {
//                
//		struct signalfd_siginfo si;
//                ssize_t res = read(sigfd, &si, sizeof(si));
//                if (res != sizeof(si)) {
//                    perror("read(signalfd)");
//                    continue;
//                }
//
//                //printf("Received signal %d from PID %d | data: %ld\n", si.ssi_signo, si.ssi_pid, (uint64_t)si.ssi_ptr);
//		txn = sigshared_mempool_access( (void**)&txn, (uint64_t)si.ssi_ptr );
//		if(unlikely(txn == NULL)){
//			printf("==frontend== txn retornou NULL\n");
//			exit(1);
//		}
//		
//		bytes_written = write(pipefd_rx[i][1], &txn, sizeof(struct http_transaction *));
//		if (unlikely(bytes_written == -1)){
//			log_error("write() error: %s", strerror(errno));
//			return NULL;
//		}
//	    }
//	}
//
//
//
//	//printf("==frontend== dps sigshared_mempool_access() | txn->addr:%ld\n", txn->addr);
//        //log_info("(ADDR RX:%ld), HOP: %u, Next Fn: %u, Caller Fn: %s (#%u) ", txn->addr, txn->hop_count, txn->next_fn, txn->caller_nf, txn->caller_fn);
//    }
//
//    return NULL;
//}

static void *nf_tx(void *arg){

    struct epoll_event event[UINT8_MAX]; /* TODO: Use Macro */
    struct http_transaction *txn;
    ssize_t bytes_read;
    uint8_t i;
    int n_fds;
    int epfd;
    int ret;
    int ret_io;

    epfd = epoll_create1(0);
    if (unlikely(epfd == -1))
    {
        log_error("epoll_create1() error: %s", strerror(errno));
        return NULL;
    }

    for (i = 0; i < sigshared_cfg->nf[fn_id - 1].n_threads; i++)
    {
        ret = set_nonblocking(pipefd_tx[i][0]);
        if (unlikely(ret == -1))
        {
            return NULL;
        }

        event[0].events = EPOLLIN;
        event[0].data.fd = pipefd_tx[i][0];

        ret = epoll_ctl(epfd, EPOLL_CTL_ADD, pipefd_tx[i][0], &event[0]);
        if (unlikely(ret == -1))
        {
            log_error("epoll_ctl() error: %s", strerror(errno));
            return NULL;
        }
    }

    while (1){

        n_fds = epoll_wait(epfd, event, sigshared_cfg->nf[fn_id - 1].n_threads, -1);
        if (unlikely(n_fds == -1)){

            log_error("epoll_wait() error: %s", strerror(errno));
            return NULL;
        }

        for (i = 0; i < n_fds; i++){

            bytes_read = read(event[i].data.fd, &txn, sizeof(struct http_transaction *));
            if (unlikely(bytes_read == -1)){
                log_error("read() error: %s", strerror(errno));
                return NULL;
            }

            //log_debug("Route id: %u, Hop Count %u, Next Hop: %u, Next Fn: %u, Caller Fn: %s (#%u), RPC Handler: %s()", txn->route_id, txn->hop_count, sigshared_cfg->route[txn->route_id].hop[txn->hop_count], txn->next_fn, txn->caller_nf, txn->caller_fn, txn->rpc_handler);

            ret_io = io_tx_matriz(txn->addr, txn->next_fn, &mapa_fd, pid, matriz, matriz[txn->next_fn][1]);
            if (unlikely(ret_io == -1)){
                log_error("io_tx() error");
                return NULL;
            }
        }
    }

    return NULL;
}

void consulta_mapa(int fd_mapa_sinal, int nf_id, pid_t pid){

    pid_t temp;
    if ( bpf_map_lookup_elem(fd_mapa_sinal, &nf_id, &temp) == 0 ){ 

        if (temp == pid){
            printf("mapa_sinal[%d]: %d\n", nf_id, temp);
            matriz[nf_id][1] = pid; 
        }
        return;
    }


    perror("Erro ao consultar o mapa eBPF"); 
}

void *signal_rcv_config(){

	char temp[256];
	char *dir_temp = getenv("SIGSHARED");
    	sprintf(temp, "%s/dados/mapa_sinal", dir_temp);
    	int fd_mapa_sinal = bpf_obj_get(temp);
	
	siginfo_t data_rcv;
	//printf("Esperando sinal...\n");
	while (sigwaitinfo(&set2, &data_rcv) > 0){

		pid_t pid_emissor = data_rcv.si_pid;
		int nf_id_rcv = (uint64_t )data_rcv.si_value.sival_ptr;
		//printf("Recebeu sinal SIGRTMIN+2 pid_emissor:%d valor:%d\n", pid_emissor, nf_id);

		consulta_mapa(fd_mapa_sinal, nf_id_rcv, pid_emissor);
		
	}

	return;
}

/* TODO: Cleanup on errors */
static int nf(uint8_t nf_id){
    pthread_t thread_worker[UINT8_MAX];
    pthread_t thread_rx;
    pthread_t thread_tx;
    uint8_t i;
    int ret;

    fn_id = nf_id;
    //int pid = getpid();
    pthread_t thread_signal;

    
    matriz[nf_id][1] = pid;
    if(unlikely( sigshared_update_map("mapa_sinal", fn_id, pid, &mapa_fd) < 0 ) ){
        printf("Erro ao atualizar mapa\n");
                return 0;
    }

    char temp[256];
    char *dir_temp = getenv("SIGSHARED");
    sprintf(temp, "%s/dados/mapa_sinal", dir_temp);
    int fd_mapa_sinal = bpf_obj_get(temp);

    int pid_gateway, key_gateway = 0;
    if( bpf_map_lookup_elem(fd_mapa_sinal, &key_gateway, &pid_gateway ) < 0 ){
	    perror("Error to lookup the gateway's PID");
	    exit(1);
    }

    matriz[0][1] = pid_gateway;


    ret = pthread_create(&thread_signal, NULL, &signal_rcv_config, NULL);
    if (unlikely(ret != 0)){
        log_error("pthread_create() error: %s", strerror(ret));
        return -1;
    }


    for (i = 0; i < sigshared_cfg->nf[fn_id - 1].n_threads; i++)
    {
        ret = pipe(pipefd_rx[i]);
        if (unlikely(ret == -1))
        {
            log_error("pipe() error: %s", strerror(errno));
            return -1;
        }

        ret = pipe(pipefd_tx[i]);
        if (unlikely(ret == -1))
        {
            log_error("pipe() error: %s", strerror(errno));
            return -1;
        }
    }

    ret = pthread_create(&thread_rx, NULL, &nf_rx, NULL);
    if (unlikely(ret != 0))
    {
        log_error("pthread_create() error: %s", strerror(ret));
        return -1;
    }

    ret = pthread_create(&thread_tx, NULL, &nf_tx, NULL);
    if (unlikely(ret != 0))
    {
        log_error("pthread_create() error: %s", strerror(ret));
        return -1;
    }

    for (i = 0; i < sigshared_cfg->nf[fn_id - 1].n_threads; i++){
        ret = pthread_create(&thread_worker[i], NULL, &nf_worker, (void *)(uint64_t)i);
        if (unlikely(ret != 0))
        {
            log_error("pthread_create() error: %s", strerror(ret));
            return -1;
        }
    }

    for (i = 0; i < sigshared_cfg->nf[fn_id - 1].n_threads; i++)
    {
        ret = pthread_join(thread_worker[i], NULL);
        if (unlikely(ret != 0))
        {
            log_error("pthread_join() error: %s", strerror(ret));
            return -1;
        }
    }
    
    ret = pthread_join(thread_signal, NULL);
    if (unlikely(ret != 0))
    {
        log_error("pthread_join() error: %s", strerror(ret));
        return -1;
    }

    ret = pthread_join(thread_rx, NULL);
    if (unlikely(ret != 0))
    {
        log_error("pthread_join() error: %s", strerror(ret));
        return -1;
    }

    ret = pthread_join(thread_tx, NULL);
    if (unlikely(ret != 0))
    {
        log_error("pthread_join() error: %s", strerror(ret));
        return -1;
    }

    for (i = 0; i < sigshared_cfg->nf[fn_id - 1].n_threads; i++)
    {
        ret = close(pipefd_rx[i][0]);
        if (unlikely(ret == -1))
        {
            log_error("close() error: %s", strerror(errno));
            return -1;
        }

        ret = close(pipefd_rx[i][1]);
        if (unlikely(ret == -1))
        {
            log_error("close() error: %s", strerror(errno));
            return -1;
        }

        ret = close(pipefd_tx[i][0]);
        if (unlikely(ret == -1))
        {
            log_error("close() error: %s", strerror(errno));
            return -1;
        }

        ret = close(pipefd_tx[i][1]);
        if (unlikely(ret == -1))
        {
            log_error("close() error: %s", strerror(errno));
            return -1;
        }
    }

    return 0;
}


int main(int argc, char **argv){

    log_set_level_from_env();
    log_set_level(LOG_INFO);

    uint8_t nf_id;
    int ret;

    sigshared_ptr = sigshared_ptr_mem();
    if(sigshared_ptr == NULL){
	log_error("Error: sigshared_ptr");
    	return 1;
    }

    sigshared_cfg = sigshared_cfg_ptr();
    if(sigshared_cfg == NULL){
	log_error("Error: sigshared_cfg");
    	return 1;
    }

    sigemptyset(&set);       
    sigaddset(&set, SIGRTMIN+1); 
    //sigaddset(&set, SIGRTMIN+2); 
    sigprocmask(SIG_BLOCK, &set, NULL);

    sigemptyset(&set2);       
    sigaddset(&set2, SIGRTMIN+2); 
    pthread_sigmask(SIG_BLOCK, &set2, NULL);

    pid = getpid();

    char settar_cpuf[30];

    log_info("Pinning process in CPU 3...\n");
    sprintf(settar_cpuf, "taskset -cp 3 %d", getpid());
    int ret_sys = system(settar_cpuf);
    if( ret_sys == -1 || ret_sys == 127 ){
            log_error("Error to pinn CPU");
            exit(1);
    }


    errno = 0;
    nf_id = strtol(argv[argc-1], NULL, 10);
    if (unlikely(errno != 0 || nf_id < 1))
    {
        log_error("Invalid value for Network Function ID");
        goto error_1;
    }

    ret = nf(nf_id);
    if (unlikely(ret == -1))
    {
        log_error("nf() error");
        goto error_1;
    }

    return 0;

error_1:
    printf("Error: nf()\n");
}
