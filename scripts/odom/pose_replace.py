import argparse
import copy
import math

import evo.main_rpe as main_rpe
import numpy as np
from evo.core import lie_algebra as lie
from evo.core.metrics import PoseRelation, Unit
from evo.core.trajectory import PosePath3D
from evo.tools import file_interface

from angles import angles


def pose_replace(traj_base, traj_replace, cond, start=None, stop=None):
	if not start:
		start = 0
	if not stop:
		stop = len(traj_base.poses_se3)

	num_replaced = 0
	poses_merge = [traj_base.poses_se3[start]]
	for i in range(start + 1, stop):
		if cond(i):
			num_replaced += 1
			p = lie.relative_se3(
				traj_replace.poses_se3[i-1], traj_replace.poses_se3[i])
		else:
			p = lie.relative_se3(
				traj_base.poses_se3[i-1], traj_base.poses_se3[i])
		poses_merge.append(poses_merge[-1] @ p)
	return PosePath3D(poses_se3=poses_merge), num_replaced


def align_stride(traj, traj_base):
	delta = math.ceil(traj.num_poses / traj_base.num_poses)
	return PosePath3D(poses_se3=traj.poses_se3[::delta])


def rpe(traj_ref, traj_est):
	result = main_rpe.rpe(copy.deepcopy(traj_ref), copy.deepcopy(traj_est),
		PoseRelation.full_transformation, 1, Unit.frames)
	return np.array(result.np_arrays['error_array'])


if __name__ == '__main__':
	parser = argparse.ArgumentParser()
	parser.add_argument('ref_file')
	parser.add_argument('upgrade_file')
	parser.add_argument('base_file')
	parser.add_argument('--strategy', choices=['rpe', 'angle'], required=True)
	parser.add_argument('--angle_file')
	parser.add_argument('--threshold', type=float, required=True)
	parser.add_argument('--shift', type=int, default=0)
	parser.add_argument('--save')
	args = parser.parse_args()

	traj_ref = file_interface.read_kitti_poses_file(args.ref_file)
	traj_upgrade = file_interface.read_kitti_poses_file(args.upgrade_file)
	traj_base = file_interface.read_kitti_poses_file(args.base_file)
	traj_ref = align_stride(traj_ref, traj_base)
	traj_upgrade = align_stride(traj_upgrade, traj_base)

	if args.strategy == 'rpe':
		rpe_upgrade = rpe(traj_ref, traj_upgrade)
		rpe_base = rpe(traj_ref, traj_base)
		metric = rpe_base - rpe_upgrade
	elif args.strategy == 'angle':
		traj_angle = file_interface.read_kitti_poses_file(args.angle_file)
		traj_angle = align_stride(traj_angle, traj_base)
		metric = angles(traj_angle)

	traj_merge, num_replaced = pose_replace(traj_base, traj_upgrade,
		lambda i: i - args.shift - 1 >= 0 and metric[i-args.shift-1] > args.threshold)
	print(num_replaced, traj_merge.num_poses - 1, sep='\t')
	if args.save:
		file_interface.write_kitti_poses_file(args.save, traj_merge)
