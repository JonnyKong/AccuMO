#!/bin/bash

# Redirect output to stdout, then use gpu parallel

inputpath="dataset/rgb"
outputpath="dataset/yuv"

datasets=(
    2022-04-13-Town06-0060-40-0
)

for d in "${datasets[@]}"
do
    mkdir -p $outputpath/$d

    idx_frame=0

    for fn in "$inputpath/$d/"*.jpg; do
        echo "Processing frame: $(basename ${fn})"
        idx_frame=$((idx_frame+1))
        outfn=$(basename $fn)
        outfn=$(echo ${outfn%%.*})
        ffmpeg -y -i $fn -pix_fmt nv12 -vf scale=832:256 -loglevel panic $outputpath/$d/$outfn.yuv
    done

    cp ${inputpath}/${d}/*.txt ${outputpath}/${d}
done
