from evo.core import lie_algebra as lie
from evo.core.trajectory import PosePath3D
from numpy import linalg as LA


def scale_per_frame(traj_ref, traj_est):
	poses_scale = [traj_est.poses_se3[0]]
	for i in range(1, len(traj_ref.poses_se3)):
		r = lie.relative_se3(traj_ref.poses_se3[i-1], traj_ref.poses_se3[i])
		e = lie.relative_se3(traj_est.poses_se3[i-1], traj_est.poses_se3[i])
		s = LA.norm(r[:3, 3]) / LA.norm(e[:3, 3])
		ss = lie.se3(e[:3, :3], s * e[:3, 3])
		poses_scale.append(poses_scale[-1] @ ss)
	return PosePath3D(poses_se3=poses_scale)


if __name__ == '__main__':
	import sys

	from evo.tools import file_interface

	from pose_replace import align_stride

	traj_ref = file_interface.read_kitti_poses_file(sys.argv[1])
	traj_est = file_interface.read_kitti_poses_file(sys.argv[2])
	traj_ref = align_stride(traj_ref, traj_est)
	traj_scale = scale_per_frame(traj_ref, traj_est)
	file_interface.write_kitti_poses_file(sys.argv[3], traj_scale)
