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

#include "../cstl/inc/c_lib.h"
#include "../include/http.h"
#include "../include/io.h"
#include "../include/spright.h"

#include "../sigshared.h"


int mapa_fd;
int matriz[11][2] = {0};
sigset_t set;



static int pipefd_rx[UINT8_MAX][2];
static int pipefd_tx[UINT8_MAX][2];

static int compare_e(void *left, void *right)
{
    return strcmp((const char *)left, (const char *)right);
}

struct clib_map *LocalCartStore;

static void PrintUserCart(Cart *cart)
{
    log_debug("Cart for user %s: ", cart->UserId);
    log_debug("## %d items in the cart: ", cart->num_items);
    int i;
    for (i = 0; i < cart->num_items; i++)
    {
        log_debug("\t%d. ProductId: %s \tQuantity: %d", i + 1, cart->Items[i].ProductId, cart->Items[i].Quantity);
    }
    printf("\n");
    return;
}

static void PrintLocalCartStore()
{
    log_debug("\t\t #### PrintLocalCartStore ####");

    struct clib_iterator *myItr;
    struct clib_object *pElement;
    myItr = new_iterator_c_map(LocalCartStore);
    pElement = myItr->get_next(myItr);

    while (pElement)
    {
        void *cart = myItr->get_value(pElement);
        PrintUserCart((Cart *)cart);
        free(cart);
        pElement = myItr->get_next(myItr);
    }
    delete_iterator_c_map(myItr);
    printf("\n");
}

static void AddItemAsync(char *userId, char *productId, int32_t quantity)
{
    log_debug("AddItemAsync called with userId=%s, productId=%s, quantity=%d", userId, productId, quantity);

    Cart newCart = {.UserId = "", .Items = {{.ProductId = "", .Quantity = quantity}}};

    strcpy(newCart.UserId, userId);
    strcpy(newCart.Items[0].ProductId, productId);

    void *cart;
    if (clib_true != find_c_map(LocalCartStore, userId, &cart))
    {
        log_debug("Add new carts for user %s", userId);
        char *key = clib_strdup(userId);
        int key_length = (int)strlen(key) + 1;
        newCart.num_items = 1;
        log_debug("Inserting [%s -> %s]", key, newCart.UserId);
        insert_c_map(LocalCartStore, key, key_length, &newCart, sizeof(Cart));
        free(key);
    }
    else
    {
        log_debug("Found carts for user %s", userId);
        int cnt = 0;
        int i;
        for (i = 0; i < ((Cart *)cart)->num_items; i++)
        {
            if (strcmp(((Cart *)cart)->Items[i].ProductId, productId) == 0)
            { // If the item exists, we update its quantity
                log_debug("Update carts for user %s - the item exists, we update its quantity", userId);
                ((Cart *)cart)->Items[i].Quantity++;
            }
            else
            {
                cnt++;
            }
        }

        if (cnt == ((Cart *)cart)->num_items)
        { // The item doesn't exist, we update it into DB
            log_debug("Update carts for user %s - The item doesn't exist, we update it into DB", userId);
            ((Cart *)cart)->num_items++;
            strcpy(((Cart *)cart)->Items[((Cart *)cart)->num_items].ProductId, productId);
            ((Cart *)cart)->Items[((Cart *)cart)->num_items].Quantity = quantity;
        }
    }
    return;
}

static void MockAddItemRequest(struct http_transaction *txn)
{
    AddItemRequest *in = &txn->add_item_request;
    strcpy(in->UserId, "spright-online-boutique");
    strcpy(in->Item.ProductId, "OLJCESPC7Z");
    in->Item.Quantity = 5;
    return;
}

static void AddItem(struct http_transaction *txn)
{
    log_debug("[AddItem] received request");

    AddItemRequest *in = &txn->add_item_request;
    AddItemAsync(in->UserId, in->Item.ProductId, in->Item.Quantity);
    return;
}

