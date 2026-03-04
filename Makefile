#
#  This file has been simplified and modified from the original project SPRIGHT for the purposes of test and experiment the latency impact of real-time signals in microsservices.
# 


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

ifneq ($(shell pkg-config --exists libconfig && echo 0), 0)
$(error "libconfig is not installed")
endif


CFLAGS  = $(shell pkg-config --cflags libconfig )
LDFLAGS = $(shell pkg-config --libs-only-L libconfig ) -L/usr/lib64
LDLIBS  = $(shell pkg-config --libs-only-l libconfig )

#-MP -O3 -Wall -Werror -DLOG_USE_COLOR
CFLAGS += -Isrc/include -Isrc/cstl/inc -Isrc/log -MMD -MP -O3 -Wall -DLOG_USE_COLOR 
#CFLAGS += -Isrc/include -Isrc/cstl/inc -Isrc/log -MMD -MP -O3 -Wall -DLOG_USE_COLOR -fsanitize=address 
LDLIBS += -lbpf -lm -pthread -luuid -lrt -lxdp
LBXDP  += -lxdp
GERA_SKEL = bpftool gen skeleton


CLANG = clang
CLANGFLAGS = -g -O2
BPF_FLAGS = -target bpf

COMMON_OBJS = src/log/log.o src/utility.o src/timer.o src/io_helper.o src/common.o src/sigshared.o

.PHONY: all shm_mgr gateway nf clean


all: sigshared bin shm_mgr gateway adservice currencyservice \
		emailservice paymentservice shippingservice productcatalogservice \
		cartservice recommendationservice frontendservice checkoutservice \
		#ebpf/sigshared_kern.o  #ebpf/xsk_kern.o sockmap_manager 

sigshared: ebpf/xsk_kern.o 
	
ebpf/xsk_kern.o: ebpf/xsk_kern.c 
	@ sudo bpftool btf dump file /sys/kernel/btf/vmlinux format c > ebpf/vmlinux.h
	@ $(CLANG) $(CLANGFLAGS) $(BPF_FLAGS) -c $^ -o $@
	@ sudo bpftool gen skeleton ebpf/xsk_kern.o > ebpf/xsk_kern.skel.h

vmlinux: ebpf/xsk_kern.o
	@ sudo bpftool btf dump file /sys/kernel/btf/vmlinux format c > ebpf/vmlinux.h
	@ sudo bpftool gen skeleton ebpf/xsk_kern.o > ebpf/xsk_kern.skel.h


#######################################################################

shm_mgr:  bin/shm_mgr_sigshared  

bin/shm_mgr_sigshared: src/io_sigshared.o src/shm_mgr.o $(COMMON_OBJS)
	@ echo "CC $@"
	@ $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

gateway: bin/gateway_sigshared  

bin/gateway_sigshared: src/io_sigshared.o src/gateway.o $(COMMON_OBJS) 
	@ echo "CC $@"
	@ $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)


bin/nf_sigshared: src/io_sigshared.o src/nf.o $(COMMON_OBJS)
	@ echo "CC $@"
	@ $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

adservice: bin/nf_adservice_sigshared 


bin/nf_adservice_sigshared: src/io_sigshared.o src/online_boutique/adservice.o $(COMMON_OBJS)
	@ echo "CC $@"
	@ $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

currencyservice:  bin/nf_currencyservice_sigshared 


bin/nf_currencyservice_sigshared: src/io_sigshared.o src/online_boutique/currencyservice.o $(COMMON_OBJS)
	@ echo "CC $@"
	@ $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS) ./src/cstl/src/libclib.a

emailservice:  bin/nf_emailservice_sigshared 


bin/nf_emailservice_sigshared: src/io_sigshared.o src/online_boutique/emailservice.o $(COMMON_OBJS)
	@ echo "CC $@"
	@ $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

paymentservice:  bin/nf_paymentservice_sigshared 


bin/nf_paymentservice_sigshared: src/io_sigshared.o src/online_boutique/paymentservice.o $(COMMON_OBJS)
	@ echo "CC $@"
	@ $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

shippingservice:  bin/nf_shippingservice_sigshared 


bin/nf_shippingservice_sigshared: src/io_sigshared.o src/online_boutique/shippingservice.o $(COMMON_OBJS)
	@ echo "CC $@"
	@ $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)

-include $(patsubst %.o, %.d, $(wildcard src/*.o))

productcatalogservice:  bin/nf_productcatalogservice_sigshared 


bin/nf_productcatalogservice_sigshared: src/io_sigshared.o src/online_boutique/productcatalogservice.o $(COMMON_OBJS)
	@ echo "CC $@"
	@ $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS) ./src/cstl/src/libclib.a

cartservice:  bin/nf_cartservice_sigshared 


bin/nf_cartservice_sigshared: src/io_sigshared.o src/online_boutique/cartservice.o $(COMMON_OBJS)
	@ echo "CC $@"
	@ $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS) ./src/cstl/src/libclib.a

recommendationservice:  bin/nf_recommendationservice_sigshared 


bin/nf_recommendationservice_sigshared: src/io_sigshared.o src/online_boutique/recommendationservice.o $(COMMON_OBJS)
	@ echo "CC $@"
	@ $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)
-include $(patsubst %.o, %.d, $(wildcard src/*.o))

frontendservice:  bin/nf_frontendservice_sigshared 



bin/nf_frontendservice_sigshared: src/io_sigshared.o src/online_boutique/frontendservice.o $(COMMON_OBJS) src/shm_rpc.o
	@ echo "CC $@"
	@ $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)
-include $(patsubst %.o, %.d, $(wildcard src/*.o))

checkoutservice:  bin/nf_checkoutservice_sigshared 



bin/nf_checkoutservice_sigshared: src/io_sigshared.o src/online_boutique/checkoutservice.o $(COMMON_OBJS)
	@ echo "CC $@"
	@ $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDLIBS)
-include $(patsubst %.o, %.d, $(wildcard src/*.o))

%.o: %.c
	@ echo "CC $@"
	@ $(CC) -c $(CFLAGS) -o $@ $<

bin:
	@ mkdir -p $@

clean:
	@ echo "RM -r src/*.d src/*.o src/*/*.o src/*/*.d bin"
	@ $(RM) -r src/*.d src/*.o src/*/*.o src/*/*.d bin
	@ echo "rm ebpf/*.o ebpf/*.h dados/*"
	@ $(RM) ebpf/*.o ebpf/*.h 
	@ $(RM) dados/*

format:
	@ clang-format -i src/*.c src/include/*.h src/online_boutique/*.c src/log/*.c src/log/*.h
