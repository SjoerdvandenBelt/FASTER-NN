# experiment for benchmarking dense models versus sparse models

gridpoints_list=(32)
stride_list=(64)
ASDEC_ds=(2)
SweepNet_ds=(1)
width=512
height=128

# normal training
for i in "${!SweepNet_ds[@]}"; do
    TRAINING_SWEEP=../datasets/SweepNet/D${SweepNet_ds[i]}/train/TEST${ASDEC_ds[i]}.txt
    TRAINING_NEUTRAL=../datasets/SweepNet/D${SweepNet_ds[i]}/train/BASE${ASDEC_ds[i]}.txt
    TESTING_SWEEP=../datasets/SweepNet/D${SweepNet_ds[i]}/test/TEST${ASDEC_ds[i]}.txt
    TESTING_NEUTRAL=../datasets/SweepNet/D${SweepNet_ds[i]}/test/BASE${ASDEC_ds[i]}.txt
    if [ ${ASDEC_ds[i]} -eq 92 ] || [ ${ASDEC_ds[i]} -eq 96 ]
    then
        # gen train data
        ./RAiSD-AI -n TrainingDataEXP8-H${height}-W${width} -I $TRAINING_NEUTRAL -L 100000 -its 50000 -op IMG-GEN -icl neutralTR -f -frm -bin -typ 1 -w 512 -ips 1
        ./RAiSD-AI -n TrainingDataEXP8-H${height}-W${width} -I $TRAINING_SWEEP -L 100000 -its 50000 -op IMG-GEN -icl sweepTR -f -bin -typ 1 -w 512 -ips 1 -b

        # # gen test data for dense inference
        ./RAiSD-AI -n TestDenseEXP8-H${height}-W${width} -I $TESTING_NEUTRAL -L 100000 -its 50000 -op IMG-GEN -icl neutralTE -f -frm -bin -typ 1 -w 2560 -ips 1 -iws 1
        ./RAiSD-AI -n TestDenseEXP8-H${height}-W${width} -I $TESTING_SWEEP -L 100000 -its 50000 -op IMG-GEN -icl sweepTE -f -bin -typ 1 -w 2560 -ips 1 -iws 1 -b
    else
        # gen train data
        ./RAiSD-AI -n TrainingDataEXP8-H${height}-W${width} -I $TRAINING_NEUTRAL -L 100000 -its 50000 -op IMG-GEN -icl neutralTR -f -frm -bin -typ 1 -w 512 -ips 1
        ./RAiSD-AI -n TrainingDataEXP8-H${height}-W${width} -I $TRAINING_SWEEP -L 100000 -its 50000 -op IMG-GEN -icl sweepTR -f -bin -typ 1 -w 512 -ips 1

        # # gen test data for dense inference
        ./RAiSD-AI -n TestDenseEXP8-H${height}-W${width} -I $TESTING_NEUTRAL -L 100000 -its 50000 -op IMG-GEN -icl neutralTE -f -frm -bin -typ 1 -w 2560 -ips 1 -iws 1
        ./RAiSD-AI -n TestDenseEXP8-H${height}-W${width} -I $TESTING_SWEEP -L 100000 -its 50000 -op IMG-GEN -icl sweepTE -f -bin -typ 1 -w 2560 -ips 1 -iws 1
    fi

    # dense inference
    python3 sources/pytorch-sources/main.py -m predict -d RAiSD_Model.FASTER-NN-EXP8-D1-H128-W512 -i RAiSD_Images.TestDenseEXP8-H128-W512/sweepTE -p cpu -c FASTER-NN -t 1 -f 1 -x 1 -y 0 -r 1 -l  0  -h 128 -w 512 -o tempOutputFolder

    # grid inference per grid density
    for j in "${!gridpoints_list[@]}"; do
        if [ ${ASDEC_ds[i]} -eq 92 ] || [ ${ASDEC_ds[i]} -eq 96 ]
        then
            ./RAiSD-AI -n TestGridEXP8-H${height}-W${width} -I $TESTING_NEUTRAL -L 100000 -its 50000 -op IMG-GEN -icl neutralTE -f -frm -bin -typ 1 -w 512 -ips ${gridpoints_list[j]} -iws ${stride_list[j]}
            ./RAiSD-AI -n TestGridEXP8-H${height}-W${width} -I $TESTING_SWEEP -L 100000 -its 50000 -op IMG-GEN -icl sweepTE -f -bin -typ 1 -w 512 -ips ${gridpoints_list[j]} -iws ${stride_list[j]} -b
        else
            ./RAiSD-AI -n TestGridEXP8-H${height}-W${width} -I $TESTING_NEUTRAL -L 100000 -its 50000 -op IMG-GEN -icl neutralTE -f -frm -bin -typ 1 -w 512 -ips ${gridpoints_list[j]} -iws ${stride_list[j]}
            ./RAiSD-AI -n TestGridEXP8-H${height}-W${width} -I $TESTING_SWEEP -L 100000 -its 50000 -op IMG-GEN -icl sweepTE -f -bin -typ 1 -w 512 -ips ${gridpoints_list[j]} -iws ${stride_list[j]}
        fi

        python3 sources/pytorch-sources/main.py -m predict -d RAiSD_Model.FASTER-NN-EXP8-D1-H128-W512 -i RAiSD_Images.TestGridEXP8-H128-W512/sweepTE -p cpu -c FASTER-NN -t 1 -f 1 -x 1 -y 0 -r 1 -l  0  -h 128 -w 512 -o tempOutputFolder
    done
done