static void GetCartAsync(struct http_transaction *txn)
{
    GetCartRequest *in = &txn->get_cart_request;
    Cart *out = &txn->get_cart_response;
    log_debug("[GetCart] GetCartAsync called with userId=%s", in->UserId);

    void *cart;
    if (clib_true != find_c_map(LocalCartStore, in->UserId, &cart))
    {
        log_debug("No carts for user %s", in->UserId);
        out->num_items = 0;
        return;
    }
    else
    {
        *out = *(Cart *)cart;
        return;
    }
}

static void GetCart(struct http_transaction *txn)
{
    GetCartAsync(txn);
    return;
}

static void MockGetCartRequest(struct http_transaction *txn)
{
    GetCartRequest *in = &txn->get_cart_request;
    strcpy(in->UserId, "spright-online-boutique");
    return;
}

static void PrintGetCartResponse(struct http_transaction *txn)
{
    log_debug("\t\t#### PrintGetCartResponse ####");
    Cart *out = &txn->get_cart_response;
    log_debug("Cart for user %s: ", out->UserId);
    int i;
    for (i = 0; i < out->num_items; i++)
    {
        log_debug("\t%d. ProductId: %s \tQuantity: %d", i + 1, out->Items[i].ProductId, out->Items[i].Quantity);
    }
    printf("\n");
    return;
}

static void EmptyCartAsync(struct http_transaction *txn)
{
    EmptyCartRequest *in = &txn->empty_cart_request;
    log_debug("EmptyCartAsync called with userId=%s", in->UserId);

    void *cart;
    if (clib_true != find_c_map(LocalCartStore, in->UserId, &cart))
    {
        log_debug("No carts for user %s", in->UserId);
        // out->num_items = -1;
        return;
    }
    else
    {
        int i;
        for (i = 0; i < ((Cart *)cart)->num_items; i++)
        {
            log_debug("Clean up item %d", i + 1);
            strcpy((*((Cart **)(&cart)))->Items[i].ProductId, "");
            ((*((Cart **)(&cart))))->Items[i].Quantity = 0;
        }
        PrintUserCart((Cart *)cart);
        return;
    }
}

static void EmptyCart(struct http_transaction *txn)
{
    log_debug("[EmptyCart] received request");
    EmptyCartAsync(txn);
    return;
}

static void MockEmptyCartRequest(struct http_transaction *txn)
{
    EmptyCartRequest *in = &txn->empty_cart_request;
    strcpy(in->UserId, "spright-online-boutique");
}

static void *nf_worker(void *arg)
{
    struct http_transaction *txn = NULL;
    ssize_t bytes_written;
    ssize_t bytes_read;
    uint8_t index;

    /* TODO: Careful with this pointer as it may point to a stack */
    index = (uint64_t)arg;

    while (1)
    {
        bytes_read = read(pipefd_rx[index][0], &txn, sizeof(struct http_transaction *));
        if (unlikely(bytes_read == -1))
        {
            log_error("read() error: %s", strerror(errno));
            return NULL;
        }

        if (strcmp(txn->rpc_handler, "AddItem") == 0)
        {
            AddItem(txn);
        }
        else if (strcmp(txn->rpc_handler, "GetCart") == 0)
        {
            GetCart(txn);
        }
        else if (strcmp(txn->rpc_handler, "EmptyCart") == 0)
        {
            EmptyCart(txn);
        }
        else
        {
            log_debug("%s() is not supported", txn->rpc_handler);
            log_debug("\t\t#### Run Mock Test ####");
            MockAddItemRequest(txn);
            AddItem(txn);
            PrintLocalCartStore();

            MockGetCartRequest(txn);
            GetCart(txn);
            PrintGetCartResponse(txn);

            MockEmptyCartRequest(txn);
            EmptyCart(txn);
            PrintLocalCartStore();
        }


        txn->next_fn = txn->caller_fn;
        txn->caller_fn = CART_SVC;

        bytes_written = write(pipefd_tx[index][1], &txn, sizeof(struct http_transaction *));
        if (unlikely(bytes_written == -1)){
            log_error("write() error: %s", strerror(errno));
            return NULL;
        }
    }

    return NULL;
}

