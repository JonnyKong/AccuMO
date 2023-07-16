# AccuMO

**[MobiCom '23]** AccuMO: Accuracy-Centric Multitask Offloading in Edge-Assisted Mobile Augmented Reality

## Hardware Requirements

1. An Android phone (tested on a Pixel 5 with Android 12, but others should also work).

1. A Mac/linux laptop with Android Studio installed.

1. A linux server with a NVIDIA GPU (tested with a machine running Ubuntu 18.04.6 LTS with NVIDIA 2080Ti GPU, but others should also work).

   * This server can be the same physical machine as the laptop. Otherwise, this server needs to have an IP address reachable from the phone.

## Software Requirements

Laptop:
  * [ffmpeg](https://ffmpeg.org/download.html)
  * [adb](https://developer.android.com/tools/adb)
  * [Android Studio](https://developer.android.com/studio)

Server:
  * [anaconda/miniconda](https://docs.conda.io/projects/continuumio-conda/en/latest/user-guide/install/macos.html)

## Quick start

#### 1. Download and preprocess datasets and pretrained models on the laptop

1. Clone this repository to any directory.

1. Download the [dataset](https://purdue0-my.sharepoint.com/:f:/g/personal/kong102_purdue_edu/Eq4yAepzaPZPsLJziTJ9PHQBqZ1sF62yp71Ay78Ob0VHjg) and [pretrained_models](https://purdue0-my.sharepoint.com/:f:/g/personal/kong102_purdue_edu/EvA6FUl0HE1LvTMHQ5NR5rQBvlVYBMQXSCmY44pi5cXVQg?e=MpAhJD) folders and place them in the top-level directory, i.e., the top-level directory will look like:

    ```
    AccuMO/
    ├─ client/
    ├─ server/
    ├─ dataset/
    ├─ pretrained_models/
    ├─ README.md
    ```

1. Convert the downloaded RGB frames to YUV format.

    ```bash
    cd dataset && ./run_convert_to_yuv.sh && cd -
    ```

1. Connect the phone to the laptop, and upload the YUV frames and model files to the phone via ADB:

    ```bash
    adb shell mkdir -p /sdcard/accumo/dataset
    adb push yuv/* /sdcard/accumo/dataset

    adb shell mkdir -p /sdcard/accumo/models
    adb push pretrained_models/client/fast-depth-64x224* /sdcard/accumo/models
    ```

#### 2. Setup the GPU server

1. Clone this repository to any directory.

1. Download the [pretrained_models](https://purdue0-my.sharepoint.com/:f:/g/personal/kong102_purdue_edu/EvA6FUl0HE1LvTMHQ5NR5rQBvlVYBMQXSCmY44pi5cXVQg?e=MpAhJD) folder and place it in the top-level directory.

1. Create a conda environment and install dependencies:

    ```bash
    conda create -n accumo python=3.9 tensorflow-gpu=2.7.0 'pytorch=1.10.1=*cuda*' \
        'torchvision=0.11.2=*cuda*' cudatoolkit cudatoolkit-dev scikit-image \
        pandas opencv av tqdm matplotlib -c conda-forge
    conda activate accumo
    cd server/flownet2-pytorch && ./install.sh && cd -
    ```

1. If the server is configured with firewall, configure it to allow TCP on port 9999.

#### 3. Install the client app on the Android phone

1. Connect the phone to the laptop.

1. Follow the steps [here](https://developer.android.com/studio/debug/dev-options#enable) to enable Developer options and USB debugging on the phone.

1. On the laptop, open the `client/` folder with Android Studio.

1. Build and install the app, by clicking **Run** <img src="https://developer.android.com/static/studio/images/buttons/toolbar-run.png" width="15">.

1. On the phone, grant permissions to the "AccuMO" app:
    * Long-press the "AccuMO" app, click `App info`, then click `Permissions`.
    * Go in `Camera permission` and select `Allow only while using the app`.
    * Go in `Files and media` and select `Allow management of all files`.


#### 4. Run the experiments and compute the results

1. Start the server process:

    ```bash
    python -m server.server
    ```

1. Run the following command on the laptop to start offloading the downloaded video.

    ```bash
    adb shell am start -n com.example.accumo/.MainActivity \
        -e com.example.accumo.VIDEO 2022-04-13-Town06-0060-40-0 \
        -e com.example.accumo.SCHED mpc \
        --ez com.example.accumo.ENABLE_FASTDEPTH true \
        -e com.example.accumo.MODE online
    ```
1. The resulting depth maps and VO trajectories will be written to files. Pull them to the laptop to compute the accuracy:

    ```bash
    adb pull /sdcard/accumo/results <RESULT_DIR>
    # TODO
    ```
