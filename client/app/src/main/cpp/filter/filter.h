#ifndef FILTER_H
#define FILTER_H

#include <vector>

#include "BasicESKF.h"
#include "dist.h"
#include "Eigen/Dense"
#include "ErrorStateIMUGPSFuserBase.h"
#include "insfilterErrorState.h"
#include "INSFilterESKF.h"
#include "rotmat.h"
#include "slerp.h"

using namespace Eigen;
using std::mutex;
using std::lock_guard;
using std::vector;

class Filter {
public:
    Filter() {
        c_insfilterErrorState_insfilter(&filter);

        Vector3d pos;
        double a, b, c, d;
        INSFilterESKF_pose(&filter, pos.data(), &a, &b, &c, &d);
        positions.push_back(pos);
        orientations.emplace_back(a, b, c, d);
    }

    void predict(double accel[3], double gyro[3]) {
        const lock_guard<mutex> lock(mutex_);
        predict_(accel, gyro);
        if (offloading) {
            meas_last_mvo.emplace_back(Map<Vector3d>(accel), Map<Vector3d>(gyro));
        }
    }

    void start_offload() {
        const lock_guard<mutex> lock(mutex_);
        offloading = true;
        insfilterErrorState_copy(&filter, &filter_last_mvo);
    }

    void fusemvo(double vo[16]) {
        const lock_guard<mutex> lock(mutex_);

        offloading = false;

        auto vom = kitti2right * Map<Matrix4d>(vo).transpose() * kitti2right.transpose();
        Transform<double, 3, Affine> vot(vom);
        Vector3d pos = vot.translation();
        Matrix3d orient = vot.rotation().transpose();

        insfilterErrorState_copy(&filter_last_mvo, &filter);
        BasicESKF_correct(&filter);
        c_ErrorStateIMUGPSFuserBase_fus(&filter, pos.data(), orient.data());

        positions.resize(positions.size() - meas_last_mvo.size());
        orientations.resize(orientations.size() - meas_last_mvo.size());
        for (auto &[a, g] : meas_last_mvo) {
            predict_(a.data(), g.data());
        }
        meas_last_mvo.clear();
    }

    const Matrix4d pose(int frame = -1) {
        const lock_guard<mutex> lock(mutex_);

        if (frame < 0) {
            frame = positions.size() + frame;
        }

        Transform<double, 3, Affine> tform;
        tform.setIdentity();
        tform.translate(positions[frame]).rotate(rotmat(orientations[frame]));

        return kitti2right.transpose() * tform * kitti2right;
    }

    const Matrix4d pose_relative(int f1 = -2, int f2 = -1) {
        return pose(f1).inverse() * pose(f2);
    }

    double angle(unsigned ms = 32) {
        unsigned s = orientations.size();
        ms = std::min(ms, s - 1);
        auto r1 = rotmat(orientations[s - ms - 1]);
        auto r2 = rotmat(orientations[s - 1]);
        return AngleAxis<double>(r1.transpose() * r2).angle();
    }

private:
    void predict_(double accel[3], double gyro[3]) {
        c_ErrorStateIMUGPSFuserBase_pre(&filter, accel, gyro);

        Vector3d pos;
        double a, b, c, d;
        INSFilterESKF_pose(&filter, pos.data(), &a, &b, &c, &d);
        auto &lo = orientations.back();
        double hlpf = quaternionBase_dist(lo(0), lo(1), lo(2), lo(3), a, b, c, d)
                      / 3.1415926535897931 * 0.5 + 0.15;
        quaternionBase_slerp(lo(0), lo(1), lo(2), lo(3), a, b, c, d, hlpf, &a, &b, &c, &d);
        positions.push_back(pos);
        orientations.emplace_back(a, b, c, d);
    }

    Matrix3d rotmat(Vector4d &q) {
        Matrix3d rt;
        quaternionBase_rotmat(q(0), q(1), q(2), q(3), rt.data());
        return rt.transpose();
    }

    insfilterErrorState filter;
    insfilterErrorState filter_last_mvo;
    vector<std::pair<Vector3d, Vector3d>> meas_last_mvo;
    bool offloading = false;
    mutex mutex_;

    vector<Vector3d> positions;
    vector<Vector4d> orientations;

    const static Matrix4d kitti2right;
};

#endif //FILTER_H
