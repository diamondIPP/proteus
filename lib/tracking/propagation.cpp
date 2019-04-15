/**
 * \file
 * \author Moritz Kiehn (msmk@cern.ch)
 * \date 2017-10
 */

#include "propagation.h"

#include "utils/definitions.h"
#include "utils/logger.h"

PT_SETUP_LOCAL_LOGGER(Propagation)

Matrix2 Tracking::jacobianSlopeSlope(const Vector4& tangent,
                                     const Matrix4& toTarget)
{
  // map source track parameters to unrestricted target tangent
  Matrix<Scalar, 4, 2> R;
  R.col(0) = toTarget.col(kU);
  R.col(1) = toTarget.col(kV);
  // source tangent in slope parametrization -> target tangent w/ same length
  Vector4 S = toTarget * tangent * (1 / tangent[kW]);
  // map changes in target tangent to slope changes restricted to plane
  Matrix<Scalar, 2, 4> F = Matrix<Scalar, 2, 4>::Zero();
  F(0, kU) = 1 / S(kW);
  F(1, kV) = 1 / S(kW);
  F(0, kW) = -S[kU] / (S[kW] * S[kW]);
  F(1, kW) = -S[kV] / (S[kW] * S[kW]);
  return F * R;
}

Matrix6 Tracking::jacobianState(const Vector4& tangent,
                                const Matrix4& toTarget,
                                Scalar w0)
{
  // the code assumes that the parameter vector is split into three
  // position-like and three tangent-like parameters with the same relative
  // ordering in each subvector.
  // the asserts are here as a code canary to blow up if someone (me) at some
  // point decides to change the parameter ordering in an incompatible way.
  static_assert(kLoc0 < 3,
                "Tangent-like parameters are before position-like ones");
  static_assert(kLoc1 < 3,
                "Tangent-like parameters are before position-like ones");
  static_assert(kTime < 3,
                "Tangent-like parameters are before position-like ones");
  static_assert((kTime - kLoc0) == (kSlopeTime - kSlopeLoc0),
                "Inconsistent parameter ordering");
  static_assert((kTime - kLoc1) == (kSlopeTime - kSlopeLoc1),
                "Inconsistent parameter ordering");
  static_assert((kLoc1 - kLoc0) == (kSlopeLoc1 - kSlopeLoc0),
                "Inconsistent parameter ordering");

  // target tangent derived from the source tangent in slope normalization
  Vector4 S = toTarget * tangent * (1 / tangent[kW]);
  // map source track parameters to unrestricted target coordinates (pos or tan)
  Matrix<Scalar, 4, 3> R;
  R.col(kLoc0) = toTarget.col(kU);
  R.col(kLoc1) = toTarget.col(kV);
  R.col(kTime) = toTarget.col(kS);
  // map changes in unrestricted target coords to changes restricted to plane
  Matrix<Scalar, 3, 4> F = Matrix<Scalar, 3, 4>::Zero();
  F(kLoc0, kU) = 1;
  F(kLoc1, kV) = 1;
  F(kTime, kS) = 1;
  F(kLoc0, kW) = -S[kU] / S[kW];
  F(kLoc1, kW) = -S[kV] / S[kW];
  F(kTime, kW) = -S[kS] / S[kW];
  Matrix6 jac;
  // clang-format off
  jac <<              F * R, (-w0 / S[kW]) * F * R,
            Matrix3::Zero(), (  1 / S[kW]) * F * R;
  // clang-format on
  return jac;
}

Storage::TrackState Tracking::propagateTo(const Storage::TrackState& state,
                                          const Mechanics::Plane& source,
                                          const Mechanics::Plane& target)
{
  // combined transformation matrix from source to target system
  Matrix4 toTarget = target.linearToLocal() * source.linearToGlobal();
  // initial unrestricted track state in the target system
  Vector4 pos = target.toLocal(source.toGlobal(state.position()));
  Vector4 tan = toTarget * state.tangent();
  // build propagation jacobian
  Matrix6 jacobian = jacobianState(state.tangent(), toTarget, pos[kW]);
  // scale target tangent to slope parametrization
  tan /= tan[kW];
  // move position to intersection w/ the target plane
  pos -= tan * pos[kW];
  // build propagated parameter vector
  Vector6 params;
  params[kLoc0] = pos[kU];
  params[kLoc1] = pos[kV];
  params[kTime] = pos[kS];
  params[kSlopeLoc0] = tan[kU];
  params[kSlopeLoc1] = tan[kV];
  params[kSlopeTime] = tan[kS];
  return {params, transformCovariance(jacobian, state.cov())};
}
