import matplotlib.pyplot as plt
import numpy as np
from matplotlib.patches import Polygon
import matplotlib as mpl

plt.rcParams['font.sans-serif'] = ['Arial']
plt.rcParams["figure.figsize"] = (16, 9)
mpl.rcParams['pdf.fonttype'] = 42
mpl.rcParams['ps.fonttype'] = 42


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
    
    with open(fileName) as fp:
      
      lines = fp.readlines()
      lines.sort(key=lambda line: float(line.split(';')[0]))
      
      timestamp_dpdk = int(float(lines[0].split(";")[0]))
      print( timestamp_dpdk )

      new_timestamp = int(timestamp_dpdk) + 60 


      for line in lines:
        if timestamp_dpdk < new_timestamp:
            timestamp_dpdk = int(float(line.split(";")[0]))
            continue
        
        line_list = line.split(";")
        
        dataset.append(float(line_list[3].split("\n")[0]))
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

        if len(line) == 0:
          break

    return data_rest_1_d,  data_rest_2_d, data_rest_3_d, data_rest_4_d, data_rest_5_d 


##################################################################################################
def draw_plot_comparativo(dados_sig, dados_spright):
    labels = ['CH-1', 'CH-2', 'CH-3', 'CH-4', 'CH-5']
    
    data_sig = [dados_sig[0], dados_sig[1], dados_sig[2], dados_sig[3], dados_sig[4]]
    data_sp = [dados_spright[0], dados_spright[1], dados_spright[2], dados_spright[3], dados_spright[4]]

    
    posicoes = np.array(range(len(labels))) + 1
    offset = 0.2  

    b1 = plt.boxplot(data_sig, vert=False, positions=posicoes + offset, widths=0.3, 
                     patch_artist=True, showfliers=False, boxprops=dict(facecolor="lightblue"))
    
    b2 = plt.boxplot(data_sp, vert=False, positions=posicoes - offset, widths=0.3, 
                     patch_artist=True, showfliers=False, boxprops=dict(facecolor="tab:red"))

    plt.yticks(posicoes, labels, size=35)
    plt.xlabel('Latency (ms)', fontsize=35)
   
    plt.xticks(size=30)
    plt.legend([b1["boxes"][0], b2["boxes"][0]], ['SIGSHARED', 'SPRIGHT'], loc='upper right', fontsize=30, edgecolor="black",   columnspacing = 0.7, labelspacing = 0.1)
    
    plt.grid(axis='x', linestyle='--', alpha=0.6)
    plt.tight_layout()
    
    plt.savefig("fig-9.pdf")
    plt.show()


##################################################################################################
if __name__ == '__main__':
    
    print("Rodando...")
    ch_1, ch_2, ch_3, ch_4, ch_5 = coleta_dados("latency_of_each_req_stats_sigshared.csv")
    ch_1sp, ch_2sp, ch_3sp, ch_4sp, ch_5sp = coleta_dados("latency_of_each_req_stats_s-spright.csv")
    
    dataset_sorted = sorted(dataset)
    tam_data_set = len(dataset)

    ch_1_sorted = np.sort(ch_1)
    ch_2_sorted = np.sort(ch_2)
    ch_3_sorted = np.sort(ch_3)
    ch_4_sorted = np.sort(ch_4)
    ch_5_sorted = np.sort(ch_5)
    sig_data = [ch_1, ch_2, ch_3, ch_4, ch_5]

    ch_1sp_sorted = np.sort(ch_1sp)
    ch_2sp_sorted = np.sort(ch_2sp)
    ch_3sp_sorted = np.sort(ch_3sp)
    ch_4sp_sorted = np.sort(ch_4sp)
    ch_5sp_sorted = np.sort(ch_5sp)
    sp_data = [ch_1sp, ch_2sp, ch_3sp, ch_4sp, ch_5sp]
 
    draw_plot_comparativo(sig_data, sp_data)

