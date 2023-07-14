# AccuMO
**[MobiCom '23]** AccuMO: Accuracy-Centric Multitask Offloading in Edge-Assisted Mobile Augmented Reality

### Hardware Requirements

1. An Android phone (tested on a Pixel 5 with Android 12, but others should also work)

1. A Mac/linux laptop with Android Studio installed

1. A linux machine with a NVIDIA GPU (tested with a machine running Ubuntu 18.04.6 LTS with NVIDIA 2080Ti GPU, but others should also work).

   * This server can be the same physical machine as the laptop. Otherwise, this server needs to have an IP address reachable from the phone.

### Software Requirements

On the laptop:
  * [ffmpeg](https://ffmpeg.org/download.html)
  * [adb](https://developer.android.com/tools/adb)

On the server:
  * [anaconda/miniconda](https://docs.conda.io/projects/continuumio-conda/en/latest/user-guide/install/macos.html)

### Laptop Preparation

1. Clone this repository.

1. On the laptop, download the [dataset](https://purdue0-my.sharepoint.com/:f:/g/personal/kong102_purdue_edu/Eq4yAepzaPZPsLJziTJ9PHQBqZ1sF62yp71Ay78Ob0VHjg) and [pretrained_models](https://purdue0-my.sharepoint.com/:f:/g/personal/kong102_purdue_edu/EvA6FUl0HE1LvTMHQ5NR5rQBvlVYBMQXSCmY44pi5cXVQg?e=MpAhJD) folders to the top-level directory, i.e., the top-level directory will look like:

    ```
    AccuMO/
    ├─ client/
    ├─ server/
    ├─ dataset/
    ├─ pretrained_models/
    ├─ README.md
    ```

1. On the laptop, convert RGB frames to YUV format.

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

### Server Preparation

1. Clone this repository.

1. Download the [pretrained_models](https://purdue0-my.sharepoint.com/:f:/g/personal/kong102_purdue_edu/EvA6FUl0HE1LvTMHQ5NR5rQBvlVYBMQXSCmY44pi5cXVQg?e=MpAhJD) folder to the top-level directory.

1. Create a conda environment and install dependencies:

    ```bash
    conda create -n accumo python=3.9 tensorflow-gpu=2.7.0 'pytorch=1.10.1=*cuda*' \
        'torchvision=0.11.2=*cuda*' cudatoolkit cudatoolkit-dev scikit-image \
        pandas opencv av tqdm matplotlib -c conda-forge
    conda activate accumo
    cd server/flownet2-pytorch && ./install.sh && cd -
    ```

1. If the server is configured with firewall, configure it to allow TCP on port 9999.

1. Start the server process:

    ```bash
    cd server && python server.server
    ```

### Phone Preparation

1. Connect the phone to the laptop.

1. On the laptop, Open the `client/` folder with Android Studio.

1. Build and install the app.

1. On the phone, grant permissions to the "AccuMO" app:
    * Long-press the "AccuMO" app, click `App info`, then click `Permissions`.
    * Go in `Camera permission` and select `Allow only while using the app`.
    * Go in `Files and media` and select `Allow management of all files`.

1. Run the following command on the laptop to start offloading a video. The `VIDEO_NAME` parameter
specifies which video is offloaded, and the `SCHEDULER_NAME` parameter specifies which scheduler
is to be used (table 3 in the paper).

    ```bash
    adb shell am start -n com.example.accumo/.MainActivity \
        -e com.example.accumo.VIDEO <VIDEO_NAME> \
        -e com.example.accumo.SCHED <SCHEDULER_NAME> \
        --ez com.example.accumo.ENABLE_FASTDEPTH true \
        -e com.example.accumo.MODE online
    ```