static void *nf_rx(void *arg){

    struct http_transaction *txn = NULL;
    ssize_t bytes_written;
    uint8_t i;

    for (i = 0;; i = (i + 1) % sigshared_cfg->nf[fn_id - 1].n_threads){
        //io_rx((void **)&txn, sigshared_ptr, &set);
        io_rx((void **)&txn, sigshared_ptr, &set, matriz);

	if(unlikely(txn == NULL)){
		printf("==cart== ERRO mempool_access retornou NULL\n");
		return NULL;
	}

	bytes_written = write(pipefd_rx[i][1], &txn, sizeof(struct http_transaction *));
        if (unlikely(bytes_written == -1)){
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



static void *nf_tx(void *arg)
{
    struct epoll_event event[UINT8_MAX]; /* TODO: Use Macro */
    struct http_transaction *txn = NULL;
    ssize_t bytes_read;
    uint8_t i;
    int n_fds;
    int epfd;
    int ret;
    int ret_io;
    int pid = getpid();

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
            if (unlikely(bytes_read == -1))
            {
                log_error("read() error: %s", strerror(errno));
                return NULL;
            }

            //log_debug("Route id: %u, Hop Count %u, Next Hop: %u, Next Fn: %u, Caller Fn: %s (#%u), RPC Handler: %s()", txn->route_id, txn->hop_count, sigshared_cfg->route[txn->route_id].hop[txn->hop_count], txn->next_fn, txn->caller_nf, txn->caller_fn, txn->rpc_handler);

	    ret_io = io_tx_matriz(txn->addr, txn->next_fn, &mapa_fd, pid, matriz, matriz[txn->next_fn][1]);
            if (unlikely(ret_io == -1))
            {
                log_error("io_tx() error");
                return NULL;
            }
        }
    }

    return NULL;
}

/* TODO: Cleanup on errors */
static int nf(uint8_t nf_id)
{
    pthread_t thread_worker[UINT8_MAX];
    pthread_t thread_rx;
    pthread_t thread_tx;
    uint8_t i;
    int ret;

    fn_id = nf_id;
    int pid = getpid();

    matriz[nf_id][1] = pid;
    if( sigshared_update_map("mapa_sinal", fn_id, pid, &mapa_fd) < 0  ){
        printf("Erro ao atualizar mapa\n");
        return 0;
    }

    char temp[256];
    char *dir_temp = getenv("SIGSHARED");
    sprintf(temp, "%s/dados/mapa_sinal", dir_temp);
    int fd_mapa_sinal = bpf_obj_get(temp);

    int pid_front, key_front = 1;
    if( bpf_map_lookup_elem(fd_mapa_sinal, &key_front, &pid_front ) < 0 ){
	    perror("Error to lookup the gateway's PID");
	    exit(1);
    }

    matriz[1][1] = pid_front;
    sigval_t data_send;
    data_send.sival_ptr = (void *)(uintptr_t)nf_id;
    if(sigqueue(pid_front, SIGRTMIN+2, data_send) < 0){
        perror("Error to send signal to gateway");
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

    for (i = 0; i < sigshared_cfg->nf[fn_id - 1].n_threads; i++)
    {
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



int main(int argc, char **argv)
{
    log_set_level_from_env();
    log_set_level(LOG_INFO);

    char settar_cpuf[30];
    log_info("Pinning process in CPU 5...\n");
    sprintf(settar_cpuf, "taskset -cp 5 %d", getpid());
    int ret_sys = system(settar_cpuf);
    if( ret_sys == -1 || ret_sys == 127 ){
            log_error("Error to pinn CPU");
            exit(1);
    }

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
    sigprocmask(SIG_BLOCK, &set, NULL);


    errno = 0;
    nf_id = strtol(argv[argc-1], NULL, 10);
    if (unlikely(errno != 0 || nf_id < 1))
    {
        log_error("Invalid value for Network Function ID");
        goto error_1;
    }

    LocalCartStore = new_c_map(compare_e, NULL, NULL);
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
