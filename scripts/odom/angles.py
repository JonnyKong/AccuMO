import numpy as np
from evo.core import lie_algebra as lie

def angle(v1, v2):
	norm = np.linalg.norm(v1) * np.linalg.norm(v2)
	return np.arccos(np.dot(v1, v2) / norm) if norm else 0

def angles(traj, trans=False, forward=[1, 0, 0], ms=1, keep_first=False):
	angles = [0] if keep_first else []
	start = 1 if keep_first else ms
	for i in range(start, len(traj.poses_se3)):
		s = max(i - ms, 0)
		pose = lie.relative_se3(traj.poses_se3[s], traj.poses_se3[i])
		if trans:
			angles.append(angle(forward, pose[:3, 3]))
		else:
			angles.append(lie.so3_log_angle(pose[:3, :3]))
	return angles


if __name__ == '__main__':
	import argparse

	import matplotlib.pyplot as plt
	from evo.tools import file_interface

	parser = argparse.ArgumentParser()
	parser.add_argument('traj', nargs='+')
	parser.add_argument('--angles_out')
	parser.add_argument('--plot', action='store_true')
	parser.add_argument('--ms', type=int, default=1)
	parser.add_argument('--keep-first', action='store_true')
	args = parser.parse_args()

	all_angles = []
	for t in args.traj:
		traj = file_interface.read_kitti_poses_file(t)
		traj_angles = angles(traj, ms=args.ms, keep_first=args.keep_first)
		all_angles.append(traj_angles)
		if args.plot:
			plt.plot(traj_angles, 'o')
	if args.angles_out:
		np.savetxt(args.angles_out, np.array(all_angles).T)
	if args.plot:
		plt.show()
