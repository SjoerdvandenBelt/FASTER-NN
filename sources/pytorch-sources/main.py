from Logic.dataLoad import get_loader
from Logic.models import SweepNet, SweepNet1DV1, SweepNet1DV2, SweepNet1DV4, SweepNet1DDet, FAST_NN_dense, FAST_NN, FAST_NN_dense_buffer, FAST_NN_det_dense, SweepNet1DDetGrouped, FAST_NN_quantized
from Logic.testing import test_model, test_model_double_label
from Logic.training import train_model, train_model_double_label
import matplotlib.pyplot as plt
import torch
import numpy as np
import os
import getopt
import time
import sys
import json
    
def train(height, width, epochs, batch, platform, opath, ipath, model_class, use_bp_distance, load_binary, train_detect, infofilename, reduction, hotspot, labelnames, quantization):
    #print("Training model", opath, end=' ')
    
    with open(opath + "/classLabels.txt", "w") as f:
        classes = sorted(entry.name for entry in os.scandir(ipath) if entry.is_dir())
        f.write("***DO_NOT_REMOVE_OR_EDIT_THIS_FILE***\n")
        f.write(str(len(classes)))
        f.write("\n")
        for i, class_name in enumerate(classes):
            f.write(class_name)
            f.write(" ({})\n".format(i))
        f.write("***DO_NOT_REMOVE_OR_EDIT_THIS_FILE***")
       
    lr = 0.5e-3
    if use_bp_distance:
        channels = 2
    else:
        channels = 1
        
    if hotspot:
        outputs = 2
    else:
        outputs = 1
    
    if model_class == "FAST-NN":
        if quantization:
            model = FAST_NN_quantized(height, width, channels=channels, outputs=outputs)
        else:
            model = FAST_NN(height, width, channels=channels, outputs=outputs)
    elif model_class == "FASTER-NN":
        model = SweepNet1DDet(height, width, channels=channels, outputs=outputs)
    elif model_class == "SweepNetRecombination":
        model = SweepNet1DV4(height, width, channels=channels, outputs=outputs)
        lr = 5e-3
    elif model_class == "SweepNet":
        model = SweepNet(height, width, channels=channels, outputs=outputs)
    elif model_class == "FASTER-NN-group":
        model = SweepNet1DDetGrouped(height, width, channels=channels, outputs=outputs)
    else:
        raise ValueError("Unknown model class")
    
    if quantization:
        lr = 0.1e-3
        model.eval()
        print(torch.ao.quantization.get_default_qat_qconfig('x86'))
        model.qconfig = torch.ao.quantization.get_default_qat_qconfig('x86')
        model = torch.ao.quantization.fuse_modules(model,
                            [['conv1', 'relu1'], ['conv2', 'relu2'], ['conv3', 'relu3'], ['fc1', 'out_relu']])
        model = torch.ao.quantization.prepare_qat(model.train())
        print(model)
        
        
        
    validation = True
    if train_detect:
        validation == False
        
    train_loader, val_loader = get_loader(ipath, 128, class_folders=True, shuffle=True, load_binary=load_binary, shuffle_row=True, mix_images=False, validation=validation, train_detect=train_detect, reduction=reduction, hotspot=hotspot, label_names=labelnames)
    if hotspot:
        model, history = train_model_double_label(model, train_loader, epochs, infofilename, lr=lr, loss_weights=None, platform=platform, val_loader=val_loader, mini_batch_size=batch)
    else:  
        model, history = train_model(model, train_loader, epochs, infofilename, lr=lr, loss_weights=None, platform=platform, val_loader=val_loader, mini_batch_size=batch)
        
    if quantization:
        model.eval()
        model = torch.ao.quantization.convert(model)
        
    
    torch.save(model.state_dict(), os.path.join(opath, 'model.pt'))
    
        
