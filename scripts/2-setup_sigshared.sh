#!/bin/bash

# 1- CMake 
# 2- LLVM
# 3- libbpf v1.6
# 4- bpftool v1.6
# 5- libxdp

cd /mydata

# prerequisitos do LLVM
sudo apt install -y ninja-build python3 git \
  build-essential libedit-dev libncurses5-dev zlib1g-dev \
  libxml2-dev libsqlite3-dev swig libssl-dev zlib1g zlib1g-dev conntrack

##################################################
# CMake
#git clone https://github.com/Kitware/CMake
git clone -b v4.2.0 https://github.com/Kitware/CMake
cd CMake
./bootstrap
make -j$(nproc)
sudo make install
cd ../

##################################################
# LLVM --> N TA INSTALANDO NO SISTEMA 
git clone --depth 1 https://github.com/llvm/llvm-project.git

cd llvm-project
mkdir build
cd build

# Compila o clang e o lld e poe na pasta /usr/local que eh o default
# Compila tbm para as arquiteturas x_86 e BPF
cmake -G Ninja ../llvm \
  -DLLVM_ENABLE_PROJECTS="clang;lld" \
  -DCMAKE_BUILD_TYPE=Release \
  -DLLVM_TARGETS_TO_BUILD="BPF;X86" \
  -DCMAKE_INSTALL_PREFIX=/usr/local

ninja
sudo ninja install

#cd ../../sudo make install
cd ../../

##################################################
# libbpf v1.6
git clone -b v1.6.0 https://github.com/libbpf/libbpf

cd libbpf/src
make -j$(nproc)
sudo make install
echo "/usr/lib64/" | sudo tee -a /etc/ld.so.conf

sudo ldconfig

cd ../../

##################################################
# BPFTOOL
sudo apt install libbfd-dev libcap-dev libbpf-dev

git clone -b v7.6.0 --recurse-submodules https://github.com/libbpf/bpftool
cd bpftool/src
make -j$(nproc)
sudo make install

cd ../../

##################################################
# libxdp
git clone --recurse-submodules https://github.com/xdp-project/xdp-tools

cd ./xdp-tools/
./configure
make -j$(nproc)
sudo make install

sudo ldconfig

cd ..

##################################################

echo "export SIGSHARED=/mydata/sigshared" >> ~/.bashrc
source ~/.bashrc

cd /mydata/sigshared
mkdir dados
sudo mount -t bpf bpffs /sys/fs/bpf
sudo mount --bind /sys/fs/bpf ./dados;

echo " "
echo "##############"
echo "To collect eBPF CPU stats:"
echo "sudo su echo 1 > /proc/sys/kernel/bpf_stats_enabled "

echo "add this line in /etc/default/grub"
echo "GRUB_CMDLINE_LINUX_DEFAULT="intel_pstate=disable intel_idle.max_cstate=0  nosmt isolcpus=0,1,2,3,4,5,6,7,8,9""
echo "sudo update-grub && sudo reboot"

echo "please: source ~/.bashrc"
echo "##############"

#################################################

    # To desativate the CPU boost
# echo 0 | sudo tee /sys/devices/system/cpu/cpufreq/boost
    # To lock the CPU frequency
# sudo cpupower -c all frequency-set -d 2.2GHz -u 2.2GHz
# sudo cpupower -c all frequency-set -g ondemand 
