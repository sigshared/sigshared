# SPRIGHT's python script modified to compare both SPRGIHT and SIGSHARED 

import sys
import matplotlib
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

import csv

plt.rcParams['axes.labelsize'] = 15  
plt.rcParams['xtick.labelsize'] = 13  
plt.rcParams['ytick.labelsize'] = 13  
plt.rcParams['hatch.linewidth'] = 0.5
matplotlib.rcParams['pdf.fonttype'] = 42
matplotlib.rcParams['ps.fonttype'] = 42
plt.rcParams['font.sans-serif'] = ['Arial']

fileName_skmsg = ['skmsg_gw.cpu', 'skmsg_fn.cpu']
fileName_sigshared = ['sigshared_gw.cpu', 'sigshared_fn.cpu']

# SIGSHARED
sigshared_gw_cpu_ts = []
tmp_cpu = [0]
with open(fileName_sigshared[0]) as fp:
  line = fp.readline()
  line_list= line.split()
  while line:
    if len(line_list) == 0:
      line = fp.readline()
      line_list= line.split()
      continue
    if line_list[0] == 'Linux':
      line = fp.readline()
      line_list= line.split()
      continue
    if line_list[0] == 'Average:':
      aggregated_cpu_1_second = sum(tmp_cpu)
      sigshared_gw_cpu_ts.append(aggregated_cpu_1_second)
      break
    if line_list[1] == 'UID':
      aggregated_cpu_1_second = sum(tmp_cpu)
      sigshared_gw_cpu_ts.append(aggregated_cpu_1_second)
      tmp_cpu = [0]
    else:
      tmp_cpu.append(float(line_list[7]))
    line = fp.readline()
    line_list= line.split()
sigshared_gw_cpu_ts = sigshared_gw_cpu_ts[1:]

sigshared_fn_cpu_ts = []
tmp_cpu = []
with open(fileName_sigshared[1]) as fp:
  line = fp.readline()
  line_list= line.split()
  while line:
    if len(line_list) == 0:
      line = fp.readline()
      line_list= line.split()
      continue
    if line_list[0] == 'Linux':
      line = fp.readline()
      line_list= line.split()
      continue
    if line_list[0] == 'Average:':
      aggregated_cpu_1_second = sum(tmp_cpu)
      sigshared_fn_cpu_ts.append(aggregated_cpu_1_second)
      break
    if line_list[1] == 'UID':
      aggregated_cpu_1_second = sum(tmp_cpu)
      sigshared_fn_cpu_ts.append(aggregated_cpu_1_second)
      tmp_cpu = [0]
    else:
      tmp_cpu.append(float(line_list[7]))
    line = fp.readline()
    line_list= line.split()
sigshared_fn_cpu_ts = sigshared_fn_cpu_ts[1:]


# SPRIGHT
skmsg_gw_cpu_ts = []
tmp_cpu = [0]
with open(fileName_skmsg[0]) as fp:
  line = fp.readline()
  line_list= line.split()
  while line:
    if len(line_list) == 0:
      line = fp.readline()
      line_list= line.split()
      continue
    if line_list[0] == 'Linux':
      line = fp.readline()
      line_list= line.split()
      continue
    if line_list[0] == 'Average:':
      aggregated_cpu_1_second = sum(tmp_cpu)
      skmsg_gw_cpu_ts.append(aggregated_cpu_1_second)
      break
    if line_list[1] == 'UID':
      aggregated_cpu_1_second = sum(tmp_cpu)
      skmsg_gw_cpu_ts.append(aggregated_cpu_1_second)
      tmp_cpu = [0]
    else:
      line_list = line.split()
      tmp_cpu.append(float(line_list[7]))
    
    line = fp.readline()
    line_list= line.split()
skmsg_gw_cpu_ts = skmsg_gw_cpu_ts[1:]