def test(height, width, platform, mpath, ipath, opath, model_class, use_bp_distance, load_binary, reduction, hotspot, labelnames, dense, quantization):
    class_folders = False
    #print("Testing model ", mpath)
    
    # set class folders to false for production
    test_loader, _ = get_loader(ipath, batch_size=1, class_folders=class_folders, shuffle=False, load_binary=load_binary, shuffle_row=False, mix_images=False, validation=False, train_detect=False, reduction=reduction, hotspot=hotspot, label_names=labelnames)
              
    if use_bp_distance:
        channels = 2
    else:
        channels = 1        

    if hotspot:
        outputs = 2
    else:
        outputs = 1    

    if model_class == "FAST-NN":
        if dense:
            model = FAST_NN_dense(height, width, channels=channels, outputs=outputs)
        elif quantization:
            model = FAST_NN_quantized(height, width, channels=channels, outputs=outputs)
        else:
            model = FAST_NN(height, width, channels=channels, outputs=outputs)
    elif model_class == "FASTER-NN":
        if dense:
            model = FAST_NN_det_dense(height, width, channels=channels, outputs=outputs)
        else:
            model = SweepNet1DDet(height, width, channels=channels, outputs=outputs)
    elif model_class == "SweepNetRecombination":
        model = SweepNet1DV4(height, width, channels=channels, outputs=outputs)
    elif model_class == "SweepNet":
        model = SweepNet(height, width, channels=channels, outputs=outputs)
    elif model_class == "FASTER-NN-group":
        model = SweepNet1DDetGrouped(height, width, channels=channels, outputs=outputs)
    else:
        raise ValueError("Unknown model class")    

    init_state = torch.load(os.path.join(mpath, 'model.pt'), map_location=torch.device(platform))
    
    # transform weights from FC layer to conv layer shape
    if model_class == "FASTER-NN" and dense:
        init_state['fc.weight'] = torch.reshape(init_state['fc.weight'], (2, 32, -1))
        
    if quantization:
        model.eval()
        qconfig = torch.ao.quantization.get_default_qat_qconfig('x86')
        print(qconfig)
        model.qconfig = qconfig
        model = torch.ao.quantization.fuse_modules(model,
                            [['conv1', 'relu1'], ['conv2', 'relu2'], ['conv3', 'relu3'], ['fc1', 'out_relu']])
        model = torch.ao.quantization.prepare_qat(model.train())
        model.eval()
        model = torch.ao.quantization.convert(model)  
        model.load_state_dict(init_state)
        print(model)
        
    else:   
        model.load_state_dict(init_state, strict=False)
    

    if hotspot:
        path_list, output_list, label_list, inference_time = test_model_double_label(model, test_loader, platform=platform)
        #print(time.perf_counter() - start)
        resultsData = np.empty((0, 7), float)
        probability_tensor_1 = torch.nn.functional.softmax(torch.Tensor(output_list)[:,0,:], dim=1)[:, 1]
        probability_tensor_2 = torch.nn.functional.softmax(torch.Tensor(output_list)[:,1,:], dim=1)[:, 1]
        for path, probability_1, probability_2, label in zip(path_list, probability_tensor_1, probability_tensor_2, label_list):
            path = os.path.split(path)[1]
            resultsData = np.append(resultsData,
                                    np.array([[path,
                                            label[0],
                                            label[1],
                                            float(1-probability_1),
                                            float(probability_1),
                                            float(1-probability_2),
                                            float(probability_2)
                                            ]]),
                                    axis=0)
    else:
        if dense:
            path_list, output_list, label_list, inference_time, distances = test_model(model, test_loader, platform=platform, dense=True)
            output_list_per_sim = output_list.squeeze()
            probability_tensor = torch.nn.functional.softmax(torch.Tensor(output_list_per_sim), dim=2)
            probability_tensor = np.array(probability_tensor[:,:,1])
            
            # get sim order and reorder outputs
            sim_indices = []
            for path in path_list:
                sim_index = path.split("/")[-1].split("_")[0]
                sim_indices.append(int(sim_index))
            sim_order = np.argsort(sim_indices)
                        
            probability_tensor = probability_tensor[sim_order]
            for i in range(5):
                plt.plot(probability_tensor[i, :])
            plt.savefig("dense_output.pdf")
            
        else:
            path_list, output_list, label_list, inference_time = test_model(model, test_loader, platform=platform, dense=False)
            
            output_list = output_list.squeeze()
            probability_tensor = torch.nn.functional.softmax(torch.Tensor(output_list), dim=1)
            probability_tensor = np.array(probability_tensor[:,1])
            
            order_numbers = []
            max_sim_index = 0
            for path in path_list:
                sim_index = int(path.split("/")[-1].split(".")[0].split("_")[0])
                if sim_index > max_sim_index:
                    max_sim_index = sim_index
                snp_number = int(path.split("/")[-1].split(".")[0].split("_")[-1])
                order_number = sim_index*100000 + snp_number
                order_numbers.append(order_number)
                
            sim_order = np.argsort(order_numbers)
            probability_tensor = probability_tensor[sim_order]
            probability_tensor = np.reshape(probability_tensor, (max_sim_index+1, -1))
            
            for i in range(5):
                plt.plot(probability_tensor[i, :])
            plt.savefig("grid_output.pdf")
        
        with open(os.path.join(mpath, 'TimingResults.txt'), "a") as f:
            f.write("Inference time for " + mpath + ": " + str(inference_time)+"\n")
        print("Inference time for", mpath, ":", inference_time)


        

