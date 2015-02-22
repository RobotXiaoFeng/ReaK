
/*
 *    Copyright 2012 Sven Mikael Persson
 *
 *    THIS SOFTWARE IS DISTRIBUTED UNDER THE TERMS OF THE GNU GENERAL PUBLIC LICENSE v3 (GPLv3).
 *
 *    This file is part of ReaK.
 *
 *    ReaK is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    ReaK is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with ReaK (as LICENSE in the root folder).  
 *    If not, see <http://www.gnu.org/licenses/>.
 */

#include <ReaK/geometry/proximity/prox_circle_crect.hpp>

/** Main namespace for ReaK */
namespace ReaK {

/** Main namespace for ReaK.Geometry */
namespace geom {


void prox_circle_crect::computeProximity(const shape_2D_precompute_pack& aPack1, 
                                         const shape_2D_precompute_pack& aPack2) {
  if((!mCircle) || (!mCRect)) {
    mLastResult.mDistance = std::numeric_limits<double>::infinity();
    mLastResult.mPoint1 = vect<double,2>(0.0,0.0);
    mLastResult.mPoint2 = vect<double,2>(0.0,0.0);
    return;
  };
  
  using std::fabs;
  
  const pose_2D<double>& ci_pose = (aPack1.parent == mCircle ? 
                                    aPack1.global_pose : aPack2.global_pose);
  const pose_2D<double>& re_pose = (aPack1.parent == mCircle ? 
                                    aPack2.global_pose : aPack1.global_pose);
  
  vect<double,2> ci_c = ci_pose.Position;
  vect<double,2> ci_c_rel = re_pose.transformFromGlobal(ci_c);
  
  bool in_x_range = ((ci_c_rel[0] > -0.5 * mCRect->getDimensions()[0]) &&
                     (ci_c_rel[0] <  0.5 * mCRect->getDimensions()[0]));
  
  if(in_x_range) {
    if(ci_c_rel[1] > 0.0) {
      mLastResult.mPoint1 = re_pose.transformToGlobal(vect<double,2>(ci_c_rel[0], ci_c_rel[1] - mCircle->getRadius()));
      mLastResult.mPoint2 = re_pose.transformToGlobal(vect<double,2>(ci_c_rel[0], 0.5 * mCRect->getDimensions()[1]));
      mLastResult.mDistance = ci_c_rel[1] - mCircle->getRadius() - 0.5 * mCRect->getDimensions()[1];
    } else {
      mLastResult.mPoint1 = re_pose.transformToGlobal(vect<double,2>(ci_c_rel[0], ci_c_rel[1] + mCircle->getRadius()));
      mLastResult.mPoint2 = re_pose.transformToGlobal(vect<double,2>(ci_c_rel[0], -0.5 * mCRect->getDimensions()[1]));
      mLastResult.mDistance = -0.5 * mCRect->getDimensions()[1] - ci_c_rel[1] - mCircle->getRadius();
    };
    return;
  };
  
  // this boils down to a circle-circle test.
  vect<double,2> re_endc(0.0,0.0);
  if(ci_c_rel[0] > 0.0)
    re_endc[0] += 0.5 * mCRect->getDimensions()[0];
  else
    re_endc[0] -= 0.5 * mCRect->getDimensions()[0];
  vect<double,2> diff_v_rel = ci_c_rel - re_endc;
  double diff_d_rel = norm_2(diff_v_rel);
  mLastResult.mPoint1 = re_pose.transformToGlobal(ci_c_rel - (mCircle->getRadius() / diff_d_rel) * diff_v_rel);
  mLastResult.mPoint2 = re_pose.transformToGlobal(re_endc + (0.5 * mCRect->getDimensions()[1] / diff_d_rel) * diff_v_rel);
  mLastResult.mDistance = diff_d_rel - 0.5 * mCRect->getDimensions()[1] - mCircle->getRadius();
  
};


prox_circle_crect::prox_circle_crect(const circle* aCircle,
                                     const capped_rectangle* aCRect) :
                                     proximity_finder_2D(),
                                     mCircle(aCircle),
                                     mCRect(aCRect) { };


};

};
