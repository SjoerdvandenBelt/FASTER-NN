# FASTER-NN
This repository contains the code used to develop and test the FASTER-NN model for selective sweep detection. This repository contains scripts to demonstrate the performance of FASTER-NN

## Python packages
This repository is confirmed to work with the following packages on Ubuntu 22.04
- python (3.10.11)
- numpy (1.24.3)
- pytorch (2.0.1)
- torchvision (0.15.2)

## Downloading datasets
To execute the scripts, the ms datasets must be downloaded. Sample datasets for each of the confounding effects tested in the FASTER-NN paper can be downloaded at https://doi.org/10.6084/m9.figshare.26139454.v1
## Generating datasets
The available datasets contain 1k neutral and 1k selective training samples. The ms, mssel, msHOT and mbs software can be used to generate larger datasets. The datasets can be generated using the following commands:
### D1
neutral: ms 128 1000 -t 2000 -r 2000 2000 -eN .10000000000000000000 0.5 -eN .10100000000000000000 1

selection: mssel 128 1000 0 128 /trajectory_files/trajectory2.txt 1000 -t 2000 -r 2000 2000 -eN .10000000000000000000 0.5 -eN .10100000000000000000 1
### D2
neutral: ms 128 1000 -t 2000 -r 2000 2000 -eN .01000000000000000000 0.005 -eN .01200000000000000000 1

selection: mssel 128 1000 0 128 /trajectory_files/trajectory48.txt 1000 -t 2000 -r 2000 2000 -eN .01000000000000000000 0.005 -eN .01200000000000000000 1
### D3
neutral: ms 128 1000 -I 2 0 128 0.0 -en 0 2 0.05 -em 0 2 1 3 -t 2000 -r 2000 2000 -ej 0.003 1 2 -en 0.003 2 1

selection: mssel 128 1000 0 128 /trajectory_files/trajectory_continent_island.txt 1000 -I 2 0 0 0 128 0.0 -en 0 2 0.05 -em 0 2 1 3 -t 2000 -r 2000 2000 -ej 0.003 1 2 -en 0.003 2 1
### D4
neutral: ms 128 1000 -I 2 0 128 0.0 -en 0 2 0.05 -em 0 2 1 3 -t 2000 -r 2000 2000 -ej 3 1 2 -en 3 2 1

selection: mssel 128 1000 0 128 /trajectory_files/trajectory_continent_island.txt 1000 -I 2 0 0 0 128 0.0 -en 0 2 0.05 -em 0 2 1 3 -t 2000 -r 2000 2000 -ej 3 1 2 -en 3 2 1
### D5
neutral: msHOT 128 1000 -t 2000 -r 2000 2000 -eN .10000000000000000000 0.5 -eN .10040000000000000000 1 -v 1 950 1050 2

selection: mbs 128 -t 0.02 -r 0.02 -s 100000 50000 -f 1 1000 /mbsFiles/traj -h /mbsFiles/rechot_47500_52500_2.txt
### D6
neutral: msHOT 128 1000 -t 2000 -r 2000 2000 -eN .10000000000000000000 0.5 -eN .10040000000000000000 1 -v 1 950 1050 20

selection: mbs 128 -t 0.02 -r 0.02 -s 100000 50000 -f 1 1000 /mbsFiles/traj -h /mbsFiles/rechot_47500_52500_20.txt
### Trajectory files
The corresponding trajectory files can be downloaded through https://doi.org/10.6084/m9.figshare.24118365.v1

## installing RAiSD-AI
FASTER-NN using a preliminary version of RAiSD-AI to generate windows from the ms files, and to train the models. Before running any of the scripts, RAiSD-AI must be installed by running the following
```
./install-RAiSD-AI.sh
```

## FASTER-NN short test script
This repository comes with a trained FASTER-NN model. This model can be used by running:
```
./faster_nn_demo_test.sh
```
Make sure to edit the base path to the datasets by changing lines 12 through 15 in the script to the basepath where you have stored the downloaded datasets.
This script runs a model that is trained on D1 using the dense FASTER-NN model and using FASTER-NN with grid-based inference.
The script prints the time to run the dense and grid-based models, indicating the speedup gained by using the dense FASTER-NN model.

## FASTER-NN train and test script
To test different datasets, and to test the time it takes to perform grid-based inference using various grid densities, the full test script can be used by running:
```
./faster_nn_demo_full.sh
```
Again, the base paths to the datasets must be changed to where you have stored the downloaded datasets by editing lines 12 through 15 in the script.

## Evaluating dense inference
The demo scripts print the inference time of the models using dense inference and using grid-based inference, demonstrating the speedup gained when using dense inference.

In addition, the script plots the classification outputs of the first 5 simulations in the dataset for both dense and grid-based inference. These plots are stored as dense_output.pdf and grid_output.pdf. The plots demonstrate that the output of grid-based inference consists of subsampled values from the output of dense inference.
