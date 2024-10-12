/**
  ******************************************************************************
  * @file           : math.hpp
  * @author         : toastoffee
  * @brief          : None
  * @attention      : None
  * @date           : 2024/10/12
  ******************************************************************************
  */



#ifndef METAL_PLAYGROUND_MATH_HPP
#define METAL_PLAYGROUND_MATH_HPP

#include <simd/simd.h>

class Math {
public:
    static simd::float3 add( const simd::float3& a, const simd::float3& b );
    static simd_float4x4 makeIdentity();
    static simd::float4x4 makePerspective( float fovRadians, float aspect, float znear, float zfar );
    static simd::float4x4 makeXRotate( float angleRadians );
    static simd::float4x4 makeYRotate( float angleRadians );
    static simd::float4x4 makeZRotate( float angleRadians );
    static simd::float4x4 makeTranslate( const simd::float3& v );
    static simd::float4x4 makeScale( const simd::float3& v );
};


#endif //METAL_PLAYGROUND_MATH_HPP
