import glob
import os
import sys

import numpy as np
import pandas as pd
from PIL import Image
from skimage.transform import resize as imresize

SAVING_FACTOR = 256.0


def load_depth(depth_path: str, saving_factor: float):
    if depth_path.endswith('.png'):
        return np.array(Image.open(depth_path)).astype(np.float32) / saving_factor
    elif depth_path.endswith('.npy'):
        return np.load(depth_path)
    else:
        assert False, 'Unrecognized format'


def compute_errors(gt, pred):
    crop_mask = np.zeros(gt.shape, dtype=bool)

    y1, y2 = 0, gt.shape[0]
    x1, x2 = 0, gt.shape[1]
    max_depth = 80
    crop_mask[y1:y2, x1:x2] = 1

    valid = (gt > 0) & (gt < max_depth) & (pred > 0) & (pred < max_depth)
    valid = valid & crop_mask

    valid_gt = gt[valid]
    valid_pred = pred[valid].clip(1e-3, max_depth)

    abs_rel = np.mean(np.abs(valid_gt - valid_pred) / valid_gt)
    abs_diff = np.mean(np.abs(valid_gt - valid_pred))
    sq_rel = np.mean(((valid_gt - valid_pred) ** 2) / valid_gt)

    # From: https://github.com/nianticlabs/monodepth2/blob/93b9ebb4c8bdc71a07e18bee64d76ec21bc3e8fc/evaluate_depth.py#L35
    rmse = (valid_gt - valid_pred) ** 2
    rmse = np.sqrt(rmse.mean())
    rmse_log = (np.log(valid_gt) - np.log(valid_pred)) ** 2
    rmse_log = np.sqrt(rmse_log.mean())

    # From: https://github.com/JiawangBian/SC-SfMLearner-Release/blob/05fc14ae72dbd142e3d37af05da14eedda51e443/eval_depth.py#L38
    thresh = np.maximum((valid_gt / valid_pred), (valid_pred / valid_gt))
    a1 = (thresh < 1.25).mean()
    a2 = (thresh < 1.25 ** 2).mean()
    a3 = (thresh < 1.25 ** 3).mean()

    error_names = ['abs_rels', 'abs_diff', 'sq_rel', 'rmse', 'rmse_log', 'a1', 'a2', 'a3']
    errors = [abs_rel, abs_diff, sq_rel, rmse, rmse_log, a1, a2, a3]
    return error_names, errors


def eval_acc(d_gt: np.ndarray, d_warped: np.ndarray, dataset_type):
    d_warped = np.clip(d_warped, a_min=0.0, a_max=80.0)

    # Upsample warped result to be the same with gt
    if d_warped.shape != d_gt.shape:
        print(f'Resizing result from {d_warped.shape} to {d_gt.shape}')
        d_warped = imresize(d_warped, d_gt.shape)

    # Interpolate warped image to remove empty points
    d_warped[d_warped == 0.0] = np.nan

    error_names, errors = compute_errors(d_gt, d_warped)
    return error_names, errors


def eval_vid_abs_rel(gt_path, pred_path):
    pred_arr = sorted(glob.glob(f'{pred_path}/??????.png'))
    assert len(pred_arr) > 0, f'No depth map found in directory: {pred_path}'
    gt_arr = sorted(glob.glob(f'{gt_path}/*.png'))
    assert len(gt_arr) > 0, f'No depth map found in directory: {gt_path}'

    df = []

    for d_pred in pred_arr:
        imgname = os.path.basename(d_pred)
        d_gt = gt_arr[int(imgname.removesuffix('.png'))]

        try:
            d_pred = load_depth(d_pred, SAVING_FACTOR)
            d_gt = load_depth(d_gt, SAVING_FACTOR)

            error_names, errors = eval_acc(d_gt, d_pred, dataset_type='carla')
            abs_rel = errors[error_names.index('abs_rels')]
            print(f'AbsRel for frame {imgname}: {abs_rel}')

        except OSError:
            abs_rel = np.nan

        df.append([int(imgname.removesuffix('.png')), abs_rel])

    if len(df) > 0:
        df = pd.DataFrame(df, columns=['aframeIdx', 'abs_rel'])
        df.to_csv(f'{pred_path}/abs_rel.csv', index=False)
        print(f'Mean AbsRel: {df.abs_rel.mean()}')


if __name__ == '__main__':
    gt_path = sys.argv[1]
    pred_path = sys.argv[2]
    eval_vid_abs_rel(gt_path, pred_path)