# a parameter to select which model to use is required to be implemented.
def main(argv):

    opts, args = getopt.getopt(argv, "m:p:t:a:r:e:i:o:d:h:w:c:f:x:y:g:b:l:n:q:H", ["mode=", "platform=", "threads=", "infofilename=", "reduce=","epochs=", "ipath=", "opath=", "modeldirect=", "height=", "width=", "class=", "file=", "distance=", "detect=", "fine-grained=", "batch", "hotspot=", "labelnames=", "quantization=", "help"])
 
    labelnames = "[]"
    hotspot = 0 
    quantization = False
    fine_grained = False
    for opt, arg in opts:
        if opt in ("-m", "--mode"):
            mode = arg
        elif opt in ("-p", "--platform"):
            platform = arg
        elif opt in ("-t", "--threads"):
            threads = arg
        elif opt in ("-a", "--infofilename"):
            infofilename = arg
        elif opt in ("-r", "--reduce"):
            allelefreq = arg
        elif opt in ("-e", "--epochs"):
            epochs = arg
        elif opt in ("-i", "--ipath"):
            ipath = arg
        elif opt in ("-o", "--opath"):
            opath = arg
        elif opt in ("-d", "--modeldirect"):
            mpath = arg
        elif opt in ("-h", "--height"):
            height = arg
        elif opt in ("-w", "--width"):
            width = arg
        elif opt in ("-c", "--class"):
            model_class = arg
        elif opt in ("-f", "--file"):
            load_binary = arg
        elif opt in ("-x", "--distance"):
            use_bp_distance = arg
        elif opt in ("-y", "--detect"):
            train_detect = arg
        elif opt in ("-g", "--fine-grained"):
            fine_grained = arg
        elif opt in ("-b", "--batch"):
            batch = arg
        elif opt in ("-l", "--hotspot"):
            hotspot = arg
        elif opt in ("-n", "--labelnames"):
            labelnames = arg
        elif opt in ("-q", "--quantization"):
            quantization = arg
        elif opt in ("-H", "--help"):
            help()
            return 0
        
    # workaround before implementation of -g in RAiSD-AI
    if "Dense" in ipath:
        fine_grained = True
    else:
        fine_grained = False
    

        
    if not os.path.exists(opath):
        os.makedirs(opath)
        
    torch.set_num_threads(int(threads))

    if (mode == "train"):
        start=time.time()
        train(int(height), int(width), int(epochs), int(batch), platform, opath, ipath, model_class, int(use_bp_distance), int(load_binary), int(train_detect), infofilename, int(allelefreq), int(hotspot), json.loads(labelnames), quantization)
        end=time.time()
       # with open(opath + "/image-dimensions.txt", "w") as f:
        #    f.write(str(str(height) + " " + str(width)))
            
            
    elif (mode == "predict"):
        start=time.time()
        test(int(height), int(width), platform, mpath, ipath, opath, model_class, int(use_bp_distance), int(load_binary), int(allelefreq), int(hotspot), json.loads(labelnames), fine_grained, quantization)
        
    else:
        print("No valid mode detected")
        return 0
			
if __name__ == "__main__":
    main(sys.argv[1:])		
