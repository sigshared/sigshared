import matplotlib.pyplot as plt
import numpy as np
from matplotlib.patches import Polygon
import matplotlib as mpl

plt.rcParams['font.sans-serif'] = ['Arial']
plt.rcParams["figure.figsize"] = (16, 9)

mpl.rcParams['pdf.fonttype'] = 42
mpl.rcParams['ps.fonttype'] = 42



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
def draw_plot_comparativo(dados_sig, dados_spright):
    labels = ['1K', '2K', '3K', '4K', '5K', '6K', '7K', '8K', '9K', '10K']
    
    data_sig = [dados_sig[1], dados_sig[2], dados_sig[3], dados_sig[4], dados_sig[5], dados_sig[6], dados_sig[7], dados_sig[8], dados_sig[9], dados_sig[10]]
    data_sp = [dados_spright[1], dados_spright[2], dados_spright[3], dados_spright[4], dados_spright[5], dados_spright[6], dados_spright[7], dados_spright[8], dados_spright[9], dados_spright[10]]

    plt.figure(figsize=(16, 9))
    
    posicoes = np.array(range(len(labels))) - 2 
    offset = 0.2  # Distância entre as barras do par

    print(len(data_sig), len(labels))
    b1 = plt.boxplot(data_sig, vert=True, positions=posicoes - offset, widths=0.3, 
                     patch_artist=True, showfliers=False, boxprops=dict(facecolor="lightblue"))
    
    b2 = plt.boxplot(data_sp, vert=True, positions=posicoes + offset, widths=0.3, 
                     patch_artist=True, showfliers=False, boxprops=dict(facecolor="tab:red"))

    # Configurações de layout
    plt.yticks( size=35)
    plt.xticks(posicoes, labels, size=30)
    plt.xlabel('RPS', fontsize=35)
    plt.ylabel('Latency (ms)', fontsize=35)
   
    plt.legend([b1["boxes"][0], b2["boxes"][0]], ['SIGSHARED', 'SPRIGHT'], loc='upper left', fontsize=30, edgecolor="black")
    
    plt.grid(axis='y', linestyle='--', alpha=0.6)
    plt.tight_layout()
    
    #plt.savefig("boxplot_all_RPS.pdf")
    plt.savefig("fig-7.pdf")
    plt.show()

##################################################################################################
if __name__ == '__main__':
   
    l_ch1 = list(range(13)) 
    l_ch2 = list(range(13)) 
    l_ch3 = list(range(13)) 
    l_ch4 = list(range(13)) 
    l_ch5 = list(range(13)) 

    l_ch1_sp = list(range(13)) 
    l_ch2_sp = list(range(13)) 
    l_ch3_sp = list(range(13)) 
    l_ch4_sp = list(range(13)) 
    l_ch5_sp = list(range(13)) 

    sig     = list(range(15)) 
    spright = list(range(15))

    print("Running...")
   
    for i in range(1, 13):
        file_sig = "./sigshared/" + str(i) + "K-latency_of_each_req_stats_sigshared.csv"
        file_sp  = "./spright/"   + str(i) + "K-latency_of_each_req_stats_s-spright.csv"
        
        print(f"Reading {file_sig}...")
        print(f"Reading {file_sp}...\n")

        sig[i]     = coleta_dados(file_sig) 
        spright[i] = coleta_dados(file_sp) 

    draw_plot_comparativo(sig, spright)