skmsg_fn_cpu_ts = []
tmp_cpu = []
with open(fileName_skmsg[1]) as fp:
  line = fp.readline()
  line_list= line.split()
  while line:
    if len(line_list) == 0:
      line = fp.readline()
      line_list= line.split()
      continue
    if line_list[0] == 'Linux':
      line = fp.readline()
      line_list= line.split()
      continue
    if line_list[0] == 'Average:':
      aggregated_cpu_1_second = sum(tmp_cpu)
      skmsg_fn_cpu_ts.append(aggregated_cpu_1_second)
      break
    if line_list[1] == 'UID':
      aggregated_cpu_1_second = sum(tmp_cpu)
      skmsg_fn_cpu_ts.append(aggregated_cpu_1_second)
      tmp_cpu = [0]
    else:
      line_list = line.split()
      tmp_cpu.append(float(line_list[7]))
    line = fp.readline()
    line_list= line.split()
skmsg_fn_cpu_ts = skmsg_fn_cpu_ts[1:]


figsize = 16,9 
figure, ax = plt.subplots(figsize=figsize)

timestamps = [x for x in range(1, len(sigshared_gw_cpu_ts) + 1, 1)]
p1 = plt.plot(timestamps, sigshared_gw_cpu_ts, marker = '', ms = 4, mew = 2, label = '', linewidth=2.5, linestyle="-", color='tab:red')
p2 = plt.plot(timestamps, sigshared_fn_cpu_ts, marker = '', ms = 4, mew = 2, label = '', linewidth=2.5, linestyle="-", color='limegreen')
timestamps = [x for x in range(1, len(skmsg_gw_cpu_ts) + 1, 1)]

# Collecting eBPF CPU values
timestamps_cpu = [x for x in range(1, len(skmsg_gw_cpu_ts)+1 , 1)]
fileName_ebpf = "ebpf.CPU"

bpf = open(fileName_ebpf) 
lines = bpf.readlines()
bpf_cpu = []

for i in lines:
    cpu = i[8]+i[9]+i[10]
    bpf_cpu.append(float(cpu))
print(bpf_cpu)

skmsg_bpf = []
print(len(skmsg_fn_cpu_ts), len(bpf_cpu))

for i in range(0, len(skmsg_fn_cpu_ts)):
    skmsg_bpf.append( skmsg_fn_cpu_ts[i]  +  bpf_cpu[i])
print(skmsg_bpf)


p5 = plt.plot(timestamps, skmsg_gw_cpu_ts, marker = '', ms = 4, mew = 2, label = '', linewidth=2.5, linestyle="-", color='tab:blue')
p6 = plt.plot(timestamps, skmsg_fn_cpu_ts, marker = '', ms = 4, mew = 2, label = '', linewidth=2.5, linestyle="-", color='black')
p7 = plt.plot(timestamps_cpu, skmsg_bpf, marker = '', ms = 4, mew = 2, label = '', linewidth=2.5, linestyle="-", color='blue')

plt.tick_params(grid_linewidth = 3, grid_linestyle = ':', pad=0)
figure.subplots_adjust(bottom=0.25, left=0.2)
plt.setp(ax.get_xticklabels(), 
  ha="center",
  rotation_mode="anchor")


plt.grid(color = 'gray', linestyle = ':', linewidth = 1)
plt.xlim((0, 180))
plt.ylim((0, 500))
plt.yticks(np.arange(0, 400.1, 50), fontsize=25) 
plt.ylabel('CPU Usage (%)', labelpad = 0, fontsize=25)

plt.xticks(np.arange(0, 180.1, 20), fontsize=25)
plt.xlabel('Timestamp (seconds)', labelpad=0, fontsize=35)

prop = dict(size=30)
plt.legend((p1[0], p2[0],p5[0], p6[0], p7[0]), 
  ('SIGSHARED GW', 'SIGSHARED FN', 'SPRIGHT GW', 'SPRIGHT FN', 'SPRIGHT FN+eBPF'),
  loc = "upper right",
  ncol = 1,
  prop = prop,
  frameon = True,
  columnspacing = 0.7,
  labelspacing = 0.1,
  edgecolor = 'black',
  fontsize=15)

#plt.savefig('cpu.pdf', bbox_inches='tight', dpi=600)
plt.savefig('fig-11.pdf', bbox_inches='tight', dpi=600)
plt.show()
