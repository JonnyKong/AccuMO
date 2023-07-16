import copy

import evo.main_rpe as rpe
import numpy as np
from evo.core.metrics import PoseRelation, Unit

from pose_replace import align_stride
from scale_per_frame import scale_per_frame

def kitti_error(traj_ref, traj_est):
    errors = np.empty(0)
    for delta in range(100, 900, 100):
        if traj_ref.path_length > delta:
            # we swap traj_est and traj_ref since evo uses traj_est
            # to calculate distances, while we want traj_ref
            result = rpe.rpe(copy.deepcopy(traj_est), copy.deepcopy(traj_ref),
                PoseRelation.translation_part, delta, Unit.meters, all_pairs=True)
            result = np.array(result.np_arrays['error_array']) / delta
            errors = np.append(errors, result)
    return errors.mean()


if __name__ == '__main__':
    import argparse

    from evo.tools import file_interface

    parser = argparse.ArgumentParser()
    parser.add_argument('traj_ref')
    parser.add_argument('traj_est')
    parser.add_argument('--scale_per_frame', action='store_true')
    args = parser.parse_args()

    traj_ref = file_interface.read_kitti_poses_file(args.traj_ref)
    traj_est = file_interface.read_kitti_poses_file(args.traj_est)
    traj_ref = align_stride(traj_ref, traj_est)
    if args.scale_per_frame:
        traj_est = scale_per_frame(traj_ref, traj_est)
    print(kitti_error(traj_ref, traj_est))
