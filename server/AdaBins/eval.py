from infer import InferenceHelper 


if __name__ == '__main__':
    test_dir = '/home/kong102/datasets/sc_dataset_processed/kitti_256/2011_09_26_drive_0001_sync_02'
    out_dir='./2011_09_26_drive_0001_sync_02'

    infer_helper = InferenceHelper(dataset='kitti')
    infer_helper.predict_dir(test_dir, out_dir)
