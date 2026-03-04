import matplotlib.pyplot as plt
import numpy as np
from matplotlib.patches import Polygon
import matplotlib as mpl


mpl.rcParams['hatch.linewidth'] = 0.3


plt.rcParams['axes.labelsize'] = 15  # xy-axis label size
plt.rcParams['xtick.labelsize'] = 13  # x-axis ticks size
plt.rcParams['ytick.labelsize'] = 13  # y-axis ticks size
plt.rcParams['hatch.linewidth'] = 0.5
mpl.rcParams['pdf.fonttype'] = 42
mpl.rcParams['ps.fonttype'] = 42

plt.rcParams['font.sans-serif'] = ['Arial']
plt.rcParams["figure.figsize"] = (16, 9)



irq = []    
dataset = [] 
tam_dataset = 0
def coleta_dados(fileName):
    data_rest_1_d = [] # GET /1
    data_rest_2_d = [] # POST /1/setCurrency
    data_rest_3_d = [] # GET /1/product?
    data_rest_4_d = [] # GET /1/cart
    data_rest_5_d = [] # POST /1/cart?product_id=
    data_rest_6_d = [] # POST /1/cart/checkout?

    data_rest_1_ts = []
    data_rest_2_ts = []
    data_rest_3_ts = []
    data_rest_4_ts = []
    data_rest_5_ts = []
    data_rest_6_ts = []
    
    data = []

    with open(fileName) as fp:
      line = fp.readline()
      timestamp_data = int(float(line.split(";")[0]))

      new_timestamp = timestamp_data + 40
  
      while timestamp_data < new_timestamp:
        line = fp.readline()
        timestamp_data = int(float(line.split(";")[0]))
  
      line = fp.readline()
      while line:
        line_list = line.split(";")
        
        data.append(float(line_list[3].split("\n")[0]))
        if line_list[1] == "GET":

          if line_list[2].find("/1/product?") != -1:
            data_rest_3_d.append(float(line_list[3].split("\n")[0]))
            data_rest_3_ts.append(float(line_list[0]))

          elif line_list[2].find("/1/cart") != -1:
            data_rest_4_d.append(float(line_list[3].split("\n")[0]))
            data_rest_4_ts.append(float(line_list[0]))

          elif line_list[2].find("/1") != -1:
            data_rest_1_d.append(float(line_list[3].split("\n")[0]))
            data_rest_1_ts.append(float(line_list[0]))

          else:
            print("Unknown (GET) Request Name!")

        elif line_list[1] == "POST":

          if line_list[2].find("/1/setCurrency") != -1:
            data_rest_2_d.append(float(line_list[3].split("\n")[0]))
            data_rest_2_ts.append(float(line_list[0]))

          elif line_list[2].find("/1/cart?product_id") != -1:
            data_rest_5_d.append(float(line_list[3].split("\n")[0]))
            data_rest_5_ts.append(float(line_list[0]))

          elif line_list[2].find("/1/cart/checkout?") != -1:
            data_rest_6_d.append(float(line_list[3].split("\n")[0]))
            data_rest_6_ts.append(float(line_list[0]))

          else:
            print("Unknown (POST) Request Name!")

        else:
          print("Unknown Request Type!")

        line = fp.readline()
        if len(line) == 0:
          break

    return data 


##################################################################################################
def draw_cdf(p_sig, p_sig99, p_sp, p_sp99):
    

    figure, ax = plt.subplots()
    prop = dict(size=35)


    sigshared99 = ax.axvline(p_sig99  , marker = '' , linewidth=5, linestyle="-", label=f"99th sigshared 10K= {p_sig99:.2f}",   color='tab:blue')
    spright99   = ax.axvline(p_sp99   , marker = '' , linewidth=5, linestyle="-", label=f"99th sigshared 12K= {p_sp99:.2f}",   color='tab:red')


    cdf_sig = ax.plot(p_sig, percentiles, marker  = '',  ms = 1, mew = 2, label = 'Sig: 10K', linewidth=7, linestyle="-",  color='tab:blue')
    cdf_sp  = ax.plot(p_sp , percentiles, marker  = '',  ms = 1, mew = 2, label = 'Sig: 12K', linewidth=7, linestyle="-",  color='tab:red')
    
    ax.tick_params(grid_linewidth = 4, grid_linestyle = ':', pad=0)

    plt.grid(color = 'gray', linestyle = ':', linewidth = 1.5)
   
    plt.ylim((0, 101))
    plt.yticks(np.arange(0, 100.001, 10), fontsize=35)
    plt.ylabel('% of requests', labelpad=-5, fontsize=35)
    
    plt.xticks(fontsize=35)
    plt.xlabel('Response time CDF(ms)', labelpad=0, fontsize=35)

    legend1 = ax.legend( handles=[cdf_sig[0], cdf_sp[0] ],
               loc = "lower right", 
               ncol = 2, 
               prop = prop, 
               borderaxespad = 0, 
               frameon = True, 
               columnspacing = 0.7, 
               labelspacing = 0.05, 
               fancybox = False, 
               edgecolor = 'black')

    legend2 = ax.legend(handles=[ sigshared99, spright99],  loc="center right", fontsize=35)
    ax.add_artist(legend1)

    plt.savefig('fig-12.pdf' , dpi=figure.dpi,pad_inches=0.01)

    plt.show()
    return



##################################################################################################
if __name__ == '__main__':
   
    teste_sig     = list(range(15)) 
    teste_spright = list(range(15))

    print("Runnig...")
   
    for i in range(1, 13):
        file_sig = "./sigshared/" + str(i) + "K-latency_of_each_req_stats_sigshared.csv"
        file_sp  = "./spright/"   + str(i) + "K-latency_of_each_req_stats_s-spright.csv"
        
        print(f"Reading {file_sig}...")
        print(f"Reading {file_sp}...\n")

        sig[i]     = coleta_dados(file_sig) 
        spright[i] = coleta_dados(file_sp) 



    p_sig   = list(range(13))
    p_sig99 = list(range(13))
    
    p_sp    = list(range(13))
    p_sp99  = list(range(13))

    percentiles = [x for x in range(0, 101, 1)]
    for i in range(1,13):
        p_sig[i]   = np.percentile(np.sort(sig[i]), percentiles)
        p_sig99[i] = np.percentile(np.sort(sig[i]), 99)
        
        p_sp[i]    = np.percentile(np.sort(spright[i]), percentiles)
        p_sp99[i]  = np.percentile(np.sort(spright[i]), 99)


    draw_cdf(p_sig[10], p_sig99[10], p_sig[12], p_sig99[12])

